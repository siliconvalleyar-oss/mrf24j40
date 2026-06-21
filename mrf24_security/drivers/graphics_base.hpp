/**
 * @file    drivers/graphics_base.hpp
 * @brief   Clase base abstracta para controladores de display
 * @details Define la interfaz común para todos los displays (OLED, TFT).
 *          Los drivers concretos (Ssd1306_t, St7789_t) heredan de esta
 *          clase e implementan los métodos virtuales.
 *
 * @author  MRF24J40 Team
 * @date    2026
 * @version 2.0.0
 */

#ifndef DRIVERS_GRAPHICS_BASE_HPP
#define DRIVERS_GRAPHICS_BASE_HPP

#include <cstdint>
#include <string_view>

namespace drivers {

/**
 * @brief Estructura de color RGB565 (16 bits) para displays a color.
 */
struct ColorRgb565 {
    uint16_t raw;  ///< Valor RGB565: R[4:0], G[5:0], B[4:0]

    constexpr ColorRgb565() : raw(0) {}
    constexpr ColorRgb565(uint16_t v) : raw(v) {}

    /** @brief Constructor from R,G,B (0-255 each) */
    constexpr ColorRgb565(uint8_t r, uint8_t g, uint8_t b)
        : raw(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))
    {}

    constexpr uint8_t r() const { return (raw >> 11) << 3; }
    constexpr uint8_t g() const { return ((raw >> 5) & 0x3F) << 2; }
    constexpr uint8_t b() const { return (raw & 0x1F) << 3; }

    static constexpr ColorRgb565 Black()      { return ColorRgb565(0, 0, 0); }
    static constexpr ColorRgb565 White()      { return ColorRgb565(255, 255, 255); }
    static constexpr ColorRgb565 Red()        { return ColorRgb565(255, 0, 0); }
    static constexpr ColorRgb565 Green()      { return ColorRgb565(0, 255, 0); }
    static constexpr ColorRgb565 Blue()       { return ColorRgb565(0, 0, 255); }
    static constexpr ColorRgb565 Yellow()     { return ColorRgb565(255, 255, 0); }
    static constexpr ColorRgb565 Cyan()       { return ColorRgb565(0, 255, 255); }
    static constexpr ColorRgb565 Magenta()    { return ColorRgb565(255, 0, 255); }
    static constexpr ColorRgb565 Orange()     { return ColorRgb565(255, 165, 0); }
};

/**
 * @brief Clase base abstracta para controladores de display.
 *
 * Define la interfaz gráfica mínima que todos los displays deben
 * implementar: dibujo de píxeles, líneas, rectángulos, círculos,
 * texto y control de framebuffer.
 */
class GraphicsBase {
public:
    virtual ~GraphicsBase() = default;

    // === Inicialización ===

    /** @brief Inicializa el display. @return true si éxito. */
    virtual bool init() = 0;

    // === Control de framebuffer ===

    /** @brief Limpia el display (todo a negro/fondo). */
    virtual void clear() = 0;

    /**
     * @brief Envía el framebuffer al display.
     * Debe llamarse después de dibujar para que los cambios se vean.
     */
    virtual void update() = 0;

    /**
     * @brief Configura el contraste del display.
     * @param level Nivel de contraste (0-255).
     */
    virtual void setContrast(uint8_t level) = 0;

    /** @brief Apaga el display (modo sleep). */
    virtual void displayOff() = 0;

    /** @brief Enciende el display (sale de sleep). */
    virtual void displayOn() = 0;

    // === Primitivas gráficas ===

    /**
     * @brief Dibuja un píxel en coordenadas (x,y).
     * @param x Columna (0 = izquierda).
     * @param y Fila (0 = arriba).
     * @param color Color del píxel (mono: true=blanco, color: RGB565).
     */
    virtual void drawPixel(int16_t x, int16_t y, bool color) = 0;

    /// @brief Sobrecarga para displays a color.
    virtual void drawPixel(int16_t x, int16_t y, ColorRgb565 color) = 0;

    /**
     * @brief Dibuja una línea entre (x0,y0) y (x1,y1) usando Bresenham.
     */
    virtual void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool color) = 0;

    /**
     * @brief Dibuja un rectángulo sin relleno.
     */
    virtual void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, bool color) = 0;

    /**
     * @brief Dibuja un rectángulo relleno.
     */
    virtual void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, bool color) = 0;

    /**
     * @brief Dibuja un círculo sin relleno.
     */
    virtual void drawCircle(int16_t x0, int16_t y0, int16_t r, bool color) = 0;

    // === Texto ===

    /**
     * @brief Dibuja una cadena de texto en el display.
     * @param x   Columna inicial.
     * @param y   Fila inicial.
     * @param str Texto a dibujar.
     * @param size Factor de escala (1 = 5x7, 2 = 10x14, etc.).
     * @param color Color del texto.
     */
    virtual void drawString(int16_t x, int16_t y, std::string_view str,
                            uint8_t size = 1, bool color = true) = 0;

    // === Metadatos ===

    /** @return Ancho del display en píxeles. */
    virtual int16_t width() const = 0;

    /** @return Alto del display en píxeles. */
    virtual int16_t height() const = 0;

    /** @return true si el display es a color. */
    virtual bool isColor() const = 0;

protected:
    int16_t m_width = 128;   ///< Ancho por defecto
    int16_t m_height = 64;   ///< Alto por defecto
    bool m_initialized = false;
};

} // namespace drivers

#endif // DRIVERS_GRAPHICS_BASE_HPP
