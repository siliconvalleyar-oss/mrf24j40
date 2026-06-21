/**
 * @file    drivers/mrf24j40.hpp
 * @brief   Driver completo para el módulo MRF24J40MA (IEEE 802.15.4)
 * @details Soporta múltiples modos de operación: Router, End Device,
 *          Gateway y Nodo. Incluye protocolo de trama seguro con
 *          cifrado AES-256-CBC y hash SHA-256.
 *
 * @author  MRF24J40 Team
 * @date    2026
 * @version 2.0.0
 */

#ifndef DRIVERS_MRF24J40_HPP
#define DRIVERS_MRF24J40_HPP

#include <hal/spi.hpp>
#include <hal/gpio.hpp>
#include <cstdint>
#include <array>
#include <vector>
#include <memory>
#include <functional>

namespace drivers {

// ============================================================================
// Constantes del MRF24J40
// ============================================================================

/** @brief Dirección base del TX Normal FIFO. */
constexpr uint16_t TXNFIFO = 0x300;

/** @brief Dirección base del RX FIFO. */
constexpr uint16_t RXFIFO = 0x300;

/** @brief Longitud máxima del payload IEEE 802.15.4. */
constexpr uint8_t MAX_PAYLOAD_LEN = 100;

/** @brief Longitud del MAC Header. */
constexpr uint8_t MAC_HDR_LEN = 9;

/** @brief Longitud del FCS (CRC16). */
constexpr uint8_t FCS_LEN = 2;

/** @brief Longitud de dirección MAC de 64 bits. */
constexpr uint8_t MAC64_LEN = 8;

// ============================================================================
// Enumeraciones
// ============================================================================

/**
 * @brief Modos de operación del nodo MRF24J40.
 */
enum class NodeMode : uint8_t {
    EndDevice  = 0, /**< Dispositivo final: solo envía/recibe, no reenvía */
    Router     = 1, /**< Router: reenvía paquetes destinados a otros nodos */
    Gateway    = 2, /**< Gateway: reenvía a red externa (Ethernet/WiFi) */
    Node       = 3  /**< Nodo simple: comunicación punto a punto */
};

/**
 * @brief Estados del driver.
 */
enum class DriverState : uint8_t {
    Disconnected,  /**< No inicializado */
    Idle,          /**< Inicializado, esperando */
    Transmitting,  /**< Transmitiendo */
    Receiving,     /**< Recibiendo */
    Error          /**< Error */
};

// ============================================================================
// Estructuras de datos
// ============================================================================

/**
 * @brief Trama completa con header y payload.
 */
struct Frame_t {
    std::array<uint8_t, MAC64_LEN> dest_mac; ///< MAC destino (64 bits)
    uint16_t size;                            ///< Tamaño del payload cifrado
    std::vector<uint8_t> iv;                  ///< IV de 16 bytes (AES-CBC)
    std::vector<uint8_t> ciphertext;          ///< Payload cifrado
    std::array<uint8_t, 32> hash;             ///< SHA-256 del ciphertext+IV
    bool has_hash;                            ///< true si se incluye hash

    Frame_t() : size(0), has_hash(false) {
        dest_mac.fill(0);
        hash.fill(0);
    }
};

/**
 * @brief Estadísticas de transmisión/recepción.
 */
struct Mrf24Stats {
    uint32_t packets_sent = 0;       ///< Paquetes enviados
    uint32_t packets_received = 0;   ///< Paquetes recibidos
    uint32_t tx_success = 0;         ///< TX exitosos
    uint32_t tx_fail = 0;            ///< TX fallidos
    uint32_t tx_retries = 0;         ///< Retransmisiones totales
    uint32_t rx_crc_errors = 0;      ///< Errores CRC en RX
    uint32_t rx_overflow = 0;        ///< Desbordamientos RX FIFO
    float    lqi_avg = 0.0f;         ///< LQI promedio
    float    rssi_avg = 0.0f;        ///< RSSI promedio (dBm)
};

// ============================================================================
// Clase principal
// ============================================================================

/**
 * @brief Driver de alto nivel para el módulo MRF24J40MA.
 *
 * Gestiona la inicialización, configuración de red, transmisión y
 * recepción de tramas IEEE 802.15.4. Soporta modos Router, End Device,
 * Gateway y Nodo. Integra un protocolo de capa de aplicación con
 * cifrado opcional.
 *
 * Uso:
 * @code
 * auto radio = drivers::Mrf24j40_t();
 * radio.init(20);                     // Canal 20
 * radio.setPan(0xCAFE);
 * radio.setMacAddress({0x00, 0x01, ...});  // MAC de 64 bits
 * radio.setMode(drivers::NodeMode::Node);
 *
 * radio.send({0x00, 0x02, ...}, "Hola", 4);
 * @endcode
 */
class Mrf24j40_t {
public:
    Mrf24j40_t();
    ~Mrf24j40_t();

    // No copiable
    Mrf24j40_t(const Mrf24j40_t&) = delete;
    Mrf24j40_t& operator=(const Mrf24j40_t&) = delete;

    // === Inicialización ===

