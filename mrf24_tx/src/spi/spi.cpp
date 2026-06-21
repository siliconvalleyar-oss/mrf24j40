/**
 * @file    spi.cpp
 * @brief   Implementación del driver de comunicación SPI
 * @details Soporta dos modos de operación seleccionables por define:
 *          - BCM2835: usa la librería bcm2835 para SPI (Raspberry Pi nativo)
 *          - ioctl: usa el dispositivo /dev/spidev0.0 directamente
 *          La selección se controla mediante #define SPI_BCM2835 en config.hpp.
 *
 * @namespace SPI
 */

#include <spi/spi.hpp>
#include <config/config.hpp>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef SPI_BCM2835
#include <bcm2835.h>
#else
  #include <linux/ioctl.h>
  #include <linux/types.h>
  #include <linux/spi/spidev.h>
#endif

// ============================================================================
// Modo BCM2835 (librería nativa de Raspberry Pi)
// ============================================================================
#ifdef SPI_BCM2835

namespace SPI {

Spi_t::Spi_t()
    : m_spi_speed(SPI_SPEED)
{
    init();
}

void Spi_t::settings_spi()
{
    /**
     * @brief Configura los parámetros de la interfaz SPI
     * - MSB first (bit order)
     * - SPI Mode 0 (CPOL=0, CPHA=0)
     * - Clock divider (velocidad)
     * - Chip Select 0, polaridad LOW
     */
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_256);
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);
}

void Spi_t::init()
{
    /**
     * @brief Inicializa la librería BCM2835 y la interfaz SPI
     * @note Requiere permisos de root (sudo) para acceder a /dev/mem
     */
    if (!bcm2835_init()) {
        std::cerr << "Error al inicializar bcm2835" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (!bcm2835_spi_begin()) {
        fprintf(stderr, "No se pudo inicializar SPI\n");
        bcm2835_close();
        return;
    }
    settings_spi();
}

uint8_t Spi_t::Transfer1bytes(const uint8_t cmd)
{
    /**
     * @brief Transfiere 1 byte por SPI
     * @param cmd Byte de comando a enviar
     * @return Byte recibido del esclavo
     */
    bcm2835_spi_transfer(cmd);
    return m_rx_buffer[0];
}

uint8_t Spi_t::Transfer2bytes(const uint16_t cmd)
{
    /**
     * @brief Transfiere 2 bytes por SPI (acceso a registros short address)
     * @param cmd Comando de 16 bits (dirección + datos)
     * @return Segundo byte recibido (dato del registro)
     */
    uint8_t buffer[2] = {
        static_cast<uint8_t>(cmd & 0xFF),
        static_cast<uint8_t>(cmd >> 8)
    };
    bcm2835_spi_transfern(reinterpret_cast<char*>(buffer), 2);
    return buffer[1];
}

uint8_t Spi_t::Transfer3bytes(const uint32_t cmd)
{
    /**
     * @brief Transfiere 3 bytes por SPI (acceso a registros long address)
     * @param cmd Comando de 24 bits (dirección larga + datos)
     * @return Tercer byte recibido (dato del registro)
     */
    uint8_t buffer[3] = {
        static_cast<uint8_t>(cmd & 0xFF),
        static_cast<uint8_t>((cmd >> 8) & 0xFF),
        static_cast<uint8_t>((cmd >> 16) & 0xFF)
    };
    bcm2835_spi_transfern(reinterpret_cast<char*>(buffer), 3);
    return buffer[2];
}

void Spi_t::spi_close()
{
    /** @brief Cierra la interfaz SPI y libera recursos BCM2835 */
    bcm2835_spi_end();
    bcm2835_close();
}

Spi_t::~Spi_t()
{
    spi_close();
}

} // namespace SPI

// ============================================================================
// Modo ioctl directo (sin BCM2835, usa /dev/spidev0.0)
// ============================================================================
#else

#include <cstring>

/** @brief Ruta del dispositivo SPI en Raspberry Pi */
#define SPI_DEVICE  "/dev/spidev0.0"

