# Guía de Configuración — Transmisor MRF24J40

## `config.hpp` — Opciones de Compilación

Archivo: `include/config/config.hpp`

### Selección de Modo (Auto-detected)

```cpp
// ARM 32 bits → MRF24_RX
// ARM 64 bits → MRF24_TX
// x86_64      → MRF24_TX
```

### Direcciones MAC

```cpp
#define USE_MAC_ADDRESS_LONG    // 64 bits (por defecto)
//#define USE_MAC_ADDRESS_SHORT // 16 bits
```

### Configuración de Red (TX)

```cpp
#define ADDRESS_LONG       0x1122334455667701  // MAC propia (64 bits)
#define ADDRESS_LONG_SLAVE 0x1122334455667702  // MAC destino (64 bits)
#define ADDRESS            0x6001              // Short address propia
#define PAN_ID             0x1234              // PAN ID
#define ADDR_SLAVE         0x6002              // Short address destino
#define CHANNEL            24                  // Canal IEEE 802.15.4 (11-26)
```

### Debug

```cpp
#define DBG_PRINT_GET_INFO  // Debug de información general
//#define DBG                // Debug general
//#define DBG_BUFFER         // Debug de buffers
//#define DBG_GPIO           // Debug de GPIO
//#define DBG_SPI            // Debug de SPI
//#define DBG_DISPLAY_OLED   // Debug de OLED
```

### Configuración del MRF24J40

```cpp
#define ADD_RSSI_AND_LQI_TO_PACKET     // Añadir RSSI/LQI a cada paquete
#define MRF24J40_DISABLE_AUTOMATIC_ACK // Deshabilitar ACK automático
#define MRF24J40_PAN_COORDINATOR       // Modo PAN Coordinator
#define MRF24J40_COORDINATOR           // Modo Coordinator
#define MRF24J40_ACCEPT_WRONG_CRC_PKT  // Aceptar paquetes con CRC erróneo
#define MRF24J40_PROMISCUOUS_MODE      // Modo promiscuo
#define INT_POLARITY_HIGH              // Polaridad de interrupción HIGH
```

### MQTT

> **🔸 `ENABLE_MQTT` se define automáticamente en el Makefile**
> cuando se detecta `libmosquitto` instalado.
> No es necesario agregarlo manualmente en `config.hpp`.

Configuración del broker (valores por defecto):
- **Host:** `localhost`
- **Puerto:** `1883`
- **Client ID:** `mrf24j40_tx`

### Tamaño de Paquete

```cpp
#define MAX_PACKET_TX 64       // Tamaño máximo de paquete TX
#define SIZE_HEAD_PACKET_DATA 23  // Tamaño del header
```

---

## Configuración del Makefile

| Variable   | Valor por Defecto | Descripción              |
|------------|-------------------|--------------------------|
| `CXX`      | `g++`             | Compilador C++           |
| `CXXFLAGS` | `-std=c++17 -Wall -Wextra -O2 -Isrc -Iinclude -pthread` | Flags |
| `LDFLAGS`  | `-pthread -lbcm2835` | Librerías              |
| `TARGET`   | `bin/mrf24_transmitter` | Binario de salida  |

### Detección Automática de Librerías

| Librería     | Define activado | Flag link |
|-------------|-----------------|-----------|
| BCM2835     | —               | `-lbcm2835` |
| libmosquitto | `ENABLE_MQTT`   | `-lmosquitto` |
| qrencode    | `USE_QR`        | `-lqrencode` |
| libpng      | —               | `-lpng` |
| zlib        | —               | `-lz` |
| MySQL       | `ENABLE_DATABASE` | `-lmysqlcppconn` |

---

## SPI

```cpp
#define SPI_DEVICE   "/dev/spidev0.0"
#define SPI_SPEED_HZ 2000000   // 2 MHz
```

Si hay errores de comunicación, reducir a 1 MHz: `#define SPI_SPEED_HZ 1000000`.

---

## GPIO

| GPIO | Función     | Pin Físico | Configuración        |
|------|-------------|------------|----------------------|
| 12   | LED TX      | 32         | Salida, LOW inicial  |
| 10   | SPI MOSI    | 19         | SPI (automático)     |
| 9    | SPI MISO    | 21         | SPI (automático)     |
| 11   | SPI SCLK    | 23         | SPI (automático)     |
| 8    | SPI CS      | 24         | SPI (automático)     |
