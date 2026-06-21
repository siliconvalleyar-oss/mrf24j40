/**
 * @file    hal/spi.cpp
 * @brief   Implementación del driver SPI con soporte dual BCM2835 / ioctl
 *
 * @author  MRF24J40 Team
 * @date    2026
 */

#include <hal/spi.hpp>
#include <stdexcept>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

// ============================================================================
// Selección de backend SPI
// ============================================================================
// Por defecto usa ioctl a /dev/spidev.
// Para usar BCM2835 nativo, compilar con -DUSE_BCM2835_SPI

#ifdef USE_BCM2835_SPI
#include <bcm2835.h>
#endif

namespace hal {

// ============================================================================
// Constructor / Destructor
// ============================================================================

Spi_t::Spi_t(const SpiConfig& config)
    : m_fd(-1)
    , m_config(config)
    , m_use_bcm(false)
{
#ifdef USE_BCM2835_SPI
    initBcm2835();
#else
    initIoctl();
#endif
}

Spi_t::~Spi_t() {
#ifdef USE_BCM2835_SPI
    if (m_use_bcm) {
        bcm2835_spi_end();
        return;
    }
#endif
    if (m_fd >= 0) {
        close(m_fd);
    }
}

Spi_t::Spi_t(Spi_t&& other) noexcept
    : m_fd(other.m_fd)
    , m_config(other.m_config)
    , m_use_bcm(other.m_use_bcm)
{
    other.m_fd = -1;
}

Spi_t& Spi_t::operator=(Spi_t&& other) noexcept {
    if (this != &other) {
        m_fd = other.m_fd;
        m_config = other.m_config;
        m_use_bcm = other.m_use_bcm;
        other.m_fd = -1;
    }
    return *this;
}

// ============================================================================
// Inicialización con BCM2835
// ============================================================================

#ifdef USE_BCM2835_SPI
void Spi_t::initBcm2835() {
    if (!bcm2835_spi_begin()) {
        throw std::runtime_error("bcm2835_spi_begin() failed");
    }

    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_256);

    if (m_config.cs_pin == 0) {
        bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
    } else {
        bcm2835_spi_chipSelect(BCM2835_SPI_CS1);
    }
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);

    m_use_bcm = true;
}
#else
void Spi_t::initBcm2835() {
    // No disponible - usar initIoctl
    initIoctl();
}
#endif

// ============================================================================
// Inicialización con ioctl (/dev/spidev)
// ============================================================================

void Spi_t::initIoctl() {
    char device[32];
    snprintf(device, sizeof(device), "/dev/spidev%d.%d",
             m_config.cs_pin > 0 ? 0 : 0,
             m_config.cs_pin > 0 ? 1 : 0);

    m_fd = open(device, O_RDWR);
    if (m_fd < 0) {
        throw std::runtime_error(std::string("Cannot open SPI device: ") + device);
    }

    uint8_t mode = static_cast<uint8_t>(m_config.mode);
    if (ioctl(m_fd, SPI_IOC_WR_MODE, &mode) < 0) {
        close(m_fd);
        m_fd = -1;
        throw std::runtime_error("Cannot set SPI mode");
    }

    uint8_t bits = m_config.bits_word;
    if (ioctl(m_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
        close(m_fd);
        m_fd = -1;
        throw std::runtime_error("Cannot set SPI bits per word");
    }

    uint32_t speed = m_config.speed_hz;
    if (ioctl(m_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        close(m_fd);
        m_fd = -1;
        throw std::runtime_error("Cannot set SPI speed");
    }

    m_use_bcm = false;
}

// ============================================================================
// Transferencias
// ============================================================================

uint8_t Spi_t::transfer1(uint8_t tx) {
#ifdef USE_BCM2835_SPI
    if (m_use_bcm) {
        return bcm2835_spi_transfer(tx);
    }
#endif

    uint8_t rx = 0;
    struct spi_ioc_transfer tr = {};
    tr.tx_buf = (unsigned long)&tx;
    tr.rx_buf = (unsigned long)&rx;
    tr.len = 1;
    tr.speed_hz = m_config.speed_hz;
    tr.bits_per_word = m_config.bits_word;

    if (ioctl(m_fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
        return 0;
    }
    return rx;
}

uint8_t Spi_t::transfer2(uint16_t tx) {
    uint8_t buf[2];
    buf[0] = (tx >> 8) & 0xFF;
    buf[1] = tx & 0xFF;

#ifdef USE_BCM2835_SPI
    if (m_use_bcm) {
        bcm2835_spi_transfern((char*)buf, 2);
        return buf[1];
    }
#endif

    uint8_t rx[2] = {0, 0};
    struct spi_ioc_transfer tr = {};
    tr.tx_buf = (unsigned long)buf;
    tr.rx_buf = (unsigned long)rx;
    tr.len = 2;
    tr.speed_hz = m_config.speed_hz;
    tr.bits_per_word = m_config.bits_word;

    if (ioctl(m_fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
        return 0;
    }
    return rx[1];
}

uint8_t Spi_t::transfer3(uint32_t tx) {
    uint8_t buf[3];
    buf[0] = (tx >> 16) & 0xFF;
    buf[1] = (tx >> 8) & 0xFF;
    buf[2] = tx & 0xFF;

#ifdef USE_BCM2835_SPI
    if (m_use_bcm) {
        bcm2835_spi_transfern((char*)buf, 3);
        return buf[2];
    }
#endif

    uint8_t rx[3] = {0, 0, 0};
    struct spi_ioc_transfer tr = {};
    tr.tx_buf = (unsigned long)buf;
    tr.rx_buf = (unsigned long)rx;
    tr.len = 3;
    tr.speed_hz = m_config.speed_hz;
    tr.bits_per_word = m_config.bits_word;

    if (ioctl(m_fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
        return 0;
    }
    return rx[2];
}

int Spi_t::transfer(const uint8_t* tx_buf, uint8_t* rx_buf, size_t len) {
    if (!tx_buf || len == 0) return -1;

#ifdef USE_BCM2835_SPI
    if (m_use_bcm) {
        // BCM2835 necesita buffer de lectura/escritura
        std::vector<uint8_t> tmp(tx_buf, tx_buf + len);
        bcm2835_spi_transfern((char*)tmp.data(), len);
        if (rx_buf) memcpy(rx_buf, tmp.data(), len);
        return static_cast<int>(len);
    }
#endif

    struct spi_ioc_transfer tr = {};
    tr.tx_buf = (unsigned long)tx_buf;
    tr.rx_buf = (unsigned long)(rx_buf ? rx_buf : tx_buf);
    tr.len = len;
    tr.speed_hz = m_config.speed_hz;
    tr.bits_per_word = m_config.bits_word;

    int ret = ioctl(m_fd, SPI_IOC_MESSAGE(1), &tr);
    return ret < 0 ? -1 : static_cast<int>(len);
}

std::vector<uint8_t> Spi_t::transfer(const std::vector<uint8_t>& tx) {
    std::vector<uint8_t> rx(tx.size(), 0);
    if (transfer(tx.data(), rx.data(), tx.size()) < 0) {
        return {};
    }
    return rx;
}

} // namespace hal
