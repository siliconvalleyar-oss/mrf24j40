/**
 * @file    drivers/st7789.hpp
 * @brief   Driver para display TFT ST7789 (SPI, 240x240)
 * @details Soporta colores RGB565, fuentes, formas geométricas,
 *          bitmaps y renderizado de QR a color. Hereda de GraphicsBase.
 *
 * @author  MRF24J40 Team
 * @date    2026
 * @version 2.0.0
 */

#ifndef DRIVERS_ST7789_HPP
#define DRIVERS_ST7789_HPP

#include <drivers/graphics_base.hpp>
#include <hal/spi.hpp>
#include <memory>
#include <string_view>
#include <vector>

namespace drivers {

/**
 * @brief Controlador para display TFT ST7789 de 240x240 (o 240x320).
 *
 * Comunicación SPI, colores RGB565, resolución configurable.
 */
class St7789_t : public GraphicsBase {
public:
    /**
     * @brief Constructor.
     * @param width  Ancho del display (default: 240).
     * @param height Alto del display (default: 240).
     */
    explicit St7789_t(int16_t width = 240, int16_t height = 240);

    ~St7789_t() override;

    // No copiable
    St7789_t(const St7789_t&) = delete;
    St7789_t& operator=(const St7789_t&) = delete;

    // === GraphicsBase ===

    bool init() override;
    void clear() override;
    void update() override;
    void setContrast(uint8_t level) override;
    void displayOff() override;
    void displayOn() override;

    void drawPixel(int16_t x, int16_t y, bool color) override;
    void drawPixel(int16_t x, int16_t y, ColorRgb565 color) override;

    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool color) override;
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, bool color) override;
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, bool color) override;
    void drawCircle(int16_t x0, int16_t y0, int16_t r, bool color) override;
    void drawString(int16_t x, int16_t y, std::string_view str,
                    uint8_t size = 1, bool color = true) override;

    int16_t width() const override { return m_width; }
    int16_t height() const override { return m_height; }
    bool isColor() const override { return true; }

    // === Específicos ST7789 ===

    /**
     * @brief Llena toda la pantalla con un color.
     * @param color Color RGB565.
     */
    void fillScreen(ColorRgb565 color);

    /**
     * @brief Dibuja un rectángulo relleno con color RGB565.
     */
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, ColorRgb565 color);

    /**
     * @brief Dibuja un bitmap en color RGB565.
     */
    void drawBitmap(int16_t x, int16_t y, int16_t w, int16_t h,
                    const uint16_t* data);

    /**
     * @brief Dibuja un QR en el display TFT.
     * @param data     Datos de la matriz QR.
     * @param qr_width Ancho/alto de la matriz.
     * @param x        Posición X.
     * @param y        Posición Y.
     * @param scale    Escala de cada módulo.
     * @param fg_color Color de los módulos (default: negro).
     * @param bg_color Color del fondo (default: blanco).
     */
    void drawQr(const uint8_t* data, int qr_width, int x, int y, int scale = 2,
                ColorRgb565 fg_color = ColorRgb565::Black(),
                ColorRgb565 bg_color = ColorRgb565::White());

private:
    std::unique_ptr<hal::Spi_t> m_spi;  ///< Bus SPI
    std::vector<uint8_t> m_framebuffer; ///< Framebuffer opcional
    int16_t m_dc_gpio;                  ///< GPIO para D/C#
    int16_t m_rst_gpio;                 ///< GPIO para RESET
    int16_t m_bl_gpio;                  ///< GPIO para backlight

    /// Envía un comando al display.
    void sendCommand(uint8_t cmd);

    /// Envía datos al display.
    void sendData(const uint8_t* data, size_t len);

    /// Envía un byte de datos.
    void sendDataByte(uint8_t data);

    /// Configura la ventana de escritura (para fillRect optimizado).
    void setWindow(int16_t x0, int16_t y0, int16_t x1, int16_t y1);

    /// Inicialización por hardware (reset).
    void hwReset();
};

} // namespace drivers

#endif // DRIVERS_ST7789_HPP
