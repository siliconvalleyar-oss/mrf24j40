/**
 * @file    mrf24/radio.cpp
 * @brief   Orquestación de alto nivel de la radio (driver completo)
 * @details Proporciona la clase Radio_t del namespace MRF24J40 que coordina
 *          la inicialización del driver Mrf24j, la transmisión periódica de
 *          paquetes ZigBee, el procesamiento de paquetes recibidos, y la
 *          integración con módulos opcionales (OLED, QR, archivos, DB).
 *
 * @note    Esta implementación es diferente de radio/radio.cpp (driver simplificado).
 *          Usa el driver completo MRF24J40::Mrf24j con direcciones de 64 bits.
 *
 * @namespace MRF24J40
 */

#include <mrf24/radio.hpp>
#include <mrf24/mrf24j40.hpp>
#include <mrf24/mrf24j40_template.tpp>
#include <file/file.hpp>
#include <display/color.hpp>
#include <work/rfflush.hpp>

#ifdef USE_OLED
    #include <oled/oled.hpp>
#endif

#include <string_view>
#include <zlib.h>
#include <string>
#include <cstdint>
#include <cstddef>

namespace MRF24J40 {

/// @brief Instancia global del driver Mrf24j
extern std::unique_ptr<Mrf24j> zigbee;

/// @brief Buffer global de recepción
extern DATA::PACKET_RX buffer_receiver;

// ============================================================================
// Constructor
// ============================================================================

Radio_t::Radio_t()
    #ifdef ENABLE_INTERRUPT_MRF24
    : status(true)
      #ifdef USE_FS
      , fs{std::make_unique<FILESYSTEM::File_t>()}
      #endif
      #ifdef ENABLE_DATABASE
      , database{std::make_unique<DATABASE::Database_t>()}
      #endif
    #else
    : status(false)
      #ifdef USE_QR
      , qr{std::make_unique<QR::Qr_t>()}
      #endif
    #endif
    , gpio{std::make_unique<GPIO::Gpio_t>(status)}
{
    /**
     * @brief Inicializa la radio, configura red y módulos opcionales
     *
     * Crea Mrf24j, inicializa el módulo, configura PAN ID, dirección
     * (16 o 64 bits), modo promiscuo, PA/LNA y buffer PHY.
     * En modo TX, genera código QR si está habilitado.
     */
    #ifdef ENABLE_INTERRUPT_MRF24
        // Modo RX con interrupción
    #else
        #ifdef USR_QR
            qr->create(QR_CODE_SRT);
        #endif
    #endif

    #ifdef DBG_RADIO
        std::cout << "Size msj : ( " << std::dec << sizeof(MSJ) << " )\n";
    #endif

    zigbee = std::make_unique<Mrf24j>();
    zigbee->init();
    zigbee->set_pan(PAN_ID);

    #ifdef MACADDR16
        zigbee->address16_write(ADDRESS);
    #elif defined(MACADDR64)
        zigbee->address64_write(ADDRESS_LONG);
    #endif

    zigbee->settings_mrf();
    zigbee->set_palna(true);
    zigbee->set_bufferPHY(true);

    flag = true;
}

// ============================================================================
// Bucle principal de procesamiento
// ============================================================================

void Radio_t::RunProccess(void)
{
    /**
     * @brief Ejecuta el bucle principal de procesamiento de la radio
     *
     * Limpia la terminal y ejecuta un bucle infinito (en modo RX) que:
     * 1. Procesa GPIO (app)
     * 2. Maneja interrupciones del MRF24J40
     * 3. Verifica flags de RX/TX
     */
    system("clear");

    #ifdef MRF24_RECEIVER_ENABLE
        while (true)
    #endif
    {
        gpio->app(flag);
        interrupt_routine();
        verif(flag);
    }
}

// ============================================================================
// Verificación de flags y transmisión periódica
// ============================================================================

void Radio_t::verif(bool& flag)
{
    /**
     * @brief Verifica flags de interrupción y envía paquetes periódicamente
     *
     * Usa check_flags() para detectar RX/TX. En modo transmisor,
     * envía paquetes cada tx_interval (1000 ms) con el mensaje
     * generado por getVectorZigbee().
     *
     * @param flag Flag de datos recibidos
     */
    flag = zigbee->check_flags(&handle_rx, &handle_tx);

    const unsigned long current_time = 100000;
    if (current_time - last_time > tx_interval) {
        last_time = current_time;

        #ifdef MRF24_TRANSMITER_ENABLE
            #ifdef DBG_RADIO
                #ifdef MACADDR64
                    std::cout << "send msj 64() ... \n";
                #else
                    std::cout << "send msj 16() ... \n";
                #endif
            #endif

            #ifdef MACADDR64
                zigbee->send64(ADDRESS_LONG_SLAVE, getVectorZigbee());
            #elif defined(MACADDR16)
                zigbee->send(ADDR_SLAVE, getVectorZigbee());
            #endif
        #endif
    }
}

// ============================================================================
// Manejador de interrupciones
// ============================================================================

void Radio_t::interrupt_routine()
{
    /** @brief Procesa interrupciones pendientes del driver Mrf24j */
    zigbee->interrupt_handler();
}

// ============================================================================
// Callback de actualización post-recepción
// ============================================================================

void update(std::string_view str_view)
{
    /**
     * @brief Procesa datos recibidos: guarda archivo, muestra OLED, genera QR
     *
     * @param str_view Datos del paquete recibido
     *
     * Avanza positionAdvance (15) bytes en los datos para omitir header,
     * redimensiona a 38 bytes, y:
     * - Si USE_OLED: muestra en display OLED
     * - Si USE_QR: genera código QR
     * - Guarda en archivo via filesystem
     */
    const int positionAdvance{15};
    auto fs{std::make_unique<FILESYSTEM::File_t>()};

    #ifdef USE_QR
        auto qr_img{std::make_unique<QR::Qr_img_t>()};
    #endif

    auto monitor{std::make_unique<FFLUSH::Fflush_t>()};

    #ifdef USE_OLED
        static auto oled{std::make_unique<OLED::Oled_t>()};
    #endif

    const auto* packet_data = reinterpret_cast<const char*>(str_view.data());
    std::string packetData(packet_data += positionAdvance);
    packetData.resize(38);

    SET_COLOR(SET_COLOR_GRAY_TEXT);

    #ifdef USE_OLED
        oled->create(packetData.c_str());
    #endif

    #ifdef USE_QR
        auto qr = std::make_unique<QR::QrOled_t>();
        std::string packet_data2 = "1234567890";
        std::vector<int> infoQrTmp;
        qr->create_qr(packet_data2, infoQrTmp);
        monitor->insert(std::to_string(infoQrTmp.size()));
        std::cout << " Size info of Qr Buffer : " << infoQrTmp.size() << std::endl;
    #endif

    fs->create(packet_data);
    std::cout << "\n";

    #ifdef USE_QR
        qr_img->create(packet_data);
    #endif
}

// ============================================================================
// Destructor
// ============================================================================

Radio_t::~Radio_t()
{
    #ifdef DBG_RADIO
        std::cout << "~Radio_t()\n";
    #endif
}

} // namespace MRF24J40
