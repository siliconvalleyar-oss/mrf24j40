#!/bin/bash

# Script para regenerar proyectos MRF24J40 completamente funcionales
# Ejecutar desde el directorio actual

CURRENT_DIR=$(pwd)

echo "========================================="
echo "Regenerando proyectos MRF24J40 en:"
echo "  $CURRENT_DIR"
echo "========================================="

# Eliminar proyectos antiguos y crear nuevos
rm -rf "$CURRENT_DIR/mrf24_tx" "$CURRENT_DIR/mrf24_rx"

# Crear directorios
mkdir -p "$CURRENT_DIR/mrf24_tx"/{src,bin,obj}
mkdir -p "$CURRENT_DIR/mrf24_rx"/{src,bin,obj}

# ============================================
# ARCHIVO COMPARTIDO: mrf24j40.h
# ============================================

cat > "$CURRENT_DIR/mrf24_tx/src/mrf24j40.h" << 'EOF'
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
EOF

# ============================================
# ARCHIVO COMPARTIDO: mrf24j40.cpp
# ============================================

cat > "$CURRENT_DIR/mrf24_tx/src/mrf24j40.cpp" << 'EOF'
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
EOF

# ============================================
# TRANSMISOR main.cpp
# ============================================

cat > "$CURRENT_DIR/mrf24_tx/src/main.cpp" << 'EOF'
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include "mrf24j40.h"

static volatile bool running = true;
static Mrf24j40 radio;
static int counter = 0;

void signalHandler(int sig) {
    (void)sig;
    printf("\n[INFO] Apagando...\n");
    running = false;
}

int main() {
    signal(SIGINT, signalHandler);
    
    printf("\n========================================\n");
    printf("      MRF24J40 TRANSMISOR v3.0\n");
    printf("========================================\n\n");
    
    if (!radio.init(24)) {
        printf("[ERROR] No se pudo inicializar\n");
        return 1;
    }
    
    // Configurar direcciones
    radio.setPan(0xCAFE);
    radio.setShortAddress(0x6001);
    
    // Verificar configuración
    uint16_t pan = radio.getPan();
    uint16_t addr = radio.getShortAddress();
    
    printf("\n[CONFIGURACION]\n");
    printf("  PAN ID: 0x%04X %s\n", pan, pan == 0xCAFE ? "✅" : "❌");
    printf("  Mi direccion: 0x%04X %s\n", addr, addr == 0x6001 ? "✅" : "❌");
    printf("  Enviando a: 0x6002\n");
    printf("  Intervalo: 2000 ms\n\n");
    
    if (pan != 0xCAFE || addr != 0x6001) {
        printf("[ERROR] Configuracion fallida!\n");
        return 1;
    }
    
    while (running) {
        char message[64];
        snprintf(message, sizeof(message), "MSG%d", counter++);
        
        printf("[TX] Enviando: %s\n", message);
        fflush(stdout);
        
        if (radio.sendString(0x6002, message)) {
            int timeout = 100;
            while (timeout-- && !radio.wasTxSuccessful() && running) {
                radio.checkFlags();
                usleep(10000);
            }
            
            if (radio.wasTxSuccessful()) {
                printf("[TX] ✅ OK (retries: %d)\n", radio.getTxRetries());
            } else {
                printf("[TX] ❌ FALLO\n");
            }
        } else {
            printf("[TX] ❌ Error\n");
        }
        fflush(stdout);
        
        for (int i = 0; i < 20 && running; i++) {
            usleep(100000);
            radio.checkFlags();
        }
    }
    
    printf("\n[INFO] Programa terminado\n");
    return 0;
}
EOF

# ============================================
# RECEPTOR main.cpp
# ============================================

# Copiar archivos al receptor
cp "$CURRENT_DIR/mrf24_tx/src/mrf24j40.h" "$CURRENT_DIR/mrf24_rx/src/"
cp "$CURRENT_DIR/mrf24_tx/src/mrf24j40.cpp" "$CURRENT_DIR/mrf24_rx/src/"

cat > "$CURRENT_DIR/mrf24_rx/src/main.cpp" << 'EOF'
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include "mrf24j40.h"

static volatile bool running = true;
static Mrf24j40 radio;

