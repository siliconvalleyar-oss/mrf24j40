#pragma once

/**
 * @file    config.hpp
 * @brief   Configuración global del proyecto MRF24J40
 * @details Este archivo define todas las opciones de compilación del proyecto
 *          MRF24J40, incluyendo la selección automática del modo TX/RX según
 *          la arquitectura, direcciones de red, canales, opciones de debug y
 *          configuración del módulo RF.
 *
 *          La selección del modo se realiza automáticamente:
 *          - ARM 32 bits → USE_MRF24_RX (receptor)
 *          - ARM 64 bits → USE_MRF24_TX (transmisor)
 *          - x86_64      → USE_MRF24_TX (transmisor)
 *          - macOS       → USE_MRF24_RX (receptor, para pruebas)
 *
 * @note Este archivo es idéntico en los proyectos mrf24_tx/ y mrf24_rx/.
 *       La selección del modo depende de la arquitectura de compilación.
 */

// =============================================================================
// Selección automática del modo TX/RX según arquitectura
// =============================================================================
#if defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ == 4)
    /** @brief Modo receptor para arquitecturas de 32 bits (ARMv7l) */
    #define USE_MRF24_RX
#elif defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ == 8)
    /** @brief Modo transmisor para arquitecturas de 64 bits (AArch64, x86_64) */
    #define USE_MRF24_TX
    #if defined(__APPLE__)
        /** @brief En macOS se fuerza modo receptor para pruebas */
        #undef USE_MRF24_TX
        #define USE_MRF24_RX
    #endif
#else
    #error "Arquitectura no soportada"
#endif

// =============================================================================
// Selección de dirección MAC
// =============================================================================
/** @brief Usar direcciones MAC largas de 64 bits (IEEE EUI-64) */
#define USE_MAC_ADDRESS_LONG
//#define USE_MAC_ADDRESS_SHORT /**< Alternativa: direcciones cortas de 16 bits */

/** @brief Header byte para identificación de paquete de datos */
#define HEAD 0x40

// =============================================================================
// Configuración de módulo (TX/RX)
// =============================================================================
#ifdef USE_MRF24_TX
    #define MODULE_TX           /**< Modo transmisor habilitado */
    #define MODULE_TX_RST       /**< Reset de módulo en modo TX */
    //#define RESET_MRF_SOFTWARE
#elif defined(USE_MRF24_RX)
    #define MODULE_RX           /**< Modo receptor habilitado */
    #define MODULE_RX_RST       /**< Reset de módulo en modo RX */
    #define RESET_MRF_SOFTWARE  /**< Usar soft reset del MRF24J40 */
#else
    #error "no se configuro el dispositivo"
#endif

// =============================================================================
// Mensaje de prueba
// =============================================================================
#ifdef USE_MAC_ADDRESS_SHORT
    #define MSJ "@ABCDE"         /**< Mensaje corto para test */
#else
    #define MSJ "ABCDEFGHIJ0123456789@ABCD"  /**< Mensaje largo para test */
#endif

// =============================================================================
// Parámetros de red
// =============================================================================

#ifdef MODULE_TX
    /** @brief Dirección MAC larga del dispositivo transmisor */
    #define ADDRESS_LONG        0x1122334455667701
    /** @brief Dirección MAC larga del dispositivo receptor */
    #define ADDRESS_LONG_SLAVE  0x1122334455667702
    /** @brief Dirección corta del transmisor (16 bits) */
    #define ADDRESS             0x6001
    /** @brief PAN ID de la red */
    #define PAN_ID              0x1234
    /** @brief Dirección corta del receptor (destino) */
    #define ADDR_SLAVE          0x6002
    #define MRF24_TRANSMITER_ENABLE
    //#define MRF24_RECEIVER_ENABLE
    //#define ENABLE_INTERRUPT_MRF24
    /** @brief Canal IEEE 802.15.4 (11-26) */
    #define CHANNEL 24

#elif defined(MODULE_RX)
    /** @brief Dirección MAC larga del dispositivo receptor */
    #define ADDRESS_LONG        0x1122334455667702
    /** @brief Dirección MAC larga del dispositivo transmisor (esperada) */
    #define ADDRESS_LONG_SLAVE  0x1122334455667701
    /** @brief Dirección corta del receptor (16 bits) */
    #define ADDRESS             0x6002
    /** @brief PAN ID de la red (debe coincidir con el transmisor) */
    #define PAN_ID              0x1234
    /** @brief Dirección corta del transmisor (origen esperado) */
    #define ADDR_SLAVE          0x6001
    #define MRF24_RECEIVER_ENABLE
    #define ENABLE_INTERRUPT_MRF24  /**< Habilitar manejo de interrupciones */
    /** @brief Canal IEEE 802.15.4 (debe coincidir con el transmisor) */
    #define CHANNEL 24
