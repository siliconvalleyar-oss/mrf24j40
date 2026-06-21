/**
 * @file    application/radio_manager.hpp
 * @brief   Orquestador del sistema de comunicación segura
 * @details Coordina el módulo radio MRF24J40, el cifrado AES-256,
 *          los displays y el protocolo de trama completo. Es el
 *          punto central de integración entre todos los módulos.
 *          Incluye validación de mensajes por roles y hash SHA-256.
 *
 * @author  MRF24J40 Team
 * @date    2026
 * @version 2.0.3
 */

#ifndef APPLICATION_RADIO_MANAGER_HPP
#define APPLICATION_RADIO_MANAGER_HPP

#include <drivers/mrf24j40.hpp>
#include <drivers/ssd1306.hpp>
#include <drivers/st7789.hpp>
#include <drivers/qr.hpp>
#include <services/crypto.hpp>
#include <services/filesystem.hpp>
#include <services/timer.hpp>
#include <memory>
#include <functional>

namespace application {

// ============================================================================
// Constantes del protocolo seguro
// ============================================================================

/** @brief TTL máximo por defecto para mensajes. */
constexpr uint8_t DEFAULT_TTL = 10;

/** @brief TTL fijo para mensajes broadcast. Bajo para evitar flooding infinito. */
constexpr uint8_t BROADCAST_TTL = 3;

/** @brief Dirección de broadcast (todos los nodos). */
constexpr uint64_t BROADCAST_ADDR = 0xFFFFFFFFFFFFFFFFULL;

/** @brief Tamaño del campo src_mac en la trama. */
constexpr size_t FRAME_SRC_MAC_LEN = 8;

/** @brief Tamaño del campo dest_mac en la trama. */
constexpr size_t FRAME_DEST_MAC_LEN = 8;

/** @brief Tamaño del campo TTL (1 byte). */
constexpr size_t FRAME_TTL_LEN = 1;

/** @brief Tamaño del campo size (2 bytes). */
constexpr size_t FRAME_SIZE_LEN = 2;

/** @brief Tamaño del IV (16 bytes). */
constexpr size_t FRAME_IV_LEN = 16;

/** @brief Tamaño del hash SHA-256 (32 bytes). */
constexpr size_t FRAME_HASH_LEN = 32;

/** @brief Overhead total de la trama de aplicación. */
constexpr size_t FRAME_OVERHEAD = FRAME_DEST_MAC_LEN + FRAME_SRC_MAC_LEN +
                                  FRAME_TTL_LEN + FRAME_SIZE_LEN +
                                  FRAME_IV_LEN + FRAME_HASH_LEN;  // = 67

/**
 * @brief Estadísticas de validación de mensajes.
 */
struct ValidationStats {
    uint32_t messages_validated  = 0;  ///< Mensajes validados correctamente
    uint32_t messages_rejected   = 0;  ///< Mensajes rechazados (hash inválido)
    uint32_t messages_forwarded  = 0;  ///< Mensajes reenviados a otro nodo
    uint32_t ttl_expired         = 0;  ///< Mensajes descartados por TTL=0
    uint32_t role_mismatch       = 0;  ///< Mensajes ignorados por rol (EndDevice)
    uint32_t hash_errors         = 0;  ///< Errores de hash SHA-256
    uint32_t routing_not_found   = 0;  ///< Sin ruta para reenviar
    uint32_t broadcasts_forwarded = 0;  ///< Broadcasts reenviados a todos los nodos
    uint32_t source_denied        = 0;  ///< MAC origen no está en whitelist
};

/**
 * @brief Resultado de la validación de un mensaje.
 */
struct ValidationResult {
    bool     valid        = false;  ///< true si el mensaje pasó todas las validaciones
    bool     for_us       = false;  ///< true si el destino es este dispositivo
    bool     should_forward = false; ///< true si debe reenviarse
    uint64_t dest_mac     = 0;      ///< MAC destino extraída
    uint64_t src_mac      = 0;      ///< MAC origen extraída
    uint8_t  ttl          = 0;      ///< TTL restante
    uint16_t payload_size = 0;      ///< Tamaño del ciphertext
    std::vector<uint8_t> iv;         ///< IV extraído
    std::vector<uint8_t> ciphertext; ///< Ciphertext extraído
    std::string error_msg;           ///< Mensaje de error si no es válido
};

/** @brief Alias para el rol definido en services. */
using NodeRole = services::NodeRole;

/** @brief Alias para la tabla de rutas. */
using RoutingTable = std::vector<std::pair<uint64_t, uint64_t>>;

/**
 * @brief Callback para notificar recepción de mensajes descifrados.
 */
using MessageCallback = std::function<void(const std::string& sender,
                                           const std::string& message,
                                           uint8_t lqi, int8_t rssi)>;

/**
 * @brief Orquestador principal del sistema de comunicación.
 *
 * Gestiona el ciclo completo: configurar → cifrar → transmitir →
 * recibir → validar → descifrar → mostrar. Integra todos los subsistemas
 * (radio, crypto, displays, QR, configuración, logging).
 *
 * Desde v2.0.3 incluye:
 * - Roles de red (EndDevice, Router, Coordinator, Mesh)
 * - Validación de mensajes con hash SHA-256
 * - Enrutamiento y reenvío con TTL
 * - Tabla de rutas persistente en JSON
 *
 * Uso:
 * @code
 * auto mgr = application::RadioManager_t();
 * mgr.init();
 * mgr.setRole(NodeRole::Router);
 * mgr.addRoute(0x0002, 0x0001);
 * mgr.sendMessage({0x00, 0x02, ...}, "Hola mundo");
 * @endcode
 */
class RadioManager_t {
public:
    RadioManager_t();
    ~RadioManager_t();

