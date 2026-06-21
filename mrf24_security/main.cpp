/**
 * @file    main.cpp
 * @brief   Punto de entrada unificado del sistema MRF24J40
 * @details Sistema IoT modular con radio IEEE 802.15.4, cifrado
 *          AES-256-CBC, displays OLED/TFT, QR, configuración JSON
 *          y menú interactivo.
 *
 *          Integración completa:
 *          - hal/     : GPIO, SPI, I2C
 *          - drivers/ : MRF24J40, SSD1306, ST7789, QR
 *          - services/: Crypto, FileSystem, Timer
 *          - application/: RadioManager, Menu
 *
 * @author  MRF24J40 Team
 * @date    2026
 * @version 2.0.0
 */

#include <application/radio_manager.hpp>
#include <application/menu.hpp>
#include <csignal>
#include <iostream>
#include <cstdlib>

// ============================================================================
// Variables globales
// ============================================================================

static volatile bool g_running = true;

// ============================================================================
// Manejador de señales
// ============================================================================

/**
 * @brief Manejador de SIGINT/SIGTERM para terminación limpia.
 */
void signalHandler(int sig) {
    (void)sig;
    g_running = false;
    std::cout << "\n\n  Recibida señal de terminación. Saliendo...\n";
}

// ============================================================================
// Banner de inicio
// ============================================================================

void printBanner() {
    std::cout << R"(
╔══════════════════════════════════════════════════════════════╗
║                                                              ║
║    ╔╦╗╔═╗╔═╗   ╦ ╦╦╦═╗╔═╗╦╔═╗╔╗╔╔═╗╦                     ║
║     ║║╠═╣╚═╗   ║║║║╠╦╝║ ║║║ ║║║║║╣ ║                     ║
║    ═╩╝╩ ╩╚═╝   ╚╩╝╩╩╚═╚═╝╩╚═╝╝╚╝╚═╝╩                     ║
║                                                              ║
║    IEEE 802.15.4 + AES-256-CBC + OLED/TFT + QR              ║
║                                                              ║
║    Versión 2.0.0                                             ║
║    Raspberry Pi + MRF24J40MA                                 ║
║                                                              ║
╚══════════════════════════════════════════════════════════════╝
)";
}

/**
 * @brief Muestra ayuda de línea de comandos.
 */
void printUsage(const char* program) {
    std::cout << "Uso: " << program << " [opciones]\n\n";
    std::cout << "Opciones:\n";
    std::cout << "  --help           Muestra esta ayuda\n";
    std::cout << "  --menu           Inicia el menú interactivo (default)\n";
    std::cout << "  --tx <mac> <msg> Envía un mensaje y termina\n";
    std::cout << "  --rx <segundos>  Modo receptor por N segundos\n";
    std::cout << "  --qr <texto>     Genera y guarda QR como PNG\n";
    std::cout << "  --config         Muestra la configuración actual\n";
    std::cout << "  --version        Muestra la versión\n";
}

// ============================================================================
// Punto de entrada principal
// ============================================================================

int main(int argc, char* argv[]) {
    // Configurar manejador de señales
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    bool run_menu = true;
    std::string tx_mac, tx_msg;
    (void)tx_mac;
    (void)tx_msg;
    int rx_seconds = 0;
    (void)rx_seconds;
    std::string qr_text;
    bool show_config = false;
    bool show_version = false;

    // Parsear argumentos
    if (argc > 1) {
        run_menu = false;
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--help") {
                printUsage(argv[0]);
                return 0;
            } else if (arg == "--menu") {
                run_menu = true;
            } else if (arg == "--tx" && i + 2 < argc) {
                tx_mac = argv[++i];
                tx_msg = argv[++i];
            } else if (arg == "--rx" && i + 1 < argc) {
                rx_seconds = std::atoi(argv[++i]);
            } else if (arg == "--qr" && i + 1 < argc) {
                qr_text = argv[++i];
            } else if (arg == "--config") {
                show_config = true;
            } else if (arg == "--version") {
                show_version = true;
            }
        }
    }

    printBanner();

    // Mostrar versión y salir
    if (show_version) {
        std::cout << "Versión 2.0.0\n";
        std::cout << "Compilado: " << __DATE__ << " " << __TIME__ << "\n";
        return 0;
    }

    // Inicializar sistema
    std::cout << "\n[INFO] Inicializando sistema...\n\n";
    auto manager = std::make_unique<application::RadioManager_t>();

    if (!manager->init()) {
        std::cerr << "[ERROR] Falló la inicialización del sistema.\n";
        return 1;
    }

    std::cout << "[INFO] Sistema inicializado correctamente.\n\n";

    // Mostrar configuración
    if (show_config) {
        auto& cfg = manager->config();
        std::cout << "PAN ID:  0x" << std::hex << cfg.pan_id << std::dec << "\n";
        std::cout << "Canal:   " << (int)cfg.channel << "\n";
        std::cout << "Cifrado: " << (cfg.enable_encryption ? "Sí" : "No") << "\n";
        return 0;
    }

    // Generar QR y salir
    if (!qr_text.empty()) {
        manager->printQr(qr_text);
        manager->saveQrPng(qr_text, "qr_output.png");
        std::cout << "QR guardado como: qr_output.png\n";
        return 0;
    }

    // Iniciar menú interactivo (modo por defecto)
    if (run_menu) {
        application::Menu_t menu(*manager);
        menu.run();
    }

    std::cout << "\n[INFO] Sistema finalizado.\n";
    return 0;
}
