# Guía de Configuración — Receptor MRF24J40

## `config.hpp` — Opciones de Compilación

Archivo: `include/config/config.hpp`

### Selección de Modo (Auto-detected)

```cpp
// ARM 32 bits → USE_MRF24_RX
// ARM 64 bits → USE_MRF24_TX
// x86_64      → USE_MRF24_TX
// macOS       → USE_MRF24_RX
```

Para forzar modo receptor en 64 bits, editar config.hpp:

```cpp
// Comentar estas líneas:
// #define USE_MRF24_TX

// Descomentar:
#define USE_MRF24_RX
```

### Direcciones MAC

```cpp
#define USE_MAC_ADDRESS_LONG    // Direcciones de 64 bits
//#define USE_MAC_ADDRESS_SHORT // Direcciones de 16 bits
```

### Configuración de Red (RX)

```cpp
#define ADDRESS_LONG       0x1122334455667702  // Dirección MAC propia (64 bits)
#define ADDRESS_LONG_SLAVE 0x1122334455667701  // Dirección MAC del transmisor
#define ADDRESS            0x6002              // Short address propia
#define PAN_ID             0x1234              // PAN ID (debe coincidir con TX)
#define ADDR_SLAVE         0x6001              // Short address del transmisor
#define CHANNEL            24                  // Canal (debe coincidir con TX)
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

### Configuración del MRF24J40 (RX)

```cpp
#define ADD_RSSI_AND_LQI_TO_PACKET     // Extraer RSSI/LQI del paquete
#define MRF24J40_DISABLE_AUTOMATIC_ACK // Deshabilitar ACK automático
#define MRF24J40_PAN_COORDINATOR       // Modo PAN Coordinator
#define MRF24J40_COORDINATOR           // Modo Coordinator
#define MRF24J40_ACCEPT_WRONG_CRC_PKT  // Aceptar paquetes con CRC erróneo
#define MRF24J40_PROMISCUOUS_MODE      // Modo promiscuo
#define INT_POLARITY_HIGH              // Polaridad de interrupción HIGH
```

### Funcionalidades RX Opcionales

```cpp
// En config.hpp sección MODULE_RX:

//#define USE_OLED           // Habilitar display OLED SSD1306
//#define ENABLE_DATABASE    // Habilitar logging a base de datos MySQL
//#define USE_QR             // Habilitar generación de QR
```

### Tamaño de Paquete

```cpp
#define MAX_PACKET_TX 64    // Tamaño máximo de paquete
#define SIZE_HEAD_PACKET_DATA 23  // Tamaño del header
```

### Control de RF

```cpp
// Opciones de modo de operación:
#define PROMISCUE           // Modo promiscuo (recibe todo)
//#define COORDINATOR       // Modo coordinador (solo dirección configurada)
//#define END               // End device
```

---

## Configuración del Makefile

### Variables

| Variable   | Valor por Defecto | Descripción              |
|------------|-------------------|--------------------------|
| `CXX`      | `g++`             | Compilador C++           |
| `CXXFLAGS` | `-std=c++17 -Wall -Wextra -O2 -I./src -pthread` | Flags |
| `LDFLAGS`  | `-pthread -lbcm2835` | Librerías              |
| `TARGET`   | `bin/mrf24_receiver` | Binario de salida    |

### Targets

| Target    | Descripción                     |
|-----------|---------------------------------|
| `make`    | Compilar (default)              |
| `make clean` | Limpiar objetos y binarios |

---

## Configuración SPI

```cpp
// En src/mrf24j40.cpp
#define SPI_DEVICE   "/dev/spidev0.0"
#define SPI_SPEED_HZ 10000000  // 10 MHz
```

---

## GPIO

| GPIO | Función     | Pin Físico | Configuración        |
|------|-------------|------------|----------------------|
| 12   | LED RX      | 32         | Salida, LOW inicial  |
| 10   | SPI MOSI    | 19         | SPI (automático)     |
| 9    | SPI MISO    | 21         | SPI (automático)     |
| 11   | SPI SCLK    | 23         | SPI (automático)     |
| 8    | SPI CS      | 24         | SPI (automático)     |

---

## Display OLED (Opcional)

Para habilitar el display OLED SSD1306:

1. Descomentar en `config.hpp`:
   ```cpp
   #define USE_OLED
   ```

2. Instalar la librería:
   ```bash
   ./scripts_tools/install_tools.sh oled
   ```

3. Conectar OLED vía I²C (pines SDA/SCL)

---

## Base de Datos MySQL (Opcional)

Para habilitar logging a MySQL:

1. Descomentar en `config.hpp`:
   ```cpp
   #define ENABLE_DATABASE
   ```

2. Instalar MySQL:
   ```bash
   ./scripts_tools/install_tools.sh database
   ```

---

## Resolución de Problemas

### No recibe paquetes
1. Verificar que TX y RX estén en el **mismo canal**
2. Verificar que TX y RX tengan el **mismo PAN ID**
3. Verificar conexiones SPI (MOSI↔MOSI, MISO↔MISO)
4. Reducir velocidad SPI: `#define SPI_SPEED_HZ 1000000`
5. Verificar que el LED parpadee al recibir

### RSSI/LQI anómalos
- Probar con `#define MRF24J40_ACCEPT_WRONG_CRC_PKT`
- Verificar antena del módulo
- Reducir distancia entre módulos

### Error "bcm2835_init falló"
- Ejecutar con `sudo`
- Verificar que BCM2835 esté instalado
