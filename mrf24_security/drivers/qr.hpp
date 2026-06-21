/**
 * @file    drivers/qr.hpp
 * @brief   Generación y renderizado de códigos QR
 * @details Genera códigos QR desde texto usando libqrencode y los
 *          renderiza en consola, OLED (SSD1306), TFT (ST7789) y
 *          archivos PNG. Usa smart pointers para gestión de memoria.
 *
 * @author  MRF24J40 Team
 * @date    2026
 * @version 2.0.0
 */

#ifndef DRIVERS_QR_HPP
#define DRIVERS_QR_HPP

#include <drivers/graphics_base.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <utility>

namespace drivers {

// Forward declarations de displays
class Ssd1306_t;
class St7789_t;

/**
 * @brief Niveles de corrección de errores para QR.
 */
enum class QrEccLevel {
    Low      = 0, /**< ~7% de recuperación */
    Medium   = 1, /**< ~15% de recuperación */
    Quartile = 2, /**< ~25% de recuperación */
    High     = 3  /**< ~30% de recuperación */
};

/**
 * @brief Estructura interna que wrappea QRcode de libqrencode.
 */
struct QrCode {
    int     version;
    int     width;
    unsigned char* data;
};

/**
 * @brief Deleter personalizado para QRcode de libqrencode.
 */
struct QrDeleter {
    void operator()(QrCode* qr) const;
};

/**
 * @brief Servicio de generación y renderizado de códigos QR.
 *
 * Uso:
 * @code
 * auto qr = drivers::Qr_t();
 *
 * // Imprimir en consola
 * qr.generateAndPrint("Hola Mundo");
 *
 * // Guardar como PNG
 * qr.generateAndSave("Hola Mundo", "qr.png");
 *
 * // Renderizar en OLED
 * auto oled = drivers::Ssd1306_t();
 * auto qr_data = qr.generateMatrix("Hola Mundo");
 * qr.renderToOled(oled, qr_data.first.data(), qr_data.second, 0, 0, 1);
 * @endcode
 */
class Qr_t {
public:
    Qr_t();
    ~Qr_t();

    // No copiable
    Qr_t(const Qr_t&) = delete;
    Qr_t& operator=(const Qr_t&) = delete;

    /**
     * @brief Genera un código QR desde un texto.
     * @param text Texto a codificar.
     * @param ecc  Nivel de corrección de errores.
     * @return Puntero único al QR generado, o nullptr si error.
     */
    std::unique_ptr<QrCode, QrDeleter> generate(std::string_view text,
                                                  QrEccLevel ecc = QrEccLevel::Medium);

    /**
     * @brief Genera y devuelve la matriz de datos del QR.
     * @param text Texto a codificar.
     * @return Par (matrix, width).
     */
    std::pair<std::vector<uint8_t>, int> generateMatrix(std::string_view text);

    /**
     * @brief Renderiza el QR en consola (ASCII).
     * @param data  Datos de la matriz QR (width*width bytes).
     * @param width Ancho/alto de la matriz.
     */
    void printToConsole(const uint8_t* data, int width);

    /**
     * @brief Renderiza el QR en un display OLED SSD1306.
     * @param display  Referencia al display.
     * @param data     Datos de la matriz QR.
     * @param qr_width Ancho de la matriz.
     * @param x        Posición X.
     * @param y        Posición Y.
     * @param scale    Escala.
     */
    void renderToOled(Ssd1306_t& display, const uint8_t* data,
                      int qr_width, int x = 0, int y = 0, int scale = 1);

    /**
     * @brief Renderiza el QR en un display TFT ST7789.
     * @param display  Referencia al display.
     * @param data     Datos de la matriz QR.
     * @param qr_width Ancho de la matriz.
     * @param x        Posición X.
     * @param y        Posición Y.
     * @param scale    Escala.
     * @param fg       Color de los módulos (default: negro).
     * @param bg       Color del fondo (default: blanco).
     */
    void renderToTft(St7789_t& display, const uint8_t* data,
                     int qr_width, int x = 0, int y = 0, int scale = 2,
                     ColorRgb565 fg = ColorRgb565::Black(),
                     ColorRgb565 bg = ColorRgb565::White());

    /**
     * @brief Guarda el QR como imagen PNG.
     * @param filename Ruta del archivo PNG.
     * @param data     Datos de la matriz QR.
     * @param qr_width Ancho de la matriz.
     * @param scale    Escala (píxeles por módulo).
     * @return true si éxito.
     */
    bool saveToPng(std::string_view filename,
                   const uint8_t* data, int qr_width, int scale = 10);

    /**
     * @brief Genera y muestra un QR en consola (conveniencia).
     * @param text Texto a codificar.
     */
    void generateAndPrint(std::string_view text);

    /**
     * @brief Genera y guarda un QR como PNG (conveniencia).
     * @param text     Texto a codificar.
     * @param filename Ruta del archivo.
     * @param scale    Escala.
     * @return true si éxito.
     */
    bool generateAndSave(std::string_view text,
                          std::string_view filename, int scale = 10);
};

} // namespace drivers

#endif // DRIVERS_QR_HPP
