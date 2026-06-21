/**
 * @file    drivers/ssd1306.hpp
 * @brief   Driver para display OLED SSD1306 (I2C)
 * @details Soporta múltiples fuentes (5x7, 8x16), gráficos básicos
 *          y renderizado de QR. Hereda de GraphicsBase.
 *
 * @author  MRF24J40 Team
 * @date    2026
 * @version 2.0.0
 */

#ifndef DRIVERS_SSD1306_HPP
#define DRIVERS_SSD1306_HPP

#include <drivers/graphics_base.hpp>
#include <hal/i2c.hpp>
#include <memory>
#include <string_view>
#include <vector>

namespace drivers {

/**
 * @brief Controlador para display OLED SSD1306 de 128x64.
 *
 * Comunicación I2C a dirección 0x3C (default).
 * Framebuffer de 1024 bytes (128*64/8).
 */
class Ssd1306_t : public GraphicsBase {
public:
    /**
     * @brief Constructor.
     * @param i2c_addr Dirección I2C del display (default: 0x3C).
     */
    explicit Ssd1306_t(uint8_t i2c_addr = 0x3C);

    ~Ssd1306_t() override;

    // No copiable
    Ssd1306_t(const Ssd1306_t&) = delete;
    Ssd1306_t& operator=(const Ssd1306_t&) = delete;

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

    int16_t width() const override { return 128; }
    int16_t height() const override { return 64; }
    bool isColor() const override { return false; }

    // === Específicos SSD1306 ===

    /** @brief Muestra una pantalla de inicio animada. */
    void showInitScreen();

    /**
     * @brief Dibuja un bitmap monocromático.
     * @param x      Posición X.
     * @param y      Posición Y.
     * @param w      Ancho del bitmap.
     * @param h      Alto del bitmap.
     * @param data   Datos del bitmap (1 bit por píxel).
     */
    void drawBitmap(int16_t x, int16_t y, int16_t w, int16_t h,
                    const uint8_t* data);

    /**
     * @brief Dibuja un QR en el display.
     * @param data   Datos de la matriz QR (width*width bytes).
     * @param width  Ancho/alto de la matriz QR.
     * @param x      Posición X en el display.
     * @param y      Posición Y en el display.
     * @param scale  Escala (cada módulo QR ocupa scale x scale píxeles).
     */
    void drawQr(const uint8_t* data, int width, int x, int y, int scale = 1);

private:
    std::unique_ptr<hal::I2c_t> m_i2c;          ///< Bus I2C
    std::vector<uint8_t> m_framebuffer;          ///< Framebuffer de 1024 bytes
    uint8_t m_i2c_addr;                          ///< Dirección I2C

    /// Envía un comando al display.
    void sendCommand(uint8_t cmd);

    /// Envía múltiples comandos.
    void sendCommands(const std::initializer_list<uint8_t>& cmds);

    /// Envía datos al display.
    void sendData(const uint8_t* data, size_t len);

    /// Dibuja un carácter (fuente 5x7).
    void drawChar5x7(int16_t x, int16_t y, unsigned char c, bool color);
};

} // namespace drivers

#endif // DRIVERS_SSD1306_HPP
