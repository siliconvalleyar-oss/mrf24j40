/**
 * @file    mrf24j40.cpp
 * @brief   Implementación principal del driver completo MRF24J40
 * @details Driver completo del transceiver MRF24J40MA en namespace MRF24J40.
 *          Proporciona inicialización, lectura/escritura de registros,
 *          manejo de interrupciones por polling, y control de la interfaz
 *          RF (PA/LNA, turbo mode, potencia TX).
 *
 * @note    Este driver usa SPI::Spi_t para la comunicación de bajo nivel,
 *          mientras que el driver simplificado (Mrf24j40) usa ioctl directo.
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

// ============================================================================
// Variables globales del módulo
// ============================================================================

/** @brief Buffer raw de recepción PHY (tamaño máximo: 127 bytes) */
uint8_t rx_buf[A_MAX_PHY_PACKET_SIZE];

/** @brief Bytes a ignorar en la trama (comportamiento de algunos módulos) */
size_t ignoreBytes{0};

/** @brief Flag para bufferizar todos los bytes del payload PHY */
bool bufPHY{false};

/** @brief Estructura con información del último paquete RX */
rx_info_t rx_info{};

/** @brief Estructura con información de la última transmisión TX */
tx_info_t tx_info{};

/** @brief Configuración del registro RXMCR (modo promiscuo, coordinator, etc.) */
RXMCR rxmcr{0x00};

// ============================================================================
// Constructor
// ============================================================================

Mrf24j::Mrf24j()
    : prt_spi{std::make_unique<SPI::Spi_t>()},
      m_bytes_nodata{m_bytes_MHR + m_bytes_FCS}
{
    /**
     * @brief Inicializa el driver creando la instancia SPI
     * y calculando los bytes sin datos (MHR + FCS).
     */
    #ifdef DBG
        std::cout << "Mrf24j( )\r\n";
    #endif
}

// ============================================================================
// Operaciones de registro (short address: 8 bits)
// ============================================================================

const uint8_t Mrf24j::read_short(const uint8_t address)
{
    /**
     * @brief Lee un registro de dirección corta
     * @param address Dirección del registro (0x00-0x3F)
     * @return Valor de 8 bits del registro
     *
     * Codifica la dirección en formato SPI short address:
     * bit 7 = 0 (short), bit 0 = 0 (read), bits 6-1 = address
     */
    const uint8_t tmp = (address << 1 & 0b01111110);
    return prt_spi->Transfer2bytes(tmp);
}

void Mrf24j::write_short(const uint8_t address, const uint8_t data)
{
    /**
     * @brief Escribe un registro de dirección corta
     * @param address Dirección del registro (0x00-0x3F)
     * @param data    Valor a escribir
     *
     * Codifica: bit 7 = 0 (short), bit 0 = 1 (write), bits 6-1 = address
     * Los datos se colocan en el byte alto de la transferencia de 16 bits.
     */
    const uint16_t lsb_tmp = ((address << 1 & 0b01111110) | 0x01) | (data << 8);
    prt_spi->Transfer2bytes(lsb_tmp);
}

// ============================================================================
// Operaciones de registro (long address: 16 bits)
// ============================================================================

const uint8_t Mrf24j::read_long(const uint16_t address)
{
    /**
     * @brief Lee un registro de dirección larga (0x200-0x3FF)
     * @param address Dirección de 16 bits
     * @return Valor de 8 bits del registro
     *
     * Codifica la dirección larga en formato SPI de 3 bytes:
     * Byte 0: [1][A10:A4], Byte 1: [A3:A0][0][0][0][R/W=0]
     */
    const uint8_t lsb_address = (address >> 3) & 0x7F;
    const uint8_t msb_address = (address << 5) & 0xE0;
    const uint32_t cmd = ((0x80 | lsb_address) | (msb_address << 8)) & 0x0000ffff;
    return prt_spi->Transfer3bytes(cmd);
}

void Mrf24j::write_long(const uint16_t address, const uint8_t data)
{
    /**
     * @brief Escribe un registro de dirección larga
     * @param address Dirección de 16 bits
     * @param data    Valor a escribir
     *
     * Similar a read_long pero con R/W=1 en el byte 1.
     */
    const uint8_t lsb_address = (address >> 3) & 0x7F;
    const uint8_t msb_address = (address << 5) & 0xE0;
    const uint32_t cmd = ((0x80 | lsb_address) | ((msb_address | 0x10) << 8) | (data << 16)) & 0xffffff;
    prt_spi->Transfer3bytes(cmd);
}

