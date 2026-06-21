/**
 * @file    application/radio_manager.cpp
 * @brief   Implementación del orquestador del sistema
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

namespace application {

// ============================================================================
// Constructor / Destructor
// ============================================================================

RadioManager_t::RadioManager_t()
    : m_initialized(false)
    , m_oled_ok(false)
    , m_tft_ok(false)
    , m_message_ready(false)
    , m_packet_count(0)
{
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
        // Datos raw recibidos (sin procesar por protocolo seguro)
        // El protocolo se maneja en process()
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
    }
    return m_config;
}

bool RadioManager_t::saveConfig() {
    return m_fs && m_fs->saveConfig(m_config);
}

void RadioManager_t::setConfig(const services::SystemConfig& config) {
    m_config = config;
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
// Mensajes (TX)
// ============================================================================

bool RadioManager_t::sendMessage(const std::array<uint8_t, 8>& dest_mac,
                                  std::string_view message) {
    if (!m_initialized || !m_radio || !m_crypto) return false;

    std::vector<uint8_t> plaintext(message.begin(), message.end());

    // Cifrar si está habilitado
    std::vector<uint8_t> tx_data;
    if (m_config.enable_encryption) {
        auto result = m_crypto->encrypt(plaintext);
        if (!result.success) {
            log("Encryption failed: " + result.error_msg, services::LogLevel::Error);
            return false;
        }
        tx_data = std::move(result.data);
    } else {
        tx_data = plaintext;
    }

    // Enviar
    bool ok = m_radio->send(dest_mac, tx_data.data(), tx_data.size());
    if (ok) {
        log("Message sent (" + std::to_string(tx_data.size()) + " bytes)");
    }
    return ok;
}

bool RadioManager_t::sendData(const std::array<uint8_t, 8>& dest_mac,
                               const uint8_t* data, uint8_t len) {
    if (!m_initialized || !m_radio) return false;
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
// Recepción
// ============================================================================

void RadioManager_t::process() {
    if (!m_initialized || !m_radio) return;

    m_radio->poll();

    if (m_radio->hasPacket()) {
        uint8_t buf[drivers::MAX_PAYLOAD_LEN];
        uint8_t len = m_radio->getData(buf);
        m_packet_count++;

        log("Packet received #" + std::to_string(m_packet_count) +
            " (" + std::to_string(len) + " bytes, LQI=" +
            std::to_string(m_radio->lastLqi()) +
            " RSSI=" + std::to_string(m_radio->lastRssi()) + " dBm)");

        // Descifrar si está habilitado
        if (m_config.enable_encryption && m_crypto) {
            std::vector<uint8_t> ciphertext(buf, buf + len);
            auto result = m_crypto->decrypt(ciphertext);
            if (result.success) {
                m_last_message.assign(result.data.begin(), result.data.end());
                m_message_ready = true;

                // Callback
                if (m_msg_callback) {
                    m_msg_callback("0x" + services::Crypto_t::toHex(
                        {std::vector<uint8_t>(m_config.mac_address.begin(),
                                              m_config.mac_address.end())}),
                        m_last_message, m_radio->lastLqi(), m_radio->lastRssi());
                }

                // Mostrar en OLED
                if (m_oled_ok && m_oled) {
                    m_oled->clear();
                    m_oled->drawString(0, 0, "MSG #" + std::to_string(m_packet_count), 1, true);
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

                log("Decrypted: " + m_last_message);
            } else {
                log("Decryption failed: " + result.error_msg,
                    services::LogLevel::Error);
            }
        } else {
            // Sin cifrado: datos raw como string
            m_last_message.assign(reinterpret_cast<char*>(buf), len);
            m_message_ready = true;
        }

        // Log a archivo
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
        "ERR:" + std::to_string(stats.tx_fail + stats.rx_crc_errors), 1, true);

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
