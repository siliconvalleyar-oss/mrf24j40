/**
 * @file    drivers/ssd1306.cpp
 * @brief   Implementación del driver SSD1306 (OLED I2C 128x64)
 *
 * @author  MRF24J40 Team
 * @date    2026
 */

#include <drivers/ssd1306.hpp>
#include <cstring>
#include <algorithm>
#include <thread>
#include <chrono>
#include <stdexcept>

namespace drivers {

// ============================================================================
// Fuente 5x7 (ASCII 32-126)
// ============================================================================

static const uint8_t FONT5x7[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, // 20 space
    0x00, 0x00, 0x5F, 0x00, 0x00, // 21 !
    0x00, 0x07, 0x00, 0x07, 0x00, // 22 "
    0x14, 0x7F, 0x14, 0x7F, 0x14, // 23 #
    0x24, 0x2A, 0x7F, 0x2A, 0x12, // 24 $
    0x23, 0x13, 0x08, 0x64, 0x62, // 25 %
    0x36, 0x49, 0x55, 0x22, 0x50, // 26 &
    0x00, 0x05, 0x03, 0x00, 0x00, // 27 '
    0x00, 0x1C, 0x22, 0x41, 0x00, // 28 (
    0x00, 0x41, 0x22, 0x1C, 0x00, // 29 )
    0x08, 0x2A, 0x1C, 0x2A, 0x08, // 2A *
    0x08, 0x08, 0x3E, 0x08, 0x08, // 2B +
    0x00, 0x50, 0x30, 0x00, 0x00, // 2C ,
    0x08, 0x08, 0x08, 0x08, 0x08, // 2D -
    0x00, 0x60, 0x60, 0x00, 0x00, // 2E .
    0x20, 0x10, 0x08, 0x04, 0x02, // 2F /
    0x3E, 0x51, 0x49, 0x45, 0x3E, // 30 0
    0x00, 0x42, 0x7F, 0x40, 0x00, // 31 1
    0x42, 0x61, 0x51, 0x49, 0x46, // 32 2
    0x21, 0x41, 0x45, 0x4B, 0x31, // 33 3
    0x18, 0x14, 0x12, 0x7F, 0x10, // 34 4
    0x27, 0x45, 0x45, 0x45, 0x39, // 35 5
    0x3C, 0x4A, 0x49, 0x49, 0x30, // 36 6
    0x01, 0x71, 0x09, 0x05, 0x03, // 37 7
    0x36, 0x49, 0x49, 0x49, 0x36, // 38 8
    0x06, 0x49, 0x49, 0x29, 0x1E, // 39 9
    0x00, 0x36, 0x36, 0x00, 0x00, // 3A :
    0x00, 0x56, 0x36, 0x00, 0x00, // 3B ;
    0x00, 0x08, 0x14, 0x22, 0x41, // 3C <
    0x14, 0x14, 0x14, 0x14, 0x14, // 3D =
    0x41, 0x22, 0x14, 0x08, 0x00, // 3E >
    0x02, 0x01, 0x51, 0x09, 0x06, // 3F ?
    0x32, 0x49, 0x79, 0x41, 0x3E, // 40 @
    0x7E, 0x11, 0x11, 0x11, 0x7E, // 41 A
    0x7F, 0x49, 0x49, 0x49, 0x36, // 42 B
    0x3E, 0x41, 0x41, 0x41, 0x22, // 43 C
    0x7F, 0x41, 0x41, 0x22, 0x1C, // 44 D
    0x7F, 0x49, 0x49, 0x49, 0x41, // 45 E
    0x7F, 0x09, 0x09, 0x01, 0x01, // 46 F
    0x3E, 0x41, 0x41, 0x51, 0x32, // 47 G
    0x7F, 0x08, 0x08, 0x08, 0x7F, // 48 H
    0x00, 0x41, 0x7F, 0x41, 0x00, // 49 I
    0x20, 0x40, 0x41, 0x3F, 0x01, // 4A J
    0x7F, 0x08, 0x14, 0x22, 0x41, // 4B K
    0x7F, 0x40, 0x40, 0x40, 0x40, // 4C L
    0x7F, 0x02, 0x04, 0x02, 0x7F, // 4D M
    0x7F, 0x04, 0x08, 0x10, 0x7F, // 4E N
    0x3E, 0x41, 0x41, 0x41, 0x3E, // 4F O
    0x7F, 0x09, 0x09, 0x09, 0x06, // 50 P
    0x3E, 0x41, 0x51, 0x21, 0x5E, // 51 Q
    0x7F, 0x09, 0x19, 0x29, 0x46, // 52 R
    0x46, 0x49, 0x49, 0x49, 0x31, // 53 S
    0x01, 0x01, 0x7F, 0x01, 0x01, // 54 T
    0x3F, 0x40, 0x40, 0x40, 0x3F, // 55 U
    0x1F, 0x20, 0x40, 0x20, 0x1F, // 56 V
    0x7F, 0x20, 0x18, 0x20, 0x7F, // 57 W
    0x63, 0x14, 0x08, 0x14, 0x63, // 58 X
    0x03, 0x04, 0x78, 0x04, 0x03, // 59 Y
    0x61, 0x51, 0x49, 0x45, 0x43, // 5A Z
    0x00, 0x00, 0x7F, 0x41, 0x41, // 5B [
    0x02, 0x04, 0x08, 0x10, 0x20, // 5C backslash
    0x41, 0x41, 0x7F, 0x00, 0x00, // 5D ]
    0x04, 0x02, 0x01, 0x02, 0x04, // 5E ^
    0x40, 0x40, 0x40, 0x40, 0x40, // 5F _
    0x00, 0x01, 0x02, 0x04, 0x00, // 60 `
    0x20, 0x54, 0x54, 0x54, 0x78, // 61 a
    0x7F, 0x48, 0x44, 0x44, 0x38, // 62 b
    0x38, 0x44, 0x44, 0x44, 0x20, // 63 c
    0x38, 0x44, 0x44, 0x48, 0x7F, // 64 d
    0x38, 0x54, 0x54, 0x54, 0x18, // 65 e
    0x08, 0x7E, 0x09, 0x01, 0x02, // 66 f
    0x08, 0x14, 0x54, 0x54, 0x3C, // 67 g
    0x7F, 0x08, 0x04, 0x04, 0x78, // 68 h
    0x00, 0x44, 0x7D, 0x40, 0x00, // 69 i
    0x20, 0x40, 0x44, 0x3D, 0x00, // 6A j
    0x00, 0x7F, 0x10, 0x28, 0x44, // 6B k
    0x00, 0x41, 0x7F, 0x40, 0x00, // 6C l
    0x7C, 0x04, 0x18, 0x04, 0x78, // 6D m
    0x7C, 0x08, 0x04, 0x04, 0x78, // 6E n
    0x38, 0x44, 0x44, 0x44, 0x38, // 6F o
    0x7C, 0x14, 0x14, 0x14, 0x08, // 70 p
    0x08, 0x14, 0x14, 0x18, 0x7C, // 71 q
    0x7C, 0x08, 0x04, 0x04, 0x08, // 72 r
    0x48, 0x54, 0x54, 0x54, 0x20, // 73 s
    0x04, 0x3F, 0x44, 0x40, 0x20, // 74 t
    0x3C, 0x40, 0x40, 0x20, 0x7C, // 75 u
    0x1C, 0x20, 0x40, 0x20, 0x1C, // 76 v
    0x3C, 0x40, 0x30, 0x40, 0x3C, // 77 w
    0x44, 0x28, 0x10, 0x28, 0x44, // 78 x
    0x0C, 0x50, 0x50, 0x50, 0x3C, // 79 y
    0x44, 0x64, 0x54, 0x4C, 0x44, // 7A z
    0x00, 0x08, 0x36, 0x41, 0x00, // 7B {
    0x00, 0x00, 0x7F, 0x00, 0x00, // 7C |
    0x00, 0x41, 0x36, 0x08, 0x00, // 7D }
    0x08, 0x08, 0x2A, 0x1C, 0x08, // 7E ->
    0x08, 0x1C, 0x2A, 0x08, 0x08  // 7F <-
};

// ============================================================================
// Constructor / Destructor
// ============================================================================

Ssd1306_t::Ssd1306_t(uint8_t i2c_addr)
    : m_i2c_addr(i2c_addr)
    , m_framebuffer(128 * 64 / 8, 0)
{
    m_width = 128;
    m_height = 64;
}

Ssd1306_t::~Ssd1306_t() {
    if (m_initialized) {
        displayOff();
    }
}

// ============================================================================
// Inicialización
// ============================================================================

bool Ssd1306_t::init() {
    try {
        m_i2c = std::make_unique<hal::I2c_t>(m_i2c_addr);
    } catch (const std::exception& e) {
        return false;
    }

    // Secuencia de inicialización SSD1306
    sendCommand(0xAE); // Display off
    sendCommand(0xD5); // Set display clock divide ratio
    sendCommand(0x80);
    sendCommand(0xA8); // Set multiplex ratio
    sendCommand(0x3F); // 64 lines
    sendCommand(0xD3); // Set display offset
    sendCommand(0x00);
    sendCommand(0x40); // Set start line
    sendCommand(0x8D); // Charge pump
    sendCommand(0x14); // Enable
    sendCommand(0x20); // Memory addressing mode
    sendCommand(0x00); // Horizontal
    sendCommand(0xA1); // Segment remap (column 127 mapped to SEG0)
    sendCommand(0xC8); // COM output scan direction (remapped mode)
    sendCommand(0xDA); // COM pins hardware configuration
    sendCommand(0x12);
    sendCommand(0x81); // Contrast
    sendCommand(0xCF); // 207
    sendCommand(0xD9); // Pre-charge period
    sendCommand(0xF1);
    sendCommand(0xDB); // VCOMH deselect level
    sendCommand(0x40);
    sendCommand(0xA4); // Display all on resume
    sendCommand(0xA6); // Normal display
    sendCommand(0x2E); // Deactivate scrolling
    sendCommand(0xAF); // Display on

    clear();
    update();
    m_initialized = true;
    return true;
}

// ============================================================================
// Control
// ============================================================================

void Ssd1306_t::clear() {
    std::fill(m_framebuffer.begin(), m_framebuffer.end(), 0);
}

void Ssd1306_t::update() {
    for (uint8_t page = 0; page < 8; page++) {
        sendCommand(0xB0 + page);  // Set page
        sendCommand(0x00);          // Lower column start
        sendCommand(0x10);          // Upper column start
        sendData(m_framebuffer.data() + page * 128, 128);
    }
}

void Ssd1306_t::setContrast(uint8_t level) {
    sendCommand(0x81);
    sendCommand(level);
}

void Ssd1306_t::displayOff() {
    sendCommand(0xAE);
}

void Ssd1306_t::displayOn() {
    sendCommand(0xAF);
}

// ============================================================================
// Primitivas gráficas
// ============================================================================

void Ssd1306_t::drawPixel(int16_t x, int16_t y, bool color) {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) return;

