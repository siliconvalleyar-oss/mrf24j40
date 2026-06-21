/**
 * @file    mrf24j40.cpp
 * @brief   Implementación del driver simplificado MRF24J40
 * @details Driver de alto nivel para el módulo MRF24J40MA. Proporciona
 *          una API limpia y minimalista para inicialización, configuración
 *          de red, transmisión y recepción de tramas IEEE 802.15.4.
 *          La comunicación SPI se realiza mediante ioctl directo a spidev.
 *
 * @note    Este driver coexiste con el driver completo en src/mrf24/.
 *          La clase Mrf24j40 es independiente de MRF24J40::Mrf24j.
 *
 * @author  Project MRF24J40
 * @date    2024-2026
 */

#include "mrf24j40.h"
#include <fcntl.h>
#include <errno.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <cstring>

// ---------------------------------------------------------------------------
// Constantes de protocolo IEEE 802.15.4
// ---------------------------------------------------------------------------
/** @brief Longitud del MAC Header (2 FCF + 1 Seq + 2 PAN + 2 Dst + 2 Src) */
static constexpr uint8_t MAC_HDR_LEN = 9;

/** @brief Longitud del Frame Check Sequence (CRC16) */
static constexpr uint8_t FCS_LEN = 2;

/** @brief Longitud mínima de trama válida */
static constexpr uint8_t MIN_FRAME_LEN = 12;

/** @brief Primer byte del Frame Control Field: data frame, ACK requested, PAN compression */
static constexpr uint8_t FCF_LO = 0x61;

/** @brief Segundo byte del Frame Control Field: 16-bit dest, 16-bit src, 802.15.4-2003 */
static constexpr uint8_t FCF_HI = 0x88;

/** @brief Dirección base del TX Normal FIFO en memoria del MRF24J40 */
static constexpr uint16_t TXNFIFO = 0x300;

/** @brief Dirección base del RX FIFO en memoria del MRF24J40 */
static constexpr uint16_t RXFIFO = 0x300;

/** @brief Bit para solicitar ACK en el registro TXNCON */
static constexpr uint8_t TXNACKREQ = (1 << 2);

/** @brief Bit para disparar transmisión en el registro TXNCON */
static constexpr uint8_t TXNTRIG = (1 << 0);

// ============================================================================
// Constructor / Destructor
// ============================================================================

Mrf24j40::Mrf24j40()
    : spi_fd(-1), initialized(false), tx_pending(false), tx_ok(false),
      tx_retries(0), seq(0), rx_length(0), rx_lqi(0), rx_rssi_dbm(0), rx_ready(false)
{
    stats = {};
}

Mrf24j40::~Mrf24j40()
{
    if (spi_fd >= 0) close(spi_fd);
}

// ============================================================================
// Operaciones SPI de bajo nivel
// ============================================================================

uint8_t Mrf24j40::readShort(uint8_t addr)
{
    /** 
     * @brief Lee un registro de dirección corta (8 bits)
     * @param addr Dirección del registro (0x00-0x3F)
     * @return Valor de 8 bits del registro
     * 
     * Formato de trama SPI:
     *   Byte 0: [0][A5][A4][A3][A2][A1][A0][R/W=0]
     *   Byte 1: dato recibido (dummy byte)
     */
    uint8_t tx[2], rx[2];
    tx[0] = (addr & 0x3F) << 1;
    tx[1] = 0x00;

    struct spi_ioc_transfer tr;
    std::memset(&tr, 0, sizeof(tr));
    tr.tx_buf = (unsigned long)tx;
    tr.rx_buf = (unsigned long)rx;
    tr.len = 2;
    tr.speed_hz = SPI_SPEED_HZ;
    tr.bits_per_word = 8;

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0)
        perror("readShort");
    return rx[1];
}

