#pragma once

/**
 * @file    mrf24j40_cmd.hpp
 * @brief   Definiciones de registros y constantes del MRF24J40MA
 * @details Este archivo contiene todas las direcciones de registros del transceiver
 *          MRF24J40MA, tanto de dirección corta (8 bits, 0x00-0x3F) como de
 *          dirección larga (16 bits, 0x200-0x3FF). También define las máscaras
 *          de bits para campos específicos dentro de los registros de control.
 *
 * @namespace MRF24J40
 * @{
 */

namespace MRF24J40 {

    // =========================================================================
    // Registros de dirección corta (Short Address: 0x00 - 0x3F)
    // =========================================================================

    /** @brief Received Signal Strength Indicator / Energy Detection (address 0x00) */
    #define MRF_RXMCR      0x00

    /** @brief PAN ID byte bajo */
    #define MRF_PANIDL     0x01

    /** @brief PAN ID byte alto */
    #define MRF_PANIDH     0x02

    /** @brief Dirección corta propia byte bajo (Short Address) */
    #define MRF_SADRL      0x03

    /** @brief Dirección corta propia byte alto */
    #define MRF_SADRH      0x04

    /** @brief Dirección extendida (EADR0-EADR7): bytes 0-7 de MAC 64 bits */
    #define MRF_EADR0      0x05
    #define MRF_EADR1      0x06
    #define MRF_EADR2      0x07
    #define MRF_EADR3      0x08
    #define MRF_EADR4      0x09
    #define MRF_EADR5      0x0A
    #define MRF_EADR6      0x0B
    #define MRF_EADR7      0x0C

    /** @brief RX FIFO flush (escribir 0x01 para limpiar) */
    #define MRF_RXFLUSH    0x0D

    #define MRF_ORDER      0x10   /**< Order register (MAC beacon order) */
    #define MRF_TXMCR      0x11   /**< TX MAC Control Register */
    #define MRF_ACKTMOUT   0x12   /**< ACK Timeout (símbolos) */
    #define MRF_ESLOTG1    0x13   /**< Enhanced slot register for GTS1 */
    #define MRF_SYMTICKL   0x14   /**< Symbol tick counter byte bajo */
    #define MRF_SYMTICKH   0x15   /**< Symbol tick counter byte alto */
    #define MRF_PACON0     0x16   /**< PA Control 0 */
    #define MRF_PACON1     0x17   /**< PA Control 1 */
    #define MRF_PACON2     0x18   /**< PA Control 2 (FIFO enable, TXONTS) */

    /** @brief TX Beacon FIFO Control 0 */
    #define MRF_TXBCON0    0x1A

    // -------------------------------------------------------------------------
    // TXNCON: TRANSMIT NORMAL FIFO CONTROL (ADDRESS: 0x1B)
    // -------------------------------------------------------------------------
    #define MRF_TXNCON     0x1B

    /** @brief Bit 0: TXNTRIG — Disparar transmisión del FIFO normal */
    #define MRF_TXNTRIG    0
    /** @brief Bit 1: TXNSECEN — Habilitar seguridad en FIFO normal */
    #define MRF_TXNSECEN   1
    /** @brief Bit 2: TXNACKREQ — Solicitar ACK en transmisión */
    #define MRF_TXNACKREQ  2
    /** @brief Bit 3: INDIRECT — Activar transmisión indirecta (coordinador) */
    #define MRF_INDIRECT   3
    /** @brief Bit 4: FPSTAT — Frame Pending Status */
    #define MRF_FPSTAT     4

    #define MRF_TXG1CON    0x1C   /**< TX GTS1 FIFO Control */
    #define MRF_TXG2CON    0x1D   /**< TX GTS2 FIFO Control */
    #define MRF_ESLOTG23   0x1E   /**< Enhanced slot GTS2-3 */
    #define MRF_ESLOTG45   0x1F   /**< Enhanced slot GTS4-5 */
    #define MRF_ESLOTG67   0x20   /**< Enhanced slot GTS6-7 */
    #define MRF_TXPEND     0x21   /**< TX Pending register */
    #define MRF_WAKECON    0x22   /**< Wake Control */
    #define MRF_FRMOFFSET  0x23   /**< Frame Offset */

    // -------------------------------------------------------------------------
    // TXSTAT: TX MAC STATUS (ADDRESS: 0x24)
    // -------------------------------------------------------------------------
    #define MRF_TXSTAT     0x24
    #define TXNRETRY1      7      /**< Bit 7: Número de retransmisión (MSB) */
    #define TXNRETRY0      6      /**< Bit 6: Número de retransmisión (LSB) */
    #define CCAFAIL        5      /**< Bit 5: CCA falló (canal ocupado) */
    #define TXG2FNT        4      /**< Bit 4: GTS2 FIFO no terminado */
    #define TXG1FNT        3      /**< Bit 3: GTS1 FIFO no terminado */
    #define TXG2STAT       2      /**< Bit 2: Estado de GTS2 */
    #define TXG1STAT       1      /**< Bit 1: Estado de GTS1 */
    #define TXNSTAT        0      /**< Bit 0: Estado TX Normal (0=éxito, 1=fallo) */

    #define MRF_TXBCON1    0x25   /**< TX Beacon FIFO Control 1 */
    #define MRF_GATECLK    0x26   /**< Gate Clock */
    #define MRF_TXTIME     0x27   /**< TX Time */
    #define MRF_HSYMTMRL   0x28   /**< Hardware Symbol Timer Low */
    #define MRF_HSYMTMRH   0x29   /**< Hardware Symbol Timer High */