    uint16_t idx = x + (y / 8) * m_width;
    if (color) {
        m_framebuffer[idx] |= (1 << (y & 7));
    } else {
        m_framebuffer[idx] &= ~(1 << (y & 7));
    }
}

void Ssd1306_t::drawPixel(int16_t x, int16_t y, ColorRgb565 color) {
    // Monocromo: usa solo el bit de brillo
    drawPixel(x, y, color.raw > 0);
}

void Ssd1306_t::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool color) {
    // Algoritmo de Bresenham
    int16_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int16_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy, e2;

    while (true) {
        drawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void Ssd1306_t::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, bool color) {
    drawLine(x, y, x + w - 1, y, color);
    drawLine(x, y + h - 1, x + w - 1, y + h - 1, color);
    drawLine(x, y, x, y + h - 1, color);
    drawLine(x + w - 1, y, x + w - 1, y + h - 1, color);
}

void Ssd1306_t::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, bool color) {
    for (int16_t i = y; i < y + h; i++) {
        drawLine(x, i, x + w - 1, i, color);
    }
}

void Ssd1306_t::drawCircle(int16_t x0, int16_t y0, int16_t r, bool color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    drawPixel(x0, y0 + r, color);
    drawPixel(x0, y0 - r, color);
    drawPixel(x0 + r, y0, color);
    drawPixel(x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        drawPixel(x0 + x, y0 + y, color);
        drawPixel(x0 - x, y0 + y, color);
        drawPixel(x0 + x, y0 - y, color);
        drawPixel(x0 - x, y0 - y, color);
        drawPixel(x0 + y, y0 + x, color);
        drawPixel(x0 - y, y0 + x, color);
        drawPixel(x0 + y, y0 - x, color);
        drawPixel(x0 - y, y0 - x, color);
    }
}

