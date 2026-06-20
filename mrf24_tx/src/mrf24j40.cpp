#include "mrf24j40.h"
#include <fcntl.h>
#include <errno.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <cstring>

Mrf24j40::Mrf24j40()
    : spi_fd(-1), initialized(false), tx_pending(false), tx_ok(false),
      tx_retries(0), seq(0), rx_length(0), rx_lqi(0), rx_rssi_dbm(0), rx_ready(false)
{
    memset(&stats, 0, sizeof(stats));
}

Mrf24j40::~Mrf24j40()
{
    if (spi_fd >= 0) close(spi_fd);
}

uint8_t Mrf24j40::readShort(uint8_t addr)
{
    uint8_t tx[2], rx[2];
    tx[0] = (addr & 0x3F) << 1;
    tx[1] = 0x00;
    
    struct spi_ioc_transfer tr = {0};
    tr.tx_buf = (unsigned long)tx;
    tr.rx_buf = (unsigned long)rx;
    tr.len = 2;
    tr.speed_hz = SPI_SPEED_HZ;
    tr.bits_per_word = 8;
    
    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0)
        perror("readShort");
    return rx[1];
}

void Mrf24j40::writeShort(uint8_t addr, uint8_t val)
{
    uint8_t tx[2];
    tx[0] = ((addr & 0x3F) << 1) | 0x01;
    tx[1] = val;
    
    struct spi_ioc_transfer tr = {0};
    tr.tx_buf = (unsigned long)tx;
    tr.len = 2;
    tr.speed_hz = SPI_SPEED_HZ;
    tr.bits_per_word = 8;
    
    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0)
        perror("writeShort");
}

uint8_t Mrf24j40::readLong(uint16_t addr)
{
    uint8_t tx[3], rx[3];
    tx[0] = 0x80 | ((addr >> 3) & 0x7F);
    tx[1] = (addr & 0x07) << 5;
    tx[2] = 0x00;
    
    struct spi_ioc_transfer tr = {0};
    tr.tx_buf = (unsigned long)tx;
    tr.rx_buf = (unsigned long)rx;
    tr.len = 3;
    tr.speed_hz = SPI_SPEED_HZ;
    tr.bits_per_word = 8;
    
    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0)
        perror("readLong");
    return rx[2];
}

void Mrf24j40::writeLong(uint16_t addr, uint8_t val)
{
    uint8_t tx[3];
    tx[0] = 0x80 | ((addr >> 3) & 0x7F);
    tx[1] = ((addr & 0x07) << 5) | 0x10;
    tx[2] = val;
    
    struct spi_ioc_transfer tr = {0};
    tr.tx_buf = (unsigned long)tx;
    tr.len = 3;
    tr.speed_hz = SPI_SPEED_HZ;
    tr.bits_per_word = 8;
    
    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0)
        perror("writeLong");
}

bool Mrf24j40::waitForReset()
{
    for (int i = 0; i < 200; i++) {
        if ((readShort(REG_SOFTRST) & 0x07) == 0x00)
            return true;
        usleep(1000);
    }
    return false;
}

bool Mrf24j40::init(uint8_t channel)
{
    spi_fd = open(SPI_DEVICE, O_RDWR);
    if (spi_fd < 0) {
        fprintf(stderr, "ERROR: No se pudo abrir %s\n", SPI_DEVICE);
        return false;
    }
    
    uint8_t mode = SPI_MODE_0;
    ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
    
    uint32_t speed = SPI_SPEED_HZ;
    ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    
    printf("[SPI] %s, velocidad: %u Hz\n", SPI_DEVICE, speed);
    
    writeShort(REG_SOFTRST, 0x07);
    usleep(2000);
    if (!waitForReset()) {
        fprintf(stderr, "ERROR: Timeout en soft reset\n");
        return false;
    }
    printf("[INIT] Soft reset OK\n");
    
    writeShort(REG_PACON2, 0x98);
    writeShort(REG_TXSTBL, 0x95);
    
    writeLong(LREG_RFCON1, 0x02);
    writeLong(LREG_RFCON2, 0x80);
    writeLong(LREG_RFCON3, 0x00);
    writeLong(LREG_RFCON6, 0x90);
    writeLong(LREG_RFCON7, 0x80);
    writeLong(LREG_RFCON8, 0x10);
    writeLong(LREG_SLPCON1, 0x21);
    
    setChannel(channel);
    printf("[INIT] Canal: %d\n", channel);
    
    writeShort(REG_BBREG2, 0x80);
    writeShort(REG_CCAEDTH, 0x60);
    writeShort(REG_BBREG6, 0x40);
    writeShort(REG_INTCON, 0xF6);
    
    flushRx();
    rfReset();
    
    initialized = true;
    printf("[INIT] MRF24J40 inicializado correctamente\n");
    return true;
}

void Mrf24j40::rfReset()
{
    writeShort(REG_RFCTL, 0x04);
    usleep(200);
    writeShort(REG_RFCTL, 0x00);
    usleep(1000);
}

void Mrf24j40::setPan(uint16_t pan)
{
    writeShort(REG_PANIDL, pan & 0xFF);
    writeShort(REG_PANIDH, (pan >> 8) & 0xFF);
}

void Mrf24j40::setShortAddress(uint16_t addr)
{
    writeShort(REG_SADRL, addr & 0xFF);
    writeShort(REG_SADRH, (addr >> 8) & 0xFF);
}

uint16_t Mrf24j40::getPan()
{
    return readShort(REG_PANIDL) | (readShort(REG_PANIDH) << 8);
}

uint16_t Mrf24j40::getShortAddress()
{
    return readShort(REG_SADRL) | (readShort(REG_SADRH) << 8);
}

