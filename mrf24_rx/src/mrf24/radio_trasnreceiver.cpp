/**
 * @file    radio_trasnreceiver.cpp
 * @brief   Handlers de recepción y transmisión del driver completo
 * @details Implementa los callbacks handle_tx() y handle_rx() para el driver
 *          Mrf24j. El handler RX procesa el frame recibido, extrae direcciones
 *          MAC (64 bits), payload, LQI y RSSI, y muestra la información
 *          formateada. Soporta dos modos de direccionamiento (USE_MAC_ADDRESS_LONG
 *          y USE_MAC_ADDRESS_SHORT).
 *
 * @namespace MRF24J40
 */

#include <mrf24/radio.hpp>
#include <mrf24/mrf24j40.hpp>
#include <mrf24/mrf24j40_template.tpp>
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
std::unique_ptr<Mrf24j> zigbee = nullptr;

/// @brief Buffer de recepción de paquetes estructurados
DATA::PACKET_RX buffer_receiver{};

/// @brief Buffer raw de recepción PHY (declarado extern en mrf24j40.cpp)
extern uint8_t rx_buf[A_MAX_PHY_PACKET_SIZE];

// ============================================================================
// Handler de TX completada
// ============================================================================

void handle_tx()
{
    /**
     * @brief Callback de transmisión completada
     *
     * Muestra si la transmisión fue exitosa (ACK recibido) o falló,
     * indicando el número de retransmisiones realizadas.
     */
    #ifdef MRF24_TRANSMITER_ENABLE
        const auto status = zigbee->get_txinfo()->tx_ok;
        if (status) {
            std::cout << "TX went ok, got ack \n";
        } else {
            std::cout << "\nTX failed after \n";
            std::cout << zigbee->get_txinfo()->retries;
            std::cout << " retries\n";
        }
    #endif
}

// ============================================================================
// Handler de RX recibido (con direcciones MAC de 64 bits)
// ============================================================================

#ifdef USE_MAC_ADDRESS_LONG

void handle_rx()
{
    /**
     * @brief Procesa un paquete recibido (direcciones MAC de 64 bits)
     *
     * Lee frame_length del RX FIFO, extrae payload, LQI, RSSI y
     * direcciones MAC (origen y destino). Compara la MAC recibida
     * con la MAC local configurada y muestra el resultado.
     * Usa FFLUSH::Fflush_t para salida formateada con timestamp.
     */
    #ifdef MRF24_RECEIVER_ENABLE
        auto monitor{std::make_unique<FFLUSH::Fflush_t>()};
        std::ostringstream oss_zigbee{};

        monitor->insert("received a packet ... ");

        const uint8_t frame_length = zigbee->get_rxinfo()->frame_length;
        oss_zigbee << "0x" << std::hex << std::setw(2) << std::setfill('0')
                   << static_cast<int>(frame_length);
        monitor->insert(oss_zigbee.str());
        oss_zigbee.str("");
        oss_zigbee.clear();

        monitor->insert(" Packet data (PHY Payload) :" + std::to_string(frame_length));
        monitor->insert(" ");

        if (zigbee->get_bufferPHY()) {
            #ifdef DBG_PRINT_GET_INFO
                for (uint8_t i = 0; i < frame_length; ++i)
                    oss_zigbee << zigbee->get_rxbuf()[i];
                monitor->insert(oss_zigbee.str());
                oss_zigbee.str("");
                oss_zigbee.clear();
            #endif
        }

        SET_COLOR(SET_COLOR_CYAN_TEXT);
        monitor->insert("ASCII data (relevant data) :");
        monitor->insert("data_length : " + std::to_string(zigbee->rx_datalength()));

        for (auto& byte : zigbee->get_rxinfo()->rx_data) {
            if (byte != 0x00)
                oss_zigbee << std::hex << std::setw(2) << std::setfill('0')
                           << static_cast<int>(byte) << ":";
        }
        monitor->insert("info_zigbee : ");
        monitor->insert(oss_zigbee.str());
        oss_zigbee.str("");
        oss_zigbee.clear();

        for (auto& byte : zigbee->get_rxinfo()->rx_data)
            oss_zigbee << byte;

        monitor->insert(" ");
        monitor->insert(" ");

        #ifdef DBG_PRINT_GET_INFO
            std::memcpy(&buffer_receiver, zigbee->get_rxbuf(),
                        zigbee->rx_datalength() + BYTES_MHR);

            const uint64_t mac_address_rx =
                (static_cast<uint64_t>(buffer_receiver.mac_msb_rx) << 32)
                | buffer_receiver.mac_lsb_rx;
            const uint64_t mac_address_tx =
                (static_cast<uint64_t>(buffer_receiver.mac_msb) << 32)
                | buffer_receiver.mac_lsb;

            monitor->insert(" ");

            uint64_t mac_address_local;
            zigbee->mrf24j40_get_extended_mac_addr(&mac_address_local);

            if (mac_address_local == mac_address_rx)
                monitor->insert("mac recibida es aceptada");
            else
                monitor->insert("mac recibida no es aceptada");

            monitor->insert("rx data_receiver->mac : 0x" + hex_to_text(mac_address_rx));
            monitor->insert("tx data_receiver->mac : 0x" + hex_to_text(mac_address_tx));
            monitor->insert("buffer_receiver->head : 0x" + hex_to_text(buffer_receiver.head));
            monitor->insert("buffer_receiver->size : " + std::to_string(buffer_receiver.size));
            monitor->insert("buffer_receiver->panid : 0x" + hex_to_text(buffer_receiver.panid));
            monitor->insert("buffer_receiver->checksum : 0x" + hex_to_text(buffer_receiver.crc8));

            std::string txt_tmp;
            txt_tmp.assign(reinterpret_cast<const char*>(buffer_receiver.data),
                           sizeof(buffer_receiver.data));
            monitor->insert("data_receiver->data ( "
                            + std::to_string(sizeof(buffer_receiver.data))
                            + " ) : " + txt_tmp);

            monitor->insert("get address mac : 0x" + hex_to_text(mac_address_local));
        #endif

        RST_COLOR();
        SET_COLOR(SET_COLOR_CYAN_TEXT);
        monitor->insert("LQI : " + std::to_string(zigbee->get_rxinfo()->lqi));
        monitor->insert("RSSI : " + std::to_string(zigbee->get_rxinfo()->rssi));

        monitor->print_all();
    #endif

    RST_COLOR();
    update(reinterpret_cast<const char*>(zigbee->get_rxinfo()->rx_data));
}