// ============================================================================
// Texto
// ============================================================================

void Ssd1306_t::drawChar5x7(int16_t x, int16_t y, unsigned char c, bool color) {
    if (c < 32 || c > 127) c = 32;
    c -= 32;

    for (int8_t i = 0; i < 5; i++) {
        uint8_t line = FONT5x7[c * 5 + i];
        for (int8_t j = 0; j < 7; j++) {
            if (line & (1 << j)) {
                drawPixel(x + i, y + j, color);
            }
        }
    }
}

void Ssd1306_t::drawString(int16_t x, int16_t y, std::string_view str,
                            uint8_t size, bool color) {
    int16_t cursor_x = x;
    for (char c : str) {
        if (c == '\n') {
            cursor_x = x;
            y += 7 * size + 1;
            continue;
        }
        // Dibujar carácter escalado
        for (int8_t col = 0; col < 5; col++) {
            uint8_t line = FONT5x7[(c - 32) * 5 + col];
            for (int8_t row = 0; row < 7; row++) {
                if (line & (1 << row)) {
                    if (size == 1) {
                        drawPixel(cursor_x + col, y + row, color);
                    } else {
                        fillRect(cursor_x + col * size, y + row * size,
                                 size, size, color);
                    }
                }
            }
        }
        cursor_x += (5 + 1) * size; // 5 width + 1 spacing
    }
}

