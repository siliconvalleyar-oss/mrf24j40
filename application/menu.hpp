/**
 * @file    application/menu.hpp
 * @brief   Menú interactivo del sistema MRF24J40
 * @details Proporciona una interfaz de usuario por consola con 9
 *          opciones para configurar, enviar, recibir, generar QR,
 *          probar displays, ver logs y más.
 *
 * @author  MRF24J40 Team
 * @date    2026
 * @version 2.0.0
 */

#ifndef APPLICATION_MENU_HPP
#define APPLICATION_MENU_HPP

#include <application/radio_manager.hpp>
#include <memory>

namespace application {

/**
 * @brief Menú interactivo principal del sistema.
 *
 * Uso:
 * @code
 * auto mgr = application::RadioManager_t();
 * if (mgr.init()) {
 *     application::Menu_t menu(mgr);
 *     menu.run();
 * }
 * @endcode
 */
class Menu_t {
public:
    /**
     * @brief Constructor.
     * @param manager Referencia al RadioManager.
     */
    explicit Menu_t(RadioManager_t& manager);

    ~Menu_t() = default;

    /**
     * @brief Ejecuta el bucle principal del menú.
     * El menú se muestra en un bucle hasta que el usuario elige salir.
     */
    void run();

private:
    RadioManager_t& m_manager;   ///< Referencia al orquestador del sistema
    bool m_running;              ///< Bandera de ejecución del menú

    // === Opciones del menú ===
    void showMenu();
    void handleOption(int option);
    void clearScreen();

    // === Funcionalidades del menú ===
    void optionConfigureMode();     // Opción 1
    void optionSendMessage();       // Opción 2
    void optionReceiveMode();       // Opción 3
    void optionGenerateQr();        // Opción 4
    void optionTestOled();          // Opción 5
    void optionTestTft();           // Opción 6
    void optionViewConfig();        // Opción 7
    void optionViewLogs();          // Opción 8

    // === Utilidad ===
    std::array<uint8_t, 8> inputMacAddress();
    std::string inputString(const char* prompt);
    int inputInt(const char* prompt, int default_val = 0);
};

} // namespace application

#endif // APPLICATION_MENU_HPP
