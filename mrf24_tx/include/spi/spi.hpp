#pragma once

/**
 * @file    spi.hpp
 * @brief   Driver de comunicación SPI para Raspberry Pi (BCM2835)
 * @details Proporciona la clase Spi_t para manejar la comunicación SPI
 *          con el módulo MRF24J40MA. Soporta transferencias de 1, 2 y 3 bytes
 *          utilizando la librería BCM2835.
 *
 * @namespace SPI
 * @{
 */

#include <cstdint>
#include <memory>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <vector>

/** @brief Velocidad por defecto del SPI en Hz */
#define SPI_SPEED   100000

/** @brief Tamaño máximo del buffer de transferencia */
#define MAX_BUFFER 256

namespace SPI {

    /**
     * @brief Clase para manejo de comunicación SPI
     *
     * Encapsula la inicialización, configuración y transferencias SPI
     * utilizando la librería BCM2835. Implementa transferencias de 1 byte
     * (comandos), 2 bytes (short address) y 3 bytes (long address).
     */
    struct Spi_t {
        /** @brief Constructor: inicializa miembros internos */
        explicit Spi_t();

        /** @brief Destructor: cierra la conexión SPI */
        ~Spi_t();

        /**
         * @brief Inicializa la interfaz SPI con el MRF24J40
         *
         * Configura modo 0, MSB first, velocidad y chip select.
         * Debe ejecutarse con permisos de root (sudo).
         */
        void init();

        /**
         * @brief Aplica configuración adicional a la interfaz SPI
         */
        void settings_spi();

        /** @brief Cierra la comunicación SPI y libera recursos */
        void spi_close();

        /**
         * @brief Transfiere 1 byte por SPI (lectura/escritura de comando)
         * @param cmd Byte de comando a enviar
         * @return Byte recibido del esclavo
         */
        const uint8_t Transfer1bytes(const uint8_t cmd);

        /**
         * @brief Transfiere 2 bytes por SPI (acceso a registros short address)
         * @param address Dirección de 16 bits (comando + datos)
         * @return Byte recibido (segundo byte de la transferencia)
         */
        const uint8_t Transfer2bytes(const uint16_t address);

        /**
         * @brief Transfiere 3 bytes por SPI (acceso a registros long address)
         * @param address Dirección de 32 bits (comando largo + datos)
         * @return Byte recibido (tercer byte de la transferencia)
         */
        const uint8_t Transfer3bytes(const uint32_t address);

        /**
         * @brief Imprime información de depuración de la configuración SPI
         */
        void printDBGSpi();

        /**
         * @brief Muestra mensaje de error si la inicialización SPI falla
         */
        void msj_fail();

        /**
         * @brief Obtiene la velocidad configurada del SPI
         * @return Velocidad en Hz
         */
        const uint32_t get_spi_speed();

    private:
        uint8_t m_tx_buffer[MAX_BUFFER]{0x00};  /**< Buffer de transmisión */
        uint8_t m_rx_buffer[MAX_BUFFER]{0x00};  /**< Buffer de recepción */

        const uint32_t m_len_data {32};         /**< Longitud de datos por transferencia */
        const uint32_t m_spi_speed {0};          /**< Velocidad SPI configurada */

        int m_fs{0};        /**< File descriptor del dispositivo SPI */
        int m_ret{0};       /**< Código de retorno de operaciones SPI */
        uint8_t looper{0};  /**< Contador interno para loops */
        uint32_t scratch32{0};  /**< Variable temporal de 32 bits */

        std::unique_ptr<struct spi_ioc_transfer> spi{nullptr};  /**< Estructura de transferencia SPI */
    };

}  // namespace SPI

/** @} */  // end of SPI namespace