    // No copiable
    RadioManager_t(const RadioManager_t&) = delete;
    RadioManager_t& operator=(const RadioManager_t&) = delete;

    // === Inicialización ===

    /**
     * @brief Inicializa todos los subsistemas.
     * @return true si todos los componentes se inicializaron correctamente.
     */
    bool init();

    /**
     * @brief Inicializa desde configuración cargada.
     * @param config Configuración del sistema.
     * @return true si éxito.
     */
    bool initFromConfig(const services::SystemConfig& config);

    // === Configuración ===

    /**
     * @brief Carga configuración desde archivo JSON.
     * @return Configuración cargada.
     */
    services::SystemConfig loadConfig();

    /**
     * @brief Guarda configuración actual a archivo JSON.
     * @return true si éxito.
     */
    bool saveConfig();

    /**
     * @brief Obtiene la configuración actual.
     */
    const services::SystemConfig& config() const { return m_config; }

    /**
     * @brief Actualiza la configuración en runtime.
     * @param config Nueva configuración.
     */
    void setConfig(const services::SystemConfig& config);

    // === Radio ===

    /**
     * @brief Cambia el modo de operación del nodo.
     */
    void setNodeMode(drivers::NodeMode mode);

    /**
     * @brief Obtiene el modo actual.
     */
    drivers::NodeMode nodeMode() const;

    /**
     * @brief Obtiene estadísticas de la radio.
     */
    drivers::Mrf24Stats radioStats() const;

    // === Roles ===

    /**
     * @brief Configura el rol del dispositivo en la red.
     * @param role EndDevice, Router, Coordinator o Mesh.
     */
    void setRole(NodeRole role);

    /**
     * @brief Obtiene el rol actual del dispositivo.
     */
    NodeRole role() const { return m_config.role; }

    /**
     * @brief Verifica si el rol actual permite reenvío.
     * @return true si Router, Coordinator o Mesh.
     */
    bool canForward() const;

    // === Whitelist ===

    /**
     * @brief Habilita o deshabilita la verificación de whitelist.
     */
    void setWhitelistEnabled(bool enabled);

    /**
     * @brief true si la whitelist está activa.
     */
    bool isWhitelistEnabled() const { return m_config.enable_whitelist; }

    /**
     * @brief Verifica si una MAC origen está permitida.
     * @param src_mac Dirección MAC de 64 bits.
     * @return true si está en la whitelist (o whitelist desactivada).
     */
    bool isSourceAllowed(uint64_t src_mac) const;

    /**
     * @brief Agrega una MAC a la whitelist de orígenes permitidos.
     * @param src_mac Dirección MAC de 64 bits.
     */
    void addAllowedSource(uint64_t src_mac);

    /**
     * @brief Elimina una MAC de la whitelist.
     * @param src_mac Dirección MAC de 64 bits.
     */
    void removeAllowedSource(uint64_t src_mac);

    /**
     * @brief Limpia toda la whitelist.
     */
    void clearAllowedSources();

    /**
     * @brief Obtiene la lista de MACs permitidas.
     * @return Referencia constante al vector de MACs.
     */
    const std::vector<uint64_t>& allowedSources() const { return m_config.allowed_sources; }