// ============================================================================
// Gestión de PAN ID
// ============================================================================

const uint16_t Mrf24j::get_pan(void)
{
    /** @return PAN ID actual de 16 bits (PANIDH:PANIDL) */
    const uint8_t panh = read_short(MRF_PANIDH);
    return (panh << 8 | read_short(MRF_PANIDL));
}

void Mrf24j::set_pan(const uint16_t panid)
{
    /** @brief Configura el PAN ID de la red */
    write_short(MRF_PANIDH, (panid >> 8) & 0xff);
    write_short(MRF_PANIDL, panid & 0xff);
}

// ============================================================================
// Gestión de direcciones
// ============================================================================

void Mrf24j::address16_write(const uint16_t address)
{
    /** @brief Escribe la dirección corta de 16 bits (SADRH:SADRL) */
    write_short(MRF_SADRH, (address >> 8) & 0xff);
    write_short(MRF_SADRL, address & 0xff);
}

void Mrf24j::address64_write(const uint64_t address)
{
    /** @brief Escribe la dirección extendida de 64 bits (EADR7:EADR0) */
    write_short(MRF_EADR7, (address >> 56) & 0xff);
    write_short(MRF_EADR6, (address >> 48) & 0xff);
    write_short(MRF_EADR5, (address >> 40) & 0xff);
    write_short(MRF_EADR4, (address >> 32) & 0xff);
    write_short(MRF_EADR3, (address >> 24) & 0xff);
    write_short(MRF_EADR2, (address >> 16) & 0xff);
    write_short(MRF_EADR1, (address >> 8) & 0xff);
    write_short(MRF_EADR0, (address) & 0xff);
}

const uint16_t Mrf24j::address16_read(void)
{
    /** @return Dirección corta de 16 bits leída del módulo */
    const uint8_t a16h = read_short(MRF_SADRH);
    return (a16h << 8 | read_short(MRF_SADRL));
}

const uint64_t Mrf24j::address64_read(void)
{
    /** @return Dirección extendida de 64 bits leída del módulo */
    uint64_t address64;
    address64 = (read_short(MRF_EADR0));
    address64 |= (read_short(MRF_EADR1)) << 8;
    address64 |= static_cast<uint64_t>(read_short(MRF_EADR2)) << 16;
    address64 |= static_cast<uint64_t>(read_short(MRF_EADR3)) << 24;
    address64 |= static_cast<uint64_t>(read_short(MRF_EADR4)) << 32;
    address64 |= static_cast<uint64_t>(read_short(MRF_EADR5)) << 40;
    address64 |= static_cast<uint64_t>(read_short(MRF_EADR6)) << 48;
    address64 |= static_cast<uint64_t>(read_short(MRF_EADR7)) << 56;
    return address64;
}

// ============================================================================
// Configuración del módulo
// ============================================================================

void Mrf24j::set_interrupts(void)
{
    /** @brief Configura las máscaras de interrupción para RX y TX normal */
    write_short(MRF_INTCON, 0b11110110);
}

void Mrf24j::set_channel(const uint8_t channel)
{
    /**
     * @brief Selecciona el canal IEEE 802.15.4
     * @param channel Número de canal (11-26)
     *
     * Mapea el canal a RFCON0 según la fórmula del fabricante:
     * valor = ((channel - 11) << 4) | 0x03
     */
    write_long(MRF_RFCON0, (((channel - 11) << 4) | 0x03));
}