void Mrf24j40::writeShort(uint8_t addr, uint8_t val)
{
    /**
     * @brief Escribe un registro de dirección corta (8 bits)
     * @param addr Dirección del registro (0x00-0x3F)
     * @param val  Valor de 8 bits a escribir
     * 
     * Formato de trama SPI:
     *   Byte 0: [0][A5][A4][A3][A2][A1][A0][R/W=1]
     *   Byte 1: dato a escribir
     */
    uint8_t tx[2];
    tx[0] = ((addr & 0x3F) << 1) | 0x01;
    tx[1] = val;

    struct spi_ioc_transfer tr;
    std::memset(&tr, 0, sizeof(tr));
    tr.tx_buf = (unsigned long)tx;
    tr.len = 2;
    tr.speed_hz = SPI_SPEED_HZ;
    tr.bits_per_word = 8;

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0)
        perror("writeShort");
}

uint8_t Mrf24j40::readLong(uint16_t addr)
{
    /**
     * @brief Lee un registro de dirección larga (16 bits, 0x200-0x3FF)
     * @param addr Dirección del registro de 16 bits
     * @return Valor de 8 bits del registro
     * 
     * Formato de trama SPI de 3 bytes para direcciones largas:
     *   Byte 0: [1][A10][A9][A8][A7][A6][A5][A4]  (R/W=1, bits altos)
     *   Byte 1: [A3][A2][A1][A0][X][X][X][R/W=0]   (bits bajos)
     *   Byte 2: dato recibido (dummy)
     */
    uint8_t tx[3], rx[3];
    tx[0] = 0x80 | ((addr >> 3) & 0x7F);
    tx[1] = (addr & 0x07) << 5;
    tx[2] = 0x00;

    struct spi_ioc_transfer tr;
    std::memset(&tr, 0, sizeof(tr));
    tr.tx_buf = (unsigned long)tx;
    tr.rx_buf = (unsigned long)rx;
    tr.len = 3;
    tr.speed_hz = SPI_SPEED_HZ;
    tr.bits_per_word = 8;

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0)
        perror("readLong");
    return rx[2];
}

void Mrf24j40::writeLong(uint16_t addr, uint8_t val)
{
    /**
     * @brief Escribe un registro de dirección larga (16 bits)
     * @param addr Dirección del registro de 16 bits
     * @param val  Valor de 8 bits a escribir
     */
    uint8_t tx[3];
    tx[0] = 0x80 | ((addr >> 3) & 0x7F);
    tx[1] = ((addr & 0x07) << 5) | 0x10;
    tx[2] = val;

    struct spi_ioc_transfer tr;
    std::memset(&tr, 0, sizeof(tr));
    tr.tx_buf = (unsigned long)tx;
    tr.len = 3;
    tr.speed_hz = SPI_SPEED_HZ;
    tr.bits_per_word = 8;

    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0)
        perror("writeLong");
}

// ============================================================================
// Inicialización
// ============================================================================

bool Mrf24j40::waitForReset()
{
    /** @brief Espera hasta que el bit de soft reset se desactive (timeout ~200ms) */
    for (int i = 0; i < 200; i++) {
        if ((readShort(REG_SOFTRST) & 0x07) == 0x00)
            return true;
        usleep(1000);
    }
    return false;
}