    // === Routing ===

    /**
     * @brief Agrega una ruta a la tabla de enrutamiento.
     * @param dest    Dirección MAC destino (64 bits).
     * @param nextHop Dirección MAC del siguiente salto.
     */
    void addRoute(uint64_t dest, uint64_t nextHop);

    /**
     * @brief Elimina una ruta de la tabla.
     * @param dest Dirección MAC destino.
     */
    void removeRoute(uint64_t dest);

    /**
     * @brief Obtiene la tabla de rutas completa.
     * @return Referencia constante a la tabla de rutas.
     */
    const RoutingTable& routingTable() const { return m_config.routing_table; }

    /**
     * @brief Busca el siguiente salto para un destino.
     * @param dest Dirección MAC destino.
     * @param[out] nextHop Siguiente salto si se encuentra.
     * @return true si se encontró una ruta.
     */
    bool findRoute(uint64_t dest, uint64_t& nextHop) const;

    // === Protocolo seguro ===

    /**
     * @brief Construye una trama segura con hash SHA-256.
     *
     * Formato de la trama:
     *   [dest_mac(8)] [src_mac(8)] [ttl(1)] [size(2)] [iv(16)] [ciphertext(N)] [hash(32)]
     *
     * El hash cubre: dest_mac + size + iv + ciphertext
     *
     * @param dest_mac  Dirección MAC destino.
     * @param iv        IV del cifrado (16 bytes).
     * @param ciphertext Datos cifrados.
     * @param ttl       TTL (hops máximos), por defecto DEFAULT_TTL.
     * @return Trama completa lista para enviar por radio.
     */
    std::vector<uint8_t> buildSecureMessage(const std::array<uint8_t, 8>& dest_mac,
                                             const std::vector<uint8_t>& iv,
                                             const std::vector<uint8_t>& ciphertext,
                                             uint8_t ttl = DEFAULT_TTL);

    /**
     * @brief Valida un mensaje recibido.
     *
     * Realiza las siguientes verificaciones:
     * 1. Parsear la trama (dest_mac, src_mac, ttl, iv, ciphertext, hash)
     * 2. Verificar TTL > 0
     * 3. Verificar hash SHA-256
     * 4. Determinar si es para nosotros o debe reenviarse
     *
     * @param rawMessage Datos crudos recibidos del radio.
     * @param len        Longitud de los datos.
     * @return ValidationResult con el resultado de la validación.
     */
    ValidationResult validateMessage(const uint8_t* rawMessage, uint8_t len);

    /**
     * @brief Reenvía un mensaje al siguiente salto.
     *
     * Decrementa el TTL, recalcula el hash y envía al nextHop.
     *
     * @param msg     Trama original (sin modificar).
     * @param nextHop Dirección MAC del siguiente salto.
     * @return true si el reenvío fue exitoso.
     */
    bool forwardMessage(const std::vector<uint8_t>& msg, uint64_t nextHop);

    /**
     * @brief Obtiene la dirección MAC local como entero de 64 bits.
     * @return MAC address en uint64_t (big-endian).
     */
    uint64_t localMac64() const;

    /**
     * @brief Obtiene estadísticas de validación.
     */
    const ValidationStats& validationStats() const { return m_validation_stats; }

    // === Mensajes ===

    /**
     * @brief Envía un mensaje de texto cifrado con protocolo seguro.
     * @param dest_mac Dirección MAC destino (64 bits).
     * @param message  Mensaje a enviar.
     * @return true si el mensaje se encoló para TX.
     */
    bool sendMessage(const std::array<uint8_t, 8>& dest_mac,
                     std::string_view message);

    /**
     * @brief Envía datos raw cifrados con protocolo seguro.
     * @param dest_mac Dirección MAC destino.
     * @param data     Datos a enviar.
     * @param len      Longitud.
     * @return true si éxito.
     */
    bool sendData(const std::array<uint8_t, 8>& dest_mac,
                  const uint8_t* data, uint8_t len);

    /**
     * @brief Genera un QR del mensaje y lo envía.
     * @param dest_mac Dirección MAC destino.
     * @param message  Mensaje a codificar en QR y enviar.
     * @return true si éxito.
     */
    bool sendQrMessage(const std::array<uint8_t, 8>& dest_mac,
                       std::string_view message);

    // === Recepción ===

