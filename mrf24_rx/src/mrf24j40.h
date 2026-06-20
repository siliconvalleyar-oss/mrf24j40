#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <chrono>

// =============================================================
// CONFIGURACIÓN SPI - Velocidad unificada
// =============================================================
#define SPI_SPEED_HZ    100000      // 100 kHz (estable y confiable)
#define SPI_DEVICE      "/dev/spidev0.0"

// =============================================================
// MRF24J40 - Registros Short Address (6 bits)
// =============================================================
#define REG_RXMCR    0x00
#define REG_PANIDL   0x01
#define REG_PANIDH   0x02
#define REG_SADRL    0x03
#define REG_SADRH    0x04
#define REG_EADR0    0x05
#define REG_EADR1    0x06
#define REG_EADR2    0x07
#define REG_EADR3    0x08
#define REG_EADR4    0x09
#define REG_EADR5    0x0A
#define REG_EADR6    0x0B
#define REG_EADR7    0x0C
#define REG_RXFLUSH  0x0D
#define REG_ORDER    0x10
#define REG_TXMCR    0x11
#define REG_ACKTMOUT 0x12
#define REG_ESLOTG0  0x13
#define REG_SYMTICKL 0x14
#define REG_SYMTICKH 0x15
#define REG_PACON0   0x16
#define REG_PACON1   0x17
#define REG_PACON2   0x18
#define REG_RSSIVAL  0x19
#define REG_TXBCON0  0x1A
#define REG_TXNCON   0x1B
#define REG_TXG1CON  0x1C
#define REG_TXG2CON  0x1D
#define REG_ESLOTG23 0x1E
#define REG_ESLOTG45 0x1F
#define REG_ESLOTG67 0x20
#define REG_TXPEND   0x21
#define REG_WAKECON  0x22
#define REG_FRMOFFSET 0x23
#define REG_TXSTAT   0x24
#define REG_TXBCON1  0x25
#define REG_GATECLK  0x26
#define REG_TXTIME   0x27
#define REG_HSYMTMRL 0x28
#define REG_HSYMTMRH 0x29
#define REG_SOFTRST  0x2A
#define REG_SECCON0  0x2C
#define REG_SECCON1  0x2D
#define REG_TXSTBL   0x2E
#define REG_RXSR     0x30
#define REG_INTSTAT  0x31
#define REG_INTCON   0x32
#define REG_GPIO     0x33
#define REG_TRISGPIO 0x34
#define REG_SLPACK   0x35
#define REG_RFCTL    0x36
#define REG_SECCR2   0x37
#define REG_BBREG0   0x38
#define REG_BBREG1   0x39
#define REG_BBREG2   0x3A
#define REG_BBREG3   0x3B
#define REG_BBREG4   0x3C
#define REG_BBREG6   0x3E
#define REG_CCAEDTH  0x3F

// Registros Long Address
#define LREG_RFCON0  0x200
#define LREG_RFCON1  0x201
#define LREG_RFCON2  0x202
#define LREG_RFCON3  0x203
#define LREG_RFCON5  0x205
#define LREG_RFCON6  0x206
#define LREG_RFCON7  0x207
#define LREG_RFCON8  0x208
#define LREG_SLPCON0 0x211
#define LREG_SLPCON1 0x220
#define LREG_SLPCON2 0x221
#define LREG_WAKETIMEL 0x222
#define LREG_WAKETIMEH 0x223
#define LREG_REMCNTL 0x224
#define LREG_REMCNTH 0x225
#define LREG_MAINCNT0 0x226
#define LREG_MAINCNT1 0x227
#define LREG_MAINCNT2 0x228
#define LREG_MAINCNT3 0x229
#define LREG_ASSOEADR0 0x230
#define LREG_TESTMODE 0x22F

// FIFOs
#define TXNFIFO     0x000
#define TXBFIFO     0x080
#define TXG1FIFO    0x100
#define TXG2FIFO    0x180
#define RXFIFO      0x300

// Bits de interrupción
#define INT_SLPIF   0x80
#define INT_WAKEIF  0x40
#define INT_HSYMTMRIF 0x20
#define INT_SECIF   0x10
#define INT_RXIF    0x08
#define INT_TXG2IF  0x04
#define INT_TXG1IF  0x02
#define INT_TXNIF   0x01

// Bits TXNCON
#define TXNTRIG     0x01
#define TXNSECEN    0x02
#define TXNACKREQ   0x04

// Frame Control Field
#define FCF_LO      0x61
#define FCF_HI      0x88

#define MAX_PAYLOAD     100

// Estadísticas avanzadas
struct RadioStats {
    uint32_t packets_sent;
    uint32_t packets_received;
    uint32_t tx_success;
    uint32_t tx_fail;
    uint32_t tx_retries_total;
    uint32_t rx_lqi_sum;
    int32_t  rx_rssi_sum;
    uint32_t rx_count;
    uint32_t crc_errors;
};

class Mrf24j40 {
public:
    Mrf24j40();
    ~Mrf24j40();

    bool init(uint8_t channel);
    void setPan(uint16_t pan);
    void setShortAddress(uint16_t addr);
    uint16_t getPan();
    uint16_t getShortAddress();
    bool setChannel(uint8_t channel);
    
    bool send(uint16_t dest_addr, uint16_t dest_pan, const uint8_t* data, uint8_t len);
    bool sendString(uint16_t dest_addr, const char* str);
    
    void poll();
    
    bool hasPacket() const { return rx_ready; }
    uint8_t rxLen() const { return rx_length; }
    void rxGet(uint8_t* buf);
    uint8_t getLQI() const { return rx_lqi; }
    int8_t getRSSI() const { return rx_rssi_dbm; }
    
    bool txDone() const { return !tx_pending; }
    bool txSuccess() const { return tx_ok; }
    uint8_t txRetries() const { return tx_retries; }
    
    void getStats(RadioStats& stats);
    void resetStats();
    void printRegisters();
    bool selfTest();

private:
    uint8_t readShort(uint8_t addr);
    void writeShort(uint8_t addr, uint8_t val);
    uint8_t readLong(uint16_t addr);
    void writeLong(uint16_t addr, uint8_t val);
    
    void flushRx();
    void handleRxIrq();
    void handleTxIrq();
    void rfReset();
    bool waitForReset();

    int spi_fd;
    bool initialized;
    
    bool tx_pending;
    bool tx_ok;
    uint8_t tx_retries;
    uint8_t seq;
    
    uint8_t rx_buf[MAX_PAYLOAD];
    uint8_t rx_length;
    uint8_t rx_lqi;
    int8_t rx_rssi_dbm;
    bool rx_ready;
    
    RadioStats stats;
};
