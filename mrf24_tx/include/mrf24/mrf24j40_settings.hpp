#pragma once

/**
 * @file    mrf24j40_settings.hpp
 * @brief   Estructura de configuración del registro RXMCR del MRF24J40MA
 * @details Define la estructura de bits del registro Receive MAC Control (RXMCR)
 *          para configurar modo promiscuo, coordinador, PAN coordinator, etc.
 *
 * @namespace MRF24J40
 * @{
 */

#include <iostream>

namespace MRF24J40 {

    /**
     * @brief Estructura de bits del registro RXMCR (0x00)
     *
     * El registro Receive MAC Control (RXMCR) configura el comportamiento
     * del receptor MAC del MRF24J40, incluyendo filtrado de direcciones,
     * modo promiscuo y roles de red (coordinador, PAN coordinator).
     *
     * Los campos se mapean directamente a los bits del registro de 8 bits.
     */
    typedef struct rxmcr {
        /** @brief Modo promiscuo
         *         1 = Recibe todos los paquetes con CRC válido
         *         0 = Descarta paquetes con mismatch de dirección MAC (default) */
        uint8_t PROMI       :1;

        /** @brief Aceptar paquetes con error CRC
         *         1 = Recibe paquetes aunque tengan CRC inválido
         *         0 = Descarta paquetes con CRC inválido (default) */
        uint8_t ERRPKT      :1;

        /** @brief Modo coordinador
         *         1 = Dispositivo actúa como coordinador
         *         0 = No es coordinador (default) */
        uint8_t COORD       :1;

        /** @brief Modo PAN coordinator
         *         1 = Dispositivo actúa como PAN coordinator
         *         0 = No es PAN coordinator (default) */
        uint8_t PANCOORD    :1;

        /** @brief Reservado */
        uint8_t Reserved_0  :1;

        /** @brief No responder con ACK automático
         *         1 = Deshabilita respuesta ACK automática
         *         0 = ACK automático habilitado (default) */
        uint8_t NOACKRSP    :1;

        /** @brief Reservado */
        uint8_t Reserved_1  :2;
    } RXMCR;

}  // namespace MRF24J40

/** @} */  // end of MRF24J40 namespace