void Mrf24j::init(void)
{
    /**
     * @brief Inicializa el módulo MRF24J40
     *
     * Realiza la secuencia completa de inicialización:
     * 1. Delay inicial de 192 µs
     * 2. Soft reset (configurable vía RESET_MRF_SOFTWARE)
     * 3. Configura PACON2, TXSTBL
     * 4. Configura registros RF (RFCON0-8, SLPCON1)
     * 5. Configura baseband (BBREG2, CCAEDTH, BBREG6)
     * 6. Configura interrupciones
     * 7. Selecciona canal
     * 8. Resetea máquina de estados RF
     */
    delay(192);

    #include <config/config.hpp>
    #ifdef RESET_MRF_SOFTWARE
        write_short(MRF_SOFTRST, 0x7);
    #endif

    write_short(MRF_PACON2, 0x98);   // FIFOEN = 1, TXONTS = 0x6
    write_short(MRF_TXSTBL, 0x95);   // RFSTBL = 0x9

    write_long(MRF_RFCON0, 0x03);    // RFOPT = 0x03
    write_long(MRF_RFCON1, 0x01);    // VCOOPT = 0x02
    write_long(MRF_RFCON2, 0x80);    // PLLEN = 1
    write_long(MRF_RFCON6, 0x90);    // TXFIL = 1, 20MRECVR = 1
    write_long(MRF_RFCON7, 0x80);    // SLPCLKSEL = 0x2
    write_long(MRF_RFCON8, 0x10);    // RFVCO = 1
    write_long(MRF_SLPCON1, 0x21);   // CLKOUTEN = 1, SLPCLKDIV = 0x01

    // Configuración para redes sin beacon
    write_short(MRF_BBREG2, 0x80);   // CCA mode ED
    write_short(MRF_CCAEDTH, 0x60);  // CCA ED threshold
    write_short(MRF_BBREG6, 0x40);   // RSSI append to RX FIFO

    set_interrupts();
    set_channel(CHANNEL);

    // Reset RF state machine
    write_short(MRF_RFCTL, 0x04);
    write_short(MRF_RFCTL, 0x00);
    delay(192);
}

// ============================================================================
// Manejador de interrupciones
// ============================================================================

void Mrf24j::interrupt_handler(void)
{
    /**
     * @brief Procesa interrupciones del MRF24J40 (polling)
     *
     * Lee INTSTAT y procesa:
     * - RXIF: Lee frame_length del RX FIFO, extrae payload, LQI y RSSI
     * - TXNIF: Lee TXSTAT, actualiza info de TX
     *
     * @note Solo mantiene los datos más recientes (sobrescribe anteriores)
     */
    const uint8_t last_interrupt = read_short(MRF_INTSTAT);

    if (last_interrupt & MRF_I_RXIF) {
        m_flag_got_rx.fetch_add(1, std::memory_order_relaxed);

        noInterrupts();
        rx_disable();

        const size_t frame_length = read_long(0x300);

    #ifdef USE_MAC_ADDRESS_LONG
        if (bufPHY) {
            int rb_ptr = 0;
            for (size_t i = 0; i < frame_length; ++i) {
                rx_buf[++rb_ptr] = read_long(0x301 + i);
            }
        }
    #else
        if (MAX_PACKET_TX < frame_length) {
            if (bufPHY) {
                int rb_ptr = 0;
                for (int i = 0; i < frame_length; ++i) {
                    rx_buf[++rb_ptr] = read_long(0x301 + i);
                }
            } else {
                write_short(MRF_RXFLUSH, 0x01);
            }
            write_short(MRF_BBREG1, 0x00);
        } else {
            write_short(MRF_RXFLUSH, 0x01);
        }
    #endif

    #ifdef ENABLE_SECURITY
        write_short(WRITE_SECCR0, 0x80);
        write_short(WRITE_RXFLUSH, 0x01);
    #endif

        // Extraer datos de RX en rx_info
        int rd_ptr = 0;
        for (int i = 0; i < frame_length; ++i) {
            rx_info.rx_data[++rd_ptr] = read_long(0x301 + m_bytes_MHR + i);
        }

        rx_info.frame_length = frame_length;
        rx_info.lqi = read_long(0x301 + frame_length);
        rx_info.rssi = read_long(0x301 + frame_length + 1);

        rx_enable();
        interrupts();
    }

    if (last_interrupt & MRF_I_TXNIF) {
        m_flag_got_tx.fetch_add(1, std::memory_order_relaxed);
        const uint8_t tmp = read_short(MRF_TXSTAT);
        tx_info.tx_ok = !(tmp & ~(1 << TXNSTAT));
        tx_info.retries = tmp >> 6;
        tx_info.channel_busy = (tmp & (1 << CCAFAIL));
    }
}

// ============================================================================
// Verificación de flags
// ============================================================================