// ============================================================================
// Handler de RX recibido (con direcciones MAC de 16 bits)
// ============================================================================

#else

void handle_rx()
{
    /**
     * @brief Procesa un paquete recibido (direcciones MAC de 16 bits)
     *
     * Similar a la versión de 64 bits pero usando ADDRESS_LONG_SLAVE
     * para la comparación de direcciones.
     */
    #ifdef MRF24_RECEIVER_ENABLE
        auto monitor{std::make_unique<FFLUSH::Fflush_t>()};
        std::ostringstream oss_zigbee{};

        monitor->insert("received a packet ... ");

        const uint8_t frame_length = zigbee->get_rxinfo()->frame_length;
        oss_zigbee << "0x" << std::hex << std::setw(2) << std::setfill('0')
                   << static_cast<int>(frame_length);
        monitor->insert(oss_zigbee.str());
        oss_zigbee.str("");
        oss_zigbee.clear();
        monitor->insert(" ");

        monitor->insert(" Packet data (PHY Payload) :" + std::to_string(frame_length));
        monitor->insert(" ");

        if (zigbee->get_bufferPHY()) {
            #ifdef DBG_PRINT_GET_INFO
                for (uint8_t i = 0; i < frame_length; ++i)
                    oss_zigbee << zigbee->get_rxbuf()[i];
                monitor->insert(oss_zigbee.str());
                oss_zigbee.str("");
                oss_zigbee.clear();
            #endif
        }

        SET_COLOR(SET_COLOR_CYAN_TEXT);
        monitor->insert("ASCII data (relevant data) :");
        monitor->insert("data_length : " + std::to_string(zigbee->rx_datalength()));

        for (auto& byte : zigbee->get_rxinfo()->rx_data) {
            if (byte != 0x00)
                oss_zigbee << std::hex << std::setw(2) << std::setfill('0')
                           << static_cast<int>(byte) << ":";
        }
        monitor->insert("info_zigbee : ");
        monitor->insert(oss_zigbee.str());
        oss_zigbee.str("");
        oss_zigbee.clear();

        for (auto& byte : zigbee->get_rxinfo()->rx_data)
            oss_zigbee << byte;

        monitor->insert(" ");
        monitor->insert("info_zigbee : ");
        monitor->insert(oss_zigbee.str());
        oss_zigbee.str("");
        oss_zigbee.clear();

        #ifdef DBG_PRINT_GET_INFO
            std::memcpy(&buffer_receiver, zigbee->get_rxbuf(), sizeof(rx_buf));

            const uint64_t mac_address_rx =
                (static_cast<uint64_t>(buffer_receiver.mac_msb_rx) << 32)
                | buffer_receiver.mac_lsb_rx;
            monitor->insert(" ");

            if (ADDRESS_LONG_SLAVE == mac_address_rx)
                monitor->insert("mac aceptada");
            else
                monitor->insert("mac no es aceptada");

            monitor->insert("rx data_receiver->mac : " + hex_to_text(mac_address_rx));

            std::string txt_tmp;
            txt_tmp.assign(reinterpret_cast<const char*>(buffer_receiver.data),
                           sizeof(buffer_receiver.data));
            monitor->insert("data_receiver->data ( "
                            + std::to_string(sizeof(buffer_receiver.data))
                            + " ) : " + txt_tmp);

            uint64_t mac_address;
            zigbee->mrf24j40_get_extended_mac_addr(&mac_address);
            monitor->insert("get address mac: " + hex_to_text(mac_address));
        #endif

        RST_COLOR();
        SET_COLOR(SET_COLOR_CYAN_TEXT);
        monitor->insert("LQI : " + std::to_string(zigbee->get_rxinfo()->lqi));
        monitor->insert("RSSI : " + std::to_string(zigbee->get_rxinfo()->rssi));
        monitor->insert("Frame Length : " + std::to_string(zigbee->get_rxinfo()->frame_length));
        monitor->insert("sizeof - buffer_receiverRX : " + std::to_string(sizeof(buffer_receiver)));
        monitor->insert("sizeof - buffer_receiverRX.data : "
                        + std::to_string(sizeof(buffer_receiver.data)));

        std::string rx_b(rx_buf, rx_buf + sizeof(rx_buf));
        monitor->insert("ZigeBee : " + rx_b);

        monitor->print_all();
    #endif

    RST_COLOR();
    update(reinterpret_cast<const char*>(zigbee->get_rxinfo()->rx_data));
}

#endif

} // namespace MRF24J40
