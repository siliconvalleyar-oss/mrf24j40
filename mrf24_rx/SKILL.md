# 🧠 SKILL — Receptor MRF24J40 (`mrf24_rx/`)

Referencia rápida para trabajar con el proyecto **receptor** legacy.

---

## 📦 Stack Tecnológico

| Componente         | Detalle                                  |
|--------------------|------------------------------------------|
| Lenguaje           | C++17                                    |
| Compilador         | g++ / clang++                            |
| Librerías base     | BCM2835 (GPIO/SPI)                       |
| Librerías opcionales | libmosquitto (MQTT), libqrencode (QR), libpng, MySQL |
| Hardware           | MRF24J40MA, Raspberry Pi, LED, OLED (SSD1306) |
| Protocolo          | IEEE 802.15.4 (ZigBee PHY/MAC)           |
| Comunicación SPI   | SPI Modo 0, 2 MHz (`/dev/spidev0.0`)     |
| MQTT               | mqtt_handler + mqtt_bridge               |
| Logging            | Archivo CSV (mrf24_receiver.log)         |
| Display opcional   | OLED SSD1306 vía I²C                     |
| DB opcional        | MySQL/MariaDB (conector C++)             |
| Arquitecturas      | armv7l (32-bit), aarch64 (64-bit), x86_64|

---

## 🚀 Comandos Esenciales

```bash
cd mrf24_rx

make                # Compilar con detección automática
make -j4            # Compilar en paralelo (4 núcleos)
make clean          # Limpiar objetos y binario
make info           # Mostrar flags, librerías y fuentes

sudo ./bin/mrf24_transmitter   # Ejecutar (requiere sudo por SPI/GPIO)
```

## 🎮 Comandos en Tiempo Real

| Tecla | Acción                          |
|-------|----------------------------------|
| `s`   | Mostrar estadísticas             |
| `c`   | Limpiar estadísticas             |
| `q`   | Salir                            |

## 📁 Estructura de Archivos

```
mrf24_rx/
├── Makefile                 → Compilación con auto-detección de librerías
├── SKILL.md                 → Esta guía
├── README.md                → Documentación principal
├── docs/                    → Documentación detallada
│   ├── ARCHITECTURE.md      →   Diagrama de capas y flujos
│   ├── API.md               →   Referencia de clases y funciones
│   └── CONFIGURATION.md     →   Opciones de compilación, GPIO, OLED
├── bin/                     → Binario compilado
│   └── mrf24_transmitter
├── src/                     → Código fuente
│   ├── main.cpp             →   Punto de entrada, bucle RX
│   ├── mrf24j40.h           →   Header driver simplificado
│   ├── mrf24j40.cpp         →   Driver MRF24J40 simplificado
│   ├── config/config.cpp    →   Carga de configuración
│   ├── gpio/gpio.cpp        →   Control de pines GPIO
│   ├── spi/spi.cpp          →   Comunicación SPI
│   ├── spi/spi_dbg.cpp      →   Debug de SPI
│   ├── mrf24/mrf24j40.cpp   →   Driver MRF24J40 completo (64-bit MAC)
│   ├── file/file.cpp        →   Sistema de archivos (log CSV)
│   ├── file/database.cpp    →   MySQL (opcional)
│   ├── qr/qr.cpp            →   Generación de QR (opcional)
│   ├── qr/qr_img.cpp        →   Exportar QR a PNG
│   ├── qr/ff.cpp            →   Fuente para QR
│   ├── tyme/tyme.cpp        →   Temporización
│   ├── interrupt/interrupt.cpp → Manejo de interrupciones
│   ├── work/rfflush.cpp     →   Flush de buffer RF
│   ├── display/epaper.cpp   →   Display E-paper (opcional)
│   ├── oled/oled/SSD1306_OLED*.cpp → Driver OLED SSD1306
│   ├── oled/oled/oled.cpp   →   Lógica OLED de alto nivel
│   ├── mosquitto/mqtt_handler.cpp → Cliente MQTT
│   └── mosquitto/mqtt_bridge.cpp  → Puente radio ⟷ MQTT
└── include/                 → Headers
    ├── config/config.hpp    →   Configuración global (#defines)
    ├── mrf24/               →   Headers del driver completo
    ├── mosquitto/           →   Headers MQTT
    ├── spi/spi.hpp          →   Header SPI
    ├── gpio/gpio.hpp        →   Header GPIO
    ├── radio/               →   Headers de radio
    ├── oled/                →   Headers OLED
    ├── file/                →   Headers de archivos
    ├── qr/                  →   Headers QR
    ├── tyme/                →   Headers de tiempo
    ├── work/                →   Headers de utilidades
    ├── display/             →   Headers de display
    └── security/            →   Headers de seguridad (stubs)
```

### Archivos Clave

