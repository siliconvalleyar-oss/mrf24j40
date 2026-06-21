# Guía de Configuración — Receptor MRF24J40

## `config.hpp` — Opciones de Compilación

Archivo: `include/config/config.hpp`

### Selección de Modo (Auto-detected)

```cpp
// ARM 32 bits → MRF24_RX
// ARM 64 bits → MRF24_TX  ← forzar a RX manualmente
// x86_64      → MRF24_TX
```

Para forzar modo receptor en 64 bits:
```cpp
// Comentar:
// #define USE_MRF24_TX
// Descomentar:
#define USE_MRF24_RX
```

### Configuración de Red (RX)

```cpp
#define ADDRESS_LONG       0x1122334455667702  // MAC propia (64 bits)
#define ADDRESS            0x6002              // Short address propia
#define PAN_ID             0x1234              // PAN ID (debe coincidir con TX)
#define ADDR_SLAVE         0x6001              // Short address del transmisor
#define CHANNEL            24                  // Canal (debe coincidir con TX)
```

### Debug

```cpp
#define DBG_PRINT_GET_INFO
//#define DBG
//#define DBG_BUFFER
//#define DBG_SPI
//#define DBG_DISPLAY_OLED
```

### Configuración del MRF24J40

```cpp
#define ADD_RSSI_AND_LQI_TO_PACKET
#define MRF24J40_DISABLE_AUTOMATIC_ACK
#define MRF24J40_PAN_COORDINATOR
#define MRF24J40_COORDINATOR
#define MRF24J40_ACCEPT_WRONG_CRC_PKT
#define MRF24J40_PROMISCUOUS_MODE
#define INT_POLARITY_HIGH
```

### Funcionalidades RX Opcionales

```cpp
//#define USE_OLED           // Habilitar display OLED SSD1306
//#define ENABLE_DATABASE    // Habilitar logging a MySQL
//#define USE_QR             // Habilitar generación de QR
```

### MQTT

> **🔸 `ENABLE_MQTT` se define automáticamente en el Makefile**
> cuando se detecta `libmosquitto` instalado.
> No es necesario agregarlo manualmente en `config.hpp`.

Configuración del broker (valores por defecto):
- **Host:** `localhost`
- **Puerto:** `1883`
- **Client ID:** `mrf24j40_rx`

### Tamaño de Paquete

```cpp
#define MAX_PACKET_TX 64
#define SIZE_HEAD_PACKET_DATA 23
```

### Control de RF

```cpp
#define PROMISCUE           // Modo promiscuo
//#define COORDINATOR       // Modo coordinador
//#define END               // End device
```

---

## Configuración del Makefile

| Variable   | Valor por Defecto | Descripción              |
|------------|-------------------|--------------------------|
| `CXX`      | `g++`             | Compilador C++           |
| `CXXFLAGS` | `-std=c++17 -Wall -Wextra -O2 -Isrc -Iinclude -pthread` | Flags |
| `LDFLAGS`  | `-pthread -lbcm2835` | Librerías              |
| `TARGET`   | `bin/mrf24_receiver` | Binario de salida    |

### Detección Automática

Misma detección que el transmisor: BCM2835, libmosquitto, qrencode, libpng, zlib, MySQL.

---

## SPI / GPIO

```cpp
#define SPI_DEVICE   "/dev/spidev0.0"
#define SPI_SPEED_HZ 2000000   // 2 MHz
```

| GPIO | Función     | Pin Físico | Configuración        |
|------|-------------|------------|----------------------|
| 12   | LED RX      | 32         | Salida, LOW inicial  |
| 10   | SPI MOSI    | 19         | SPI (automático)     |
| 9    | SPI MISO    | 21         | SPI (automático)     |
| 11   | SPI SCLK    | 23         | SPI (automático)     |
| 8    | SPI CS      | 24         | SPI (automático)     |

---

## Display OLED (Opcional)

1. Descomentar en config.hpp: `#define USE_OLED`
2. Instalar: `./scripts_tools/install_tools.sh oled`
3. Conectar OLED vía I²C

---

## Base de Datos MySQL (Opcional)

1. Descomentar: `#define ENABLE_DATABASE`
2. Instalar: `./scripts_tools/install_tools.sh database`

---

## Resolución de Problemas

### No recibe paquetes
1. Verificar que TX y RX estén en el **mismo canal**
2. Verificar que TX y RX tengan el **mismo PAN ID**
3. Verificar conexiones SPI (MOSI↔MOSI, MISO↔MISO)
4. Reducir velocidad SPI: `#define SPI_SPEED_HZ 1000000`
5. Verificar que el LED parpadee al recibir

### RSSI/LQI anómalos
- Probar con `MRF24J40_ACCEPT_WRONG_CRC_PKT`
- Verificar antena del módulo
- Reducir distancia entre módulos

### Error "bcm2835_init falló"
- Ejecutar con `sudo`
- Verificar que BCM2835 esté instalado