bool Mrf24j40::init(uint8_t channel)
{
    /**
     * @brief Inicializa el módulo MRF24J40
     * @param channel Canal IEEE 802.15.4 (11-26)
     * @return true si la inicialización fue exitosa
     * 
     * Pasos:
     * 1. Abre el dispositivo SPI (/dev/spidev0.0)
     * 2. Configura modo SPI 0, velocidad definida en SPI_SPEED_HZ
     * 3. Realiza soft reset del módulo
     * 4. Configura registros RF, canal y baseband
     * 5. Inicializa FIFO de recepción y máquina de estados RF
     */
    spi_fd = open(SPI_DEVICE, O_RDWR);
    if (spi_fd < 0) {
        fprintf(stderr, "ERROR: No se pudo abrir %s\n", SPI_DEVICE);
        return false;
    }

    uint8_t mode = SPI_MODE_0;
    ioctl(spi_fd, SPI_IOC_WR_MODE, &mode);

    uint32_t speed = SPI_SPEED_HZ;
    ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);

    printf("[SPI] %s, velocidad: %u Hz\n", SPI_DEVICE, speed);

    // Soft reset: escribe 0x07 en SOFTRST y espera que se desactive
    writeShort(REG_SOFTRST, 0x07);
    usleep(2000);
    if (!waitForReset()) {
        fprintf(stderr, "ERROR: Timeout en soft reset\n");
        return false;
    }
    printf("[INIT] Soft reset OK\n");

    // Configuración base del transceiver
    writeShort(REG_PACON2, 0x98);   // FIFO enable, TXONTS = 0x6
    writeShort(REG_TXSTBL, 0x95);   // RF stabilization time

    // Configuración RF
    writeLong(LREG_RFCON1, 0x02);
    writeLong(LREG_RFCON2, 0x80);   // PLL enable
    writeLong(LREG_RFCON3, 0x00);
    writeLong(LREG_RFCON6, 0x90);   // TX filter, 20MHz receiver
    writeLong(LREG_RFCON7, 0x80);   // Sleep clock select
    writeLong(LREG_RFCON8, 0x10);   // RF VCO
    writeLong(LREG_SLPCON1, 0x21);  // Clock out enable

    // Selección de canal
    setChannel(channel);
    printf("[INIT] Canal: %d\n", channel);

    // Configuración baseband
    writeShort(REG_BBREG2, 0x80);   // CCA mode ED
    writeShort(REG_CCAEDTH, 0x60);  // ED threshold
    writeShort(REG_BBREG6, 0x40);   // Append RSSI to RX FIFO
    writeShort(REG_INTCON, 0xF6);   // Interrupt mask

    flushRx();
    rfReset();

    initialized = true;
    printf("[INIT] MRF24J40 inicializado correctamente\n");
    return true;
}

void Mrf24j40::rfReset()
{
    /** @brief Resetea la máquina de estados RF escribiendo RFCTL */
    writeShort(REG_RFCTL, 0x04);
    usleep(200);
    writeShort(REG_RFCTL, 0x00);
    usleep(1000);
}

// ============================================================================
// Configuración de red
// ============================================================================

void Mrf24j40::setPan(uint16_t pan)
{
    /** @brief Configura el PAN ID (registros PANIDL y PANIDH) */
    writeShort(REG_PANIDL, pan & 0xFF);
    writeShort(REG_PANIDH, (pan >> 8) & 0xFF);
}

void Mrf24j40::setShortAddress(uint16_t addr)
{
    /** @brief Configura la dirección corta de 16 bits */
    writeShort(REG_SADRL, addr & 0xFF);
    writeShort(REG_SADRH, (addr >> 8) & 0xFF);
}

uint16_t Mrf24j40::getPan()
{
    /** @brief Obtiene el PAN ID actual del módulo */
    return readShort(REG_PANIDL) | (readShort(REG_PANIDH) << 8);
}

uint16_t Mrf24j40::getShortAddress()
{
    /** @brief Obtiene la dirección corta actual del módulo */
    return readShort(REG_SADRL) | (readShort(REG_SADRH) << 8);
}

bool Mrf24j40::setChannel(uint8_t ch)
{
    /**
     * @brief Cambia el canal de operación
     * @param ch Canal (11-26 según IEEE 802.15.4)
     * @return true si el canal es válido
     */
    if (ch < 11 || ch > 26) return false;
    uint8_t val = ((ch - 11) << 4) | 0x03;
    writeLong(LREG_RFCON0, val);
    rfReset();
    return true;
}

void Mrf24j40::flushRx()
{
    /** @brief Limpia el FIFO de recepción */
    writeShort(REG_BBREG1, 0x04);   // Disable receiver
    writeShort(REG_RXFLUSH, 0x01);  // Flush RX FIFO
    usleep(100);
    writeShort(REG_BBREG1, 0x00);   // Re-enable receiver
}

