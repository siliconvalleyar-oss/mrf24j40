# Guía de Configuración — Transmisor MRF24J40

## `config.hpp` — Opciones de Compilación

Archivo: `include/config/config.hpp`

### Selección de Modo (Auto-detected)

```cpp
// ARM 32 bits → USE_MRF24_RX
// ARM 64 bits → USE_MRF24_TX
// x86_64      → USE_MRF24_TX
// macOS       → USE_MRF24_RX
```

### Direcciones MAC

```cpp
#define USE_MAC_ADDRESS_LONG    // Direcciones de 64 bits
//#define USE_MAC_ADDRESS_SHORT // Direcciones de 16 bits
```

### Configuración de Red (TX)

```cpp
#define ADDRESS_LONG       0x1122334455667701  // Dirección MAC propia (64 bits)
#define ADDRESS_LONG_SLAVE 0x1122334455667702  // Dirección MAC destino (64 bits)
#define ADDRESS            0x6001              // Short address propia
#define PAN_ID             0x1234              // PAN ID
#define ADDR_SLAVE         0x6002              // Short address destino
#define CHANNEL            24                  // Canal IEEE 802.15.4 (11-26)
```

### Debug

```cpp
#define DBG_PRINT_GET_INFO  // Debug de información general
//#define DBG                // Debug de GPIO
//#define DBG_BUFFER         // Debug de buffers
//#define DBG_GPIO           // Debug de GPIO específico
//#define DBG_FILES          // Debug de archivos
//#define DBG_DISPLAY_OLED   // Debug de OLED
//#define DBG_SPI            // Debug de SPI
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

### Tamaño de Paquete

```cpp
#define MAX_PACKET_TX 64    // Tamaño máximo de paquete TX
#define SIZE_HEAD_PACKET_DATA 23  // Tamaño del header
```

---

## Configuración del Makefile

### Variables

| Variable   | Valor por Defecto | Descripción              |
|------------|-------------------|--------------------------|
| `CXX`      | `g++`             | Compilador C++           |
| `CXXFLAGS` | `-std=c++17 -Wall -Wextra -O2 -I./src -pthread` | Flags |
| `LDFLAGS`  | `-pthread -lbcm2835` | Librerías              |
| `TARGET`   | `bin/mrf24_transmitter` | Binario de salida  |

### Targets

| Target    | Descripción                     |
|-----------|---------------------------------|
| `make`    | Compilar (default)              |
| `make clean` | Limpiar objetos y binarios |
| `sudo make run` | Ejecutar (desde Makefile raíz) |

---

## Configuración SPI

Parámetros definidos en `src/mrf24j40.cpp`:

```cpp
#define SPI_DEVICE   "/dev/spidev0.0"  // Dispositivo SPI
#define SPI_SPEED_HZ 10000000          // Velocidad: 10 MHz
```

---

## GPIO

| GPIO | Función     | Pin Físico | Configuración        |
|------|-------------|------------|----------------------|
| 12   | LED TX      | 32         | Salida, LOW inicial  |
| 10   | SPI MOSI    | 19         | SPI (automático)     |
| 9    | SPI MISO    | 21         | SPI (automático)     |
| 11   | SPI SCLK    | 23         | SPI (automático)     |
| 8    | SPI CS      | 24         | SPI (automático)     |

---

## Ajuste de Velocidad SPI

Si experimentas errores de comunicación, reduce la velocidad SPI:

```cpp
// En src/mrf24j40.cpp, línea ~25:
#define SPI_SPEED_HZ 5000000  // Reducir a 5 MHz
```

O más conservador:

```cpp
#define SPI_SPEED_HZ 1000000  // 1 MHz
```
