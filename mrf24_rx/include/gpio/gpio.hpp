#pragma once

/**
 * @file    gpio.hpp
 * @brief   Driver de GPIO para Raspberry Pi
 * @details Proporciona control de pines GPIO en Raspberry Pi para el módulo
 *          MRF24J40. Soporta exportación sysfs, configuración de dirección,
 *          lectura/escritura de valor y detección de bordes (interrupciones).
 *          Compatible con sistemas 32 y 64 bits.
 *
 * @namespace GPIO
 * @{
 */

#include <string_view>
#include <fstream>
#include <string>
#include <iostream>
#include <cstdint>

/**
 * @def IN_INTERRUPT
 * Pin GPIO usado para interrupción de entrada (MRF24J40 INT)
 */
#ifdef LIBRARIES_BCM2835
    #define IN_INTERRUPT    RPI_GPIO_P1_16  /**< GPIO 23 (BCM2835) */
    #define OUT_INTERRUPT   RPI_GPIO_P1_12  /**< GPIO 18 (BCM2835) */
#else
    #define IN_INTERRUPT    23              /**< GPIO de interrupción */
    #define OUT_INTERRUPT   12              /**< GPIO de LED debug */
#endif

/** @brief Cantidad de pasos de lectura en la detección de bordes */
#define READING_STEPS   2

/** @brief Ruta base del sistema de archivos sysfs GPIO */
#define SYSFS_GPIO_PATH         "/sys/class/gpio"

/** @brief Archivo sysfs para exportar GPIO */
#define SYSFS_GPIO_EXPORT_FN    "/export"

/** @brief Archivo sysfs para desexportar GPIO */
#define SYSFS_GPIO_UNEXPORT_FN  "/unexport"

/** @brief Archivo sysfs para leer/escribir valor GPIO */
#define SYSFS_GPIO_VALUE        "/value"

/** @brief Archivo sysfs para configurar dirección GPIO */
#define SYSFS_GPIO_DIRECTION    "/direction"

/** @brief Archivo sysfs para configurar detección de bordes */
#define SYSFS_GPIO_EDGE         "/edge"

/** @brief String de dirección de entrada */
#define DIR_IN  "in"

/** @brief String de dirección de salida */
#define DIR_OUT "out"

/** @brief Valor sysfs HIGH */
#define VALUE_HIGH "1"

/** @brief Valor sysfs LOW */
#define VALUE_LOW  "0"

/** @brief Detección de borde de subida */
#define EDGE_RISING   "rising"

/** @brief Detección de borde de bajada */
#define EDGE_FALLING  "falling"

/** @brief Timeout de polling en microsegundos */
#define POLL_TIMEOUT   10*1000

namespace GPIO {

    /**
     * @brief Clase de control de GPIO
     *
     * Proporciona métodos para configurar y controlar pines GPIO vía sysfs,
     * incluyendo exportación, dirección, escritura/lectura de valor y
     * detección de bordes para interrupciones por polling.
     */
    struct Gpio_t {
        /**
         * @brief Constructor: inicializa y configura los pines GPIO
         * @param st Referencia al flag de estado (se actualiza en init)
         */
        explicit Gpio_t(bool& st);

        /** @brief Destructor: cierra los file descriptors GPIO */
        ~Gpio_t();

        /**
         * @brief Bucle principal de aplicación GPIO
         * @param flag Referencia al flag de control
         * @return true si hay una interrupción detectada
         */
        const bool app(bool& flag);

    protected:
        /**
         * @brief Abre un archivo sysfs y escribe un valor
         * @param path  Ruta del archivo sysfs
         * @param value Valor a escribir
         * @return Código de retorno de la operación
         */
        int file_open_and_write_value(const std::string_view path, const std::string_view value);

        /**
         * @brief Exporta un pin GPIO vía sysfs
         * @param gpio Número del pin GPIO
         * @return 0 si éxito
         */
        int gpio_export(int gpio);

        /**
         * @brief Desexporta un pin GPIO vía sysfs
         * @param gpio Número del pin GPIO
         * @return 0 si éxito
         */
        int gpio_unexport(int gpio);

        /**
         * @brief Configura la dirección de un GPIO (in/out)
         * @param gpio      Número del pin
         * @param direction "in" o "out"
         * @return 0 si éxito
         */
        int gpio_set_direction(int gpio, const std::string_view direction);

        /**
         * @brief Escribe un valor en un GPIO
         * @param gpio  Número del pin
         * @param value "1" o "0"
         * @return 0 si éxito
         */
        int gpio_set_value(int gpio, const std::string_view value);

        /**
         * @brief Configura la detección de borde en un GPIO
         * @param gpio Número del pin
         * @param edge "rising", "falling" o "both"
         * @return 0 si éxito
         */
        int gpio_set_edge(int gpio, const std::string_view edge);

        /**
         * @brief Obtiene el file descriptor para leer el valor de un GPIO
         * @param gpio Número del pin
         * @return File descriptor (positivo) o -1 si error
         */
        int gpio_get_fd_to_value(int gpio);

        /**
         * @brief Lee la configuración de un GPIO
         * @param gpio   Número del pin
         * @param edge   Tipo de borde a detectar
         * @param stream Stream de archivo para lectura
         * @return true si la configuración es válida
         */
        bool settings(int gpio, const std::string_view edge, std::ifstream& stream);

        /** @brief Cierra y limpia todos los GPIOs abiertos */
        void CloseGpios();

        /** @brief Configuración inicial de los pines GPIO */
        void set();

    private:
        bool m_state{false};            /**< Estado del módulo */
        int m_gpio_in_fd{0};            /**< File descriptor del GPIO de entrada */
        int m_res{0};                   /**< Código de retorno general */
        const int m_gpio_out{OUT_INTERRUPT};  /**< GPIO de salida (LED) */
        const int m_gpio_in{IN_INTERRUPT};    /**< GPIO de entrada (INT) */
        std::ifstream filenameGpio;     /**< Stream de archivo GPIO */
    };

}  // namespace GPIO

/** @} */  // end of GPIO namespace
