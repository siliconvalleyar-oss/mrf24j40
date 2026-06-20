/**
 * @file    gpio.cpp
 * @brief   Implementación del control de GPIO vía sysfs
 * @details Proporciona funciones para exportar, configurar dirección,
 *          leer/escribir valor y detectar bordes en pines GPIO de
 *          Raspberry Pi utilizando el interfaz sysfs (/sys/class/gpio).
 *          Incluye soporte para interrupciones por polling con poll().
 *
 * @namespace GPIO
 */

#include <gpio/gpio.hpp>
#include <config/config.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>

namespace GPIO {

// ============================================================================
// Constructor / Destructor
// ============================================================================

Gpio_t::Gpio_t(bool& st)
    : m_state{st}
{
    /**
     * @brief Inicializa los pines GPIO de interrupción y salida
     * @param st Referencia al estado del módulo
     */
    settings(m_gpio_in, DIR_IN, filenameGpio);
    settings(m_gpio_out, DIR_OUT, filenameGpio);
}

Gpio_t::~Gpio_t()
{
    /** @brief Limpia recursos GPIO al destruir el objeto */
    CloseGpios();
    std::cout << "~Gpio()\n";
}

// ============================================================================
// Configuración inicial
// ============================================================================

void Gpio_t::set()
{
    /**
     * @brief Configura la detección de bordes y valor inicial de salida
     *
     * Configura el pin de entrada para detectar flanco de bajada (EDGE_FALLING)
     * y establece el valor inicial del pin de salida según el modo TX/RX.
     */
    gpio_set_edge(m_gpio_in, EDGE_FALLING);
    #ifdef USE_MRF24_RX
        gpio_set_value(m_gpio_out, VALUE_LOW);
    #else
        gpio_set_value(m_gpio_out, VALUE_HIGH);
    #endif
}

// ============================================================================
// Operaciones sysfs de bajo nivel
// ============================================================================

int Gpio_t::file_open_and_write_value(const std::string_view fname, const std::string_view wdata)
{
    /**
     * @brief Abre un archivo sysfs y escribe un valor
     * @param fname Ruta del archivo sysfs
     * @param wdata Valor a escribir
     * @return 0 si éxito, -1 si error
     */
    std::ofstream file(fname.data(), std::ios::out);
    if (!file.is_open()) {
        std::cerr << "Could not open file " << fname << "\n";
        return -1;
    }
    file << wdata;
    return 0;
}

int Gpio_t::gpio_export(int gpio_num)
{
    /** @brief Exporta un pin GPIO escribiendo su número en /sys/class/gpio/export */
    return file_open_and_write_value(SYSFS_GPIO_PATH SYSFS_GPIO_EXPORT_FN, std::to_string(gpio_num));
}

int Gpio_t::gpio_unexport(int gpio_num)
{
    /** @brief Desexporta un pin GPIO escribiendo su número en /sys/class/gpio/unexport */
    return file_open_and_write_value(SYSFS_GPIO_PATH SYSFS_GPIO_UNEXPORT_FN, std::to_string(gpio_num));
}

int Gpio_t::gpio_set_direction(int gpio_num, const std::string_view dir)
{
    /**
     * @brief Configura la dirección de un GPIO
     * @param gpio_num Número del pin
     * @param dir "in" o "out"
     */
    std::string path = std::string(SYSFS_GPIO_PATH) + "/gpio" + std::to_string(gpio_num) + "/direction";
    return file_open_and_write_value(path, dir);
}

int Gpio_t::gpio_set_value(int gpio_num, const std::string_view value)
{
    /**
     * @brief Escribe un valor en un GPIO
     * @param gpio_num Número del pin
     * @param value "1" (HIGH) o "0" (LOW)
     */
    std::string path = std::string(SYSFS_GPIO_PATH) + "/gpio" + std::to_string(gpio_num) + "/value";
    return file_open_and_write_value(path, value);
}

int Gpio_t::gpio_set_edge(int gpio_num, const std::string_view edge)
{
    /**
     * @brief Configura la detección de bordes en un GPIO
     * @param gpio_num Número del pin
     * @param edge "rising", "falling" o "both"
     */
    std::string path = std::string(SYSFS_GPIO_PATH) + "/gpio" + std::to_string(gpio_num) + "/edge";
    return file_open_and_write_value(path, edge);
}

int Gpio_t::gpio_get_fd_to_value(int gpio_num)
{
    /**
     * @brief Obtiene el file descriptor para monitorear un GPIO
     * @param gpio_num Número del pin
     * @return File descriptor (lectura no bloqueante) o -1 si error
     */
    std::string path = std::string(SYSFS_GPIO_PATH) + "/gpio" + std::to_string(gpio_num) + "/value";
    return open(path.c_str(), O_RDONLY | O_NONBLOCK);
}

// ============================================================================
// Configuración de pin
// ============================================================================

bool Gpio_t::settings(int pin, const std::string_view str_v, std::ifstream& fileTmp)
{
    /**
     * @brief Configura un pin GPIO completo
     *
     * Verifica si el GPIO ya está exportado; si no, lo exporta.
     * Luego lo desexporta y reexporta para asegurar estado limpio,
     * y configura la dirección solicitada.
     *
     * @param pin    Número del GPIO
     * @param str_v  Dirección ("in" o "out")
     * @param fileTmp Stream para verificar existencia del GPIO
     * @return true si la configuración fue exitosa
     */
    const std::string filePathGpio = "/sys/class/gpio/gpio" + std::to_string(pin) + "/direction";
    const std::string fNameResult = "echo " + std::to_string(pin) + " > /sys/class/gpio/export";

    // Verificar si el GPIO ya está exportado
    fileTmp.open(filePathGpio);
    if (!fileTmp.is_open()) {
        const int result_output = std::system(fNameResult.c_str());
        if (result_output != 0) {
            std::cerr << "Error exporting GPIO " << pin << ".\n";
            return false;
        }
    }
    fileTmp.close();

    gpio_unexport(pin);
    gpio_export(pin);
    gpio_set_direction(pin, str_v);
    return true;
}

// ============================================================================
// Bucle principal de aplicación
// ============================================================================

const bool Gpio_t::app(bool& flag)
{
    /**
     * @brief Bucle de monitoreo de GPIO por polling
     *
     * Utiliza poll() para detectar cambios en el pin de interrupción.
     * Cuando se detecta un flanco, lee el valor y actualiza el flag.
     * En modo RX, el pin de salida refleja el estado del flag.
     *
     * @param flag Flag de control: true si hay datos disponibles
     * @return true si se detectó una interrupción válida
     */
    struct pollfd fdpoll = {};
    char buf[64];
    static uint16_t val_steps{0};

    set();
    m_gpio_in_fd = gpio_get_fd_to_value(m_gpio_in);

    if (m_state) {
        for (int m_looper = 0; m_looper < READING_STEPS; ++m_looper) {
            fdpoll.fd = m_gpio_in_fd;
            fdpoll.events = POLLPRI;

            m_res = poll(&fdpoll, 1, POLL_TIMEOUT);
            if (m_res < 0) {
                #ifdef DBG_GPIO
                    std::cerr << "Poll failed... " << m_res << "\n";
                #endif
                flag = false;
                return flag;
            }
            if (m_res == 0) {
                system("clear");
                std::cout << "\nPoll timed out, esperando recibir datos ... Step: ("
                          << std::to_string(++val_steps) << " )\n";
                flag = false;
                return flag;
            }
            if (fdpoll.revents & POLLPRI) {
                lseek(fdpoll.fd, 0, SEEK_SET);
                read(fdpoll.fd, buf, sizeof(buf));
            }
        }
    } else {
        #ifdef USE_MRF24_RX
            gpio_set_value(m_gpio_out, flag ? VALUE_HIGH : VALUE_LOW);
        #else
            gpio_set_value(m_gpio_out, VALUE_LOW);
        #endif
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    #ifdef USE_MRF24_RX
        gpio_set_value(m_gpio_out, flag ? VALUE_HIGH : VALUE_LOW);
    #else
        gpio_set_value(m_gpio_out, VALUE_HIGH);
    #endif
    return false;
}

// ============================================================================
// Limpieza
// ============================================================================

void Gpio_t::CloseGpios()
{
    /**
     * @brief Cierra todos los recursos GPIO
     *
     * Cierra file descriptors, pone el pin de salida en LOW,
     * y desexporta ambos pines (entrada y salida).
     */
    if (filenameGpio.is_open()) filenameGpio.close();
    close(m_gpio_in_fd);
    gpio_set_value(m_gpio_out, VALUE_LOW);
    gpio_unexport(m_gpio_out);
    gpio_unexport(m_gpio_in);
}

} // namespace GPIO
