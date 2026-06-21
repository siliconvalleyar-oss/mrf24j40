/**
 * @file    services/filesystem.cpp
 * @brief   Implementación del servicio de configuración y logging
 *
 * @author  MRF24J40 Team
 * @date    2026
 */

#include <services/filesystem.hpp>

// Intentar incluir nlohmann/json (disponible vía pkg-config)
#ifdef ENABLE_JSON
#include <nlohmann/json.hpp>
using json = nlohmann::json;
#endif

#include <fstream>
#include <sstream>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include <cstdio>

namespace services {

// ============================================================================
// Constructor / Destructor
// ============================================================================

FileSystem_t::FileSystem_t(std::string_view config_file)
    : m_config_file(config_file)
    , m_log_open(false)
{
    // Derivar nombre del archivo de log desde config_file
    auto pos = m_config_file.rfind('.');
    if (pos != std::string::npos) {
        m_log_file = m_config_file.substr(0, pos) + ".log";
    } else {
        m_log_file = "mrf24j40.log";
    }

    openLog();
}

FileSystem_t::~FileSystem_t() {
    closeLog();
}

// ============================================================================
// Configuración
// ============================================================================

SystemConfig FileSystem_t::loadConfig() {
    SystemConfig cfg;

    std::ifstream file(m_config_file);
    if (!file.is_open()) {
        // Archivo no existe, devolver valores por defecto
        log("Config file not found, using defaults", LogLevel::Warning);
        return cfg;
    }

#ifdef ENABLE_JSON
    try {
        json j;
        file >> j;

        if (j.find("pan_id") != j.end()) cfg.pan_id = j["pan_id"].get<uint16_t>();
        if (j.find("channel") != j.end()) cfg.channel = j["channel"].get<uint8_t>();
        if (j.find("node_mode") != j.end()) cfg.node_mode = j["node_mode"].get<uint8_t>();
        if (j.find("passphrase") != j.end()) cfg.passphrase = j["passphrase"].get<std::string>();
        if (j.find("use_oled") != j.end()) cfg.use_oled = j["use_oled"].get<bool>();
        if (j.find("use_tft") != j.end()) cfg.use_tft = j["use_tft"].get<bool>();
        if (j.find("enable_encryption") != j.end()) cfg.enable_encryption = j["enable_encryption"].get<bool>();
        if (j.find("enable_hash") != j.end()) cfg.enable_hash = j["enable_hash"].get<bool>();
        if (j.find("enable_whitelist") != j.end()) cfg.enable_whitelist = j["enable_whitelist"].get<bool>();
        if (j.find("log_file") != j.end()) cfg.log_file = j["log_file"].get<std::string>();

        // Role
        if (j.find("role") != j.end()) {
            auto role_str = j["role"].get<std::string>();
            cfg.role = roleFromString(role_str);
        }

        // Routing table
        if (j.find("routing_table") != j.end() && j["routing_table"].is_array()) {
            cfg.routing_table.clear();
            for (const auto& entry : j["routing_table"]) {
                uint64_t dest = 0, next = 0;
                if (entry.find("dest") != entry.end()) dest = std::stoull(entry["dest"].get<std::string>(), nullptr, 16);
                if (entry.find("next") != entry.end()) next = std::stoull(entry["next"].get<std::string>(), nullptr, 16);
                cfg.routing_table.emplace_back(dest, next);
            }
        }

        // Allowed sources (whitelist)
        if (j.find("allowed_sources") != j.end() && j["allowed_sources"].is_array()) {
            cfg.allowed_sources.clear();
            for (const auto& entry : j["allowed_sources"]) {
                cfg.allowed_sources.push_back(
                    std::stoull(entry.get<std::string>(), nullptr, 16));
            }
        }

        // MAC address (array de 8 bytes)
        if (j.find("mac_address") != j.end() && j["mac_address"].is_array()) {
            auto mac = j["mac_address"];
            for (size_t i = 0; i < std::min(mac.size(), size_t{8}); i++) {
                cfg.mac_address[i] = mac[i].get<uint8_t>();
            }
        }

        log("Config loaded successfully", LogLevel::Info);
    } catch (const std::exception& e) {
        log(std::string("Error parsing config: ") + e.what(), LogLevel::Error);
    }
#else
    // Sin JSON: cargar valores por defecto
    log("JSON support not enabled, using defaults", LogLevel::Warning);
    file.close();
#endif

    return cfg;
}

bool FileSystem_t::saveConfig(const SystemConfig& config) {
#ifdef ENABLE_JSON
    try {
        json j;
        j["pan_id"] = config.pan_id;
        j["channel"] = config.channel;
        j["node_mode"] = config.node_mode;
        j["passphrase"] = config.passphrase;
        j["use_oled"] = config.use_oled;
        j["use_tft"] = config.use_tft;
        j["enable_encryption"] = config.enable_encryption;
        j["enable_hash"] = config.enable_hash;
        j["enable_whitelist"] = config.enable_whitelist;
        j["log_file"] = config.log_file;

        // Role
        j["role"] = std::string(roleToString(config.role));

        // Allowed sources (whitelist)
        for (const auto& mac : config.allowed_sources) {
            char buf_mac[20];
            snprintf(buf_mac, sizeof(buf_mac), "%016llX", (unsigned long long)mac);
            j["allowed_sources"].push_back(std::string(buf_mac));
        }

        // Routing table
        for (const auto& [dest, next] : config.routing_table) {
            nlohmann::json entry;
            char buf_dest[20], buf_next[20];
            snprintf(buf_dest, sizeof(buf_dest), "%016llX", (unsigned long long)dest);
            snprintf(buf_next, sizeof(buf_next), "%016llX", (unsigned long long)next);
            entry["dest"] = std::string(buf_dest);
            entry["next"] = std::string(buf_next);
            j["routing_table"].push_back(std::move(entry));
        }

        // MAC address como array
        for (auto b : config.mac_address) {
            j["mac_address"].push_back(b);
        }

        std::ofstream file(m_config_file);
        if (!file.is_open()) {
            log("Cannot write config file", LogLevel::Error);
            return false;
        }
        file << j.dump(2);
        log("Config saved successfully", LogLevel::Info);
        return true;
    } catch (const std::exception& e) {
        log(std::string("Error saving config: ") + e.what(), LogLevel::Error);
        return false;
    }
#else
    log("JSON support not enabled, cannot save", LogLevel::Warning);
    return false;
#endif
}

std::string FileSystem_t::configToString(const SystemConfig& config) {
#ifdef ENABLE_JSON
    json j;
    j["pan_id"] = config.pan_id;
    j["channel"] = config.channel;
    j["node_mode"] = config.node_mode;
    j["passphrase"] = "********";  // Ocultar passphrase
    j["use_oled"] = config.use_oled;
    j["use_tft"] = config.use_tft;
    j["enable_encryption"] = config.enable_encryption;
    j["enable_hash"] = config.enable_hash;
    for (auto b : config.mac_address) {
        j["mac_address"].push_back(b);
    }
    return j.dump(2);
#else
    return "{\"error\": \"JSON support not enabled\"}";
#endif
}

SystemConfig FileSystem_t::configFromString(std::string_view json_str) {
    SystemConfig cfg;
#ifdef ENABLE_JSON
    try {
        json j = json::parse(json_str);
        if (j.find("pan_id") != j.end()) cfg.pan_id = j["pan_id"].get<uint16_t>();
        if (j.find("channel") != j.end()) cfg.channel = j["channel"].get<uint8_t>();
        if (j.find("node_mode") != j.end()) cfg.node_mode = j["node_mode"].get<uint8_t>();
        if (j.find("passphrase") != j.end() && j["passphrase"] != "********")
            cfg.passphrase = j["passphrase"].get<std::string>();
        if (j.find("use_oled") != j.end()) cfg.use_oled = j["use_oled"].get<bool>();
        if (j.find("use_tft") != j.end()) cfg.use_tft = j["use_tft"].get<bool>();
        if (j.find("enable_encryption") != j.end()) cfg.enable_encryption = j["enable_encryption"].get<bool>();
        if (j.find("enable_hash") != j.end()) cfg.enable_hash = j["enable_hash"].get<bool>();
        if (j.find("mac_address") != j.end() && j["mac_address"].is_array()) {
            auto mac = j["mac_address"];
            for (size_t i = 0; i < std::min(mac.size(), size_t{8}); i++) {
                cfg.mac_address[i] = mac[i].get<uint8_t>();
            }
        }
    } catch (...) {}
#endif
    return cfg;
}

bool FileSystem_t::verifyConfig() {
    std::ifstream file(m_config_file);
    if (!file.is_open()) return false;

#ifdef ENABLE_JSON
    try {
        json j;
        file >> j;
        return true;
    } catch (...) {
        return false;
    }
#else
    return false;
#endif
}

// ============================================================================
// Logging
// ============================================================================

void FileSystem_t::openLog() {
    m_log_stream.open(m_log_file, std::ios::app);
    if (m_log_stream.is_open()) {
        m_log_open = true;
        log("=== Log started ===", LogLevel::Info);
    } else {
        m_log_open = false;
    }
}

void FileSystem_t::closeLog() {
    if (m_log_open) {
        log("=== Log ended ===", LogLevel::Info);
        m_log_stream.close();
        m_log_open = false;
    }
}

void FileSystem_t::log(std::string_view message, LogLevel level) {
    if (!m_log_open) return;

    std::lock_guard<std::mutex> lock(m_log_mutex);
    m_log_stream << currentTimestamp()
                 << " [" << levelToString(level) << "] "
                 << message << std::endl;
}

void FileSystem_t::logPacket(std::string_view direction, uint8_t len,
                              uint8_t lqi, int8_t rssi,
                              std::string_view hex_data) {
    if (!m_log_open) return;

    std::lock_guard<std::mutex> lock(m_log_mutex);
    m_log_stream << currentTimestamp()
                 << " [" << direction << "]"
                 << " len=" << static_cast<int>(len)
                 << " lqi=" << static_cast<int>(lqi)
                 << " rssi=" << static_cast<int>(rssi)
                 << " data=" << hex_data
                 << std::endl;
}

std::vector<std::string> FileSystem_t::getRecentLog(size_t lines) {
    std::vector<std::string> result;
    std::ifstream file(m_log_file);
    if (!file.is_open()) return result;

    std::string line;
    while (std::getline(file, line)) {
        result.push_back(line);
        if (result.size() > lines) {
            result.erase(result.begin());
        }
    }
    return result;
}

bool FileSystem_t::clearLog() {
    closeLog();
    m_log_stream.open(m_log_file, std::ios::trunc);
    m_log_open = m_log_stream.is_open();
    if (m_log_open) {
        log("Log cleared", LogLevel::Info);
    }
    return m_log_open;
}

// ============================================================================
// Utilidad
// ============================================================================

std::string_view FileSystem_t::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Error:   return "ERROR";
        default:                return "?";
    }
}