namespace SPI {

void Spi_t::settings_spi()
{
    /**
     * @brief Configura la estructura de transferencia SPI
     * Define tx_buf, rx_buf, bits_per_word, speed, delay y longitud (3 bytes)
     */
    spi->tx_buf = (unsigned long)m_tx_buffer;
    spi->rx_buf = (unsigned long)m_rx_buffer;
    spi->bits_per_word = 0;
    spi->speed_hz = m_spi_speed;
    spi->delay_usecs = 1;
    spi->len = 3;

    std::memset(m_tx_buffer, 0x00, sizeof(m_tx_buffer));
    std::memset(m_rx_buffer, 0xff, sizeof(m_rx_buffer));
}

void Spi_t::init()
{
    /**
     * @brief Inicializa la interfaz SPI vía ioctl
     * Abre /dev/spidev0.0, configura modo SPI 0 y velocidad
     */
    m_fs = open(SPI_DEVICE, O_RDWR);
    if (m_fs < 0) {
        msj_fail();
        exit(EXIT_FAILURE);
    }
    m_ret = ioctl(m_fs, SPI_IOC_RD_MODE, &scratch32);
    if (m_ret != 0) {
        msj_fail();
        if (m_fs) close(m_fs);
        exit(EXIT_FAILURE);
    }
    scratch32 |= SPI_MODE_0;
    m_ret = ioctl(m_fs, SPI_IOC_WR_MODE, &scratch32);
    if (m_ret != 0) {
        msj_fail();
        close(m_fs);
        exit(EXIT_FAILURE);
    }
    m_ret = ioctl(m_fs, SPI_IOC_RD_MAX_SPEED_HZ, &scratch32);
    if (m_ret != 0) {
        close(m_fs);
        exit(EXIT_FAILURE);
    }
    scratch32 = m_spi_speed;
    m_ret = ioctl(m_fs, SPI_IOC_WR_MAX_SPEED_HZ, &scratch32);
    if (m_ret != 0) {
        msj_fail();
        close(m_fs);
        exit(EXIT_FAILURE);
    }
}

uint32_t Spi_t::get_spi_speed()
{
    /** @return Velocidad SPI configurada en Hz */
    return m_spi_speed;
}

uint8_t Spi_t::Transfer1bytes(const uint8_t cmd)
{
    /**
     * @brief Transfiere 1 byte por SPI
     * @param cmd Byte de comando a enviar
     * @return 0 si éxito, -1 si error
     */
    if (m_fs < 0) {
        std::cerr << "SPI device not open." << std::endl;
        return -1;
    }
    std::memset(m_rx_buffer, 0xff, sizeof(m_tx_buffer));
    std::memset(m_tx_buffer, 0xff, sizeof(m_rx_buffer));
    std::memset(spi.get(), 0, sizeof(struct spi_ioc_transfer));

    spi->len = 1;
    m_tx_buffer[0] = cmd;
    spi->tx_buf = reinterpret_cast<unsigned long>(m_tx_buffer);
    spi->rx_buf = reinterpret_cast<unsigned long>(m_rx_buffer);
    spi->speed_hz = get_spi_speed();
    spi->bits_per_word = 8;
    spi->cs_change = 0;
    spi->delay_usecs = 0;

    int ret = ioctl(m_fs, SPI_IOC_MESSAGE(1), spi.get());
    if (ret < 0) {
        std::cerr << "Error en Transfer1bytes: " << strerror(errno) << std::endl;
        return -1;
    }
    return 0;
}

uint8_t Spi_t::Transfer2bytes(const uint16_t cmd)
{
    /**
     * @brief Transfiere 2 bytes por SPI
     * @param cmd Comando de 16 bits
     * @return Segundo byte del buffer RX
     */
    spi->len = sizeof(cmd);
    m_rx_buffer[0] = m_rx_buffer[1] = 0xff;
    m_rx_buffer[2] = m_rx_buffer[3] = 0x00;
    memcpy(m_tx_buffer, &cmd, sizeof(cmd));
    m_ret = ioctl(m_fs, SPI_IOC_MESSAGE(1), spi.get());
    if ((cmd >> 8 & 0xff) == 0x00)
        printDBGSpi();
    return m_rx_buffer[1];
}

uint8_t Spi_t::Transfer3bytes(const uint32_t cmd)
{
    /**
     * @brief Transfiere 3 bytes por SPI
     * @param cmd Comando de 24 bits
     * @return Tercer byte del buffer RX
     */
    spi->len = 3;
    m_rx_buffer[0] = m_rx_buffer[1] = m_rx_buffer[2] == 0xff;
    m_rx_buffer[3] = 0x00;
    memcpy(m_tx_buffer, &cmd, sizeof(cmd));
    m_ret = ioctl(m_fs, SPI_IOC_MESSAGE(1), spi.get());
    if ((cmd >> 16 & 0xff) == 0x00)
        printDBGSpi();
    return m_rx_buffer[2];
}

void Spi_t::spi_close()
{
    /** @brief Cierra el file descriptor del dispositivo SPI */
    if (m_fs) close(m_fs);
}

Spi_t::Spi_t()
    : m_tx_buffer{0x00},
      m_rx_buffer{0x00},
      m_spi_speed{SPI_SPEED},
      spi{std::make_unique<struct spi_ioc_transfer>()}
{
    #ifdef DBG_SPI
        std::cout << "Spi()\n";
    #endif
    settings_spi();
    init();
}

Spi_t::~Spi_t()
{
    spi_close();
    #ifdef DBG_SPI
        std::cout << "~Spi()\n";
    #endif
    exit(EXIT_SUCCESS);
}

} // namespace SPI

#endif // SPI_BCM2835
