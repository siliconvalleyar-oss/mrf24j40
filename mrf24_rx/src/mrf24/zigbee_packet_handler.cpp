/**
 * @file    zigbee_packet_handler.cpp
 * @brief   Construcción y manejo de paquetes ZigBee
 * @details Proporciona funciones auxiliares para la construcción de
 *          paquetes de datos ZigBee, incluyendo cálculo de CRC8,
 *          impresión formateada de buffers y generación del vector
 *          de datos para transmisión a través del MRF24J40.
 *
 * @namespace MRF24J40
 */

#include <mrf24/radio.hpp>
#include <work/data_analisis.hpp>
#include <mrf24/mrf24j40.hpp>
#include <config/config.hpp>
#include <iostream>
#include <vector>
#include <cstring>
#include <iomanip>
#include <algorithm>

namespace MRF24J40 {

/// @brief Instancia global del driver Mrf24j (declarada extern)
extern std::unique_ptr<Mrf24j> zigbee;

// ============================================================================
// Impresión hexadecimal de direcciones MAC
// ============================================================================

/**
 * @brief Imprime un buffer en formato hexadecimal little-endian
 * @param data Puntero a los datos
 * @param size Cantidad de bytes a imprimir
 */
void print_hex(const uint8_t* data, size_t size)
{
    std::cout << "0x";
    for (int i = size - 1; i > -1; i--) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
    }
    std::cout << std::dec << std::endl;
}

// ============================================================================
// Cálculo de CRC8
// ============================================================================

/**
 * @brief Calcula el CRC8 de un buffer de datos
 * @param data Puntero al buffer
 * @param size Longitud del buffer
 * @return CRC8 calculado
 *
 * Implementación de CRC-8 con polinomio 0x07 (estándar Dallas/Maxim).
 * Valor inicial: 0xFF.
 */
const uint8_t calculate_crc8(const uint8_t* data, size_t size)
{
    uint8_t crc = 0xFF;
    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; ++j) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x07;   /**< Polinomio CRC-8: x^8 + x^2 + x + 1 */
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

// ============================================================================
// Impresión de buffers
// ============================================================================

/**
 * @brief Imprime un vector de bytes en formato hexadecimal
 * @param buffer Vector con los datos a imprimir
 */
void print_buffer(const std::vector<uint8_t>& buffer)
{
    std::cout << "\nBuffer Send (in Hex): ";
    for (const auto& byte : buffer) {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
    }
    std::cout << std::dec << "\n";
}

/**
 * @brief Imprime los detalles de un paquete estructurado
 * @param packet Estructura packet_tx con head, size, data, crc8
 * @param data   Vector de datos enviados
 */
void print_packet_details(const DATA::packet_tx& packet, const std::vector<uint8_t>& data)
{
    std::cout << "\n---- Packet Details ----\n";
    std::cout << "Head: 0x" << std::hex << std::setw(2) << std::setfill('0') << (int)packet.head << std::dec << std::endl;
    std::cout << "Size: " << packet.size << " bytes" << std::endl;
    std::cout << "CRC8: 0x" << std::hex << std::setw(2) << std::setfill('0') << (int)packet.crc8 << std::dec << std::endl;
    std::cout << "Data size: " << data.size() << " bytes" << std::endl;
    std::string txt(data.begin(), data.end());
    std::cout << "Data: " << txt.c_str() << "\n------------------------\n";
}

// ============================================================================
// Generación del vector de transmisión ZigBee
// ============================================================================

/**
 * @brief Genera el vector de datos para transmisión ZigBee
 *
 * Construye un paquete DATA::packet_tx con:
 * - HEAD: byte de identificación (0x40)
 * - size: longitud total (payload + head + crc8)
 * - data[:MAX_PACKET_TX]: mensaje de prueba de hasta 64 caracteres
 * - crc8: checksum calculado sobre el payload
 *
 * @return Vector de bytes con la estructura packet_tx serializada
 */
const std::vector<uint8_t> Radio_t::getVectorZigbee()
{
    const std::string msj_to_zb =
        "ABCDEFGHIJ0123456789@KLMNOPQRSTUVWXYZ0123456789"
        "abcdefghijklmnopqrstuvwxyz0123456ABCDEFGHIJKLMNOPQRST"
        "@VWXYZ0123@ABCDEFGHIJKLMNOPQ";

    const std::string msj_to_zb_short = msj_to_zb.substr(0, MAX_PACKET_TX);
    auto crc8 = calculate_crc8(
        reinterpret_cast<const uint8_t*>(msj_to_zb_short.c_str()),
        msj_to_zb_short.size()
    );

    std::vector<uint8_t> buffer_zb(msj_to_zb_short.begin(), msj_to_zb_short.end());
    uint16_t max = buffer_zb.size() + sizeof(HEAD) + sizeof(crc8);

    DATA::packet_tx bufferTransReceiver{HEAD, max, {}, crc8};
    std::memcpy(bufferTransReceiver.data, buffer_zb.data(), buffer_zb.size());

    print_packet_details(bufferTransReceiver, buffer_zb);

    std::vector<uint8_t> vect(sizeof(bufferTransReceiver));
    std::memcpy(vect.data(), &bufferTransReceiver, sizeof(bufferTransReceiver));

    print_buffer(vect);

    uint64_t mac_address;
    zigbee->mrf24j40_get_extended_mac_addr(&mac_address);

    std::cout << "Local MAC address: ";
    print_hex(reinterpret_cast<uint8_t*>(&mac_address), sizeof(mac_address));

    return vect;
}

} // namespace MRF24J40
