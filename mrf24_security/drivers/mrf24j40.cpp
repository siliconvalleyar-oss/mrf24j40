/**
 * @file    drivers/mrf24j40.cpp
 * @brief   Implementación del driver MRF24J40 con modos de operación
 *
 * @author  MRF24J40 Team
 * @date    2026
 */

#include <drivers/mrf24j40.hpp>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <cstdio>
#include <stdexcept>

namespace drivers {

// ============================================================================
// Registros del MRF24J40
// ============================================================================

static constexpr uint8_t REG_SOFTRST = 0x00;
static constexpr uint8_t REG_PACON2  = 0x02;
static constexpr uint8_t REG_TXSTBL  = 0x0E;
static constexpr uint8_t REG_RFCTL   = 0x36;
static constexpr uint8_t REG_BBREG1  = 0x39;
static constexpr uint8_t REG_BBREG2  = 0x3A;
static constexpr uint8_t REG_BBREG6  = 0x3F;
static constexpr uint8_t REG_CCAEDTH = 0x3B;
static constexpr uint8_t REG_INTCON  = 0x3B;
static constexpr uint8_t REG_INTSTAT = 0x31;
static constexpr uint8_t REG_TXNCON  = 0x1B;
static constexpr uint8_t REG_TXSTAT  = 0x24;
static constexpr uint8_t REG_RXFLUSH = 0x2A;
static constexpr uint8_t REG_PANIDL  = 0x01;
static constexpr uint8_t REG_PANIDH  = 0x02;
static constexpr uint8_t REG_SADRL   = 0x03;
static constexpr uint8_t REG_SADRH   = 0x04;

static constexpr uint16_t LREG_RFCON0 = 0x200;
static constexpr uint16_t LREG_RFCON1 = 0x201;
static constexpr uint16_t LREG_RFCON2 = 0x202;
static constexpr uint16_t LREG_RFCON3 = 0x203;
static constexpr uint16_t LREG_RFCON6 = 0x206;
static constexpr uint16_t LREG_RFCON7 = 0x207;
static constexpr uint16_t LREG_RFCON8 = 0x208;
static constexpr uint16_t LREG_SLPCON1 = 0x211;

static constexpr uint8_t TXNACKREQ = (1 << 2);
static constexpr uint8_t TXNTRIG   = (1 << 0);
static constexpr uint8_t INT_TXNIF = (1 << 0);
static constexpr uint8_t INT_RXIF  = (1 << 3);

static constexpr uint8_t SPI_MODE_0 = 0;
static constexpr const char* SPI_DEVICE = "/dev/spidev0.0";
static constexpr uint32_t SPI_SPEED_HZ = 1000000;

// ============================================================================
// Constructor / Destructor
// ============================================================================

Mrf24j40_t::Mrf24j40_t()
    : m_initialized(false)
    , m_tx_pending(false)
    , m_tx_ok(false)
    , m_tx_retries(0)
    , m_seq(0)
    , m_rx_ready(false)
    , m_rx_len(0)
    , m_last_lqi(0)
    , m_last_rssi(0)
{
    m_mac64.fill(0);
    m_rx_buf[0] = 0;
    m_stats = {};
}

Mrf24j40_t::~Mrf24j40_t() = default;

// ============================================================================
// SPI de bajo nivel
// ============================================================================

uint8_t Mrf24j40_t::readShort(uint8_t addr) const {
    uint8_t tx[2] = {static_cast<uint8_t>((addr & 0x3F) << 1), 0x00};
    uint8_t rx[2] = {0, 0};
    m_spi->transfer(tx, rx, 2);
    return rx[1];
}

void Mrf24j40_t::writeShort(uint8_t addr, uint8_t val) {
    uint8_t tx[2] = {static_cast<uint8_t>(((addr & 0x3F) << 1) | 0x01), val};
    m_spi->transfer(tx, nullptr, 2);
}

uint8_t Mrf24j40_t::readLong(uint16_t addr) const {
    uint8_t tx[3] = {
        static_cast<uint8_t>(0x80 | ((addr >> 3) & 0x7F)),
        static_cast<uint8_t>((addr & 0x07) << 5),
        0x00
    };
    uint8_t rx[3] = {0, 0, 0};
    m_spi->transfer(tx, rx, 3);
    return rx[2];
}

void Mrf24j40_t::writeLong(uint16_t addr, uint8_t val) {
    uint8_t tx[3] = {
        static_cast<uint8_t>(0x80 | ((addr >> 3) & 0x7F)),
        static_cast<uint8_t>(((addr & 0x07) << 5) | 0x10),
        val
    };
    m_spi->transfer(tx, nullptr, 3);
}

// ============================================================================
// Inicialización
// ============================================================================

bool Mrf24j40_t::waitForReset() {
    for (int i = 0; i < 200; i++) {
        if ((readShort(REG_SOFTRST) & 0x07) == 0x00)
            return true;
        usleep(1000);
    }
    return false;
}

void Mrf24j40_t::rfReset() {
    writeShort(REG_RFCTL, 0x04);
    usleep(200);
    writeShort(REG_RFCTL, 0x00);
    usleep(1000);
}

void Mrf24j40_t::flushRx() {
    writeShort(REG_BBREG1, 0x04);
    writeShort(REG_RXFLUSH, 0x01);
    usleep(100);
    writeShort(REG_BBREG1, 0x00);
}

bool Mrf24j40_t::init(uint8_t channel) {
    // Inicializar SPI
    try {
        hal::SpiConfig cfg;
        cfg.speed_hz = SPI_SPEED_HZ;
        cfg.mode = hal::SpiMode::Mode0;
        m_spi = std::make_unique<hal::Spi_t>(cfg);
    } catch (const std::exception& e) {
        return false;
    }

    // Soft reset
    writeShort(REG_SOFTRST, 0x07);
    usleep(2000);
    if (!waitForReset()) return false;

    // Configuración base
    writeShort(REG_PACON2, 0x98);
    writeShort(REG_TXSTBL, 0x95);

    // Configuración RF
    writeLong(LREG_RFCON1, 0x02);
    writeLong(LREG_RFCON2, 0x80);
    writeLong(LREG_RFCON3, 0x00);
    writeLong(LREG_RFCON6, 0x90);
    writeLong(LREG_RFCON7, 0x80);
    writeLong(LREG_RFCON8, 0x10);
    writeLong(LREG_SLPCON1, 0x21);

    // Canal
    setChannel(channel);

    // Baseband
    writeShort(REG_BBREG2, 0x80);
    writeShort(REG_CCAEDTH, 0x60);
    writeShort(REG_BBREG6, 0x40);
    writeShort(REG_INTCON, 0xF6);

    flushRx();
    rfReset();

    m_initialized = true;
    m_channel = channel;
    return true;
}

// ============================================================================
// Configuración de red
// ============================================================================

void Mrf24j40_t::setPan(uint16_t pan) {
    m_pan_id = pan;
    writeShort(REG_PANIDL, pan & 0xFF);
    writeShort(REG_PANIDH, (pan >> 8) & 0xFF);
}

uint16_t Mrf24j40_t::pan() const {
    return readShort(REG_PANIDL) | (readShort(REG_PANIDH) << 8);
}

void Mrf24j40_t::setShortAddress(uint16_t addr) {
    m_short_addr = addr;
    writeShort(REG_SADRL, addr & 0xFF);
    writeShort(REG_SADRH, (addr >> 8) & 0xFF);
}

uint16_t Mrf24j40_t::shortAddress() const {
    return readShort(REG_SADRL) | (readShort(REG_SADRH) << 8);
}

void Mrf24j40_t::setMacAddress(const std::array<uint8_t, MAC64_LEN>& mac) {
    m_mac64 = mac;
}

std::array<uint8_t, MAC64_LEN> Mrf24j40_t::macAddress() const {
    return m_mac64;
}

bool Mrf24j40_t::setChannel(uint8_t channel) {
    if (channel < 11 || channel > 26) return false;
    uint8_t val = ((channel - 11) << 4) | 0x03;
    writeLong(LREG_RFCON0, val);
    rfReset();
    m_channel = channel;
    return true;
}

void Mrf24j40_t::setMode(NodeMode mode) {
    m_mode = mode;
    // El modo afecta el comportamiento de reenvío en el protocolo
}

// ============================================================================
// Transmisión
// ============================================================================

bool Mrf24j40_t::send(const std::array<uint8_t, MAC64_LEN>& dest_mac,
                       const uint8_t* data, uint8_t len) {
    if (!m_initialized || len > MAX_PAYLOAD_LEN) return false;

    // Esperar TX anterior
    int wait = 500;
    while (m_tx_pending && wait-- > 0) {
        poll();
        usleep(1000);
    }
    if (m_tx_pending) return false;

    uint16_t src = m_short_addr;
    uint16_t dst = (dest_mac[6] << 8) | dest_mac[7];
    const uint8_t frm_len = MAC_HDR_LEN + len;

    // Construir trama en TX FIFO
    writeLong(TXNFIFO + 0, MAC_HDR_LEN);
    writeLong(TXNFIFO + 1, frm_len);
    writeLong(TXNFIFO + 2, 0x61); // FCF lo
    writeLong(TXNFIFO + 3, 0x88); // FCF hi
    writeLong(TXNFIFO + 4, m_seq++);
    writeLong(TXNFIFO + 5, m_pan_id & 0xFF);
    writeLong(TXNFIFO + 6, (m_pan_id >> 8) & 0xFF);
    writeLong(TXNFIFO + 7, dst & 0xFF);
    writeLong(TXNFIFO + 8, (dst >> 8) & 0xFF);
    writeLong(TXNFIFO + 9, src & 0xFF);
    writeLong(TXNFIFO + 10, (src >> 8) & 0xFF);

    for (uint8_t i = 0; i < len; i++)
        writeLong(TXNFIFO + 11 + i, data[i]);

    m_tx_pending = true;
    m_tx_ok = false;
    m_tx_retries = 0;
    m_stats.packets_sent++;

    writeShort(REG_TXNCON, TXNACKREQ | TXNTRIG);
    return true;
}

bool Mrf24j40_t::sendString(const std::array<uint8_t, MAC64_LEN>& dest_mac,
                              const std::string_view& str) {
    return send(dest_mac, reinterpret_cast<const uint8_t*>(str.data()),
                static_cast<uint8_t>(str.size()));
}

bool Mrf24j40_t::sendFrame(const Frame_t& frame) {
    // Serializar trama a buffer
    uint8_t buf[MAX_PAYLOAD_LEN];
    uint8_t pos = 0;

    // Dest MAC (ya incluido en header 802.15.4)
    for (int i = 0; i < MAC64_LEN; i++)
        buf[pos++] = frame.dest_mac[i];

    // Size
    buf[pos++] = frame.size & 0xFF;
    buf[pos++] = (frame.size >> 8) & 0xFF;

    // IV
    for (auto b : frame.iv) buf[pos++] = b;

    // Ciphertext
    for (auto b : frame.ciphertext) buf[pos++] = b;

    // Hash
    if (frame.has_hash) {
        for (auto b : frame.hash) buf[pos++] = b;
    }

    return send(frame.dest_mac, buf, pos);
}

// ============================================================================
// Recepción
// ============================================================================

void Mrf24j40_t::getFrame(Frame_t& frame) {
    if (!m_rx_ready) return;

    frame.size = m_rx_len;
    frame.ciphertext.assign(m_rx_buf, m_rx_buf + m_rx_len);
    frame.has_hash = false;
    m_rx_ready = false;
}

uint8_t Mrf24j40_t::getData(uint8_t* buf) {
    if (!m_rx_ready || !buf) return 0;
    std::memcpy(buf, m_rx_buf, m_rx_len);
    uint8_t len = m_rx_len;
    m_rx_ready = false;
    return len;
}

// ============================================================================
// Polling
// ============================================================================

void Mrf24j40_t::poll() {
    uint8_t irq = readShort(REG_INTSTAT);
    if (irq == 0) return;

    if (irq & INT_TXNIF) handleTxIrq();
    if (irq & INT_RXIF) handleRxIrq();
    writeShort(REG_INTSTAT, irq);
}

void Mrf24j40_t::handleTxIrq() {
    uint8_t txstat = readShort(REG_TXSTAT);
    m_tx_ok = !(txstat & 0x01);
    m_tx_retries = (txstat >> 6) & 0x03;
    m_tx_pending = false;

    if (m_tx_ok) m_stats.tx_success++;
    else m_stats.tx_fail++;
    m_stats.tx_retries += m_tx_retries;

    if (onTransmit) onTransmit(m_tx_ok, m_tx_retries);
}

void Mrf24j40_t::handleRxIrq() {
    writeShort(REG_BBREG1, 0x04);

    uint8_t frame_len = readLong(RXFIFO + 0);
    if (frame_len < 12 || frame_len > 127) {
        flushRx();
        return;
    }

    int payload_len = frame_len - MAC_HDR_LEN - FCS_LEN;
    if (payload_len <= 0 || payload_len > MAX_PAYLOAD_LEN) {
        flushRx();
        return;
    }

    m_rx_len = payload_len;
    for (uint8_t i = 0; i < m_rx_len; i++)
        m_rx_buf[i] = readLong(RXFIFO + 1 + MAC_HDR_LEN + i);

    m_last_lqi = readLong(RXFIFO + 1 + frame_len);
    uint8_t raw_rssi = readLong(RXFIFO + 1 + frame_len + 1);
    m_last_rssi = -90 + raw_rssi / 3;

    m_rx_ready = true;
    m_stats.packets_received++;
    m_stats.lqi_avg = (m_stats.lqi_avg * (m_stats.packets_received - 1) + m_last_lqi) /
                       m_stats.packets_received;
    m_stats.rssi_avg = (m_stats.rssi_avg * (m_stats.packets_received - 1) + m_last_rssi) /
                        m_stats.packets_received;

    flushRx();
    writeShort(REG_BBREG1, 0x00);

    if (onReceive) onReceive(m_rx_buf, m_rx_len);
}

// ============================================================================
// Estadísticas
// ============================================================================

void Mrf24j40_t::getStats(Mrf24Stats& stats) const {
    stats = m_stats;
}

void Mrf24j40_t::resetStats() {
    m_stats = {};
}

// ============================================================================
// Diagnóstico
// ============================================================================

void Mrf24j40_t::printRegisters() {
    printf("[MRF24J40] SOFTRST=0x%02X INTSTAT=0x%02X TXSTAT=0x%02X\n",
           readShort(REG_SOFTRST), readShort(REG_INTSTAT), readShort(REG_TXSTAT));
}

bool Mrf24j40_t::selfTest() {
    uint16_t orig = pan();
    setPan(0x1234);
    uint16_t test = pan();
    setPan(orig);
    return test == 0x1234;
}

} // namespace drivers
