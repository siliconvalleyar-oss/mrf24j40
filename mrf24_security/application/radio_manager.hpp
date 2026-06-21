/**
 * @file    application/radio_manager.hpp
 * @brief   Orquestador del sistema de comunicación segura
 * @details Coordina el módulo radio MRF24J40, el cifrado AES-256,
 *          los displays y el protocolo de trama completo. Es el
 *          punto central de integración entre todos los módulos.
 *
 * @author  MRF24J40 Team
 * @date    2026
 * @version 2.0.0
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
 * recibir → descifrar → mostrar. Integra todos los subsistemas
 * (radio, crypto, displays, QR, configuración, logging).
 *
 * Uso:
 * @code
 * auto mgr = application::RadioManager_t();
 * mgr.init();
 * mgr.sendMessage({0x00, 0x02, ...}, "Hola mundo");
 *
 * mgr.onMessage([](auto& from, auto& msg, auto lqi, auto rssi) {
 *     std::cout << "De: " << from << " Msg: " << msg << "\n";
 * });
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

    // === Mensajes ===

    /**
     * @brief Envía un mensaje de texto cifrado.
     * @param dest_mac Dirección MAC destino (64 bits).
     * @param message  Mensaje a enviar.
     * @return true si el mensaje se encoló para TX.
     */
    bool sendMessage(const std::array<uint8_t, 8>& dest_mac,
                     std::string_view message);

    /**
     * @brief Envía datos raw cifrados.
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
     * @brief Procesa paquetes entrantes. Debe llamarse periódicamente.
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
    bool m_oled_ok;
    bool m_tft_ok;

    // === Mensajes ===
    std::string m_last_message;
    bool m_message_ready;
    MessageCallback m_msg_callback;

    // === Estadísticas ===
    uint32_t m_packet_count;

    // === Internos ===
    void setupRadio();
    void log(std::string_view msg, services::LogLevel level = services::LogLevel::Info);
};

} // namespace application

#endif // APPLICATION_RADIO_MANAGER_HPP
