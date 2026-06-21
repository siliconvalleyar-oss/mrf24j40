/**
 * @file    mrf24j40.h
 * @brief   Driver simplificado MRF24J40MA (IEEE 802.15.4 / ZigBee)
 * @details Proporciona la clase Mrf24j40 con una API minimalista para
 *          inicialización, configuración de red, transmisión/recepción
 *          de tramas IEEE 802.15.4 y estadísticas. La comunicación SPI
 *          se realiza mediante ioctl directo a spidev.
 *
 * @author  MRF24J40 Team
 * @date    2026
 * @version 2.0.2
 */

#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <unistd.h>

// ============================================================================
// Constantes de hardware y protocolo
// ============================================================================

/** @brief Dispositivo SPI para el MRF24J40 */
#define SPI_DEVICE      "/dev/spidev0.0"

/** @brief Velocidad del bus SPI en Hz */
#define SPI_SPEED_HZ    2000000

/** @brief Tamaño máximo de payload útil (127 - header 9 - FCS 2) */
#define MAX_PAYLOAD     116

// ============================================================================
// Direcciones de registros (short)
// ============================================================================
#define REG_PANIDL      0x01
#define REG_PANIDH      0x02
#define REG_SADRL       0x03
#define REG_SADRH       0x04
#define REG_RXFLUSH     0x0D
#define REG_TXNCON      0x1B
#define REG_TXSTAT      0x24
#define REG_SOFTRST     0x2A
#define REG_RFCTL       0x36
#define REG_BBREG1      0x39
#define REG_BBREG2      0x3A
#define REG_BBREG6      0x3E
#define REG_CCAEDTH     0x3F
#define REG_PACON2      0x18
#define REG_TXSTBL      0x2E
#define REG_INTCON      0x32
#define REG_INTSTAT     0x31

// ============================================================================
// Direcciones de registros (long)
// ============================================================================
#define LREG_RFCON0     0x200
#define LREG_RFCON1     0x201
#define LREG_RFCON2     0x202
#define LREG_RFCON3     0x203
#define LREG_RFCON6     0x206
#define LREG_RFCON7     0x207
#define LREG_RFCON8     0x208
#define LREG_SLPCON1    0x220

// ============================================================================
// Máscaras de interrupción
// ============================================================================
#define INT_TXNIF       0x01
#define INT_RXIF        0x08

// ============================================================================
// Estructura de estadísticas
// ============================================================================

/**
 * @brief Estadísticas acumuladas de transmisión y recepción
 */
struct RadioStats {
    uint32_t packets_sent{};        /**< Paquetes enviados (totales) */
    uint32_t tx_success{};          /**< TX con ACK recibido */
    uint32_t tx_fail{};             /**< TX sin ACK */
    uint8_t  tx_retries_total{};    /**< Retransmisiones acumuladas */
    uint32_t packets_received{};    /**< Paquetes recibidos */
    int32_t  rx_lqi_sum{};          /**< Suma de LQI para promedio */
    int32_t  rx_rssi_sum{};         /**< Suma de RSSI para promedio */
    uint32_t rx_count{};            /**< Contador de muestras RSSI/LQI */
};

// ============================================================================
// Clase Mrf24j40 — Driver simplificado
// ============================================================================

/**
 * @brief   Driver simplificado para el módulo MRF24J40MA
 *
 * Proporciona una API limpia y minimalista para operar el transceiver
 * IEEE 802.15.4. La comunicación SPI se realiza mediante ioctl directo
 * a spidev, sin dependencia de librerías externas.
 *
 * Uso básico:
 * @code
 *   Mrf24j40 radio;
 *   if (!radio.init(20)) return 1;          // Canal 20
 *   radio.setPan(0xCAFE);
 *   radio.setShortAddress(0x0001);
 *   radio.send(0x0002, radio.getPan(), data, len);
 *   radio.poll();                            // Llamar periódicamente
 * @endcode
 */
class Mrf24j40 {
public:
    /** @brief Constructor: inicializa miembros a cero */
    Mrf24j40();

    /** @brief Destructor: cierre del file descriptor SPI */
    ~Mrf24j40();

    // ========================================================================
    // Inicialización y configuración
    // ========================================================================

    /**
     * @brief Inicializa el módulo MRF24J40
     * @param channel Canal IEEE 802.15.4 (11-26)
     * @return true si la inicialización fue exitosa
     */
    bool init(uint8_t channel);

    /**
     * @brief Configura el PAN ID de la red
     * @param pan Identificador de red (16 bits)
     */
    void setPan(uint16_t pan);

    /**
     * @brief Configura la dirección corta de 16 bits
     * @param addr Dirección corta del dispositivo
     */
    void setShortAddress(uint16_t addr);

    /**
     * @brief Obtiene el PAN ID actual del módulo
     * @return PAN ID de 16 bits
     */
    uint16_t getPan();

    /**
     * @brief Obtiene la dirección corta actual del módulo
     * @return Dirección corta de 16 bits
     */
    uint16_t getShortAddress();