// ============================================================================
// Transmisión
// ============================================================================

bool Mrf24j40::send(uint16_t dest_addr, uint16_t dest_pan, const uint8_t* data, uint8_t len)
{
    /**
     * @brief Envía un paquete de datos
     * @param dest_addr Dirección corta de destino (16 bits)
     * @param dest_pan  PAN ID de destino
     * @param data      Puntero a los datos a enviar
     * @param len       Longitud de los datos (máx MAX_PAYLOAD)
     * @return true si el paquete se encoló para TX
     * 
     * Construye una trama IEEE 802.15.4 en el TX Normal FIFO:
     *   [Header Length][Frame Length][FCF(2)][Seq][PAN(2)][Dst(2)][Src(2)][Payload...]
     * Luego dispara la transmisión escribiendo TXNTRIG en TXNCON.
     */
    if (!initialized || len > MAX_PAYLOAD) return false;

    // Esperar a que TX anterior termine
    int wait = 500;
    while (tx_pending && wait-- > 0) {
        poll();
        usleep(1000);
    }
    if (tx_pending) return false;

    uint16_t src_addr = getShortAddress();
    const uint8_t frm_len = MAC_HDR_LEN + len;

    // Construir trama en TX FIFO
    writeLong(TXNFIFO + 0, MAC_HDR_LEN);      // Header length
    writeLong(TXNFIFO + 1, frm_len);           // Frame length
    writeLong(TXNFIFO + 2, FCF_LO);            // FCF byte 0
    writeLong(TXNFIFO + 3, FCF_HI);            // FCF byte 1
    writeLong(TXNFIFO + 4, seq++);             // Sequence number
    writeLong(TXNFIFO + 5, dest_pan & 0xFF);   // Dest PAN low
    writeLong(TXNFIFO + 6, (dest_pan >> 8) & 0xFF);  // Dest PAN high
    writeLong(TXNFIFO + 7, dest_addr & 0xFF);  // Dest address low
    writeLong(TXNFIFO + 8, (dest_addr >> 8) & 0xFF);  // Dest address high
    writeLong(TXNFIFO + 9, src_addr & 0xFF);   // Src address low
    writeLong(TXNFIFO + 10, (src_addr >> 8) & 0xFF);  // Src address high

    // Payload
    for (uint8_t i = 0; i < len; i++)
        writeLong(TXNFIFO + 11 + i, data[i]);

    tx_pending = true;
    tx_ok = false;
    tx_retries = 0;
    stats.packets_sent++;

    // Disparar transmisión con ACK request
    writeShort(REG_TXNCON, TXNACKREQ | TXNTRIG);
    return true;
}

bool Mrf24j40::sendString(uint16_t dest_addr, const char* str)
{
    /** @brief Envía un string como paquete (wrapper de send) */
    if (!str) return false;
    uint8_t len = strnlen(str, MAX_PAYLOAD);
    return send(dest_addr, getPan(), (const uint8_t*)str, len);
}

// ============================================================================
// Manejo de interrupciones (polling)
// ============================================================================

void Mrf24j40::handleTxIrq()
{
    /** @brief Procesa interrupción de TX completada */
    uint8_t txstat = readShort(REG_TXSTAT);
    tx_ok = !(txstat & 0x01);              // Bit 0: TXNSTAT (0=success)
    tx_retries = (txstat >> 6) & 0x03;     // Bits 6-7: retry count
    tx_pending = false;

    if (tx_ok) {
        stats.tx_success++;
    } else {
        stats.tx_fail++;
    }
    stats.tx_retries_total += tx_retries;
}