void signalHandler(int sig) {
    (void)sig;
    printf("\n[INFO] Apagando...\n");
    running = false;
}

int main() {
    signal(SIGINT, signalHandler);
    
    printf("\n========================================\n");
    printf("       MRF24J40 RECEPTOR v3.0\n");
    printf("========================================\n\n");
    
    if (!radio.init(24)) {
        printf("[ERROR] No se pudo inicializar\n");
        return 1;
    }
    
    // Configurar direcciones
    radio.setPan(0xCAFE);
    radio.setShortAddress(0x6002);
    
    // Verificar configuración
    uint16_t pan = radio.getPan();
    uint16_t addr = radio.getShortAddress();
    
    printf("[CONFIGURACION]\n");
    printf("  PAN ID: 0x%04X %s\n", pan, pan == 0xCAFE ? "✅" : "❌");
    printf("  Mi direccion: 0x%04X %s\n", addr, addr == 0x6002 ? "✅" : "❌");
    printf("  Esperando paquetes...\n\n");
    
    int loop_count = 0;
    
    while (running) {
        radio.checkFlags();
        
        if (radio.hasRxPacket()) {
            uint8_t data[100];
            uint8_t len = radio.getRxLength();
            radio.getRxData(data);
            
            printf("\n┌─────────────────────────────────────────┐\n");
            printf("│         PAQUETE RECIBIDO               │\n");
            printf("├─────────────────────────────────────────┤\n");
            printf("│ Datos (%d bytes): ", len);
            for (int i = 0; i < len && i < 80; i++) {
                if (data[i] >= 32 && data[i] <= 126)
                    printf("%c", data[i]);
                else
                    printf(".");
            }
            printf("\n");
            printf("│ LQI: %d, RSSI: %d dBm\n", radio.getLQI(), -(int)radio.getRSSI());
            printf("└─────────────────────────────────────────┘\n");
            fflush(stdout);
        }
        
        loop_count++;
        if (loop_count % 100 == 0) {
            printf("."); fflush(stdout);
        }
        
        usleep(10000);
    }
    
    printf("\n\n[INFO] Programa terminado\n");
    return 0;
}
EOF

# ============================================
# MAKEFILES
# ============================================

cat > "$CURRENT_DIR/mrf24_tx/Makefile" << 'EOF'
CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2 -I./src
LDFLAGS = 

SRCS = src/main.cpp src/mrf24j40.cpp
OBJS = $(SRCS:src/%.cpp=obj/%.o)
TARGET = bin/mrf24_transmitter

all: directories $(TARGET)

directories:
	mkdir -p obj bin

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

obj/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf obj bin

.PHONY: all clean directories
EOF

cat > "$CURRENT_DIR/mrf24_rx/Makefile" << 'EOF'
CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2 -I./src
LDFLAGS = 

SRCS = src/main.cpp src/mrf24j40.cpp
OBJS = $(SRCS:src/%.cpp=obj/%.o)
TARGET = bin/mrf24_receiver

all: directories $(TARGET)

directories:
	mkdir -p obj bin

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

obj/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf obj bin

.PHONY: all clean directories
EOF

# ============================================
# COMPILAR
# ============================================

echo ""
echo "=== COMPILANDO TRANSMISOR ==="
cd "$CURRENT_DIR/mrf24_tx"
make clean
make

echo ""
echo "=== COMPILANDO RECEPTOR ==="
cd "$CURRENT_DIR/mrf24_rx"
make clean
make

echo ""
echo "========================================="
echo "¡PROYECTOS REGENERADOS Y COMPILADOS!"
echo "========================================="
echo ""
echo "Ubicacion:"
echo "  Transmisor: $CURRENT_DIR/mrf24_tx"
echo "  Receptor:   $CURRENT_DIR/mrf24_rx"
echo ""
echo "Para ejecutar:"
echo ""
echo "  Terminal 1 (RECEPTOR):"
echo "    cd $CURRENT_DIR/mrf24_rx && sudo ./bin/mrf24_receiver"
echo ""
echo "  Terminal 2 (TRANSMISOR):"
echo "    cd $CURRENT_DIR/mrf24_tx && sudo ./bin/mrf24_transmitter"
echo ""
echo "========================================="

