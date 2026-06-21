/**
 * @file    drivers/st7789.cpp
 * @brief   Implementación del driver TFT ST7789 (SPI, 240x240)
 *
 * @author  MRF24J40 Team
 * @date    2026
 */

#include <drivers/st7789.hpp>
#include <cstring>
#include <thread>
#include <chrono>
#include <algorithm>
#include <stdexcept>

namespace drivers {

// ============================================================================
// Comandos ST7789
// ============================================================================

static constexpr uint8_t CMD_SWRESET   = 0x01;
static constexpr uint8_t CMD_SLEEP_OUT = 0x11;
static constexpr uint8_t CMD_DISP_ON   = 0x29;
static constexpr uint8_t CMD_CASET     = 0x2A;
static constexpr uint8_t CMD_RASET     = 0x2B;
static constexpr uint8_t CMD_RAMWR     = 0x2C;
static constexpr uint8_t CMD_MADCTL    = 0x36;
static constexpr uint8_t CMD_COLMOD    = 0x3A;

// ============================================================================
// Constructor / Destructor
// ============================================================================

St7789_t::St7789_t(int16_t width, int16_t height)
    : m_dc_gpio(23)
    , m_rst_gpio(24)
    , m_bl_gpio(18)
{
    m_width = width;
    m_height = height;
}

St7789_t::~St7789_t() {
    if (m_initialized) {
        displayOff();
    }
}

// ============================================================================
// Comandos SPI de bajo nivel
// ============================================================================

void St7789_t::sendCommand(uint8_t /*cmd*/) {
    // D/C# = LOW (comando)
    // m_spi->transfer1(cmd);  // Implementar con GPIO para D/C#
}

void St7789_t::sendData(const uint8_t* /*data*/, size_t /*len*/) {
    // D/C# = HIGH (datos)
    // m_spi->transfer(data, len);  // Implementar con GPIO para D/C#
}

void St7789_t::sendDataByte(uint8_t data) {
    sendData(&data, 1);
}

void St7789_t::hwReset() {
    // GPIO reset pulse
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void St7789_t::setWindow(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
    sendCommand(CMD_CASET);
    sendDataByte(x0 >> 8); sendDataByte(x0 & 0xFF);
    sendDataByte(x1 >> 8); sendDataByte(x1 & 0xFF);

    sendCommand(CMD_RASET);
    sendDataByte(y0 >> 8); sendDataByte(y0 & 0xFF);
    sendDataByte(y1 >> 8); sendDataByte(y1 & 0xFF);
}

// ============================================================================
// Inicialización
// ============================================================================

bool St7789_t::init() {
    try {
        hal::SpiConfig cfg;
        cfg.speed_hz = 40000000; // 40 MHz
        cfg.mode = hal::SpiMode::Mode0;
        m_spi = std::make_unique<hal::Spi_t>(cfg);
    } catch (const std::exception& e) {
        return false;
    }

    hwReset();

    sendCommand(CMD_SWRESET);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    sendCommand(CMD_SLEEP_OUT);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    sendCommand(CMD_COLMOD);
    sendDataByte(0x55); // 16-bit color (RGB565)

    sendCommand(CMD_MADCTL);
    sendDataByte(0x00); // Orientación normal

    sendCommand(CMD_DISP_ON);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    setWindow(0, 0, m_width - 1, m_height - 1);
    clear();
    m_initialized = true;
    return true;
}

// ============================================================================
// Control
// ============================================================================

void St7789_t::clear() {
    fillScreen(ColorRgb565::Black());
}

void St7789_t::update() {
    // ST7789 no tiene framebuffer interno - los datos se escriben directo
}

void St7789_t::setContrast(uint8_t /*level*/) {
    // No aplica para TFT
}

void St7789_t::displayOff() {
    sendCommand(0x28); // DISPOFF
}

void St7789_t::displayOn() {
    sendCommand(CMD_DISP_ON);
}

// ============================================================================
// Primitivas gráficas
// ============================================================================

void St7789_t::drawPixel(int16_t x, int16_t y, bool color) {
    drawPixel(x, y, color ? ColorRgb565::White() : ColorRgb565::Black());
}

void St7789_t::drawPixel(int16_t x, int16_t y, ColorRgb565 color) {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) return;

    setWindow(x, y, x, y);
    sendCommand(CMD_RAMWR);
    sendDataByte(color.raw >> 8);
    sendDataByte(color.raw & 0xFF);
}

void St7789_t::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool color) {
    ColorRgb565 c = color ? ColorRgb565::White() : ColorRgb565::Black();
    int16_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int16_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy, e2;

    while (true) {
        drawPixel(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void St7789_t::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, bool color) {
    drawLine(x, y, x + w - 1, y, color);
    drawLine(x, y + h - 1, x + w - 1, y + h - 1, color);
    drawLine(x, y, x, y + h - 1, color);
    drawLine(x + w - 1, y, x + w - 1, y + h - 1, color);
}

void St7789_t::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, bool color) {
    fillRect(x, y, w, h, color ? ColorRgb565::White() : ColorRgb565::Black());
}

void St7789_t::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, ColorRgb565 color) {
    if (x >= m_width || y >= m_height || w <= 0 || h <= 0) return;

    int16_t x0 = std::max<int16_t>(x, 0);
    int16_t y0 = std::max<int16_t>(y, 0);
    int16_t x1 = std::min<int16_t>(x + w - 1, m_width - 1);
    int16_t y1 = std::min<int16_t>(y + h - 1, m_height - 1);

    setWindow(x0, y0, x1, y1);
    sendCommand(CMD_RAMWR);

    uint8_t hi = color.raw >> 8;
    uint8_t lo = color.raw & 0xFF;
    uint32_t pixels = (x1 - x0 + 1) * (y1 - y0 + 1);

    for (uint32_t i = 0; i < pixels; i++) {
        sendDataByte(hi);
        sendDataByte(lo);
    }
}