    /**
     * @brief Procesa paquetes entrantes con validación completa.
     *        Debe llamarse periódicamente.
     */
    void process();

    /**
     * @brief Verifica si hay un nuevo mensaje disponible.
     */
    bool hasMessage() const;

    /**
     * @brief Obtiene el último mensaje descifrado.
     * @param[out] message Mensaje recibido.
     * @return true si hay mensaje.
     */
    bool getMessage(std::string& message);

    /**
     * @brief Callback invocado al recibir un mensaje.
     */
    void onMessage(MessageCallback callback) { m_msg_callback = std::move(callback); }

    // === Displays ===

    /** @brief Muestra información en OLED (si está disponible). */
    void updateOled();

    /** @brief Muestra un QR en OLED. */
    void showQrOnOled(std::string_view text);

    /** @brief Muestra un QR en TFT (si está disponible). */
    void showQrOnTft(std::string_view text);

    // === QR ===

    /**
     * @brief Genera y guarda un QR como PNG.
     * @param text Texto a codificar.
     * @param filename Ruta del archivo.
     * @return true si éxito.
     */
    bool saveQrPng(std::string_view text, std::string_view filename);

    /** @brief Imprime un QR en consola. */
    void printQr(std::string_view text);

    // === Logging ===

    /** @brief Muestra las últimas N líneas del log. */
    void showLog(size_t lines = 50);

    /** @brief Limpia el archivo de log. */
    void clearLog();

    // === Estado ===

    /** @brief true si el sistema está inicializado. */
    bool isReady() const { return m_initialized; }

    /** @brief true si la radio está operativa. */
    bool isRadioReady() const { return m_radio_ok; }

    /** @brief Acceso al driver de radio (para debug). */
    drivers::Mrf24j40_t& radio() { return *m_radio; }

    /** @brief Acceso al servicio crypto. */
    services::Crypto_t& crypto() { return *m_crypto; }

private:
    // === Subsistemas ===
    std::unique_ptr<drivers::Mrf24j40_t> m_radio;
    std::unique_ptr<drivers::Ssd1306_t>  m_oled;
    std::unique_ptr<drivers::St7789_t>   m_tft;
    std::unique_ptr<drivers::Qr_t>        m_qr;
    std::unique_ptr<services::Crypto_t>   m_crypto;
    std::unique_ptr<services::FileSystem_t> m_fs;

    // === Configuración ===
    services::SystemConfig m_config;
    bool m_initialized;
    bool m_radio_ok;
    bool m_oled_ok;
    bool m_tft_ok;

    // === Roles & Routing ===
    NodeRole m_role;
    RoutingTable m_routing_table;

    // === Estadísticas ===
    uint32_t m_packet_count;

    // === Estadísticas de validación ===
    ValidationStats m_validation_stats;

    // === Broadcast (prevención de flooding duplicado) ===
    static constexpr size_t BROADCAST_HISTORY_SIZE = 16;
    std::vector<std::array<uint8_t, FRAME_HASH_LEN>> m_broadcast_history;

    /**
     * @brief Verifica si ya procesamos este broadcast (por hash).
     * @param hash Hash SHA-256 de 32 bytes.
     * @return true si el hash ya está en el historial.
     */
    bool isBroadcastDuplicate(const std::array<uint8_t, FRAME_HASH_LEN>& hash);

    // === Mensajes ===
    std::string m_last_message;
    bool m_message_ready;
    MessageCallback m_msg_callback;

    // === Internos ===
    void setupRadio();

    /**
     * @brief Procesa un mensaje validado localmente (descifra, muestra, callback).
     * @param buf        Buffer del paquete original.
     * @param len        Longitud del paquete.
     * @param validation Resultado de la validación ya procesado.
     */
    void processMessage(const uint8_t* buf, uint8_t len, const ValidationResult& validation);

    void log(std::string_view msg, services::LogLevel level = services::LogLevel::Info);

    /**
     * @brief Convierte un array de 8 bytes MAC a uint64_t (big-endian).
     */
    static uint64_t macToUint64(const std::array<uint8_t, 8>& mac);

    /**
     * @brief Convierte un uint64_t a array de 8 bytes MAC (big-endian).
     */
    static std::array<uint8_t, 8> uint64ToMac(uint64_t addr);

    /**
     * @brief Convierte MAC uint64_t a string hex (para logging).
     */
    static std::string macToHex(uint64_t addr);
};

} // namespace application

#endif // APPLICATION_RADIO_MANAGER_HPP