| Archivo                           | Propósito                                 |
|-----------------------------------|-------------------------------------------|
| `src/main.cpp`                    | Bucle principal RX: poll, extraer, mostrar |
| `src/mrf24j40.h` / `.cpp`        | Driver simplificado Mrf24j40 (init, poll, getData) |
| `include/config/config.hpp`       | Configuración global: canal, PAN ID, debug |
| `include/mrf24/mrf24j40.hpp`     | Driver completo MRF24J40::Mrf24j (64-bit MAC) |
| `src/mosquitto/mqtt_handler.*`    | Cliente MQTT con reconexión automática |
| `src/mosquitto/mqtt_bridge.*`     | Puente radio ⟷ MQTT, traducción de comandos |
| `src/oled/oled/oled.cpp`         | Lógica de display OLED (SSD1306) |
| `mrf24_receiver.log`              | Archivo de log CSV generado en runtime |

---

## 🔌 GPIO

| GPIO | Función  | Pin Físico | Descripción              |
|------|----------|------------|--------------------------|
| 10   | MOSI     | 19         | SPI Master Out, Slave In |
| 9    | MISO     | 21         | SPI Master In, Slave Out |
| 11   | SCLK     | 23         | SPI Clock (2 MHz)        |
| 8    | CS       | 24         | Chip Select (CE0)        |
| 25   | INT      | 22         | Interrupción del módulo  |
| 18   | WAKE     | 12         | Wake up                  |
| 17   | RESET    | 11         | Reset del módulo         |
| 12   | LED RX   | 32         | Indicador de recepción   |

## ⚙️ Configuración Rápida (`config.hpp`)

```cpp
#define CHANNEL     20     // Canal IEEE 802.15.4 (11-26, debe coincidir con TX)
#define ADDRESS     0x6002 // Short address propia
#define PAN_ID      0x1234 // PAN ID (debe coincidir con TX)

// Opcionales:
// #define USE_OLED            // Habilitar display OLED SSD1306
// #define ENABLE_DATABASE     // Habilitar logging a MySQL
// #define USE_QR              // Habilitar generación de QR

// Debug:
// #define DBG
// #define DBG_BUFFER
// #define DBG_SPI
// #define DBG_DISPLAY_OLED
```

### Configuración de red (config.hpp)

```cpp
#define USE_MAC_ADDRESS_LONG          // MAC de 64 bits
#define ADDRESS_LONG       0x1122334455667702  // MAC propia
#define ADDRESS_LONG_SLAVE 0x1122334455667701  // MAC del transmisor

#define ADD_RSSI_AND_LQI_TO_PACKET    // RSSI/LQI en cada paquete
#define MRF24J40_PROMISCUOUS_MODE     // Modo promiscuo
#define PROMISCUE                     // Promiscuo (define alternativo)
#define INT_POLARITY_HIGH             // Interrupción activa HIGH

// Modos de operación:
// #define COORDINATOR                 // Modo coordinador
// #define END                         // End device
```

### Display OLED (Opcional)

```cpp
#define USE_OLED    // Descomentar para habilitar
```

1. Descomentar `USE_OLED` en `config.hpp`
2. Instalar dependencias: `../scripts_tools/install_tools.sh oled`
3. Conectar OLED vía I²C (pines SDA/SCL)

---

## 📊 Estadísticas

El receptor reporta en tiempo real:

| Métrica          | Descripción                        |
|------------------|------------------------------------|
| Paquetes recibidos | Total acumulado                  |
| LQI promedio      | Link Quality Indicator (0-255)    |
| RSSI promedio     | Received Signal Strength (dBm)    |
| Último LQI/RSSI  | Valores del último paquete        |

## 🔄 Flujo de Recepción

```
main.cpp loop:
  │
  ├── poll() periódico (cada 10-100ms):
  │     → leer INTSTAT
  │     → si RXIF:
  │         → leer FIFO: frame_length + header + payload + LQI + RSSI
  │         → flush RX FIFO
  │         → LED blink (GPIO12 HIGH 100ms)
  │
  ├── Mostrar en OLED (si está habilitado):
  │     → clear()
  │     → drawString("RX #N")
  │     → drawString(payload[:20])
  │     → drawString("LQI:X RSSI:Y dBm")
  │     → update()
  │
  ├── Log a archivo CSV:
  │     → timestamp, packet_num, payload_hex, len, lqi, rssi
  │
  ├── Publicar en MQTT (si está habilitado):
  │     → domotics/zigbee/rx: payload hex + RSSI + LQI
  │     → domotics/{device}/status: estado traducido
  │
  └── Mostrar stats (tecla 's'):
        → getStats(): total, avg LQI, avg RSSI
```

### Extracción de RSSI y LQI

```
LQI  = readLong(RXFIFO + 1 + frame_len)           // 0-255
RSSI = -90 + (readLong(RXFIFO + 1 + frame_len + 1) / 3)  // dBm
```

### Formato del Log CSV

```
#timestamp,packet_num,payload_hex,len,lqi,rssi
1712345678,1,00:01:02:03:...,100,200,-85
```

---

## 🔸 MQTT (Mosquitto)

Si `libmosquitto` está instalado (detección automática en Makefile):

### Topics de comando (RX recibe)

| Topic | Formato | Dispositivos |
|-------|---------|-------------|
| `domotics/{device}/set` | `{"command": "on"}` | light, temperature, fan, lock, curtain, energy, rgb |
| `domotics/{device}/set` | `{"command": "set", "value": 50}` | fan, light (dim), temperature |