bool Mrf24j40::setChannel(uint8_t ch)
{
    if (ch < 11 || ch > 26) return false;
    uint8_t val = ((ch - 11) << 4) | 0x03;
    writeLong(LREG_RFCON0, val);
    rfReset();
    return true;
}

void Mrf24j40::flushRx()
{
    writeShort(REG_BBREG1, 0x04);
    writeShort(REG_RXFLUSH, 0x01);
    usleep(100);
    writeShort(REG_BBREG1, 0x00);
}

bool Mrf24j40::send(uint16_t dest_addr, uint16_t dest_pan, const uint8_t* data, uint8_t len)
{
    if (!initialized || len > MAX_PAYLOAD) return false;
    
    int wait = 500;
    while (tx_pending && wait-- > 0) {
        poll();
        usleep(1000);
    }
    if (tx_pending) return false;
    
    uint16_t src_addr = getShortAddress();
    const uint8_t hdr_len = 9;
    const uint8_t frm_len = hdr_len + len;
    
    writeLong(TXNFIFO + 0, hdr_len);
    writeLong(TXNFIFO + 1, frm_len);
    writeLong(TXNFIFO + 2, FCF_LO);
    writeLong(TXNFIFO + 3, FCF_HI);
    writeLong(TXNFIFO + 4, seq++);
    writeLong(TXNFIFO + 5, dest_pan & 0xFF);
    writeLong(TXNFIFO + 6, (dest_pan >> 8) & 0xFF);
    writeLong(TXNFIFO + 7, dest_addr & 0xFF);
    writeLong(TXNFIFO + 8, (dest_addr >> 8) & 0xFF);
    writeLong(TXNFIFO + 9, src_addr & 0xFF);
    writeLong(TXNFIFO + 10, (src_addr >> 8) & 0xFF);
    
    for (uint8_t i = 0; i < len; i++)
        writeLong(TXNFIFO + 11 + i, data[i]);
    
    tx_pending = true;
    tx_ok = false;
    tx_retries = 0;
    stats.packets_sent++;
    
    writeShort(REG_TXNCON, TXNACKREQ | TXNTRIG);
    return true;
}

bool Mrf24j40::sendString(uint16_t dest_addr, const char* str)
{
    if (!str) return false;
    uint8_t len = strnlen(str, MAX_PAYLOAD);
    return send(dest_addr, getPan(), (const uint8_t*)str, len);
}

void Mrf24j40::handleTxIrq()
{
    uint8_t txstat = readShort(REG_TXSTAT);
    tx_ok = !(txstat & 0x01);
    tx_retries = (txstat >> 6) & 0x03;
    tx_pending = false;
    
    if (tx_ok) {
        stats.tx_success++;
    } else {
        stats.tx_fail++;
    }
    stats.tx_retries_total += tx_retries;
}

void Mrf24j40::handleRxIrq()
{
    writeShort(REG_BBREG1, 0x04);
    
    uint8_t frame_len = readLong(RXFIFO + 0);
    const uint8_t MIN_FRAME = 12;
    
    if (frame_len < MIN_FRAME || frame_len > 127) {
        flushRx();
        return;
    }
    
    const uint8_t HDR = 9;
    const uint8_t FCS = 2;
    int payload_len = frame_len - HDR - FCS;
    
    if (payload_len <= 0 || payload_len > MAX_PAYLOAD) {
        flushRx();
        return;
    }
    
    rx_length = payload_len;
    for (uint8_t i = 0; i < rx_length; i++)
        rx_buf[i] = readLong(RXFIFO + 1 + HDR + i);
    
    rx_lqi = readLong(RXFIFO + 1 + frame_len);
    uint8_t raw_rssi = readLong(RXFIFO + 1 + frame_len + 1);
    rx_rssi_dbm = -90 + raw_rssi / 3;
    
    rx_ready = true;
    
    stats.packets_received++;
    stats.rx_lqi_sum += rx_lqi;
    stats.rx_rssi_sum += rx_rssi_dbm;
    stats.rx_count++;
    
    flushRx();
    writeShort(REG_BBREG1, 0x00);
}

void Mrf24j40::poll()
{
    uint8_t irq = readShort(REG_INTSTAT);
    if (irq == 0) return;
    
    if (irq & INT_TXNIF) handleTxIrq();
    if (irq & INT_RXIF) handleRxIrq();
    writeShort(REG_INTSTAT, irq);
}

void Mrf24j40::rxGet(uint8_t* buf)
{
    if (buf && rx_ready) {
        memcpy(buf, rx_buf, rx_length);
    }
    rx_ready = false;
    rx_length = 0;
}

void Mrf24j40::getStats(RadioStats& s)
{
    s = stats;
}

void Mrf24j40::resetStats()
{
    memset(&stats, 0, sizeof(stats));
}

void Mrf24j40::printRegisters()
{
    printf("\n=== Registros MRF24J40 ===\n");
    printf("SOFTRST: 0x%02X\n", readShort(REG_SOFTRST));
    printf("INTSTAT: 0x%02X\n", readShort(REG_INTSTAT));
    printf("TXSTAT:  0x%02X\n", readShort(REG_TXSTAT));
    printf("PANID:   0x%04X\n", getPan());
    printf("SADDR:   0x%04X\n", getShortAddress());
    printf("RFCON2:  0x%02X\n", readLong(LREG_RFCON2));
    printf("========================\n");
}

bool Mrf24j40::selfTest()
{
    uint16_t original_pan = getPan();
    setPan(0x1234);
    uint16_t test_pan = getPan();
    setPan(original_pan);
    
    if (test_pan != 0x1234) {
        fprintf(stderr, "Self-test falló: escritura PAN\n");
        return false;
    }
    printf("Self-test OK\n");
    return true;
}
