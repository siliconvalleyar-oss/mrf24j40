#include "mrf24j40.h"
#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <errno.h>

Mrf24j40::Mrf24j40() 
    : spi_fd(-1), initialized(false), tx_pending(false), tx_ok(false), 
      tx_retries(0), sequence(0), rx_length(0), rx_ready(false) {}

Mrf24j40::~Mrf24j40() { if (spi_fd >= 0) close(spi_fd); }

uint8_t Mrf24j40::readShort(uint8_t addr) {
    uint8_t tx[2] = { (uint8_t)((addr << 1) & 0x7E), 0x00 };
    uint8_t rx[2];
    struct spi_ioc_transfer tr = { 
        .tx_buf = (unsigned long)tx, 
        .rx_buf = (unsigned long)rx, 
        .len = 2, 
        .speed_hz = 1000000 
    };
    ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
    return rx[1];
}

void Mrf24j40::writeShort(uint8_t addr, uint8_t value) {
    uint8_t tx[2] = { (uint8_t)(((addr << 1) & 0x7E) | 0x01), value };
    struct spi_ioc_transfer tr = { 
        .tx_buf = (unsigned long)tx, 
        .len = 2, 
        .speed_hz = 1000000 
    };
    ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
}

uint8_t Mrf24j40::readLong(uint16_t addr) {
    uint8_t tx[3] = { 
        (uint8_t)(0x80 | ((addr >> 3) & 0x7F)), 
        (uint8_t)((addr << 5) & 0xE0), 
        0x00 
    };
    uint8_t rx[3];
    struct spi_ioc_transfer tr = { 
        .tx_buf = (unsigned long)tx, 
        .rx_buf = (unsigned long)rx, 
        .len = 3, 
        .speed_hz = 1000000 
    };
    ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
    return rx[2];
}

void Mrf24j40::writeLong(uint16_t addr, uint8_t value) {
    uint8_t tx[3] = { 
        (uint8_t)(0x80 | ((addr >> 3) & 0x7F)), 
        (uint8_t)(((addr << 5) & 0xE0) | 0x10), 
        value 
    };
    struct spi_ioc_transfer tr = { 
        .tx_buf = (unsigned long)tx, 
        .len = 3, 
        .speed_hz = 1000000 
    };
    ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
}

bool Mrf24j40::init(uint8_t channel) {
    // Abrir SPI
    spi_fd = open("/dev/spidev0.0", O_RDWR);
    if (spi_fd < 0) { 
        printf("[ERROR] No se pudo abrir SPI\n"); 
        return false; 
    }
    
    // Configurar SPI Modo 0
    uint8_t mode = SPI_MODE_0;
    ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);
    uint8_t bits = 8;
    ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    uint32_t speed = 1000000;
    ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    usleep(10000);
    
    // Soft reset
    writeShort(REG_SOFTRST, 0x07);
    usleep(10000);
    int timeout = 100;
    while ((readShort(REG_SOFTRST) & 0x07) != 0 && timeout--) {
        usleep(1000);
    }
    if (timeout == 0) {
        printf("[ERROR] Timeout en soft reset\n");
        return false;
    }
    
    // Configuración básica
    writeShort(REG_PACON2, 0x98);
    writeShort(REG_TXSTBL, 0x95);
    
    // Configuración RF
    writeLong(REG_RFCON0, 0x03);
    writeLong(REG_RFCON1, 0x01);
    writeLong(REG_RFCON2, 0x80);
    writeLong(REG_RFCON6, 0x90);
    writeLong(REG_RFCON7, 0x80);
    writeLong(REG_RFCON8, 0x10);
    writeLong(REG_SLPCON1, 0x21);
    
    // Configuración del receptor
    writeShort(REG_BBREG2, 0x80);
    writeShort(REG_CCAEDTH, 0x60);
    writeShort(REG_BBREG6, 0x40);
    
    // Configurar interrupciones
    writeShort(REG_INTCON, 0xF6);
    
    // Configurar canal
    setChannel(channel);
    
    // Deshabilitar/Habilitar RX
    writeShort(REG_BBREG1, 0x04);
    usleep(100);
    writeShort(REG_BBREG1, 0x00);
    
    // Activar RF
    writeShort(REG_RFCTL, 0x04);
    usleep(1000);
    writeShort(REG_RFCTL, 0x00);
    usleep(20000);
    
    initialized = true;
    return true;
}

void Mrf24j40::reset() {
    writeShort(REG_SOFTRST, 0x07);
    usleep(10000);
    int timeout = 100;
    while ((readShort(REG_SOFTRST) & 0x07) != 0 && timeout--) {
        usleep(1000);
    }
}

void Mrf24j40::setPan(uint16_t pan) {
    writeShort(REG_PANIDL, pan & 0xFF);
    writeShort(REG_PANIDH, (pan >> 8) & 0xFF);
    printf("[DEBUG] PAN configurado: 0x%04X\n", pan);
}