### Topics de estado (RX publica)

| Topic | Datos |
|-------|-------|
| `domotics/{device}/status` | `{"isOn": true, "value": 0}` |
| `domotics/zigbee/rx` | `{"data": "hex...", "rssi": -85, "lqi": 200}` |

---

## 🛠️ Técnicas de Desarrollo

### 1. Driver Simplificado vs Completo

El proyecto tiene **dos implementaciones** del driver MRF24J40 (misma estructura que tx):

| Driver | Archivo | Enfoque |
|--------|---------|---------|
| **Simplificado** | `src/mrf24j40.cpp` / `mrf24j40.h` | main.cpp, interfaz simple |
| **Completo** | `src/mrf24/mrf24j40.cpp` / `include/mrf24/mrf24j40.hpp` | 64-bit MAC, templates, más control |

### 2. Polling RX

A diferencia de un sistema con interrupciones dedicadas, el receptor opera por polling:

```cpp
while (running) {
    radio.poll();
    if (radio.hasPacket()) {
        uint8_t buf[MAX_PAYLOAD];
        uint8_t len = radio.getData(buf);
        procesar_paquete(buf, len);
    }
    sleep(10ms);
}
```

### 3. Log CSV Rotativo

El log se escribe en `mrf24_receiver.log` con formato CSV:
```cpp
fprintf(log, "%ld,%d,", timestamp, packet_num);
for (int i = 0; i < len; i++)
    fprintf(log, "%02X:", buf[i]);
fprintf(log, ",%d,%d,%d\n", len, lqi, rssi);
```

### 4. OLED (SSD1306)

Si está habilitado, muestra en pantalla:
- Línea 1: "MRF24J40 RX" + versión
- Línea 2: "RX #packet_num"
- Línea 3: payload (truncado a 20 chars)
- Línea 4: "LQI:X RSSI:Y dBm"

### 5. Detección de Arquitectura

El Makefile detecta automáticamente ARM 32/64 bits:
```makefile
ARCH := $(shell uname -m)
# ARM 32 bits → MRF24_RX (modo por defecto)
# ARM 64 bits → MRF24_TX (forzar MRF24_RX manualmente en config.hpp)
```

### 6. Puente MQTT

`MqttBridge` traduce comandos MQTT a GPIO:
```cpp
// Comando: domotics/light_1/set {"command": "on"}
// → gpio.write(pin_light_1, HIGH)
// → publish("domotics/light_1/status", {"isOn": true, "value": 0})
```

---

## 🐛 Debug

Descomentar en `config.hpp`:

```cpp
#define DBG                 // Mensajes generales de debug
#define DBG_BUFFER          // Dump de buffers RX en hex
#define DBG_SPI             // Mostrar transacciones SPI
#define DBG_DISPLAY_OLED    // Debug de OLED (bytes enviados)
```

---

## 📚 Documentación Relacionada

| Documento | Descripción |
|-----------|-------------|
| [README.md](README.md) | Documentación principal del receptor |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Diagrama de capas y flujos RX |
| [docs/API.md](docs/API.md) | Referencia de API: Mrf24j40, Mrf24j, MqttHandler, rx_info_t |
| [docs/CONFIGURATION.md](docs/CONFIGURATION.md) | Opciones de compilación, defines, GPIO, OLED, DB |
| [../README.md](../README.md) | README global del proyecto |
| [../COMPILE.md](../COMPILE.md) | Guía de compilación remota |
| [../RULES.md](../RULES.md) | Reglas de desarrollo del proyecto |
| [../mrf24-dashboard/SKILL.md](../mrf24-dashboard/SKILL.md) | Skill del dashboard web |
| [../mrf24_security/ARCHITECTURE.md](../mrf24_security/ARCHITECTURE.md) | Arquitectura del proyecto unificado |

---

## 🔧 Scripts Relacionados

| Script | Función |
|--------|---------|
| `../scripts_tools/installBCM2835.sh` | Instalar librería BCM2835 |
| `../scripts_tools/installMosquitto.sh` | Instalar Mosquitto MQTT |
| `../scripts_tools/installLibs.sh` | Instalar todas las dependencias |
| `../scripts_tools/installOled.sh` | Instalar librería OLED SSD1306 |
| `../scripts_tools/config_radio.sh` | Configurar interfaz de radio |
| `../scripts_tools/gpio.sh` | Configurar pines GPIO |
| `../scripts_tools/build_single_tx_rx.sh` | Compilar rx individual |
| `../scripts_tools/git_commit.sh` | Automatizar commits |

---

## ⚠️ Nota: Proyecto Legacy

El proyecto `mrf24_rx/` es **legacy** y será reemplazado por `mrf24_security/` (proyecto unificado).  
Actualmente se mantiene por compatibilidad y para pruebas de hardware.  
Las nuevas funcionalidades (roles, validación SHA-256, enrutamiento, menú interactivo)  
se implementan exclusivamente en `mrf24_security/`.
