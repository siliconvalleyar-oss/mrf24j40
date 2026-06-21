#pragma once

/**
 * @file    mrf24j40.hpp
 * @brief   Driver completo para el módulo MRF24J40MA (IEEE 802.15.4 / ZigBee)
 * @details Proporciona la clase Mrf24j con soporte para direcciones de 16 y 64 bits,
 *          manejo de interrupciones, control de PA/LNA y bufferización de payload PHY.
 *          La comunicación con el hardware se realiza vía SPI a través de SPI::Spi_t.
 *
 * @namespace MRF24J40
 * @{
 */

#include <config/config.hpp>
#include <spi/spi.hpp>

#include <iostream>
#include <memory>
#include <cstring>
#include <atomic>
#include <vector>

namespace DATA {
    struct packet_tx;  ///< Estructura forward-declarada para paquetes de transmisión
}

namespace MRF24J40 {

    /**
     * @brief Información de un paquete recibido
     *
     * Almacena la trama PHY completa, su longitud, y los indicadores
     * de calidad de enlace (LQI) y potencia de señal recibida (RSSI).
     */
    typedef struct _rx_info_t {
        uint8_t frame_length;          /**< Longitud total de la trama (header + payload + FCS) */
        uint8_t rx_data[124];          /**< Buffer de datos recibidos (máx: 127 - 3 bytes overhead) */
        uint8_t lqi;                   /**< Link Quality Indicator (0-255) */
        uint8_t rssi;                  /**< RSSI raw (convertir a dBm según hoja de datos) */
    } rx_info_t;

    /**
     * @brief Estado de la última transmisión
     *
     * Basado en el registro TXSTAT pero con campos más legibles.
     */
    typedef struct _tx_info_t {
        uint8_t tx_ok        : 1;   /**< 1 = Transmisión exitosa con ACK recibido */
        uint8_t retries      : 2;   /**< Número de retransmisiones realizadas (0-3) */
        uint8_t channel_busy : 1;   /**< 1 = CCA detectó canal ocupado */
    } tx_info_t;

    /**
     * @brief Clase principal del driver MRF24J40
     *
     * Proporciona la interfaz completa para operar el transceiver MRF24J40MA,
     * incluyendo inicialización, configuración de red, transmisión/recepción
     * de tramas IEEE 802.15.4 y manejo de interrupciones por polling.
     */
    struct Mrf24j {
    public:
        /** @brief Constructor: inicializa el puntero SPI y constantes internas */
        Mrf24j();

        /** @brief Destructor: libera recursos SPI */
        ~Mrf24j();

        /**
         * @brief Inicializa el módulo MRF24J40
         * 
         * Configura registros RF, canal, interrupciones y estado de la máquina RF.
         * Debe llamarse una vez antes de cualquier operación de TX/RX.
         */
        void init(void);

        /** @brief Inicialización alternativa (wrapper de init) */
        void mrf24j40_init(void);

        /**
         * @brief Envía datos a una dirección de 64 bits usando direccionamiento largo
         * @param dest64 Dirección MAC destino (64 bits)
         * @param data   Vector con los datos a transmitir
         */
        void send(const uint64_t dest64, const std::vector<uint8_t> data);

        /**
         * @brief Envía datos a una dirección de 64 bits (versión long address)
         * @param dest64 Dirección MAC destino (64 bits)
         * @param data   Vector con los datos a transmitir
         */
        void send64(const uint64_t dest64, const std::vector<uint8_t> data);

        /**
         * @brief Envía un paquete estructurado a dirección de 64 bits
         * @param dest64 Dirección MAC destino (64 bits)
         * @param packet Estructura packet_tx con head, size, data, checksum
         */
        void send64(const uint64_t dest64, const DATA::packet_tx packet);

        /**
         * @brief Template de envío genérico
         *
         * Función template para enviar datos a una dirección de 64 bits.
         * El tipo T se deduce automáticamente del argumento data.
         * Actualmente el cuerpo está pendiente de implementación completa.
         *
         * @tparam T Tipo de datos a enviar
         * @param dest64 Dirección MAC destino (64 bits)
         * @param data   Datos a transmitir
         */
        template <typename T>
        void send_template(const uint64_t dest64, const T& data);