// ============================================================================
// Bitmap
// ============================================================================

void Ssd1306_t::drawBitmap(int16_t x, int16_t y, int16_t w, int16_t h,
                            const uint8_t* data) {
    for (int16_t j = 0; j < h; j++) {
        for (int16_t i = 0; i < w; i++) {
            uint16_t byte_idx = j * ((w + 7) / 8) + (i / 8);
            bool pixel = data[byte_idx] & (0x80 >> (i & 7));
            drawPixel(x + i, y + j, pixel);
        }
    }
}

// ============================================================================
// QR
// ============================================================================

void Ssd1306_t::drawQr(const uint8_t* data, int qr_width,
                        int x, int y, int scale) {
    for (int row = 0; row < qr_width; row++) {
        for (int col = 0; col < qr_width; col++) {
            bool pixel = data[row * qr_width + col] & 1;
            int px = x + col * scale;
            int py = y + row * scale;
            if (scale == 1) {
                drawPixel(px, py, pixel);
            } else {
                fillRect(px, py, scale, scale, pixel);
            }
        }
    }
}

// ============================================================================
// Pantalla de inicio
// ============================================================================

void Ssd1306_t::showInitScreen() {
    clear();
    drawString(16, 0, "MRF24J40", 2, true);
    drawString(8, 20, "Raspberry Pi", 1, true);
    drawRect(10, 18, 108, 10, true);
    drawString(8, 40, "Inicializando...", 1, true);
    update();
}

// ============================================================================
// Comandos I2C
// ============================================================================

void Ssd1306_t::sendCommand(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};
    m_i2c->write(buf, 2);
}

void Ssd1306_t::sendCommands(const std::initializer_list<uint8_t>& cmds) {
    for (auto cmd : cmds) {
        sendCommand(cmd);
    }
}

void Ssd1306_t::sendData(const uint8_t* data, size_t len) {
    // Máximo 128 bytes por transferencia
    const size_t chunk = 128;
    for (size_t i = 0; i < len; i += chunk) {
        size_t count = std::min(chunk, len - i);
        std::vector<uint8_t> buf(count + 1);
        buf[0] = 0x40; // Co=0, D/C#=1 (data)
        std::memcpy(buf.data() + 1, data + i, count);
        m_i2c->write(buf);
    }
}

} // namespace drivers
