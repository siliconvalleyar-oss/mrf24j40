/**
 * @file    mrf24j40_template.cpp
 * @brief   Implementación template de envío para Mrf24j_t
 * @details Proporciona una función template genérica send_template()
 *          para el driver Mrf24j_t. Actualmente el cuerpo de la función
 *          está comentado a la espera de ser habilitado.
 *          Incluye declaraciones extern de variables globales del módulo.
 *
 * @namespace MRF24J40
 */

#include <mrf24/mrf24j40.hpp>
#include <mrf24/mrf24j40_cmd.hpp>
#include <mrf24/mrf24j40_settings.hpp>
#include <config/config.hpp>

#include <iostream>

namespace MRF24J40 {

    /**
     * @name Variables globales externas del módulo MRF24J40
     * @{
     */
    extern uint8_t rx_;             /**< Buffer RX temporal */
    extern int ignoreBytes;         /**< Bytes a ignorar (comportamiento de algunos módulos) */
    extern bool bufPHY;             /**< Flag de bufferización PHY */
    extern rx_info_t rx_info;       /**< Información del último paquete RX */
    extern tx_info_t tx_info;       /**< Información de la última TX */
    extern RXMCR rxmcr;             /**< Configuración del registro RXMCR */
    /** @} */

} // namespace MRF24J40

namespace MRF24J40 {

    /**
     * @brief Template de envío genérico para Mrf24j_t
     *
     * Función template que construye y envía una trama IEEE 802.15.4
     * a una dirección de 64 bits. Actualmente el cuerpo de la función
     * está comentado (bloque con inicio/fin de comentario C).
     *
     * @tparam T Tipo de datos a enviar (deducido automáticamente)
     * @param dest64 Dirección MAC destino de 64 bits
     * @param data   Datos a transmitir
     *
     * @note El código dentro del bloque comentado implementa la construcción
     *       completa de la trama: header length, frame length, FCF, PAN ID,
     *       direcciones destino/origen (64 bits), payload, y disparo TX.
     *
     * @todo Descomentar y probar esta función cuando se requiera el uso
     *       del driver Mrf24j_t con tipos genéricos.
     */
    template <typename T>
    void Mrf24j_t::send_template(const uint64_t dest64, const T& data)
    {
        /* Código comentado — ver archivo fuente para implementación completa */
    }

} // namespace MRF24J40