    /** @brief Soft Reset register (escribir 0x07 para reset) */
    #define MRF_SOFTRST    0x2A

    #define MRF_SECCON0    0x2C   /**< Security Control 0 */
    #define MRF_SECCON1    0x2D   /**< Security Control 1 */
    #define MRF_TXSTBL     0x2E   /**< TX Stabilization */

    #define MRF_RXSR       0x30   /**< RX Status Register */

    /** @brief Interrupt Status Register (leer para identificar interrupción) */
    #define MRF_INTSTAT    0x31

    /** @brief Interrupt Control Register (escribir para habilitar/deshabilitar IRQ) */
    #define MRF_INTCON     0x32

    #define MRF_GPIO       0x33   /**< GPIO Control */
    #define MRF_TRISGPIO   0x34   /**< GPIO Tris (dirección) */
    #define MRF_SLPACK     0x35   /**< Sleep ACK */

    /** @brief RF Control Register (reset máquina de estados RF) */
    #define MRF_RFCTL      0x36

    #define MRF_SECCR2     0x37   /**< Security Control 2 */
    #define MRF_BBREG0     0x38   /**< Baseband Register 0 */
    #define MRF_BBREG1     0x39   /**< Baseband Register 1 (control RX) */
    #define MRF_BBREG2     0x3A   /**< Baseband Register 2 (CCA mode) */
    #define MRF_BBREG3     0x3B   /**< Baseband Register 3 */
    #define MRF_BBREG4     0x3C   /**< Baseband Register 4 */
    #define MRF_BBREG6     0x3E   /**< Baseband Register 6 (RSSI append) */
    #define MRF_CCAEDTH    0x3F   /**< CCA Energy Detection Threshold */

    // =========================================================================
    // Registros de dirección larga (Long Address: 0x200 - 0x3FF)
    // =========================================================================

    #define MRF_RFCON0     0x200  /**< RF Configuration 0 (sel. canal) */
    #define MRF_RFCON1     0x201  /**< RF Configuration 1 */
    #define MRF_RFCON2     0x202  /**< RF Configuration 2 (PLL enable) */
    #define MRF_RFCON3     0x203  /**< RF Configuration 3 (potencia TX) */
    #define MRF_RFCON5     0x205  /**< RF Configuration 5 */
    #define MRF_RFCON6     0x206  /**< RF Configuration 6 */
    #define MRF_RFCON7     0x207  /**< RF Configuration 7 */
    #define MRF_RFCON8     0x208  /**< RF Configuration 8 */

    #define MRF_SLPCAL0    0x209  /**< Sleep Calibration 0 */
    #define MRF_SLPCAL1    0x20A  /**< Sleep Calibration 1 */
    #define MRF_SLPCAL2    0x20B  /**< Sleep Calibration 2 */

    #define MRF_RSSI       0x210  /**< RSSI Value (lectura del nivel de señal) */
    #define MRF_SLPCON0    0x211  /**< Sleep Configuration 0 */
    #define MRF_SLPCON1    0x220  /**< Sleep Configuration 1 */

    #define MRF_WAKETIMEL  0x222  /**< Wake Timer Low */
    #define MRF_WAKETIMEH  0x223  /**< Wake Timer High */
    #define MRF_REMCNTL    0x224  /**< Remote Control Low */
    #define MRF_REMCNTH    0x225  /**< Remote Control High */
    #define MRF_MAINCNT0   0x226  /**< Main Timer Counter 0 */
    #define MRF_MAINCNT1   0x227  /**< Main Timer Counter 1 */
    #define MRF_MAINCNT2   0x228  /**< Main Timer Counter 2 */
    #define MRF_MAINCNT3   0x229  /**< Main Timer Counter 3 */

    #define MRF_TESTMODE   0x22F  /**< Test Mode (PA/LNA control) */

    /** @brief Associated Extended Address registers (0x231-0x237) */
    #define MRF_ASSOEADR1  0x231
    #define MRF_ASSOEADR2  0x232
    #define MRF_ASSOEADR3  0x233
    #define MRF_ASSOEADR4  0x234
    #define MRF_ASSOEADR5  0x235
    #define MRF_ASSOEADR6  0x236
    #define MRF_ASSOEADR7  0x237

    /** @brief Associated Short Address registers */
    #define MRF_ASSOSADR0  0x238
    #define MRF_ASSOSADR1  0x239

    /** @brief Upper Layer Nonce registers (0x240-0x24C) para seguridad */
    #define MRF_UPNONCE0   0x240
    #define MRF_UPNONCE1   0x241
    #define MRF_UPNONCE2   0x242
    #define MRF_UPNONCE3   0x243
    #define MRF_UPNONCE4   0x244
    #define MRF_UPNONCE5   0x245
    #define MRF_UPNONCE6   0x246
    #define MRF_UPNONCE7   0x247
    #define MRF_UPNONCE8   0x248
    #define MRF_UPNONCE9   0x249
    #define MRF_UPNONCE10  0x24A
    #define MRF_UPNONCE11  0x24B
    #define MRF_UPNONCE12  0x24C

    // =========================================================================
    // Máscaras de bits de interrupción
    // =========================================================================

    /** @brief Bit mask: RX FIFO received interrup */
    #define MRF_I_RXIF     0b00001000

    /** @brief Bit mask: TX Normal FIFO complete interrup */
    #define MRF_I_TXNIF    0b00000001

}  // namespace MRF24J40

/** @} */  // end of MRF24J40 namespace