        /**
         * @brief Manejador de interrupciones (polling)
         * 
         * Debe llamarse periódicamente. Lee INTSTAT, procesa RX/TX,
         * extrae payload, LQI y RSSI del FIFO de recepción.
         */
        void interrupt_handler(void);

        /**
         * @brief Configura el PAN ID de la red
         * @param panid Identificador de red PAN (16 bits)
         */
        void set_pan(const uint16_t panid);

        /**
         * @brief Escribe la dirección corta (16 bits) del dispositivo
         * @param address Dirección corta a configurar
         */
        void address16_write(const uint16_t address);

        /**
         * @brief Escribe la dirección larga (64 bits) del dispositivo
         * @param address Dirección MAC extendida a configurar
         */
        void address64_write(const uint64_t address);

        /**
         * @brief Aplica configuración adicional al MRF24J40
         * 
         * Configura modo promiscuo, coordinador y PAN coordinator
         * según los defines en config.hpp.
         */
        void settings_mrf(void);

        /**
         * @brief Configura los registros de seguridad del MRF24J40
         *
         * Por defecto deshabilita el cifrado/descifrado por hardware (SECCON1),
         * desactiva los suites de cifrado (SECCON0) y limpia los registros de
         * control de seguridad para GTS (SECCR2).
         *
         * Llamar después de init() y antes de cualquier operación TX/RX.
         */
        void settingsSecurity(void);

        /**
         * @brief Activa/desactiva modo promiscuo
         * @param enabled true = recibe todos los paquetes, false = filtra por dirección
         */
        void set_promiscuous(const bool enabled);

    protected:
        /**
         * @brief Lee un registro de dirección corta (8 bits)
         * @param address Dirección del registro (0x00-0x3F)
         * @return Valor leído del registro
         */
        const uint8_t read_short(const uint8_t address);

        /**
         * @brief Lee un registro de dirección larga (16 bits)
         * @param address Dirección del registro (0x200-0x3FF)
         * @return Valor leído del registro
         */
        const uint8_t read_long(const uint16_t address);

        /**
         * @brief Escribe un registro de dirección corta
         * @param address Dirección del registro
         * @param data    Valor a escribir
         */
        void write_short(const uint8_t address, const uint8_t data);

        /**
         * @brief Escribe un registro de dirección larga
         * @param address Dirección del registro
         * @param data    Valor a escribir
         */
        void write_long(const uint16_t address, const uint8_t data);

        /** @brief Obtiene el PAN ID actual del módulo */
        const uint16_t get_pan(void);

        /** @brief Lee la dirección corta (16 bits) configurada en el módulo */
        const uint16_t address16_read(void);

        /** @brief Lee la dirección larga (64 bits) configurada en el módulo */
        const uint64_t address64_read(void);

        /** @brief Configura las máscaras de interrupción para TX y RX */
        void set_interrupts(void);

        /**
         * @brief Selecciona el canal de operación (estándar IEEE 802.15.4)
         * @param channel Número de canal (11-26)
         */
        void set_channel(const uint8_t channel);

        /** @brief Habilita el receptor */
        void rx_enable(void);

        /** @brief Deshabilita el receptor */
        void rx_disable(void);

        /** @brief Establece modo del pin (compatibilidad Arduino, no implementado) */
        void pinMode(const int, const bool);

        /** @brief Escribe un pin digital (compatibilidad Arduino, no implementado) */
        void digitalWrite(const int, const bool);

        /**
         * @brief Delay en milisegundos
         * @param t Tiempo de espera en ms
         */
        void delay(const uint16_t t);

        /** @brief Habilita interrupciones globales */
        void interrupts(void);

        /** @brief Deshabilita interrupciones globales */
        void noInterrupts(void);

    public:
        /** @brief Limpia el FIFO de recepción */
        void rx_flush(void);