#else
    #error "no se configuro el dispositivo"
#endif

// =============================================================================
// Logging
// =============================================================================
/** @brief Nombre base del archivo de log */
#define LOG_FILENAME "log_mrf_"

// =============================================================================
// Opciones de Debug
// =============================================================================

/** @brief Imprimir información detallada de paquetes recibidos */
#define DBG_PRINT_GET_INFO
//#define DBG                    /**< Debug general de GPIO */
//#define DBG_BUFFER             /**< Debug de buffers de datos */
//#define DBG_GPIO               /**< Debug de GPIO específico */
//#define DBG_FILES              /**< Debug de operaciones de archivos */
//#define DBG_DISPLAY_OLED       /**< Debug de display OLED */
//#define DBG_SPI                /**< Debug de comunicación SPI */

// =============================================================================
// Configuración de QR (opcional)
// =============================================================================
//#define QR_CODE_SRT "WIFI:T:WPA;S:MiRedWiFi;P:MiContraseña123;;";

#ifdef MODULE_TX
    #ifdef USE_QR
        #define QR_CODE_SRT MSJ  /**< String para código QR en transmisor */
    #endif
#endif

// =============================================================================
// Funcionalidades opcionales del receptor
// =============================================================================
#ifdef MODULE_RX
    //#define USE_OLED             /**< Habilitar display OLED SSD1306 */
    //#define ENABLE_DATABASE      /**< Habilitar almacenamiento en BD MySQL */
    //#define USE_QR               /**< Habilitar generación de códigos QR */
#endif

// =============================================================================
// Tamaños de paquete
// =============================================================================
#ifdef USE_MAC_ADDRESS_LONG
    /** @brief Tamaño máximo del paquete TX con direcciones largas */
    #define MAX_PACKET_TX 64
    /** @brief Tamaño del header MAC con direcciones de 64 bits */
    #define SIZE_HEAD_PACKET_DATA 23
    #define BYTES_MHR SIZE_HEAD_PACKET_DATA
#else
    /** @brief Tamaño máximo del paquete TX con direcciones cortas */
    #define MAX_PACKET_TX 113
    /** @brief Tamaño del header MAC con direcciones de 16 bits */
    #define SIZE_HEAD_PACKET_DATA 11
#endif

// =============================================================================
// Configuración del MRF24J40
// =============================================================================

/** @brief Añadir RSSI y LQI al final de cada paquete en el FIFO RX */
#define ADD_RSSI_AND_LQI_TO_PACKET

/** @brief Deshabilitar respuesta ACK automática del módulo */
#define MRF24J40_DISABLE_AUTOMATIC_ACK

/** @brief Configurar como PAN Coordinator */
#define MRF24J40_PAN_COORDINATOR

/** @brief Configurar como Coordinator */
#define MRF24J40_COORDINATOR

/** @brief Aceptar paquetes aunque tengan CRC incorrecto */
#define MRF24J40_ACCEPT_WRONG_CRC_PKT

/** @brief Activar modo promiscuo (recibe todos los paquetes) */
#define MRF24J40_PROMISCUOUS_MODE

/** @brief Polaridad de la interrupción: HIGH = activa por nivel alto */
#define INT_POLARITY_HIGH

// =============================================================================
// Constantes del protocolo
// =============================================================================

/** @brief Tamaño máximo de paquete PHY según IEEE 802.15.4 */
#define A_MAX_PHY_PACKET_SIZE 127

// =============================================================================
// Modo de operación
// =============================================================================
#ifdef USE_MRF24_TX
    #define PROMISCUE           /**< Modo promiscuo o router */
#elif defined(MODULE_RX)
    #define PROMISCUE           /**< Modo promiscuo */
    //#define COORDINATOR        /**< Modo coordinador (solo dirección configurada) */
#else
    #define END                 /**< End device */
#endif

// =============================================================================
// Configuración SPI
// =============================================================================
#ifdef MODULE_TX
    //#define ENABLE_BCM2835     /**< Usar librería BCM2835 para SPI */
#else
    //#define ENABLE_BCM2835
#endif

#ifdef ENABLE_BCM2835
    #define SPI_BCM2835         /**< SPI vía librería BCM2835 */
    #define LIBRARIES_BCM2835   /**< Librerías BCM2835 habilitadas */
#else
    #define SPI_NO_BCM2835      /**< SPI sin BCM2835 (ioctl directo) */
    #define LIBRARIES_NO_BCM2835
#endif