    /**
     * @brief Inicializa el módulo MRF24J40.
     * @param channel Canal IEEE 802.15.4 (11-26).
     * @return true si éxito.
     */
    bool init(uint8_t channel);

    /**
     * @brief Configura el modo de operación del nodo.
     * @param mode Router, EndDevice, Gateway o Node.
     */
    void setMode(NodeMode mode);

    /** @return Modo de operación actual. */
    NodeMode mode() const { return m_mode; }

    // === Configuración de red ===

    /** @brief Configura el PAN ID. */
    void setPan(uint16_t pan);

    /** @brief Obtiene el PAN ID. */
    uint16_t pan() const;

    /** @brief Configura dirección corta de 16 bits. */
    void setShortAddress(uint16_t addr);

    /** @brief Obtiene dirección corta. */
    uint16_t shortAddress() const;

    /** @brief Configura dirección MAC de 64 bits. */
    void setMacAddress(const std::array<uint8_t, MAC64_LEN>& mac);

    /** @brief Obtiene dirección MAC de 64 bits. */
    std::array<uint8_t, MAC64_LEN> macAddress() const;

    /** @brief Cambia el canal de operación. */
    bool setChannel(uint8_t channel);

    // === Transmisión ===

    /**
     * @brief Envía un paquete de datos (modo direccionamiento corto).
     * @param dest_dir MAC destino de 64 bits.
     * @param data     Payload a enviar.
     * @param len      Longitud del payload.
     * @return true si el paquete se encoló.
     */
    bool send(const std::array<uint8_t, MAC64_LEN>& dest_mac,
              const uint8_t* data, uint8_t len);

    /**
     * @brief Envía un string como paquete.
     */
    bool sendString(const std::array<uint8_t, MAC64_LEN>& dest_mac,
                    const std::string_view& str);

    /**
     * @brief Envía una trama completa (con cifrado opcional).
     * @param frame Trama con header, IV, ciphertext y hash.
     * @return true si se encoló.
     */
    bool sendFrame(const Frame_t& frame);

    // === Recepción ===

    /**
     * @brief Verifica si hay un paquete disponible.
     * @return true si hay datos listos para leer.
     */
    bool hasPacket() const { return m_rx_ready; }

    /**
     * @brief Obtiene el último paquete recibido.
     * @param[out] frame Trama recibida completa.
     */
    void getFrame(Frame_t& frame);

    /**
     * @brief Obtiene los datos raw del último paquete.
     * @param[out] buf Buffer para los datos.
     * @return Longitud de los datos.
     */
    uint8_t getData(uint8_t* buf);

    // === Polling ===

    /**
     * @brief Procesa eventos TX/RX. Debe llamarse periódicamente.
     */
    void poll();

    // === Estadísticas ===

    /** @brief Obtiene estadísticas actuales. */
    void getStats(Mrf24Stats& stats) const;

    /** @brief Reinicia estadísticas. */
    void resetStats();

    /** @brief LQI del último paquete recibido. */
    uint8_t lastLqi() const { return m_last_lqi; }

    /** @brief RSSI del último paquete recibido (dBm). */
    int8_t lastRssi() const { return m_last_rssi; }

    // === Diagnóstico ===

    /** @brief Imprime registros clave. */
    void printRegisters();

    /** @brief Autodiagnóstico del módulo. */
    bool selfTest();

    // === Callbacks ===

    /** @brief Callback invocado al recibir un paquete. */
    std::function<void(const uint8_t*, uint8_t)> onReceive;

    /** @brief Callback invocado al completar una TX. */
    std::function<void(bool success, uint8_t retries)> onTransmit;

private:
    // === SPI ===
    std::unique_ptr<hal::Spi_t> m_spi;  ///< Bus SPI

    // === Configuración ===
    NodeMode m_mode = NodeMode::Node;
    uint16_t m_pan_id = 0xCAFE;
    uint16_t m_short_addr = 0x0001;
    std::array<uint8_t, MAC64_LEN> m_mac64{};
    uint8_t m_channel = 20;
    bool m_initialized = false;

    // === Estado TX ===
    bool m_tx_pending = false;
    bool m_tx_ok = false;
    uint8_t m_tx_retries = 0;
    uint8_t m_seq = 0;

    // === Estado RX ===
    bool m_rx_ready = false;
    uint8_t m_rx_buf[MAX_PAYLOAD_LEN];
    uint8_t m_rx_len = 0;
    uint8_t m_last_lqi = 0;
    int8_t m_last_rssi = 0;

    // === Estadísticas ===
    Mrf24Stats m_stats;

    // === Operaciones SPI de bajo nivel ===
    uint8_t readShort(uint8_t addr);
    void    writeShort(uint8_t addr, uint8_t val);
    uint8_t readLong(uint16_t addr);
    void    writeLong(uint16_t addr, uint8_t val);

    // === Inicialización ===
    bool waitForReset();
    void rfReset();
    void flushRx();

    // === Handlers de interrupción ===
    void handleTxIrq();
    void handleRxIrq();
};

} // namespace drivers

#endif // DRIVERS_MRF24J40_HPP
