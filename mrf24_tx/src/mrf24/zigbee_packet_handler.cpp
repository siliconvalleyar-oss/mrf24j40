#include <mrf24/radio.hpp>
#include <work/data_analisis.hpp>
#include <mrf24/mrf24j40.hpp>
#include <config/config.hpp>
#include <iostream>
#include <vector>
#include <cstring>
#include <iomanip> // Para formatear la salida en hexadecimal
#include <algorithm> // Para std::min

namespace MRF24J40{

extern std::unique_ptr<Mrf24j> zigbee ;

    // Función auxiliar para imprimir en formato hexadecimal
    void 
    print_hex(const uint8_t* data, size_t size) {
        std::cout << "0x";
        for (int i = size-1; i > -1; i--) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
        }
        std::cout << std::dec << std::endl; // Volver al formato decimal
    }


    // Función auxiliar para calcular CRC8 (implementación de ejemplo)
    const uint8_t 
    calculate_crc8(const uint8_t* data, size_t size) {
        uint8_t crc = 0xFF;
        for (size_t i = 0; i < size; ++i) {
            crc ^= data[i];
            for (uint8_t j = 0; j < 8; ++j) {
                if (crc & 0x80) {
                    crc = (crc << 1) ^ 0x07; // Polinomio CRC-8
                } else {
                    crc <<= 1;
                }
            }
        }
        return crc;
    }


    // Función para imprimir el contenido de un buffer en formato hexadecimal
    void 
    print_buffer(const std::vector<uint8_t>& buffer) {
        std::cout << "\nBuffer Send (in Hex): ";
        for (const auto& byte : buffer) {
            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
        }
        std::cout << std::dec << "\n";
    }

    // Función para imprimir los detalles del paquete (header, size, crc, data)
    void 
    print_packet_details(const DATA::packet_tx& packet, const std::vector<uint8_t>& data) {
        std::cout << "\n---- Packet Details ----\n";
        std::cout << "Head: 0x" << std::hex << std::setw(2) << std::setfill('0') << (int)packet.head << std::dec << std::endl;
        std::cout << "Size: " << packet.size << " bytes" << std::endl;
        std::cout << "CRC8: 0x" << std::hex << std::setw(2) << std::setfill('0') << (int)packet.crc8 << std::dec << std::endl;
        std::cout << "Data size: " << data.size() << " bytes" << std::endl;
        std::cout << "Data: ";
        // Convertir directamente a std::string si los datos son texto ASCII
        std::string txt(data.begin(), data.end());  // Usamos el constructor de string para convertir el vector
        std::cout << "Data: " << txt.c_str()<<"\n" << "------------------------\n";
    }


    // Función para generar el vector de datos de Zigbee
    const std::vector<uint8_t> 
    Radio_t::getVectorZigbee() {
        // Mensaje a transmitir (ejemplo)
        //const std::string msj_to_zb = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz0123456ABCDEFGHIJKLMNOPQRST@VWXYZ0123@ABCDEFGHIJKLMNOPQ";
        const std::string msj_to_zb = "ABCDEFGHIJ0123456789@KLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz0123456ABCDEFGHIJKLMNOPQRST@VWXYZ0123@ABCDEFGHIJKLMNOPQ";
        // Acortar a los primeros 100 caracteres
        const std::string msj_to_zb_short = msj_to_zb.substr(0, MAX_PACKET_TX);

        // Calcular CRC8 para el mensaje
        auto crc8 = calculate_crc8(reinterpret_cast<const uint8_t*>(msj_to_zb_short.c_str()), msj_to_zb_short.size());

        // Copiar los caracteres del mensaje al buffer
        std::vector<uint8_t> buffer_zb(msj_to_zb_short.begin(), msj_to_zb_short.end());

        // Calcular el tamaño total del paquete (encabezado + datos + CRC)
        uint16_t max = buffer_zb.size() + sizeof(HEAD) + sizeof(crc8);

        // Crear la estructura de paquete para transmisión
        DATA::packet_tx bufferTransReceiver{ HEAD, max, { } , crc8 };

        // Copiar los datos del mensaje al buffer de transmisión
        //std::memcpy(bufferTransReceiver.data, buffer_zb.data(), std::min(buffer_zb.size(), sizeof(bufferTransReceiver.data)));
        
        std::memcpy(bufferTransReceiver.data, buffer_zb.data(), buffer_zb.size());

        // Imprimir los detalles del paquete
        print_packet_details(bufferTransReceiver, buffer_zb);

        // Crear un vector para almacenar el paquete completo
        std::vector<uint8_t> vect(sizeof(bufferTransReceiver));

        // Copiar la estructura del paquete al vector
        std::memcpy(vect.data(), &bufferTransReceiver, sizeof(bufferTransReceiver));

        // Imprimir el tamaño y el contenido del buffer enviado
        print_buffer(vect);

        // Obtener la dirección MAC extendida (Ejemplo, esta función debe existir en tu código)
        uint64_t mac_address;
        zigbee->mrf24j40_get_extended_mac_addr(&mac_address);

        std::cout << "Local MAC address: ";
        print_hex(reinterpret_cast<uint8_t*>(&mac_address), sizeof(mac_address));

        // Retornar el vector con los datos del paquete
        return vect;
    }
}//end namespace MRF24J40