void Mrf24j40::setShortAddress(uint16_t addr) {
    writeShort(REG_SADRL, addr & 0xFF);
    writeShort(REG_SADRH, (addr >> 8) & 0xFF);
    printf("[DEBUG] Direccion configurada: 0x%04X\n", addr);
}

uint16_t Mrf24j40::getPan() {
    uint16_t low = readShort(REG_PANIDL);
    uint16_t high = readShort(REG_PANIDH);
    return (high << 8) | low;
}

uint16_t Mrf24j40::getShortAddress() {
    uint16_t low = readShort(REG_SADRL);
    uint16_t high = readShort(REG_SADRH);
    return (high << 8) | low;
}

bool Mrf24j40::setChannel(uint8_t channel) {
    if (channel < 11 || channel > 26) return false;
    uint8_t value = ((channel - 11) << 4) | 0x03;
    writeLong(REG_RFCON0, value);
    writeShort(REG_RFCTL, 0x04);
    usleep(1000);
    writeShort(REG_RFCTL, 0x00);
    return true;
}

bool Mrf24j40::send16(uint16_t dest, const uint8_t* data, uint8_t len) {
    if (len > MAX_PAYLOAD) return false;
    
    int timeout = 100;
    while (tx_pending && timeout--) usleep(1000);
    if (tx_pending) return false;
    
    tx_pending = true;
    tx_ok = false;
    
    uint16_t reg = 0x300;
    uint16_t pan = getPan();
    uint16_t src = getShortAddress();
    
    writeLong(reg++, MAC_HEADER_SIZE);
    writeLong(reg++, MAC_HEADER_SIZE + len);
    writeLong(reg++, 0x61);
    writeLong(reg++, 0x88);
    writeLong(reg++, sequence++);
    writeLong(reg++, pan & 0xFF);
    writeLong(reg++, (pan >> 8) & 0xFF);
    writeLong(reg++, dest & 0xFF);
    writeLong(reg++, (dest >> 8) & 0xFF);
    writeLong(reg++, src & 0xFF);
    writeLong(reg++, (src >> 8) & 0xFF);
    
    for (int i = 0; i < len; i++) {
        writeLong(reg++, data[i]);
    }
    
    writeShort(REG_TXNCON, (1 << 2) | (1 << 0));
    return true;
}

bool Mrf24j40::sendString(uint16_t dest, const char* str) {
    return send16(dest, (const uint8_t*)str, strlen(str));
}

void Mrf24j40::checkFlags() {
    uint8_t intstat = readShort(REG_INTSTAT);
    
    // TXNIF (bit 0)
    if (intstat & INT_TXNIF) {
        uint8_t txstat = readShort(REG_TXSTAT);
        tx_ok = !(txstat & 0x01);
        tx_retries = (txstat >> 6) & 0x03;
        tx_pending = false;
        writeShort(REG_INTSTAT, INT_TXNIF);
    }
    
    // RXIF (bit 3)
    if (intstat & INT_RXIF) {
        writeShort(REG_BBREG1, 0x04);
        
        uint8_t frame_len = readLong(0x300);
        if (frame_len >= MAC_HEADER_SIZE + FCS_SIZE) {
            rx_length = frame_len - MAC_HEADER_SIZE - FCS_SIZE;
            if (rx_length > MAX_PAYLOAD) rx_length = MAX_PAYLOAD;
            
            for (int i = 0; i < rx_length; i++) {
                rx_buffer[i] = readLong(0x301 + MAC_HEADER_SIZE + i);
            }
            
            rx_lqi = readLong(0x301 + frame_len);
            rx_rssi = readLong(0x301 + frame_len + 1);
            rx_ready = true;
        }
        
        writeShort(REG_RXFLUSH, 0x01);
        writeShort(REG_BBREG1, 0x00);
        writeShort(REG_INTSTAT, INT_RXIF);
    }
}

bool Mrf24j40::hasRxPacket() { return rx_ready; }
uint8_t Mrf24j40::getRxLength() { return rx_length; }
void Mrf24j40::getRxData(uint8_t* buffer) { memcpy(buffer, rx_buffer, rx_length); rx_ready = false; }
uint8_t Mrf24j40::getLQI() { return rx_lqi; }
uint8_t Mrf24j40::getRSSI() { return rx_rssi; }
bool Mrf24j40::wasTxSuccessful() { return tx_ok; }
uint8_t Mrf24j40::getTxRetries() { return tx_retries; }

void Mrf24j40::setPromiscuous(bool enable) {
    uint8_t val = readShort(REG_RXMCR);
    if (enable) val |= 0x01;
    else val &= ~0x01;
    writeShort(REG_RXMCR, val);
}