        /** @brief Obtiene puntero a la estructura de información RX */
        rx_info_t* get_rxinfo(void);

        /** @brief Obtiene puntero a la estructura de información TX */
        tx_info_t* get_txinfo(void);

        /** @brief Obtiene puntero al buffer raw de recepción PHY */
        uint8_t* get_rxbuf(void);

        /**
         * @brief Obtiene la longitud de los datos relevantes (sin header ni FCS)
         * @return Longitud de payload en bytes
         */
        const int rx_datalength(void);

        /**
         * @brief Configura la cantidad de bytes a ignorar en la trama
         * @param ib Número de bytes a ignorar (comportamiento de algunos módulos)
         */
        void set_ignoreBytes(const int ib);

        /**
         * @brief Habilita/deshabilita bufferización completa del payload PHY
         * @param enable true = bufferiza todos los bytes PHY
         */
        void set_bufferPHY(const bool enable);

        /** @brief Retorna el estado del flag de bufferización PHY */
        bool get_bufferPHY(void);

        /**
         * @brief Control externo de PA/LNA para módulo MRF24J40MB
         * @param enable true = habilita PA/LNA, false = deshabilita
         */
        void set_palna(const bool enable);

        /**
         * @brief Verifica los flags de interrupción y ejecuta handlers
         * 
         * Si hay datos RX pendientes, invoca rx_handler.
         * Si hay TX completada, invoca tx_handler.
         * 
         * @param rx_handler Función callback para recepción
         * @param tx_handler Función callback para transmisión
         * @return true si hay datos RX disponibles
         */
        const bool check_flags(void (*rx_handler)(), void (*tx_handler)());

    protected:
        /** @brief Activa modo turbo (625 kbps) si TURBO_MODE está definido */
        void mode_turbo();

        /**
         * @brief Escribe una dirección MAC de 64 bits en el FIFO TX
         * @param i          Índice del registro FIFO (se actualiza)
         * @param mac_adress Dirección MAC a escribir
         */
        void set_macaddress(int& i, const uint64_t mac_adress);

        /** @brief Resetea la máquina de estados RF */
        void reset_rf_state_machine(void);

        /** @brief Limpia el FIFO de recepción */
        void flush_rx_fifo(void);

        /**
         * @brief Lee o escribe la potencia de transmisión
         * @param pwr Referencia al valor de potencia
         */
        void mrf24j40_set_tx_power(uint8_t& pwr);

    public:
        /**
         * @brief Obtiene la dirección MAC extendida (64 bits) del módulo
         * @param address Puntero donde almacenar la dirección leída
         */
        void mrf24j40_get_extended_mac_addr(uint64_t* address);

        /**
         * @brief Obtiene la dirección MAC corta (16 bits) del módulo
         * @param address Puntero donde almacenar la dirección leída
         */
        void mrf24j40_get_short_mac_addr(uint16_t* address);

    private:
        std::unique_ptr<SPI::Spi_t> prt_spi {};  /**< Puntero único al driver SPI */

        /**
         * @brief Tamaño del MHR (MAC Header)
         * 
         * bytes_MHR = 2 (Frame Control) + 1 (sequence) + 2 (PAN) + 2 (dest) + 2 (src) = 9
         * Para direcciones largas: +2+2+6+6 = 23 bytes
         */
        const size_t m_bytes_MHR {SIZE_HEAD_PACKET_DATA};

        /** @brief FCS length (2 bytes = CRC16) */
        const size_t m_bytes_FCS {2};

        /** @brief Bytes sin datos en payload PHY (header + FCS) */
        const size_t m_bytes_nodata {0};

        std::atomic<uint8_t> m_flag_got_rx{};  /**< Flag atómico de RX recibido */
        std::atomic<uint8_t> m_flag_got_tx{};  /**< Flag atómico de TX completada */
    };

    /** @brief Alias para compatibilidad con código existente que usa Mrf24j_t */
    using Mrf24j_t = Mrf24j;

}  // namespace MRF24J40

/** @} */  // end of MRF24J40 namespace
