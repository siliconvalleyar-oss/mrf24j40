#pragma once
#include <cstdint>
#include <cstdio>
#include <unistd.h>
#include <cstring>

// Registros del MRF24J40
#define REG_RXMCR   0x00
#define REG_PANIDL  0x01
#define REG_PANIDH  0x02
#define REG_SADRL   0x03
#define REG_SADRH   0x04
#define REG_EADR0   0x05
#define REG_EADR1   0x06
#define REG_EADR2   0x07
#define REG_EADR3   0x08
#define REG_EADR4   0x09
#define REG_EADR5   0x0A
#define REG_EADR6   0x0B
#define REG_EADR7   0x0C
#define REG_RXFLUSH 0x0D
#define REG_TXNCON  0x1B
#define REG_TXSTAT  0x24
#define REG_SOFTRST 0x2A
#define REG_INTSTAT 0x31
#define REG_INTCON  0x32
#define REG_RFCTL   0x36
#define REG_BBREG1  0x39
#define REG_BBREG2  0x3A
#define REG_BBREG6  0x3E
#define REG_CCAEDTH 0x3F
#define REG_PACON2  0x18
#define REG_TXSTBL  0x2E

// Registros long address
#define REG_RFCON0  0x200
#define REG_RFCON1  0x201
#define REG_RFCON2  0x202
#define REG_RFCON6  0x206
#define REG_RFCON7  0x207
#define REG_RFCON8  0x208
#define REG_SLPCON1 0x220

// Bits de interrupción
#define INT_RXIF    0x08
#define INT_TXNIF   0x01

// Constantes
#define MAC_HEADER_SIZE 9
#define FCS_SIZE 2
#define MAX_PAYLOAD 100

class Mrf24j40 {
public:
    Mrf24j40();
    ~Mrf24j40();
    
    bool init(uint8_t channel);
    void reset();
    
    void setPan(uint16_t pan);
    void setShortAddress(uint16_t addr);
    uint16_t getPan();
    uint16_t getShortAddress();
    
    bool setChannel(uint8_t channel);
    
    bool send16(uint16_t dest, const uint8_t* data, uint8_t len);
    bool sendString(uint16_t dest, const char* str);
    
    void checkFlags();
    bool hasRxPacket();
    uint8_t getRxLength();
    void getRxData(uint8_t* buffer);
    uint8_t getLQI();
    uint8_t getRSSI();
    
    bool wasTxSuccessful();
    uint8_t getTxRetries();
    
    void setPromiscuous(bool enable);
    
private:
    uint8_t readShort(uint8_t addr);
    void writeShort(uint8_t addr, uint8_t value);
    uint8_t readLong(uint16_t addr);
    void writeLong(uint16_t addr, uint8_t value);
    
    int spi_fd;
    bool initialized;
    
    // TX
    bool tx_pending;
    bool tx_ok;
    uint8_t tx_retries;
    uint8_t sequence;
    
    // RX
    uint8_t rx_buffer[MAX_PAYLOAD];
    uint8_t rx_length;
    uint8_t rx_lqi;
    uint8_t rx_rssi;
    bool rx_ready;
};