void Mrf24j40::handleRxIrq()
{
    /**
     * @brief Procesa interrupción de RX (paquete recibido)
     * 
     * Lee la trama del RX FIFO, extrae payload, LQI y RSSI.
     * Convierte RSSI raw a dBm aproximadamente.
     */
    writeShort(REG_BBREG1, 0x04);  // Disable RX durante lectura

    uint8_t frame_len = readLong(RXFIFO + 0);

    // Validar longitud mínima de trama IEEE 802.15.4
    if (frame_len < MIN_FRAME_LEN || frame_len > 127) {
        flushRx();
        return;
    }

    int payload_len = frame_len - MAC_HDR_LEN - FCS_LEN;
    if (payload_len <= 0 || payload_len > MAX_PAYLOAD) {
        flushRx();
        return;
    }

    // Leer payload del FIFO
    rx_length = payload_len;
    for (uint8_t i = 0; i < rx_length; i++)
        rx_buf[i] = readLong(RXFIFO + 1 + MAC_HDR_LEN + i);

    // Leer LQI y RSSI (añadidos al final del frame por BBREG6)
    rx_lqi = readLong(RXFIFO + 1 + frame_len);
    uint8_t raw_rssi = readLong(RXFIFO + 1 + frame_len + 1);
    rx_rssi_dbm = -90 + raw_rssi / 3;  // Conversión aproximada a dBm

    rx_ready = true;

    // Actualizar estadísticas
    stats.packets_received++;
    stats.rx_lqi_sum += rx_lqi;
    stats.rx_rssi_sum += rx_rssi_dbm;
    stats.rx_count++;

    flushRx();
    writeShort(REG_BBREG1, 0x00);  // Re-enable RX
}

void Mrf24j40::poll()
{
    /**
     * @brief Polling de interrupciones del MRF24J40
     * 
     * Debe llamarse periódicamente para procesar eventos TX/RX.
     * Lee INTSTAT y despacha a los handlers correspondientes.
     */
    uint8_t irq = readShort(REG_INTSTAT);
    if (irq == 0) return;

    if (irq & INT_TXNIF) handleTxIrq();
    if (irq & INT_RXIF) handleRxIrq();
    writeShort(REG_INTSTAT, irq);  // Clear interrupt flags
}

// ============================================================================
// Recepción
// ============================================================================

void Mrf24j40::rxGet(uint8_t* buf)
{
    /** @brief Obtiene el último paquete recibido */
    if (buf && rx_ready) {
        memcpy(buf, rx_buf, rx_length);
    }
    rx_ready = false;
    rx_length = 0;
}

// ============================================================================
// Estadísticas
// ============================================================================

void Mrf24j40::getStats(RadioStats& s)
{
    /** @brief Copia las estadísticas actuales a la estructura proporcionada */
    s = stats;
}

void Mrf24j40::resetStats()
{
    /** @brief Reinicia todas las estadísticas a cero */
    stats = {};
}

// ============================================================================
// Diagnóstico
// ============================================================================

void Mrf24j40::printRegisters()
{
    /** @brief Imprime el estado de los registros clave del MRF24J40 */
    printf("\n=== Registros MRF24J40 ===\n");
    printf("SOFTRST: 0x%02X\n", readShort(REG_SOFTRST));
    printf("INTSTAT: 0x%02X\n", readShort(REG_INTSTAT));
    printf("TXSTAT:  0x%02X\n", readShort(REG_TXSTAT));
    printf("PANID:   0x%04X\n", getPan());
    printf("SADDR:   0x%04X\n", getShortAddress());
    printf("RFCON2:  0x%02X\n", readLong(LREG_RFCON2));
    printf("========================\n");
}

bool Mrf24j40::selfTest()
{
    /**
     * @brief Autodiagnóstico del módulo MRF24J40
     * 
     * Verifica la comunicación SPI escribiendo y leyendo el registro PAN ID.
     * @return true si el módulo responde correctamente
     */
    uint16_t original_pan = getPan();
    setPan(0x1234);
    uint16_t test_pan = getPan();
    setPan(original_pan);

    if (test_pan != 0x1234) {
        fprintf(stderr, "Self-test falló: escritura PAN\n");
        return false;
    }
    printf("Self-test OK\n");
    return true;
}
