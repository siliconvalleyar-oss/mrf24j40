/**
 * @file    drivers/qr.cpp
 * @brief   Implementación del generador y renderizador de QR
 *
 * @author  MRF24J40 Team
 * @date    2026
 */

#include <drivers/qr.hpp>
#include <qrencode.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <memory>
#include <algorithm>

// Para exportar a PNG
extern "C" {
#include <png.h>
#include <zlib.h>
}

namespace drivers {

// ============================================================================
// QrDeleter
// ============================================================================

void QrDeleter::operator()(QrCode* qr) const {
    if (qr) {
        QRcode_free(reinterpret_cast<QRcode*>(qr));
    }
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

Qr_t::Qr_t() = default;
Qr_t::~Qr_t() = default;

// ============================================================================
// Generación del código QR
// ============================================================================

std::unique_ptr<QrCode, QrDeleter> Qr_t::generate(std::string_view text,
                                                    QrEccLevel ecc) {
    if (text.empty()) return nullptr;

    QRecLevel qr_ecc;
    switch (ecc) {
        case QrEccLevel::Low:     qr_ecc = QR_ECLEVEL_L; break;
        case QrEccLevel::Medium:  qr_ecc = QR_ECLEVEL_M; break;
        case QrEccLevel::Quartile: qr_ecc = QR_ECLEVEL_Q; break;
        case QrEccLevel::High:    qr_ecc = QR_ECLEVEL_H; break;
        default:                  qr_ecc = QR_ECLEVEL_L; break;
    }

    QRcode* qr = QRcode_encodeString(text.data(), 0, qr_ecc, QR_MODE_8, 1);
    if (!qr) return nullptr;

    return std::unique_ptr<QrCode, QrDeleter>(
        reinterpret_cast<QrCode*>(qr));
}

// ============================================================================
// Renderizado a consola
// ============================================================================

void Qr_t::printToConsole(const uint8_t* data, int width) {
    // Línea superior
    std::cout << "\n";
    for (int i = 0; i < width + 2; i++) std::cout << "██";
    std::cout << "\n";

    for (int y = 0; y < width; y++) {
        std::cout << "██"; // Borde izquierdo
        for (int x = 0; x < width; x++) {
            std::cout << (data[y * width + x] & 1 ? "  " : "██");
        }
        std::cout << "██\n"; // Borde derecho
    }

    for (int i = 0; i < width + 2; i++) std::cout << "██";
    std::cout << "\n\n";
}

// ============================================================================
// Renderizado a OLED (SSD1306)
// ============================================================================

void Qr_t::renderToOled(Ssd1306_t& display, const uint8_t* data,
                         int qr_width, int x, int y, int scale) {
    display.drawQr(data, qr_width, x, y, scale);
}

// ============================================================================
// Renderizado a TFT (ST7789)
// ============================================================================

void Qr_t::renderToTft(St7789_t& display, const uint8_t* data,
                        int qr_width, int x, int y, int scale,
                        ColorRgb565 fg, ColorRgb565 bg) {
    display.drawQr(data, qr_width, x, y, scale, fg, bg);
}

// ============================================================================
// Exportar a PNG
// ============================================================================

bool Qr_t::saveToPng(const std::string_view filename,
                      const uint8_t* data, int qr_width,
                      int scale) {
    const int border = 10;
    const int img_size = qr_width * scale + 2 * border;

    std::ofstream file(filename.data(), std::ios::binary);
    if (!file.is_open()) return false;

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                               nullptr, nullptr, nullptr);
    if (!png) return false;

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_write_struct(&png, nullptr);
        return false;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        return false;
    }

    // Callback de escritura
    png_set_write_fn(png, &file,
        [](png_structp p, png_bytep d, png_size_t l) {
            auto* f = static_cast<std::ostream*>(png_get_io_ptr(p));
            f->write(reinterpret_cast<char*>(d), l);
        }, nullptr);

    png_set_IHDR(png, info, img_size, img_size, 8,
                 PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png, info);

    auto row = std::make_unique<png_byte[]>(img_size);
    std::memset(row.get(), 255, img_size);

    // Borde superior blanco
    for (int i = 0; i < border; i++) {
        png_write_row(png, row.get());
    }

    for (int qy = 0; qy < qr_width; qy++) {
        for (int sy = 0; sy < scale; sy++) {
            // Borde izquierdo blanco
            std::memset(row.get(), 255, border);
            for (int qx = 0; qx < qr_width; qx++) {
                uint8_t pixel = (data[qy * qr_width + qx] & 1) ? 0 : 255;
                for (int sx = 0; sx < scale; sx++) {
                    row[border + qx * scale + sx] = pixel;
                }
            }
            // Borde derecho blanco
            std::memset(row.get() + border + qr_width * scale, 255, border);
            png_write_row(png, row.get());
        }
    }

    // Borde inferior blanco
    std::memset(row.get(), 255, img_size);
    for (int i = 0; i < border; i++) {
        png_write_row(png, row.get());
    }

    png_write_end(png, info);
    png_destroy_write_struct(&png, &info);

    return true;
}

// ============================================================================
// Conveniencia: generar y mostrar
// ============================================================================

void Qr_t::generateAndPrint(std::string_view text) {
    auto qr = generate(text);
    if (!qr) {
        std::cerr << "Error generating QR code\n";
        return;
    }
    const uint8_t* data = reinterpret_cast<const uint8_t*>(qr->data);
    printToConsole(data, qr->width);
}

bool Qr_t::generateAndSave(std::string_view text,
                            std::string_view filename, int scale) {
    auto qr = generate(text);
    if (!qr) return false;

    return saveToPng(filename,
                     reinterpret_cast<const uint8_t*>(qr->data),
                     qr->width, scale);
}

std::pair<std::vector<uint8_t>, int> Qr_t::generateMatrix(std::string_view text) {
    auto qr = generate(text);
    if (!qr) return {};

    std::vector<uint8_t> matrix(qr->width * qr->width);
    std::memcpy(matrix.data(), qr->data, matrix.size());
    return {std::move(matrix), qr->width};
}

} // namespace drivers
