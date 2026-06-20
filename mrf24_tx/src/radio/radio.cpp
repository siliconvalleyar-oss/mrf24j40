/**
 * @file    radio.cpp
 * @brief   Implementación de la orquestación de radio de alto nivel
 * @details Proporciona el bucle principal Radio_t que coordina la
 *          inicialización del driver MRF24J40, la configuración de red,
 *          el envío periódico de tramas ZigBee (TX) y el procesamiento
 *          de paquetes recibidos (RX) con callbacks.
 *
 * @namespace MRF24J40
 */

#include <radio/radio.hpp>
#include <mrf24/mrf24j40.hpp>
#include <string_view>
#include <memory>

namespace MRF24J40 {

/// @brief Puntero global al driver Mrf24j_t (instancia única)
std::unique_ptr<Mrf24j_t> mrf24j40_spi;

/// @brief Último mensaje RX recibido (para acceso externo)
std::string msj_txt = {"MRF24J40 RX"};

// ============================================================================
// Constructor
// ============================================================================

Radio_t::Radio_t()
    #ifdef ENABLE_INTERRUPT_MRF24
    : m_status(true)
    #else
    : m_status(false)
    #endif
    , gpio{std::make_unique<GPIO::Gpio_t>()}
{
    /**
     * @brief Inicializa el módulo radio y configura la red
     *
     * Crea la instancia del driver Mrf24j_t, inicializa el MRF24J40,
     * configura PAN ID, dirección (16 o 64 bits), modo promiscuo,
     * control PA/LNA y bufferización PHY.
     */
    #ifdef DBG
        std::cout << "Size msj : ( " << std::dec << sizeof(MSJ) << " )\n";
    #endif

    mrf24j40_spi = std::make_unique<Mrf24j_t>();
    mrf24j40_spi->init();
    mrf24j40_spi->settingsSecurity();
    mrf24j40_spi->interrupt_handler();
    mrf24j40_spi->set_pan(PAN_ID);

    // Configurar dirección según el modo MAC
    #ifdef MACADDR16
        mrf24j40_spi->address16_write(ADDRESS);
    #elif defined(MACADDR64)
        mrf24j40_spi->address64_write(ADDRESS_LONG);
    #endif

    mrf24j40_spi->set_promiscuous(true);
    mrf24j40_spi->set_palna(true);
    mrf24j40_spi->set_bufferPHY(true);

    m_flag = true;
}

// ============================================================================
// Bucle principal
// ============================================================================

const bool Radio_t::Run(void)
{
    /**
     * @brief Ejecuta un ciclo del bucle principal de la radio
     *
     * Procesa GPIO, verifica flags de interrupción TX/RX y
     * ejecuta transmisión periódica si está en modo transmisor.
     *
     * @return Estado del flag de interrupción
     */
    gpio->app(m_flag);
    Start(m_flag);
    interrupt_routine();
    return m_flag;
}

// ============================================================================
// Control de transmisión periódica
// ============================================================================

void Radio_t::Start(bool& flag)
{
    /**
     * @brief Procesa interrupciones y envía paquetes periódicamente
     *
     * Verifica los flags del driver mediante check_flags().
     * En modo transmisor (MRF24_TRANSMITER_ENABLE), construye un paquete
     * con HEAD, tamaño invertido y mensaje MSJ, y lo envía vía send() o send64().
     *
     * @param flag Flag de datos recibidos (se actualiza vía check_flags)
     */
    if (flag == false) {
        std::cout << "Radio_t::Start :  interrupt !!! \n";
    }

    flag = mrf24j40_spi->check_flags(&handle_rx, &handle_tx);

    const unsigned long current_time = 1000;
    if (current_time - m_last_time > m_tx_interval) {
        m_last_time = current_time;

    #ifdef MRF24_TRANSMITER_ENABLE
        #ifdef DBG_RADIO
            #ifdef MACADDR64
                std::cout << "send msj 64() ... \n";
            #else
                std::cout << "send msj 16() ... \n";
            #endif
        #endif

        // Construir paquete de transmisión
        buffer_transmiter.head = HEAD;
        buffer_transmiter.size = (~strlen(MSJ)) & 0xffff;
        std::strcpy(buffer_transmiter.data, MSJ);

        const uint8_t* struct_ptr = reinterpret_cast<const uint8_t*>(&buffer_transmiter);
        size_t struct_size = sizeof(DATA::packet_tx);
        std::vector<char> msj(struct_ptr, struct_ptr + struct_size);

        #ifdef USE_MRF24_TX
            #ifdef MACADDR64
                mrf24j40_spi->send(ADDRESS_LONG_SLAVE, msj.data());
            #elif defined(MACADDR16)
                mrf24j40_spi->send(ADDRESS_SLAVE, msj.data());
            #endif
        #endif

        // Verificar estado del ACK
        const auto status = mrf24j40_spi->getStatusInfoTx();
        if (status == 0) {
            std::cout << "\nTX ACK failed\n";
        }
        if (status >= 1) {
            std::cout << "\tTX ACK Ok\n";
        }
    #endif
    }
}

// ============================================================================
// Manejador de interrupciones
// ============================================================================

void Radio_t::interrupt_routine()
{
    /** @brief Procesa interrupciones pendientes del MRF24J40 */
    mrf24j40_spi->interrupt_handler();
}

// ============================================================================
// Callback de actualización
// ============================================================================

void update(std::string_view str_view)
{
    /**
     * @brief Procesa datos recibidos (callback)
     * @param str_view Datos del paquete recibido
     */
    const int positionAdvance{15};
    const auto* packet_data = reinterpret_cast<const char*>(str_view.data());
    std::string PacketDataTmp(packet_data += positionAdvance);
    PacketDataTmp.resize(38);
    std::cout << "\n";
}

// ============================================================================
// Handlers estáticos TX/RX
// ============================================================================

void Radio_t::handle_tx()
{
    /**
     * @brief Callback de TX completada
     * Muestra si la transmisión fue exitosa y el número de retransmisiones.
     */
    #ifdef MRF24_TRANSMITER_ENABLE
        const auto status = mrf24j40_spi->get_txinfo()->tx_ok;
        if (status) {
            std::cout << "\thandle_tx() : TX went ok, got ACK success !\n";
        } else {
            std::cout << "\n\tTX failed after \n";
            std::cout << "retries : " << mrf24j40_spi->get_txinfo()->retries;
            std::cout << " \n";
        }
    #endif
}

void Radio_t::handle_rx()
{
    /**
     * @brief Callback de RX recibido
     *
     * Lee el frame_length del RX FIFO, copia los datos PHY,
     * muestra payload en ASCII y hexadecimal, e imprime
     * información de dirección MAC, LQI y RSSI.
     */
    #ifdef MRF24_RECEIVER_ENABLE
        auto size = static_cast<std::size_t>(mrf24j40_spi->get_rxinfo()->frame_length);
        std::vector<char> bufferMonitor(size);

        std::printf("received a packet ... ");
        std::memcpy(bufferMonitor.data(), mrf24j40_spi->get_rxbuf(), size);

        std::cout << "tamaño del paquete : " << std::to_string(bufferMonitor.size()) << "\n";
        std::cout << bufferMonitor.data();
        std::cout << "\n\n\n";

        if (mrf24j40_spi->get_bufferPHY()) {
            std::printf(" Packet data (PHY Payload) :");
            #ifdef DBG_PRINT_GET_INFO
                for (int i = 0; i < mrf24j40_spi->get_rxinfo()->frame_length; i++) {
                    std::cout << " " << std::hex << mrf24j40_spi->get_rxbuf()[i];
                }
            #endif
        }
        std::cout << "\n";

        std::printf("ASCII data (relevant data) :");
        const auto recevive_data_length = mrf24j40_spi->rx_datalength();
        std::cout << "\tdata_length : " << std::to_string(recevive_data_length);
        std::printf("\n");
        const std::string get_rx_info = reinterpret_cast<const char*>(mrf24j40_spi->get_rxinfo()->rx_data);
        std::cout << get_rx_info << "\n";

        for (auto& byte : mrf24j40_spi->get_rxinfo()->rx_data)
            std::cout << byte;
        std::cout << "\n";

        std::cout << "LQI : " << std::to_string(mrf24j40_spi->get_rxinfo()->lqi) << "\n";
        std::cout << "RSSI : " << std::to_string(mrf24j40_spi->get_rxinfo()->rssi) << "\n";

        update(reinterpret_cast<const char*>(mrf24j40_spi->get_rxinfo()->rx_data));

        msj_txt = reinterpret_cast<const char*>(mrf24j40_spi->get_rxinfo()->rx_data);
        std::cout << msj_txt << "\n";
    #endif
}

// ============================================================================
// Destructor
// ============================================================================

Radio_t::~Radio_t()
{
    #ifdef DBG
        std::cout << "~Radio_t()\n";
    #endif
}

} // namespace MRF24J40
