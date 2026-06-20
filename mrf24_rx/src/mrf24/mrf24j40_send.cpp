/**
 * @file    mrf24j40_send.cpp
 * @brief   Implementación de los métodos de envío del driver MRF24J40
 * @details Proporciona las funciones send() y send64() para transmitir
 *          tramas IEEE 802.15.4 a través del TX Normal FIFO del MRF24J40.
 *          Soporta direcciones de 64 bits con y sin estructura packet_tx.
 *
 * @namespace MRF24J40
 */

#include <mrf24/mrf24j40_cmd.hpp>
#include <mrf24/mrf24j40_settings.hpp>
#include <mrf24/mrf24j40.hpp>
#include <tyme/tyme.hpp>
#include <config/config.hpp>
#include <work/data_analisis.hpp>
#include <spi/spi.hpp>

namespace MRF24J40 {

/** @brief Referencia externa a la variable ignoreBytes */
extern size_t ignoreBytes;

// ============================================================================
// send() — Envío genérico con vector de bytes
// ============================================================================

void Mrf24j::send(const uint64_t mac_address_dest, const std::vector<uint8_t> vect)
{
    /**
     * @brief Envía datos a una dirección de 64 bits
     *
     * Construye una trama IEEE 802.15.4 en el TX Normal FIFO con:
     * - MAC Header (MHR): FCF(2) + Seq(1) + PAN(2) + Dst(8) + Src(8)
     * - Payload: datos del vector
     * Solicita ACK (TXNACKREQ) y dispara transmisión (TXNTRIG).
     *
     * @param mac_address_dest Dirección MAC destino de 64 bits
     * @param vect             Vector con los datos a transmitir
     */
    const auto size = vect.size();
    int incr = 0;

    // Header length
    write_long(++incr, m_bytes_MHR);
    // Frame length (header + ignoreBytes + payload)
    write_long(++incr, m_bytes_MHR + ignoreBytes + size);

    // Frame Control Field: data frame, ACK request, PAN compression
    write_long(++incr, 0b01100001);
    // Frame Control Field: 16-bit src/dst, 802.15.4-2003
    write_long(++incr, 0b10001000);
    // Sequence number
    write_long(++incr, 1);

    // Dest PAN ID
    const uint16_t panid = get_pan();
    write_long(++incr, panid & 0xff);
    write_long(++incr, panid >> 8);

    // Dest and source MAC addresses
    set_macaddress(incr, mac_address_dest);
    set_macaddress(incr, address64_read());

    // Ignore bytes (comportamiento de algunos módulos)
    incr += ignoreBytes;

    // Payload
    for (const auto& byte : vect)
        write_long(++incr, byte);

    // Disparar transmisión con ACK
    write_short(MRF_TXNCON, (1 << MRF_TXNACKREQ | 1 << MRF_TXNTRIG));
    mode_turbo();
}

// ============================================================================
// send64() — Envío con estructura packet_tx
// ============================================================================

void Mrf24j::send64(const uint64_t dest64, const DATA::packet_tx packet_tx)
{
    /**
     * @brief Envía un paquete estructurado packet_tx a dirección de 64 bits
     *
     * Similar a send() pero el payload se toma de una estructura
     * DATA::packet_tx que contiene head, size, data[] y checksum.
     *
     * @param dest64    Dirección MAC destino de 64 bits
     * @param packet_tx Estructura con los datos del paquete
     */
    const size_t len = sizeof(packet_tx);
    int i = 0;

    write_long(i++, m_bytes_MHR);
    write_long(i++, m_bytes_MHR + ignoreBytes + len);

    write_long(i++, 0b01100001);
    write_long(i++, 0b10001000);
    write_long(i++, 1);

    const uint16_t panid = get_pan();
    write_long(i++, panid >> 8);
    write_long(i++, panid & 0xff);

    set_macaddress(i, dest64);
    set_macaddress(i, address64_read());

    #include <mrf24/mrf24j40._microchip.hpp>
    write_long(RFCTRL2, 0x80);

    i += ignoreBytes;

    // Copiar estructura packet_tx a vector y escribir en FIFO
    std::vector<uint8_t> vect(sizeof(packet_tx));
    std::memcpy(vect.data(), &packet_tx, sizeof(packet_tx));

    for (const auto& byte : vect)
        write_long(i++, byte);

    write_short(MRF_TXNCON, (1 << MRF_TXNACKREQ | 1 << MRF_TXNTRIG));
    mode_turbo();
}

// ============================================================================
// send64() — Envío con vector (sobrecarga)
// ============================================================================

void Mrf24j::send64(const uint64_t dest64, const std::vector<uint8_t> vect)
{
    /**
     * @brief Envía un vector de datos a dirección de 64 bits
     *
     * Sobrecarga de send64() que toma directamente un vector<uint8_t>
     * como payload, sin estructura intermedia.
     *
     * @param dest64 Dirección MAC destino de 64 bits
     * @param vect   Vector con los datos a transmitir
     */
    const auto len = vect.size();
    int i = 0;

    write_long(i++, m_bytes_MHR);
    write_long(i++, m_bytes_MHR + ignoreBytes + len);

    write_long(i++, 0b01100001);
    write_long(i++, 0b10001000);
    write_long(i++, 1);

    const uint16_t panid = get_pan();
    write_long(i++, panid & 0xff);
    write_long(i++, panid >> 8);

    set_macaddress(i, dest64);
    set_macaddress(i, address64_read());

    #include <mrf24/mrf24j40._microchip.hpp>
    write_long(RFCTRL2, 0x80);

    i += ignoreBytes;

    for (const auto& byte : vect)
        write_long(i++, byte);

    write_short(MRF_TXNCON, (1 << MRF_TXNACKREQ | 1 << MRF_TXNTRIG));
    mode_turbo();
}

} // namespace MRF24J40