const bool Mrf24j::check_flags(void (*rx_handler)(), void (*tx_handler)())
{
    /**
     * @brief Verifica flags de interrupción y ejecuta handlers
     *
     * Si hay datos RX pendientes, invoca rx_handler.
     * Si hay TX completada, invoca tx_handler.
     * Los flags se limpian después de procesar.
     *
     * @todo Verificar si flags > 1 indica pérdida de datos
     * @param rx_handler Callback de recepción
     * @param tx_handler Callback de transmisión
     * @return true si hay datos RX disponibles
     */
    if (m_flag_got_rx) {
        m_flag_got_rx = 0;
        #ifdef DBG
            std::cout << "recibe algo \n";
        #endif
        rx_handler();
        return true;
    }
    if (m_flag_got_tx) {
        m_flag_got_tx = 0;
        #ifdef DBG_MRF
            std::cout << "transmite algo \n";
        #endif
        tx_handler();
        return false;
    }
    return false;
}

// ============================================================================
// Modos de operación
// ============================================================================

void Mrf24j::set_promiscuous(const bool enabled)
{
    /** @brief Activa/desactiva modo promiscuo en RXMCR */
    if (enabled)
        write_short(MRF_RXMCR, 0x01);
    else
        write_short(MRF_RXMCR, 0x00);
}

void Mrf24j::settings_mrf(void)
{
    /**
     * @brief Aplica configuración adicional al MRF24J40
     *
     * Configura los bits de RXMCR según defines COORDINATOR,
     * ROUTER y PROMISCUE en config.hpp.
     */
    #ifdef COORDINATOR
        rxmcr.PANCOORD = true;
    #else
        rxmcr.PANCOORD = true;
    #endif

    #ifdef ROUTER
        rxmcr.COORD = false;
    #else
        rxmcr.COORD = false;
    #endif

    #ifdef PROMISCUE
        rxmcr.PROMI = true;
    #else
        rxmcr.PROMI = true;
    #endif

    #ifdef DBG_MRF
        printf("*reinterpret_cast : 0x%x\n", *reinterpret_cast<uint8_t*>(&rxmcr));
    #endif
    write_short(MRF_RXMCR, *reinterpret_cast<uint8_t*>(&rxmcr));
}

// ============================================================================
// Accesores
// ============================================================================

rx_info_t* Mrf24j::get_rxinfo(void) { return &rx_info; }
tx_info_t* Mrf24j::get_txinfo(void) { return &tx_info; }
uint8_t*   Mrf24j::get_rxbuf(void)  { return rx_buf; }

const int Mrf24j::rx_datalength(void)
{
    /** @return Longitud del payload (frame_length - MHR - FCS) */
    return rx_info.frame_length - m_bytes_nodata;
}

void Mrf24j::set_ignoreBytes(const int ib) { ignoreBytes = ib; }
void Mrf24j::set_bufferPHY(const bool bp)  { bufPHY = bp; }
bool Mrf24j::get_bufferPHY(void)           { return bufPHY; }

// ============================================================================
// Control PA/LNA
// ============================================================================

void Mrf24j::set_palna(const bool enabled)
{
    /**
     * @brief Controla el amplificador externo PA/LNA
     * @param enabled true = habilita PA/LNA (MRF24J40MB)
     */
    if (enabled)
        write_long(MRF_TESTMODE, 0x07);
    else
        write_long(MRF_TESTMODE, 0x08);
}

// ============================================================================
// Control RX
// ============================================================================

void Mrf24j::rx_flush(void)  { write_short(MRF_RXFLUSH, 0x01); }
void Mrf24j::rx_disable(void) { write_short(MRF_BBREG1, 0x04); }
void Mrf24j::rx_enable(void)  { write_short(MRF_BBREG1, 0x00); }

// ============================================================================
// Compatibilidad Arduino (stubs)
// ============================================================================

void Mrf24j::pinMode(const int, const bool)      {}
void Mrf24j::digitalWrite(const int, const bool) {}
void Mrf24j::noInterrupts()                      {}

void Mrf24j::interrupts()
{
    /** @brief Reconfigura las máscaras de interrupción */
    set_interrupts();
}

void Mrf24j::delay(const uint16_t t)
{
    /** @brief Delay en milisegundos usando TYME::Time_t */
    TYME::Time_t time;
    time.delay_ms(t);
}

// ============================================================================
// Destructor
// ============================================================================

