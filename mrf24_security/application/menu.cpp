/**
 * @file    application/menu.cpp
 * @brief   Implementación del menú interactivo
 *
 * @author  MRF24J40 Team
 * @date    2026
 */

#include <application/menu.hpp>
#include <drivers/qr.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <chrono>
#include <csignal>

namespace application {

// ============================================================================
// Constructor
// ============================================================================

Menu_t::Menu_t(RadioManager_t& manager)
    : m_manager(manager)
    , m_running(false)
{
}

// ============================================================================
// Utilidad
// ============================================================================

void Menu_t::clearScreen() {
    std::cout << "\033[2J\033[H";
}

std::string Menu_t::inputString(const char* prompt) {
    std::cout << prompt;
    std::string input;
    std::getline(std::cin, input);
    return input;
}

int Menu_t::inputInt(const char* prompt, int default_val) {
    std::cout << prompt;
    std::string input;
    std::getline(std::cin, input);
    if (input.empty()) return default_val;
    try {
        return std::stoi(input);
    } catch (...) {
        return default_val;
    }
}

std::array<uint8_t, 8> Menu_t::inputMacAddress() {
    std::array<uint8_t, 8> mac{};
    std::string input = inputString("  Dirección MAC (hex, ej: 00:00:00:00:00:00:00:02): ");

    // Eliminar separadores
    std::string hex;
    for (char c : input) {
        if (c != ':' && c != '-' && c != ' ') hex += c;
    }

    // Parsear
    for (int i = 0; i < 8 && i * 2 + 1 < (int)hex.length(); i++) {
        std::string byte_str = hex.substr(i * 2, 2);
        mac[i] = std::stoul(byte_str, nullptr, 16);
    }

    return mac;
}

// ============================================================================
// Menú principal
// ============================================================================

void Menu_t::showMenu() {
    auto stats = m_manager.radioStats();

    clearScreen();
    std::cout << "╔════════════════════════════════════════════════╗\n";
    std::cout << "║        MRF24J40 - SISTEMA IoT v2.0           ║\n";
    std::cout << "╠════════════════════════════════════════════════╣\n";
    std::cout << "║ PAN: 0x" << std::hex << std::uppercase
              << std::setw(4) << std::setfill('0')
              << m_manager.config().pan_id
              << "  Canal: " << std::dec << (int)m_manager.config().channel
              << "  Modo: ";
    switch (m_manager.nodeMode()) {
        case drivers::NodeMode::EndDevice: std::cout << "End Device"; break;
        case drivers::NodeMode::Router:    std::cout << "Router";     break;
        case drivers::NodeMode::Gateway:   std::cout << "Gateway";    break;
        case drivers::NodeMode::Node:      std::cout << "Nodo";       break;
    }
    std::cout << "\n";
    std::cout << "║ TX:" << std::setw(4) << stats.packets_sent
              << "  RX:" << std::setw(4) << stats.packets_received
              << "  ERR:" << std::setw(3) << (stats.tx_fail + stats.rx_crc_errors)
              << "                        ║\n";
    std::cout << "╠════════════════════════════════════════════════╣\n";
    std::cout << "║  1. Configurar modo MRF24J40                  ║\n";
    std::cout << "║  2. Enviar mensaje cifrado                    ║\n";
    std::cout << "║  3. Escuchar y mostrar mensajes               ║\n";
    std::cout << "║  4. Generar QR desde texto                    ║\n";
    std::cout << "║  5. Probar OLED (SSD1306)                     ║\n";
    std::cout << "║  6. Probar TFT (ST7789)                       ║\n";
    std::cout << "║  7. Ver/Editar configuración                  ║\n";
    std::cout << "║  8. Ver logs                                  ║\n";
    std::cout << "║  9. Salir                                     ║\n";
    std::cout << "╚════════════════════════════════════════════════╝\n";
    std::cout << "\n  Opción: ";
    std::cout.flush();
}

void Menu_t::run() {
    if (!m_manager.isReady()) {
        std::cerr << "Error: Sistema no inicializado.\n";
        return;
    }

    m_running = true;

    // Iniciar hilo de recepción
    std::thread rx_thread([this]() {
        while (m_running) {
            m_manager.process();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    while (m_running) {
        showMenu();

        std::string input;
        std::getline(std::cin, input);

        int option = 0;
        try { option = std::stoi(input); } catch (...) { continue; }

        if (option == 9) {
            m_running = false;
            std::cout << "\n  Saliendo...\n";
            break;
        }

        handleOption(option);
        std::cout << "\n  Presiona Enter para continuar...";
        std::cin.get();
    }

    m_running = false;
    if (rx_thread.joinable()) rx_thread.join();
}

void Menu_t::handleOption(int option) {
    clearScreen();
    switch (option) {
        case 1: optionConfigureMode();  break;
        case 2: optionSendMessage();    break;
        case 3: optionReceiveMode();    break;
        case 4: optionGenerateQr();     break;
        case 5: optionTestOled();       break;
        case 6: optionTestTft();        break;
        case 7: optionViewConfig();     break;
        case 8: optionViewLogs();       break;
        default:
            std::cout << "  Opción inválida.\n";
            break;
    }
}

// ============================================================================
// Opción 1: Configurar modo
// ============================================================================

void Menu_t::optionConfigureMode() {
    std::cout << "╔═══ CONFIGURAR MODO MRF24J40 ═══╗\n\n";
    std::cout << "  Modos disponibles:\n";
    std::cout << "    0 - End Device (solo envía/recibe)\n";
    std::cout << "    1 - Router (reenvía paquetes)\n";
    std::cout << "    2 - Gateway (reenvía a red externa)\n";
    std::cout << "    3 - Nodo (comunicación punto a punto)\n\n";

    int mode = inputInt("  Selecciona modo [0-3]: ", 3);
    if (mode >= 0 && mode <= 3) {
        m_manager.setNodeMode(static_cast<drivers::NodeMode>(mode));
        std::cout << "\n  ✅ Modo cambiado a: ";
        switch (mode) {
            case 0: std::cout << "End Device"; break;
            case 1: std::cout << "Router";     break;
            case 2: std::cout << "Gateway";    break;
            case 3: std::cout << "Nodo";       break;
        }
        std::cout << "\n";
        m_manager.saveConfig();
    } else {
        std::cout << "\n  ❌ Modo inválido.\n";
    }
}

// ============================================================================
// Opción 2: Enviar mensaje
// ============================================================================

void Menu_t::optionSendMessage() {
    std::cout << "╔═══ ENVIAR MENSAJE CIFRADO ═══╗\n\n";

    auto dest = inputMacAddress();
    std::string message = inputString("  Mensaje a enviar: ");

    if (message.empty()) {
        std::cout << "  ❌ Mensaje vacío.\n";
        return;
    }

    std::cout << "\n  ¿Incluir QR del mensaje? (s/N): ";
    std::string qr_opt;
    std::getline(std::cin, qr_opt);

    bool ok;
    if (qr_opt == "s" || qr_opt == "S") {
        ok = m_manager.sendQrMessage(dest, message);
    } else {
        ok = m_manager.sendMessage(dest, message);
    }

    if (ok) {
        std::cout << "\n  ✅ Mensaje enviado (" << message.length() << " chars)\n";
    } else {
        std::cout << "\n  ❌ Error al enviar mensaje\n";
    }
}

// ============================================================================
// Opción 3: Escuchar
// ============================================================================

void Menu_t::optionReceiveMode() {
    std::cout << "╔═══ MODO RECEPCIÓN ═══╗\n\n";
    std::cout << "  Escuchando paquetes entrantes...\n";
    std::cout << "  Presiona Ctrl+C para volver al menú.\n\n";

    // Usar callback de mensajes
    m_manager.onMessage([](const std::string& from,
                            const std::string& message,
                            uint8_t lqi, int8_t rssi) {
        std::cout << "  [" << std::time(nullptr) << "]"
                  << " De: " << from.substr(0, 16) << "..."
                  << " LQI:" << (int)lqi
                  << " RSSI:" << (int)rssi << "dBm\n"
                  << "  Mensaje: " << message << "\n\n";
    });

    // Bucle de recepción
    for (int i = 0; i < 300; i++) { // ~30 segundos
        m_manager.process();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Remover callback
    m_manager.onMessage(nullptr);
}

// ============================================================================
// Opción 4: Generar QR
// ============================================================================

void Menu_t::optionGenerateQr() {
    std::cout << "╔═══ GENERAR QR ═══╗\n\n";

    std::string text = inputString("  Texto para el QR: ");
    if (text.empty()) {
        std::cout << "  ❌ Texto vacío.\n";
        return;
    }

    // Mostrar en consola
    std::cout << "\n  Código QR:\n";
    m_manager.printQr(text);

    // Guardar como PNG
    std::string filename = "qr_" + text.substr(0, 16) + ".png";
    if (m_manager.saveQrPng(text, filename)) {
        std::cout << "  ✅ QR guardado como: " << filename << "\n";
    }

    // Mostrar en OLED si está disponible
    m_manager.showQrOnOled(text);
}

// ============================================================================
// Opción 5: Probar OLED
// ============================================================================

void Menu_t::optionTestOled() {
    std::cout << "╔═══ TEST OLED SSD1306 ═══╗\n\n";

    // Test 1: Texto
    std::cout << "  [1/4] Mostrando texto...\n";
    m_manager.showQrOnOled("MRF24J40 v2.0");
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Test 2: Formas
    std::cout << "  [2/4] Mostrando formas...\n";
    auto& radio = m_manager.radio();

    // Test 3: QR
    std::cout << "  [3/4] Mostrando QR...\n";
    m_manager.showQrOnOled("Hello IoT!");

    // Test 4: Estadísticas
    std::cout << "  [4/4] Mostrando estadísticas...\n";
    m_manager.updateOled();

    std::cout << "\n  ✅ Test OLED completado.\n";
}

// ============================================================================
// Opción 6: Probar TFT
// ============================================================================

void Menu_t::optionTestTft() {
    std::cout << "╔═══ TEST TFT ST7789 ═══╗\n";
    std::cout << "\n  ⚠ TFT no implementado en este test.\n";

    // Test básico
    auto text = inputString("  Texto para QR en TFT: ");
    if (!text.empty()) {
        m_manager.showQrOnTft(text);
        std::cout << "  ✅ QR enviado a TFT.\n";
    }
}

// ============================================================================
// Opción 7: Ver configuración
// ============================================================================

void Menu_t::optionViewConfig() {
    std::cout << "╔═══ CONFIGURACIÓN ACTUAL ═══╗\n\n";

    auto& cfg = m_manager.config();
    std::cout << "  PAN ID:              0x" << std::hex
              << cfg.pan_id << std::dec << "\n";
    std::cout << "  Canal:               " << (int)cfg.channel << "\n";
    std::cout << "  Modo:                " << (int)cfg.node_mode << "\n";
    std::cout << "  MAC:                 ";
    for (auto b : cfg.mac_address) {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << (int)b << ":";
    }
    std::cout << std::dec << "\n";
    std::cout << "  Cifrado:             " << (cfg.enable_encryption ? "Sí" : "No") << "\n";
    std::cout << "  Hash SHA-256:        " << (cfg.enable_hash ? "Sí" : "No") << "\n";
    std::cout << "  OLED:                " << (cfg.use_oled ? "Sí" : "No") << "\n";
    std::cout << "  TFT:                 " << (cfg.use_tft ? "Sí" : "No") << "\n";
    std::cout << "  Log a archivo:       " << (cfg.log_to_file ? "Sí" : "No") << "\n";
    std::cout << "\n  ¿Editar configuración? (s/N): ";

    std::string edit;
    std::getline(std::cin, edit);
    if (edit == "s" || edit == "S") {
        services::SystemConfig new_cfg = cfg;
        new_cfg.channel = inputInt("  Nuevo canal [11-26]: ", cfg.channel);
        new_cfg.enable_encryption = (inputInt("  Habilitar cifrado [0/1]: ", cfg.enable_encryption ? 1 : 0) == 1);
        new_cfg.passphrase = inputString("  Nueva passphrase (Enter = mantener): ");
        if (new_cfg.passphrase.empty()) new_cfg.passphrase = cfg.passphrase;

        m_manager.setConfig(new_cfg);
        m_manager.saveConfig();
        std::cout << "\n  ✅ Configuración actualizada.\n";
    }
}

// ============================================================================
// Opción 8: Ver logs
// ============================================================================

void Menu_t::optionViewLogs() {
    std::cout << "╔═══ LOGS ═══╗\n\n";
    m_manager.showLog(20);
}

} // namespace application