std::string FileSystem_t::currentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::tm* tm = std::localtime(&time_t);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
    return std::string(buf) + "." + std::to_string(ms.count());
}

// ============================================================================
// Roles & Routing
// ============================================================================

NodeRole FileSystem_t::loadRole() {
    auto cfg = loadConfig();
    return cfg.role;
}

bool FileSystem_t::saveRole(NodeRole role) {
    auto cfg = loadConfig();
    cfg.role = role;
    return saveConfig(cfg);
}

std::vector<std::pair<uint64_t, uint64_t>> FileSystem_t::loadRoutingTable() {
    auto cfg = loadConfig();
    return cfg.routing_table;
}

bool FileSystem_t::saveRoutingTable(const std::vector<std::pair<uint64_t, uint64_t>>& table) {
    auto cfg = loadConfig();
    cfg.routing_table = table;
    return saveConfig(cfg);
}

std::string_view FileSystem_t::roleToString(NodeRole role) {
    switch (role) {
        case NodeRole::EndDevice:   return "end_device";
        case NodeRole::Router:      return "router";
        case NodeRole::Coordinator: return "coordinator";
        case NodeRole::Mesh:        return "mesh";
        default:                    return "end_device";
    }
}

NodeRole FileSystem_t::roleFromString(std::string_view str) {
    if (str == "router")      return NodeRole::Router;
    if (str == "coordinator") return NodeRole::Coordinator;
    if (str == "mesh")        return NodeRole::Mesh;
    return NodeRole::EndDevice;
}

uint8_t FileSystem_t::roleToByte(NodeRole role) {
    return static_cast<uint8_t>(role);
}

NodeRole FileSystem_t::roleFromByte(uint8_t byte) {
    if (byte <= 3) return static_cast<NodeRole>(byte);
    return NodeRole::EndDevice;
}

} // namespace services
