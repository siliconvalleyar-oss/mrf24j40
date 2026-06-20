#pragma once

#include <mrf24/mrf24j40.hpp>
#include <config/config.hpp>
#include <mrf24/mrf24j40_cmd.hpp>
#include <mrf24/mrf24j40_settings.hpp>
#include <iostream>

namespace MRF24J40{
            // aMaxPHYPacketSize = 127, from the 802.15.4-2006 standard.
    extern uint8_t rx_;
    extern int ignoreBytes ; // bytes to ignore, some modules behaviour.
    extern bool bufPHY ; // flag to buffer all bytes in PHY Payload, or not
    extern rx_info_t rx_info;
    extern tx_info_t tx_info;
    extern RXMCR rxmcr;
}


namespace MRF24J40
{

    //@brief 
    //@params
    //@params        
    template<typename T>
    void print_to_hex(const T int_to_hex) {
    // El tamaño en bytes del tipo de dato se multiplica por 2 para obtener el número de dígitos en hexadecimal.
    std::cout << std::hex 
              << std::setw(sizeof(T) * 2)  // Ancho basado en el tamaño del tipo de dato.
              << std::setfill('0')         // Rellena con ceros si es necesario.
              << +int_to_hex               // El símbolo '+' asegura que el tipo char se trate como número.
              << "\n";
    }

    template<typename T>
    std::string hex_to_text(const T int_to_hex) {
        // Crear un flujo de salida para construir la cadena hexadecimal.
        std::ostringstream oss;

        // El tamaño en bytes del tipo de dato se multiplica por 2 para obtener el número de dígitos en hexadecimal.
        oss << std::hex 
            << std::setw(sizeof(T) * 2)   // Ancho basado en el tamaño del tipo de dato.
            << std::setfill('0')          // Rellena con ceros si es necesario.
            << +int_to_hex;               // El símbolo '+' asegura que el tipo char se trate como número.

        // Devolver la cadena construida.
        return oss.str();
    }



    template<typename T>
    void to_hex(const T int_to_hex) {
    // El tamaño en bytes del tipo de dato se multiplica por 2 para obtener el número de dígitos en hexadecimal.
    std::cout << std::hex 
              << std::setw(sizeof(T) * 2)  // Ancho basado en el tamaño del tipo de dato.
              << std::setfill('0')         // Rellena con ceros si es necesario.
              << +int_to_hex    ;           // El símbolo '+' asegura que el tipo char se trate como número.
              //<< "\n";
    }

}