Mrf24j::~Mrf24j()
{
    #ifdef DBG_MRF
        std::cout << "~Mrf24j( )\r\n";
    #endif
}

// ============================================================================
// Modo turbo (625 kbps)
// ============================================================================

void Mrf24j::mode_turbo()
{
    /**
     * @brief Activa modo turbo para máximo throughput
     *
     * Configura registros BBREG0, BBREG3, BBREG4 para operar
     * a 625 kbps en lugar de los 250 kbps estándar.
     * Solo se activa si TURBO_MODE está definido.
     */
    #ifdef TURBO_MODE
        write_short(WRITE_BBREG0, 0x01);
        write_short(WRITE_BBREG3, 0x38);
        write_short(WRITE_BBREG4, 0x5C);
        write_short(WRITE_RFCTL, 0x04);
        write_short(WRITE_RFCTL, 0x00);
    #endif
}

// ============================================================================
// Inserción de dirección MAC en FIFO
// ============================================================================

void Mrf24j::set_macaddress(int& i, const uint64_t mac_adress)
{
    /**
     * @brief Escribe una dirección MAC de 64 bits en el FIFO TX
     * @param i           Índice actual en el FIFO (se incrementa)
     * @param mac_adress  Dirección MAC a escribir
     *
     * Si la dirección es de 16 bits, solo escribe 2 bytes.
     * Si es de 64 bits, escribe los 8 bytes completos.
     */
    write_long(i++, mac_adress & 0xff);
    write_long(i++, (mac_adress >> 8) & 0xff);
    if (sizeof(mac_adress) > 2) {
        #ifdef DBG_MRF
            std::cout << "es un mac de 64 bytes\n";
        #endif
        write_long(i++, (mac_adress >> 16) & 0xff);
        write_long(i++, (mac_adress >> 24) & 0xff);
        write_long(i++, (mac_adress >> 32) & 0xff);
        write_long(i++, (mac_adress >> 40) & 0xff);
        write_long(i++, (mac_adress >> 48) & 0xff);
        write_long(i++, (mac_adress >> 56) & 0xff);
    }
}

// ============================================================================
// Utilidades RF
// ============================================================================

void Mrf24j::flush_rx_fifo(void)
{
    /** @brief Limpia el FIFO RX manteniendo otros bits */
    write_short(MRF_RXFLUSH, read_short(MRF_RXFLUSH) | 0b00000001);
}

void Mrf24j::reset_rf_state_machine(void)
{
    /**
     * @brief Resetea la máquina de estados RF
     *
     * Lee RFCTL, escribe bit 2 = 1, luego bit 2 = 0.
     */
    const uint8_t rfctl = read_short(MRF_RFCTL);
    write_short(MRF_RFCTL, rfctl | 0b00000100);
    write_short(MRF_RFCTL, rfctl & 0b11111011);
}

// ============================================================================
// Lectura de direcciones MAC
// ============================================================================

void Mrf24j::mrf24j40_get_extended_mac_addr(uint64_t* address)
{
    /** @brief Lee la dirección MAC de 64 bits desde los registros EADR */
    uint8_t* addr_ptr = reinterpret_cast<uint8_t*>(address);
    addr_ptr[7] = read_short(MRF_EADR7);
    addr_ptr[6] = read_short(MRF_EADR6);
    addr_ptr[5] = read_short(MRF_EADR5);
    addr_ptr[4] = read_short(MRF_EADR4);
    addr_ptr[3] = read_short(MRF_EADR3);
    addr_ptr[2] = read_short(MRF_EADR2);
    addr_ptr[1] = read_short(MRF_EADR1);
    addr_ptr[0] = read_short(MRF_EADR0);
}

void Mrf24j::mrf24j40_get_short_mac_addr(uint16_t* address)
{
    /** @brief Lee la dirección MAC de 16 bits desde SADRH:SADRL */
    uint16_t low_addr = read_short(MRF_SADRL);
    uint16_t high_addr = read_short(MRF_SADRH);
    *address = (high_addr << 8) | low_addr;
}

void Mrf24j::mrf24j40_set_tx_power(uint8_t& pwr)
{
    /**
     * @brief Lee la potencia de transmisión actual
     * @param pwr Referencia donde se almacena el valor de RFCON3
     */
    pwr = read_long(MRF_RFCON3);
}

} // namespace MRF24J40