void St7789_t::drawCircle(int16_t x0, int16_t y0, int16_t r, bool color) {
    (void)color;
    ColorRgb565 c = color ? ColorRgb565::White() : ColorRgb565::Black();
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    drawPixel(x0, y0 + r, c);
    drawPixel(x0, y0 - r, c);
    drawPixel(x0 + r, y0, c);
    drawPixel(x0 - r, y0, c);

    while (x < y) {
        if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
        x++; ddF_x += 2; f += ddF_x;

        drawPixel(x0 + x, y0 + y, c); drawPixel(x0 - x, y0 + y, c);
        drawPixel(x0 + x, y0 - y, c); drawPixel(x0 - x, y0 - y, c);
        drawPixel(x0 + y, y0 + x, c); drawPixel(x0 - y, y0 + x, c);
        drawPixel(x0 + y, y0 - x, c); drawPixel(x0 - y, y0 - x, c);
    }
}

// ============================================================================
// Texto
// ============================================================================

void St7789_t::drawString(int16_t x, int16_t y, std::string_view str,
                           uint8_t size, bool /*color*/) {
    // Fuente 5x7 simplificada
    int16_t cx = x;
    for (char ch : str) {
        if (ch == '\n') { cx = x; y += 8 * size; continue; }
        cx += (5 + 1) * size;
        if (cx > m_width) { cx = x; y += 8 * size; }
    }
}

void St7789_t::fillScreen(ColorRgb565 color) {
    fillRect(0, 0, m_width, m_height, color);
}

void St7789_t::drawBitmap(int16_t x, int16_t y, int16_t w, int16_t h,
                           const uint16_t* data) {
    setWindow(x, y, x + w - 1, y + h - 1);
    sendCommand(CMD_RAMWR);
    for (int16_t i = 0; i < w * h; i++) {
        sendDataByte(data[i] >> 8);
        sendDataByte(data[i] & 0xFF);
    }
}

void St7789_t::drawQr(const uint8_t* data, int qr_width,
                       int x, int y, int scale,
                       ColorRgb565 fg, ColorRgb565 /*bg*/) {
    for (int row = 0; row < qr_width; row++) {
        for (int col = 0; col < qr_width; col++) {
            bool pixel = data[row * qr_width + col] & 1;
            if (!pixel) { // Módulo negro
                fillRect(x + col * scale, y + row * scale,
                         scale, scale, fg);
            }
        }
    }
}

} // namespace drivers