    /**
     * @brief Cambia el canal de operación
     * @param ch Canal (11-26)
     * @return true si el canal es válido
     */
    bool setChannel(uint8_t ch);

    // ========================================================================
    // Transmisión
    // ========================================================================

    /**
     * @brief Envía un paquete de datos
     * @param dest_addr Dirección corta de destino (16 bits)
     * @param dest_pan  PAN ID de destino
     * @param data      Puntero a los datos a enviar
     * @param len       Longitud de los datos (máx MAX_PAYLOAD)
     * @return true si el paquete se encoló para TX
     */
    bool send(uint16_t dest_addr, uint16_t dest_pan, const uint8_t* data, uint8_t len);

    /**
     * @brief Envía un string como paquete (wrapper de send)
     * @param dest_addr Dirección corta de destino
     * @param str       String null-terminated a enviar
     * @return true si se encoló correctamente
     */
    bool sendString(uint16_t dest_addr, const char* str);

    // ========================================================================
    // Polling de interrupciones
    // ========================================================================

    /**
     * @brief Polling de eventos TX/RX
     *
     * Debe llamarse periódicamente. Lee INTSTAT y procesa
     * las interrupciones de transmisión y recepción pendientes.
     */
    void poll();

    // ========================================================================
    // Estado de TX
    // ========================================================================

    /** @brief Verifica si hay una transmisión pendiente */
    bool txDone() { return !tx_pending; }

    /** @brief Verifica si la última transmisión fue exitosa */
    bool txSuccess() { return tx_ok; }

    /** @brief Obtiene el número de retransmisiones de la última TX */
    uint8_t txRetries() { return tx_retries; }

    // ========================================================================
    // Recepción
    // ========================================================================

    /** @brief Verifica si hay un paquete recibido disponible */
    bool rxAvailable() { return rx_ready; }

    /** @brief Obtiene la longitud del último paquete recibido */
    uint8_t rxLen() { return rx_length; }

    /** @brief Obtiene el LQI del último paquete recibido */
    uint8_t rxLqi() { return rx_lqi; }

    /** @brief Obtiene el RSSI en dBm del último paquete recibido */
    int8_t rxRssi() { return rx_rssi_dbm; }

    /**
     * @brief Obtiene el último paquete recibido
     * @param buf Buffer donde copiar los datos (debe tener al menos rx_length bytes)
     */
    void rxGet(uint8_t* buf);

    // ========================================================================
    // Estadísticas y diagnóstico
    // ========================================================================

    /**
     * @brief Copia las estadísticas actuales
     * @param s Estructura RadioStats donde copiar los datos
     */
    void getStats(RadioStats& s);

    /** @brief Reinicia las estadísticas acumuladas */
    void resetStats();

    /** @brief Imprime el estado de los registros clave */
    void printRegisters();

    /**
     * @brief Autodiagnóstico (prueba escritura/lectura PAN ID)
     * @return true si el módulo responde correctamente
     */
    bool selfTest();

    /** @brief Limpia el FIFO de recepción */
    void flushRx();

    /** @brief Resetea la máquina de estados RF */
    void rfReset();

protected:
    // ========================================================================
    // Operaciones SPI de bajo nivel
    // ========================================================================

    /** @brief Lee un registro de dirección corta (8 bits) */
    uint8_t readShort(uint8_t addr);

    /** @brief Escribe un registro de dirección corta (8 bits) */
    void writeShort(uint8_t addr, uint8_t val);

    /** @brief Lee un registro de dirección larga (16 bits) */
    uint8_t readLong(uint16_t addr);

    /** @brief Escribe un registro de dirección larga (16 bits) */
    void writeLong(uint16_t addr, uint8_t val);

    /** @brief Espera hasta que el soft reset se complete */
    bool waitForReset();

    /** @brief Procesa interrupción de TX completada */
    void handleTxIrq();

    /** @brief Procesa interrupción de RX (paquete recibido) */
    void handleRxIrq();

private:
    int     spi_fd{-1};             /**< File descriptor del dispositivo SPI */
    bool    initialized{false};     /**< Flag de inicialización */
    bool    tx_pending{false};      /**< Hay una TX en curso */
    bool    tx_ok{false};           /**< Resultado de la última TX */
    uint8_t tx_retries{0};          /**< Retransmisiones de la última TX */
    uint8_t seq{0};                 /**< Número de secuencia IEEE 802.15.4 */
    uint8_t rx_buf[MAX_PAYLOAD];    /**< Buffer del último paquete recibido */
    uint8_t rx_length{0};           /**< Longitud del último RX */
    uint8_t rx_lqi{0};              /**< LQI del último RX */
    int8_t  rx_rssi_dbm{0};         /**< RSSI en dBm del último RX */
    bool    rx_ready{false};        /**< Hay un paquete RX disponible */
    RadioStats stats{};             /**< Estadísticas acumuladas */
};
