/**
 * @file    services/filesystem.hpp
 * @brief   Servicio de sistema de archivos: configuración JSON y logs
 * @details Gestiona la configuración persistente en formato JSON
 *          (usando nlohmann/json) y el registro de eventos en
 *          archivos de log rotativos.
 *
 * @author  MRF24J40 Team
 * @date    2026
 * @version 2.0.0
 */

#ifndef SERVICES_FILESYSTEM_HPP
#define SERVICES_FILESYSTEM_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <fstream>
#include <mutex>
#include <array>

namespace services {

/**
 * @brief Estructura de configuración completa del sistema.
 */
struct SystemConfig {
    // Radio
    uint16_t pan_id = 0xCAFE;
    uint8_t channel = 20;
    uint8_t node_mode = 0;         // 0=EndDevice, 1=Router, 2=Gateway, 3=Node
    std::array<uint8_t, 8> mac_address{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
    std::string passphrase = "mrf24j40-secret";

    // Display
    bool use_oled = true;
    bool use_tft = false;
    uint8_t contrast = 207;

    // Logging
    bool log_to_file = true;
    std::string log_file = "mrf24j40.log";
    bool log_rx = true;
    bool log_tx = true;
    bool log_errors = true;

    // Security
    bool enable_encryption = true;
    bool enable_hash = true;
};

/**
 * @brief Niveles de severidad para el logging.
 */
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

/**
 * @brief Servicio de configuración y logging.
 *
 * Carga/guarda configuración en JSON y registra eventos
 * con timestamp en un archivo de log.
 *
 * Uso:
 * @code
 * auto fs = services::FileSystem_t("config.json");
 * auto cfg = fs.loadConfig();
 * cfg.channel = 25;
 * fs.saveConfig(cfg);
 * fs.log("Sistema iniciado", services::LogLevel::Info);
 * @endcode
 */
class FileSystem_t {
public:
    /**
     * @brief Constructor.
     * @param config_file Ruta al archivo de configuración JSON.
     */
    explicit FileSystem_t(std::string_view config_file = "config.json");

    ~FileSystem_t();

    // No copiable
    FileSystem_t(const FileSystem_t&) = delete;
    FileSystem_t& operator=(const FileSystem_t&) = delete;

    // === Configuración ===

    /**
     * @brief Carga la configuración desde el archivo JSON.
     * @return Estructura SystemConfig con valores cargados
     *         (o valores por defecto si el archivo no existe).
     */
    SystemConfig loadConfig();

    /**
     * @brief Guarda la configuración en el archivo JSON.
     * @param config Configuración a guardar.
     * @return true si la operación fue exitosa.
     */
    bool saveConfig(const SystemConfig& config);

    /**
     * @brief Convierte la configuración a string JSON formateado.
     */
    std::string configToString(const SystemConfig& config);

    /**
     * @brief Aplica configuración desde un string JSON.
     */
    SystemConfig configFromString(std::string_view json);

    // === Logging ===

    /**
     * @brief Registra un mensaje en el archivo de log.
     * @param message Mensaje a registrar.
     * @param level   Nivel de severidad.
     */
    void log(std::string_view message, LogLevel level = LogLevel::Info);

    /**
     * @brief Registra un evento de paquete TX/RX.
     * @param direction "TX" o "RX"
     * @param len       Longitud del paquete.
     * @param lqi       LQI (solo RX).
     * @param rssi      RSSI en dBm (solo RX).
     * @param hex_data  Datos en hexadecimal.
     */
    void logPacket(std::string_view direction, uint8_t len,
                   uint8_t lqi, int8_t rssi,
                   std::string_view hex_data);

    /**
     * @brief Obtiene las últimas N líneas del log.
     * @param lines Número de líneas.
     * @return Vector con las líneas solicitadas.
     */
    std::vector<std::string> getRecentLog(size_t lines = 50);

    /**
     * @brief Limpia el archivo de log.
     * @return true si éxito.
     */
    bool clearLog();

    /**
     * @brief Verifica la integridad del archivo de configuración.
     * @return true si el JSON es válido.
     */
    bool verifyConfig();

    // === Utilidad ===

    /**
     * @brief Obtiene la ruta del archivo de configuración.
     */
    std::string configPath() const { return m_config_file; }

    /**
     * @brief Obtiene la ruta del archivo de log.
     */
    std::string logPath() const { return m_log_file; }

private:
    std::string m_config_file;   ///< Ruta al archivo JSON de configuración
    std::string m_log_file;      ///< Ruta al archivo de log
    std::mutex m_log_mutex;      ///< Mutex para acceso seguro al log
    std::ofstream m_log_stream;  ///< Stream del archivo de log
    bool m_log_open;             ///< true si el archivo de log está abierto

    /**
     * @brief Abre el archivo de log (modo append).
     */
    void openLog();

    /**
     * @brief Cierra el archivo de log.
     */
    void closeLog();

    /**
     * @brief Convierte LogLevel a string.
     */
    static std::string_view levelToString(LogLevel level);

    /**
     * @brief Obtiene timestamp actual como string.
     */
    static std::string currentTimestamp();
};

} // namespace services

#endif // SERVICES_FILESYSTEM_HPP
