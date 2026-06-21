/**
 * @file    hal/gpio.hpp
 * @brief   Abstracción de GPIO para Raspberry Pi (bcm2835)
 * @details Proporciona una interfaz limpia y moderna para controlar pines
 *          GPIO mediante la librería BCM2835. Soporta entrada, salida,
 *          interrupciones por polling y gestión automática de recursos.
 *
 * @author  MRF24J40 Team
 * @date    2026
 * @version 2.0.0
 */

#ifndef HAL_GPIO_HPP
#define HAL_GPIO_HPP

#include <cstdint>
#include <memory>
#include <functional>
#include <chrono>
#include <thread>
#include <string_view>

namespace hal {

/**
 * @brief Enumeración de modos de dirección para un pin GPIO.
 */
enum class GpioDirection {
    Input,   /**< Pin configurado como entrada */
    Output   /**< Pin configurado como salida */
};

/**
 * @brief Enumeración de estados lógicos para un pin GPIO.
 */
enum class GpioValue {
    Low  = 0, /**< Nivel lógico bajo (0V) */
    High = 1  /**< Nivel lógico alto (3.3V) */
};

/**
 * @brief Enumeración de tipos de flanco para detección de interrupciones.
 */
enum class GpioEdge {
    None,    /**< Sin detección de flancos */
    Rising,  /**< Detectar flanco de subida */
    Falling, /**< Detectar flanco de bajada */
    Both     /**< Detectar ambos flancos */
};

/**
 * @brief Clase de alto nivel para control de GPIO.
 *
 * Encapsula un pin GPIO con gestión RAII: exporta el pin en el
 * constructor y lo libera en el destructor. Soporta operación
 * síncrona (lectura/escritura) y asíncrona (callback por flanco).
 *
 * Uso:
 * @code
 * auto led = hal::Gpio_t(17, hal::GpioDirection::Output);
 * led.write(hal::GpioValue::High);
 *
 * auto btn = hal::Gpio_t(27, hal::GpioDirection::Input, hal::GpioEdge::Rising);
 * btn.onInterrupt([]() { std::cout << "Botón presionado!\n"; });
 * @endcode
 */
class Gpio_t {
public:
    /**
     * @brief Constructor principal: configura un pin GPIO.
     * @param pin       Número del pin GPIO (BCM).
     * @param direction Dirección (entrada o salida).
     * @param edge      Tipo de flanco a detectar (default: None).
     *
     * @throws std::runtime_error si BCM2835 no está inicializado o el pin no se puede configurar.
     */
    explicit Gpio_t(uint8_t pin, GpioDirection direction, GpioEdge edge = GpioEdge::None);

    /// Destructor: libera el pin GPIO.
    ~Gpio_t();

    // No copiable
    Gpio_t(const Gpio_t&) = delete;
    Gpio_t& operator=(const Gpio_t&) = delete;

    // Movible
    Gpio_t(Gpio_t&&) noexcept;
    Gpio_t& operator=(Gpio_t&&) noexcept;

    /**
     * @brief Escribe un valor en el pin (solo si está configurado como salida).
     * @param value HIGH o LOW.
     */
    void write(GpioValue value);

    /**
     * @brief Lee el valor actual del pin.
     * @return HIGH o LOW.
     */
    GpioValue read() const;

    /**
     * @brief Alterna el estado del pin.
     * @return El nuevo valor después de alternar.
     */
    GpioValue toggle();

    /**
     * @brief Configura un callback para interrupción por flanco.
     *
     * El callback se invocará en un hilo separado cuando se detecte
     * el flanco configurado en el constructor. Debe ser rápido y no
     * bloqueante.
     *
     * @param callback Función a invocar en cada flanco.
     */
    void onInterrupt(std::function<void()> callback);

    /**
     * @brief Obtiene el número del pin GPIO.
     * @return Número BCM del pin.
     */
    uint8_t pin() const { return m_pin; }

private:
    uint8_t m_pin;                                  ///< Número del pin BCM
    GpioDirection m_direction;                      ///< Dirección configurada
    int m_fd;                                       ///< File descriptor para sysfs (o -1 si usa BCM2835)
    bool m_owned;                                   ///< true si este objeto posee el pin
    std::unique_ptr<std::thread> m_irq_thread;      ///< Hilo de monitoreo de interrupciones
    bool m_irq_running;                             ///< Bandera para el hilo de IRQ

    /**
     * @brief Inicializa el pin con BCM2835.
     * @throws std::runtime_error si falla la inicialización.
     */
    void initBcm2835();

    /**
     * @brief Inicializa el pin con sysfs (fallback).
     * @throws std::runtime_error si falla la exportación.
     */
    void initSysfs();
};

/**
 * @brief Inicializa el subsistema BCM2835 globalmente.
 * @return true si la inicialización fue exitosa.
 *
 * Debe llamarse una vez al inicio del programa ANTES de crear
 * cualquier instancia de Gpio_t.
 */
bool gpioInit();

/**
 * @brief Libera el subsistema BCM2835 globalmente.
 * Debe llamarse al final del programa después de destruir todas
 * las instancias de Gpio_t.
 */
void gpioClose();

} // namespace hal

#endif // HAL_GPIO_HPP
