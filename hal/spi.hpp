/**
 * @file    hal/spi.hpp
 * @brief   Abstracción de SPI para Raspberry Pi
 * @details Soporta BCM2835 nativo e ioctl directo a /dev/spidev.
 *          Encapsula transferencias de 1, 2 y 3 bytes con gestión RAII.
 *
 * @author  MRF24J40 Team
 * @date    2026
 * @version 2.0.0
 */

#ifndef HAL_SPI_HPP
#define HAL_SPI_HPP

#include <cstdint>
#include <memory>
#include <vector>

namespace hal {

/**
 * @brief Modos de operación SPI.
 */
enum class SpiMode {
    Mode0 = 0, /**< CPOL=0, CPHA=0 */
    Mode1 = 1, /**< CPOL=0, CPHA=1 */
    Mode2 = 2, /**< CPOL=1, CPHA=0 */
    Mode3 = 3  /**< CPOL=1, CPHA=1 */
};

/**
 * @brief Configuración de la interfaz SPI.
 */
struct SpiConfig {
    uint8_t  cs_pin     = 0;     /**< Chip Select (0 o 1) */
    uint32_t speed_hz   = 1000000; /**< Velocidad en Hz */
    SpiMode  mode       = SpiMode::Mode0; /**< Modo SPI */
    uint8_t  bits_word  = 8;     /**< Bits por palabra */
};

/**
 * @brief Driver de alto nivel para comunicación SPI.
 *
 * Gestiona la apertura y cierre del dispositivo SPI con RAII.
 * Soporta transferencias de 1, 2 y 3 bytes, así como buffers
 * arbitrarios mediante transfer().
 *
 * Uso:
 * @code
 * auto spi = hal::Spi_t({.cs_pin=0, .speed_hz=1000000});
 * uint8_t rx = spi.transfer2(0x3E01);  // Leer registro
 * spi.transfer({0x01, 0xAA, 0xBB});     // Transferir buffer
 * @endcode
 */
class Spi_t {
public:
    /**
     * @brief Constructor: abre y configura el dispositivo SPI.
     * @param config Configuración de la interfaz SPI.
     * @throws std::runtime_error si no se puede abrir /dev/spidev.
     */
    explicit Spi_t(const SpiConfig& config = {});

    /// Destructor: cierra el dispositivo SPI.
    ~Spi_t();

    // No copiable
    Spi_t(const Spi_t&) = delete;
    Spi_t& operator=(const Spi_t&) = delete;

    // Movible
    Spi_t(Spi_t&&) noexcept;
    Spi_t& operator=(Spi_t&&) noexcept;

    /**
     * @brief Transfiere 1 byte por SPI.
     * @param tx Byte a enviar.
     * @return Byte recibido.
     */
    uint8_t transfer1(uint8_t tx);

    /**
     * @brief Transfiere 2 bytes por SPI (para registros short address).
     * @param tx Comando de 16 bits.
     * @return Segundo byte recibido.
     */
    uint8_t transfer2(uint16_t tx);

    /**
     * @brief Transfiere 3 bytes por SPI (para registros long address).
     * @param tx Comando de 24 bits (parte alta primero).
     * @return Tercer byte recibido.
     */
    uint8_t transfer3(uint32_t tx);

    /**
     * @brief Transfiere un buffer completo por SPI.
     * @param tx_buf Datos a enviar.
     * @param rx_buf Buffer para datos recibidos (opcional, nullptr = ignorar).
     * @param len    Número de bytes a transferir.
     * @return Número de bytes transferidos, o -1 si error.
     */
    int transfer(const uint8_t* tx_buf, uint8_t* rx_buf, size_t len);

    /// @brief Sobrecarga con vector para mayor comodidad.
    std::vector<uint8_t> transfer(const std::vector<uint8_t>& tx);

    /**
     * @brief Obtiene la velocidad SPI configurada.
     * @return Velocidad en Hz.
     */
    uint32_t speed() const { return m_config.speed_hz; }

private:
    int     m_fd;         ///< File descriptor del dispositivo SPI (-1 si no abierto)
    SpiConfig m_config;   ///< Configuración activa
    bool    m_use_bcm;    ///< true si usa BCM2835, false si usa ioctl

    /// Inicializa usando librería BCM2835
    void initBcm2835();

    /// Inicializa usando ioctl a /dev/spidev
    void initIoctl();
};

} // namespace hal

#endif // HAL_SPI_HPP
