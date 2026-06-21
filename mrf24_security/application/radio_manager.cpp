/**
 * @file    application/radio_manager.cpp
 * @brief   Implementación del orquestador del sistema
 *          Incluye protocolo seguro con roles, hash y enrutamiento
 *
 * @author  MRF24J40 Team
 * @date    2026
 */

#include <application/radio_manager.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdio>
#include <chrono>
#include <cstring>
#include <algorithm>

namespace application {

// ============================================================================
// Constructor / Destructor
// ============================================================================

RadioManager_t::RadioManager_t()
    : m_initialized(false)
    , m_oled_ok(false)
    , m_tft_ok(false)
    , m_role(NodeRole::EndDevice)
    , m_message_ready(false)
    , m_packet_count(0)
{
    std::memset(&m_validation_stats, 0, sizeof(m_validation_stats));
}

RadioManager_t::~RadioManager_t() {
    log("System shutdown", services::LogLevel::Info);
    if (m_oled_ok && m_oled) {
        m_oled->clear();
        m_oled->drawString(0, 20, "Terminado", 1, true);
        m_oled->update();
    }
}

// ============================================================================
// Inicialización
// ============================================================================

bool RadioManager_t::init() {
    log("Initializing system...");

    // 1. Cargar configuración
    m_fs = std::make_unique<services::FileSystem_t>("config.json");
    m_config = m_fs->loadConfig();

    // 2. Inicializar rol y tabla de rutas
    m_role = m_config.role;
    m_routing_table = m_config.routing_table;

    log(std::string("Role: ") + std::string(services::FileSystem_t::roleToString(m_role)));
    log("Routes: " + std::to_string(m_routing_table.size()));

    return initFromConfig(m_config);
}

bool RadioManager_t::initFromConfig(const services::SystemConfig& config) {
    m_config = config;

    // 2. Inicializar crypto
    try {
        m_crypto = std::make_unique<services::Crypto_t>(m_config.passphrase);
        log("Crypto initialized");
    } catch (const std::exception& e) {
        log(std::string("Crypto init failed: ") + e.what(), services::LogLevel::Error);
    }

    // 3. Inicializar displays
    if (m_config.use_oled) {
        m_oled = std::make_unique<drivers::Ssd1306_t>();
        m_oled_ok = m_oled->init();
        if (m_oled_ok) {
            m_oled->showInitScreen();
            services::Timer_t::delayMs(500);
            m_oled->clear();
            m_oled->drawString(0, 0, "MRF24J40 v2.0", 1, true);
            m_oled->update();
            log("OLED initialized");
        } else {
            log("OLED not detected", services::LogLevel::Warning);
        }
    }

    if (m_config.use_tft) {
        m_tft = std::make_unique<drivers::St7789_t>();
        m_tft_ok = m_tft->init();
        if (m_tft_ok) {
            m_tft->fillScreen(drivers::ColorRgb565::Blue());
            log("TFT initialized");
        } else {
            log("TFT not detected", services::LogLevel::Warning);
        }
    }

    // 4. Inicializar QR
    m_qr = std::make_unique<drivers::Qr_t>();

    // 5. Inicializar radio
    setupRadio();

    // 6. Callbacks
    m_radio->onReceive = [this](const uint8_t* data, uint8_t len) {
        // El protocolo seguro se maneja en process()
        (void)data;
        (void)len;
    };

    m_radio->onTransmit = [this](bool success, uint8_t retries) {
        if (success) {
            log("TX OK");
        } else {
            log("TX failed", services::LogLevel::Warning);
        }
    };

    m_initialized = true;
    log("System initialized successfully");
    return true;
}

void RadioManager_t::setupRadio() {
    m_radio = std::make_unique<drivers::Mrf24j40_t>();

    if (!m_radio->init(m_config.channel)) {
        log("Radio init failed!", services::LogLevel::Error);
        return;
    }

    m_radio->setPan(m_config.pan_id);
    m_radio->setShortAddress(
        (m_config.mac_address[6] << 8) | m_config.mac_address[7]);
    m_radio->setMacAddress(m_config.mac_address);
    m_radio->setMode(static_cast<drivers::NodeMode>(m_config.node_mode));
    m_radio->selfTest();

    log("Radio initialized: PAN=0x" +
        services::Crypto_t::toHex({static_cast<uint8_t>(m_config.pan_id >> 8),
                                    static_cast<uint8_t>(m_config.pan_id & 0xFF)}));
}

// ============================================================================
// Configuración
// ============================================================================

services::SystemConfig RadioManager_t::loadConfig() {
    if (m_fs) {
        m_config = m_fs->loadConfig();
        m_role = m_config.role;
        m_routing_table = m_config.routing_table;
    }
    return m_config;
}

bool RadioManager_t::saveConfig() {
    // Sincronizar rol y tabla antes de guardar
    m_config.role = m_role;
    m_config.routing_table = m_routing_table;
    return m_fs && m_fs->saveConfig(m_config);
}

void RadioManager_t::setConfig(const services::SystemConfig& config) {
    m_config = config;
    m_role = config.role;
    m_routing_table = config.routing_table;
    if (m_crypto) {
        m_crypto->setPassphrase(m_config.passphrase);
    }
}

// ============================================================================
// Radio
// ============================================================================

void RadioManager_t::setNodeMode(drivers::NodeMode mode) {
    if (m_radio) {
        m_radio->setMode(mode);
        m_config.node_mode = static_cast<uint8_t>(mode);
    }
}

drivers::NodeMode RadioManager_t::nodeMode() const {
    return m_radio ? m_radio->mode() : drivers::NodeMode::Node;
}

drivers::Mrf24Stats RadioManager_t::radioStats() const {
    drivers::Mrf24Stats stats;
    if (m_radio) m_radio->getStats(stats);
    return stats;
}

// ============================================================================
// Roles
// ============================================================================

void RadioManager_t::setRole(NodeRole role) {
    m_role = role;
    m_config.role = role;
    log(std::string("Role changed to: ") +
        std::string(services::FileSystem_t::roleToString(role)));
}

bool RadioManager_t::canForward() const {
    return m_role == NodeRole::Router ||
           m_role == NodeRole::Coordinator ||
           m_role == NodeRole::Mesh;
}

// ============================================================================
// Routing
// ============================================================================

void RadioManager_t::addRoute(uint64_t dest, uint64_t nextHop) {
    // Verificar si ya existe la ruta
    for (auto& [d, n] : m_routing_table) {
        if (d == dest) {
            n = nextHop;  // Actualizar existente
            log("Route updated: " + macToHex(dest) +
                " → " + macToHex(nextHop));
            return;
        }
    }
    m_routing_table.emplace_back(dest, nextHop);
    log("Route added: " + macToHex(dest) +
        " → " + macToHex(nextHop));
}

void RadioManager_t::removeRoute(uint64_t dest) {
    auto it = std::remove_if(m_routing_table.begin(), m_routing_table.end(),
        [dest](const auto& pair) { return pair.first == dest; });
    if (it != m_routing_table.end()) {
        m_routing_table.erase(it, m_routing_table.end());
        log("Route removed: " + macToHex(dest));
    }
}

bool RadioManager_t::findRoute(uint64_t dest, uint64_t& nextHop) const {
    for (const auto& [d, n] : m_routing_table) {
        if (d == dest) {
            nextHop = n;
            return true;
        }
    }
    return false;
}

// ============================================================================
// Protocolo seguro — Construcción de trama
// ============================================================================

std::vector<uint8_t> RadioManager_t::buildSecureMessage(
    const std::array<uint8_t, 8>& dest_mac,
    const std::vector<uint8_t>& iv,
    const std::vector<uint8_t>& ciphertext,
    uint8_t ttl)
{
    // Formato: [dest_mac(8)] [src_mac(8)] [ttl(1)] [size(2)] [iv(16)] [ciphertext(N)] [hash(32)]
    size_t total = FRAME_DEST_MAC_LEN + FRAME_SRC_MAC_LEN +
                   FRAME_TTL_LEN + FRAME_SIZE_LEN +
                   iv.size() + ciphertext.size() + FRAME_HASH_LEN;

    std::vector<uint8_t> frame(total);
    size_t pos = 0;

    // 1. Dest MAC
    std::memcpy(frame.data() + pos, dest_mac.data(), FRAME_DEST_MAC_LEN);
    pos += FRAME_DEST_MAC_LEN;

    // 2. Source MAC (nuestra dirección local)
    std::memcpy(frame.data() + pos, m_config.mac_address.data(), FRAME_SRC_MAC_LEN);
    pos += FRAME_SRC_MAC_LEN;

    // 3. TTL
    frame[pos++] = ttl;

    // 4. Size (big-endian)
    uint16_t ct_size = static_cast<uint16_t>(ciphertext.size());
    frame[pos++] = (ct_size >> 8) & 0xFF;
    frame[pos++] = ct_size & 0xFF;

    // 5. IV
    std::memcpy(frame.data() + pos, iv.data(), iv.size());
    pos += iv.size();

    // 6. Ciphertext
    std::memcpy(frame.data() + pos, ciphertext.data(), ciphertext.size());
    pos += ciphertext.size();

    // 7. Hash SHA-256 sobre: [dest_mac] + [size] + [iv] + [ciphertext]
    //    Construir buffer para hash
    std::vector<uint8_t> hash_input;
    hash_input.reserve(FRAME_DEST_MAC_LEN + FRAME_SIZE_LEN + iv.size() + ciphertext.size());
    hash_input.insert(hash_input.end(), dest_mac.begin(), dest_mac.end());
    hash_input.push_back((ct_size >> 8) & 0xFF);
    hash_input.push_back(ct_size & 0xFF);
    hash_input.insert(hash_input.end(), iv.begin(), iv.end());
    hash_input.insert(hash_input.end(), ciphertext.begin(), ciphertext.end());

    auto hash = m_crypto->sha256(hash_input);
    std::memcpy(frame.data() + pos, hash.data(), FRAME_HASH_LEN);
    pos += FRAME_HASH_LEN;

    log("Secure message built: " + std::to_string(frame.size()) + " bytes, TTL=" +
        std::to_string(ttl) + ", hash=SHA-256");

    return frame;
}

// ============================================================================
// Protocolo seguro — Validación de mensaje
// ============================================================================

ValidationResult RadioManager_t::validateMessage(const uint8_t* rawMessage, uint8_t len) {
    ValidationResult result;

    if (!rawMessage || len < FRAME_OVERHEAD) {
        result.error_msg = "Message too short: " + std::to_string(len) + " bytes (min " +
                           std::to_string(FRAME_OVERHEAD) + ")";
        m_validation_stats.messages_rejected++;
        return result;
    }

    size_t pos = 0;

    // 1. Extraer dest_mac
    uint64_t dest_mac = 0;
    for (int i = 0; i < FRAME_DEST_MAC_LEN; i++) {
        dest_mac = (dest_mac << 8) | rawMessage[pos++];
    }
    result.dest_mac = dest_mac;

    // 2. Extraer src_mac
    uint64_t src_mac = 0;
    for (int i = 0; i < FRAME_SRC_MAC_LEN; i++) {
        src_mac = (src_mac << 8) | rawMessage[pos++];
    }
    result.src_mac = src_mac;

    // 3. Extraer TTL
    result.ttl = rawMessage[pos++];
    if (result.ttl == 0) {
        result.error_msg = "TTL expired (0)";
        m_validation_stats.ttl_expired++;
        return result;
    }

    // 4. Extraer size (big-endian)
    result.payload_size = (static_cast<uint16_t>(rawMessage[pos]) << 8) | rawMessage[pos + 1];
    pos += FRAME_SIZE_LEN;

    // 5. Extraer IV
    if (pos + FRAME_IV_LEN > len) {
        result.error_msg = "Message too short for IV";
        m_validation_stats.messages_rejected++;
        return result;
    }
    result.iv.assign(rawMessage + pos, rawMessage + pos + FRAME_IV_LEN);
    pos += FRAME_IV_LEN;

    // 6. Extraer ciphertext
    if (pos + result.payload_size > len) {
        result.error_msg = "Message too short for ciphertext";
        m_validation_stats.messages_rejected++;
        return result;
    }
    result.ciphertext.assign(rawMessage + pos, rawMessage + pos + result.payload_size);
    pos += result.payload_size;

    // 7. Extraer hash recibido (SHA-256, 32 bytes)
    if (pos + FRAME_HASH_LEN > len) {
        result.error_msg = "Message too short for hash";
        m_validation_stats.messages_rejected++;
        return result;
    }
    std::array<uint8_t, FRAME_HASH_LEN> received_hash;
    std::memcpy(received_hash.data(), rawMessage + pos, FRAME_HASH_LEN);

    // 8. Verificar hash
    // Hash = SHA-256([dest_mac] + [size] + [iv] + [ciphertext])
    std::vector<uint8_t> hash_input;
    hash_input.reserve(FRAME_DEST_MAC_LEN + FRAME_SIZE_LEN +
                       result.iv.size() + result.ciphertext.size());

    // dest_mac como bytes
    auto dest_mac_arr = uint64ToMac(dest_mac);
    hash_input.insert(hash_input.end(), dest_mac_arr.begin(), dest_mac_arr.end());
    hash_input.push_back((result.payload_size >> 8) & 0xFF);
    hash_input.push_back(result.payload_size & 0xFF);
    hash_input.insert(hash_input.end(), result.iv.begin(), result.iv.end());
    hash_input.insert(hash_input.end(), result.ciphertext.begin(), result.ciphertext.end());

    auto computed_hash = m_crypto->sha256(hash_input);
    if (computed_hash != received_hash) {
        result.error_msg = "Hash mismatch: message integrity check failed";
        m_validation_stats.hash_errors++;
        m_validation_stats.messages_rejected++;
        return result;
    }

    // 9. Verificar destino
    uint64_t local_mac = macToUint64(m_config.mac_address);
    if (dest_mac == BROADCAST_ADDR || dest_mac == local_mac) {
        result.for_us = true;
    } else if (canForward()) {
        result.should_forward = true;
    }

    result.valid = true;
    m_validation_stats.messages_validated++;
    log("Message validated: src=0x" + macToHex(src_mac) +
        " TTL=" + std::to_string(result.ttl) +
        " for_us=" + (result.for_us ? "Y" : "N") +
        " fwd=" + (result.should_forward ? "Y" : "N"));

    return result;
}

// ============================================================================
// Protocolo seguro — Reenvío
// ============================================================================

bool RadioManager_t::forwardMessage(const std::vector<uint8_t>& msg, uint64_t nextHop) {
    if (!canForward()) {
        log("Cannot forward: role does not allow forwarding", services::LogLevel::Warning);
        return false;
    }

    if (msg.size() < FRAME_DEST_MAC_LEN + FRAME_SRC_MAC_LEN + FRAME_TTL_LEN + 1) {
        log("Cannot forward: message too short", services::LogLevel::Warning);
        return false;
    }

    // Extraer TTL actual
    uint8_t ttl = msg[FRAME_DEST_MAC_LEN + FRAME_SRC_MAC_LEN];
    if (ttl == 0) {
        log("Cannot forward: TTL=0", services::LogLevel::Warning);
        m_validation_stats.ttl_expired++;
        return false;
    }

    // Decrementar TTL
    std::vector<uint8_t> fwd_msg = msg;
    fwd_msg[FRAME_DEST_MAC_LEN + FRAME_SRC_MAC_LEN] = ttl - 1;

    log("Forwarding message: TTL " + std::to_string(ttl) + " → " +
        std::to_string(ttl - 1) + ", nextHop=0x" +
        macToHex(nextHop));

    // Enviar al siguiente salto
    auto next_mac_arr = uint64ToMac(nextHop);
    bool ok = m_radio->send(next_mac_arr, fwd_msg.data(),
                            static_cast<uint8_t>(fwd_msg.size()));

    if (ok) {
        m_validation_stats.messages_forwarded++;
    }
    return ok;
}

// ============================================================================
// Utilidades de conversión MAC
// ============================================================================

uint64_t RadioManager_t::macToUint64(const std::array<uint8_t, 8>& mac) {
    uint64_t val = 0;
    for (int i = 0; i < 8; i++) {
        val = (val << 8) | mac[i];
    }
    return val;
}

std::array<uint8_t, 8> RadioManager_t::uint64ToMac(uint64_t addr) {
    std::array<uint8_t, 8> mac{};
    for (int i = 7; i >= 0; i--) {
        mac[i] = static_cast<uint8_t>(addr & 0xFF);
        addr >>= 8;
    }
    return mac;
}

std::string RadioManager_t::macToHex(uint64_t addr) {
    auto arr = uint64ToMac(addr);
    return services::Crypto_t::toHex({arr.begin(), arr.end()});
}

uint64_t RadioManager_t::localMac64() const {
    return macToUint64(m_config.mac_address);
}

// ============================================================================
// Mensajes (TX) — Con protocolo seguro
// ============================================================================

bool RadioManager_t::sendMessage(const std::array<uint8_t, 8>& dest_mac,
                                  std::string_view message) {
    if (!m_initialized || !m_radio || !m_crypto) return false;

    std::vector<uint8_t> plaintext(message.begin(), message.end());

    // 1. Cifrar si está habilitado
    std::vector<uint8_t> tx_data;
    std::vector<uint8_t> iv;

    if (m_config.enable_encryption) {
        auto result = m_crypto->encrypt(plaintext);
        if (!result.success) {
            log("Encryption failed: " + result.error_msg, services::LogLevel::Error);
            return false;
        }
        // Extraer IV (primeros 16 bytes) y ciphertext (resto)
        iv.assign(result.data.begin(), result.data.begin() + FRAME_IV_LEN);
        tx_data.assign(result.data.begin() + FRAME_IV_LEN, result.data.end());
    } else {
        // Sin cifrado: IV vacío
        iv.assign(FRAME_IV_LEN, 0);
        tx_data = plaintext;
    }

    // 2. Construir trama segura con hash
    auto secure_frame = buildSecureMessage(dest_mac, iv, tx_data, DEFAULT_TTL);

    // 3. Enviar
    bool ok = m_radio->send(dest_mac, secure_frame.data(),
                            static_cast<uint8_t>(secure_frame.size()));
    if (ok) {
        log("Secure message sent (" + std::to_string(secure_frame.size()) +
            " bytes, payload=" + std::to_string(plaintext.size()) + " chars)");
    }
    return ok;
}

bool RadioManager_t::sendData(const std::array<uint8_t, 8>& dest_mac,
                               const uint8_t* data, uint8_t len) {
    if (!m_initialized || !m_radio) return false;

    // Enviar datos raw sin protocolo seguro (para debug/compatibilidad)
    return m_radio->send(dest_mac, data, len);
}

bool RadioManager_t::sendQrMessage(const std::array<uint8_t, 8>& dest_mac,
                                    std::string_view message) {
    if (!m_qr) return false;

    // Mostrar QR en consola
    m_qr->generateAndPrint(message);

    // Guardar QR como PNG
    std::string filename = "qr_message_";
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    filename += std::to_string(ms) + ".png";
    m_qr->generateAndSave(message, filename);

    log("QR saved: " + filename);

    // Enviar el texto como mensaje
    return sendMessage(dest_mac, message);
}

// ============================================================================
// Recepción — Con validación y enrutamiento
// ============================================================================

void RadioManager_t::process() {
    if (!m_initialized || !m_radio) return;

    m_radio->poll();

    if (m_radio->hasPacket()) {
        uint8_t buf[drivers::MAX_PAYLOAD_LEN];
        uint8_t len = m_radio->getData(buf);
        m_packet_count++;

        log("Packet received #" + std::to_string(m_packet_count) +
            " (" + std::to_string(len) + " bytes)");

        // 1. Validar mensaje (hash, TTL, destino)
        auto validation = validateMessage(buf, len);

        if (!validation.valid) {
            log("Validation failed: " + validation.error_msg,
                services::LogLevel::Warning);
            return;
        }

        // 2. Si debe reenviarse
        if (validation.should_forward) {
            log("Message needs forwarding", services::LogLevel::Info);

            // Buscar ruta
            uint64_t nextHop = 0;
            if (findRoute(validation.dest_mac, nextHop)) {
                std::vector<uint8_t> msg(buf, buf + len);
                forwardMessage(msg, nextHop);
            } else if (m_role == NodeRole::Coordinator) {
                // Coordinator: flood a todos los routers conocidos
                log("No route found, flooding to known routers",
                    services::LogLevel::Warning);
                for (const auto& [dest, next] : m_routing_table) {
                    std::vector<uint8_t> msg(buf, buf + len);
                    forwardMessage(msg, next);
                }
            } else {
                log("No route found for destination",
                     services::LogLevel::Warning);
                m_validation_stats.routing_not_found++;
            }
            return;
        }

        // 3. Si es para nosotros, descifrar
        if (validation.for_us) {
            // Construir ciphertext_with_iv como espera decrypt()
            std::vector<uint8_t> ciphertext_with_iv;
            ciphertext_with_iv.reserve(validation.iv.size() + validation.ciphertext.size());
            ciphertext_with_iv.insert(ciphertext_with_iv.end(),
                                      validation.iv.begin(), validation.iv.end());
            ciphertext_with_iv.insert(ciphertext_with_iv.end(),
                                      validation.ciphertext.begin(),
                                      validation.ciphertext.end());

            if (m_config.enable_encryption && m_crypto) {
                auto result = m_crypto->decrypt(ciphertext_with_iv);
                if (result.success) {
                    m_last_message.assign(result.data.begin(), result.data.end());
                    m_message_ready = true;

                    // Callback
                    std::string sender_str = "0x" + services::Crypto_t::toHex(
                        uint64ToMac(validation.src_mac));

                    if (m_msg_callback) {
                        m_msg_callback(sender_str, m_last_message,
                                       m_radio->lastLqi(), m_radio->lastRssi());
                    }

                    // Mostrar en OLED
                    if (m_oled_ok && m_oled) {
                        m_oled->clear();
                        m_oled->drawString(0, 0,
                            "MSG #" + std::to_string(m_packet_count), 1, true);
                        if (m_last_message.length() > 20) {
                            m_oled->drawString(0, 16,
                                m_last_message.substr(0, 20), 1, true);
                            m_oled->drawString(0, 32,
                                m_last_message.substr(20, 20), 1, true);
                        } else {
                            m_oled->drawString(0, 16, m_last_message, 1, true);
                        }
                        m_oled->drawString(0, 48,
                            "LQI:" + std::to_string(m_radio->lastLqi()) +
                            " RSSI:" + std::to_string(m_radio->lastRssi()) + "dBm",
                            1, true);
                        m_oled->update();
                    }

                    log("Decrypted: " + m_last_message +
                        " (from 0x" +macToHex(validation.src_mac) + ")");
                } else {
                    log("Decryption failed: " + result.error_msg,
                        services::LogLevel::Error);
                }
            } else {
                // Sin cifrado
                m_last_message.assign(validation.ciphertext.begin(),
                                      validation.ciphertext.end());
                m_message_ready = true;
            }
        }

        // 4. Log a archivo
        if (m_fs) {
            std::string hex;
            char tmp[4];
            for (uint8_t i = 0; i < len; i++) {
                snprintf(tmp, sizeof(tmp), "%02X", buf[i]);
                hex += tmp;
            }
            m_fs->logPacket("RX", len, m_radio->lastLqi(),
                            m_radio->lastRssi(), hex);
        }
    }
}

bool RadioManager_t::hasMessage() const {
    return m_message_ready;
}

bool RadioManager_t::getMessage(std::string& message) {
    if (!m_message_ready) return false;
    message = m_last_message;
    m_message_ready = false;
    return true;
}

// ============================================================================
// Displays
// ============================================================================

void RadioManager_t::updateOled() {
    if (!m_oled_ok || !m_oled) return;

    m_oled->clear();
    m_oled->drawString(0, 0, "MRF24J40 v2.0", 1, true);

    auto stats = radioStats();
    m_oled->drawString(0, 16,
        "TX:" + std::to_string(stats.packets_sent), 1, true);
    m_oled->drawString(0, 32,
        "RX:" + std::to_string(stats.packets_received), 1, true);
    m_oled->drawString(0, 48,
        "ROLE:" + std::string(services::FileSystem_t::roleToString(m_role)),
        1, true);

    m_oled->update();
}

void RadioManager_t::showQrOnOled(std::string_view text) {
    if (!m_qr || !m_oled_ok || !m_oled) return;

    auto matrix = m_qr->generateMatrix(text);
    if (matrix.first.empty()) return;

    m_oled->clear();
    m_qr->renderToOled(*m_oled, matrix.first.data(), matrix.second,
                        (128 - matrix.second) / 2, 0, 1);
    m_oled->update();
}

void RadioManager_t::showQrOnTft(std::string_view text) {
    if (!m_qr || !m_tft_ok || !m_tft) return;

    auto matrix = m_qr->generateMatrix(text);
    if (matrix.first.empty()) return;

    m_tft->fillScreen(drivers::ColorRgb565::White());
    m_qr->renderToTft(*m_tft, matrix.first.data(), matrix.second,
                      (240 - matrix.second * 2) / 2,
                      (240 - matrix.second * 2) / 2,
                      2, drivers::ColorRgb565::Black(),
                      drivers::ColorRgb565::White());
}

// ============================================================================
// QR
// ============================================================================

bool RadioManager_t::saveQrPng(std::string_view text,
                                std::string_view filename) {
    if (!m_qr) return false;
    return m_qr->generateAndSave(text, filename);
}

void RadioManager_t::printQr(std::string_view text) {
    if (m_qr) m_qr->generateAndPrint(text);
}

// ============================================================================
// Logging
// ============================================================================

void RadioManager_t::showLog(size_t lines) {
    if (!m_fs) return;
    auto entries = m_fs->getRecentLog(lines);
    for (const auto& entry : entries) {
        std::cout << entry << "\n";
    }
}

void RadioManager_t::clearLog() {
    if (m_fs) m_fs->clearLog();
}

void RadioManager_t::log(std::string_view msg, services::LogLevel level) {
    if (m_fs) {
        m_fs->log(msg, level);
    }
}

} // namespace application
