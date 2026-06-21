# 🧠 SKILL — Transmisor MRF24J40 (`mrf24_tx/`)

Referencia rápida para trabajar con el proyecto **transmisor** legacy.

---

## 📦 Stack Tecnológico

| Componente       | Detalle                         |
|------------------|---------------------------------|
| Lenguaje         | C++17                           |
| Compilador       | g++ / clang++                   |
| Librerías        | BCM2835 (GPIO/SPI), libmosquitto (MQTT) |
| Hardware         | MRF24J40MA, Raspberry Pi, LED   |
| Protocolo        | IEEE 802.15.4 (ZigBee PHY/MAC)  |
| Comunicación SPI | SPI Modo 0, 2 MHz (`/dev/spidev0.0`) |
| MQTT             | mqtt_handler + mqtt_bridge      |
| Arquitecturas    | armv7l (32-bit), aarch64 (64-bit), x86_64 |
| Make             | Auto-detección de librerías     |

---

## 🚀 Comandos Esenciales

```bash
cd mrf24_tx

make                # Compilar con detección automática
make -j4            # Compilar en paralelo (4 núcleos)
make clean          # Limpiar objetos y binario
make info           # Mostrar flags, librerías y fuentes

sudo ./bin/mrf24_transmitter   # Ejecutar (requiere sudo por SPI/GPIO)
```

## 🎮 Comandos en Tiempo Real

| Tecla | Acción                          |
|-------|----------------------------------|
| `n`   | Modo normal (envío cada 2s)      |
| `b`   | Burst: 10 paquetes rápidos       |
| `s`   | Mostrar estadísticas             |
| `q`   | Salir                            |

## 📁 Estructura de Archivos

```
mrf24_tx/
├── Makefile                 → Compilación con auto-detección de librerías
├── SKILL.md                 → Esta guía
├── README.md                → Documentación principal
├── docs/                    → Documentación detallada
│   ├── ARCHITECTURE.md      →   Diagrama de capas y flujos
│   ├── API.md               →   Referencia de clases y funciones
│   └── CONFIGURATION.md     →   Opciones de compilación y GPIO
├── bin/                     → Binario compilado
│   └── mrf24_transmitter
├── src/                     → Código fuente
│   ├── main.cpp             →   Punto de entrada, bucle principal
│   ├── mrf24j40.h           →   Header driver simplificado
│   ├── mrf24j40.cpp         →   Driver MRF24J40 simplificado
│   ├── config/config.cpp    →   Carga de configuración
│   ├── gpio/gpio.cpp        →   Control de pines GPIO
│   ├── spi/spi.cpp          →   Comunicación SPI
│   ├── spi/spi_dbg.cpp      →   Debug de SPI
│   ├── mrf24/mrf24j40.cpp   →   Driver MRF24J40 completo (64-bit MAC)
│   ├── file/file.cpp        →   Sistema de archivos
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

| Archivo                        | Propósito                                   |
|--------------------------------|---------------------------------------------|
| `src/main.cpp`                 | Bucle principal: inicializa radio, envía periódicamente |
| `src/mrf24j40.h` / `.cpp`     | Driver simplificado Mrf24j40 (init, send, poll) |
| `include/config/config.hpp`    | Configuración global: canal, direcciones, PAN ID, debug |
| `include/mrf24/mrf24j40.hpp`  | Driver completo MRF24J40::Mrf24j (64-bit MAC, template) |
| `src/mosquitto/mqtt_handler.*` | Cliente MQTT con reconexión automática |
| `src/mosquitto/mqtt_bridge.*`  | Puente radio ⟷ MQTT, publish de estadísticas |

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
| 12   | LED TX   | 32         | Indicador de transmisión |

## ⚙️ Configuración Rápida (`config.hpp`)

```cpp
#define CHANNEL     20     // Canal IEEE 802.15.4 (11-26)
#define ADDRESS     0x6001 // Short address propia
#define ADDR_SLAVE  0x6002 // Short address destino (RX)
#define PAN_ID      0x1234 // PAN ID (debe coincidir con RX)

// Opcionales para debug:
// #define DBG                    // Debug general
// #define DBG_BUFFER             // Debug de buffers TX/RX
// #define DBG_SPI                // Debug de transacciones SPI
```

### Configuración de red (config.hpp)

```cpp
#define USE_MAC_ADDRESS_LONG        // MAC de 64 bits
#define ADDRESS_LONG       0x1122334455667701  // MAC propia
#define ADDRESS_LONG_SLAVE 0x1122334455667702  // MAC destino

#define ADD_RSSI_AND_LQI_TO_PACKET  // RSSI/LQI en cada paquete
#define MRF24J40_PROMISCUOUS_MODE   // Modo promiscuo
#define INT_POLARITY_HIGH           // Interrupción activa HIGH
```

---

## 📊 Estadísticas

El transmisor reporta en tiempo real:

| Métrica          | Descripción                        |
|------------------|------------------------------------|
| Paquetes enviados | Total acumulado                   |
| TX exitosos       | ACK recibido                      |
| TX fallidos       | Sin ACK después de retransmisiones |
| Retransmisiones   | Total de reintentos                |
| Tasa de éxito (%) | (exitosos / enviados) × 100        |

## 🔄 Flujo de Transmisión

```
main.cpp loop:
  │
  ├── Modo Normal (tecla 'n'):
  │     sleep(2000ms)
  │     → radio.send(dest, payload, 100)
  │     → poll() hasta TXNIF
  │     → leer TXSTAT (éxito/fallo)
  │     → LED blink (GPIO12 HIGH 100ms)
  │     → publicar estadísticas en MQTT
  │
  ├── Modo Burst (tecla 'b'):
  │     10 paquetes con 50ms de separación
  │     → mismo flujo que normal
  │
  ├── Mostrar stats (tecla 's'):
  │     → printRegisters()
  │     → getStats() con tasa de éxito
  │
  └── Salir (tecla 'q'):
        → shutdown limpio de radio y MQTT
```

### Protocolo de trama (IEEE 802.15.4)

```
┌──────┬─────┬───────┬──────────┬──────────┬──────────┬─────┐
│ FCF  │ Seq │  PAN  │ DestAddr │ SrcAddr  │ Payload  │ FCS │
│  2B  │  1  │   2   │    2     │    2     │  100 B   │  2  │
└──────┴─────┴───────┴──────────┴──────────┴──────────┴─────┘
Header: 9 bytes  |  Payload: 100 bytes  |  FCS: automático (CRC16)
```

## 🔸 MQTT (Mosquitto)

Si `libmosquitto` está instalado (detección automática en Makefile):

### Clases implementadas

| Clase | Archivo | Propósito |
|-------|---------|-----------|
| `MqttHandler` | `mosquitto/mqtt_handler.*` | Conexión async, reconexión automática |
| `MqttBridge` | `mosquitto/mqtt_bridge.*` | Traduce eventos de radio en topics MQTT |

### Topics publicados

| Topic | Datos | Frecuencia |
|-------|-------|------------|
| `domotics/zigbee/tx/status` | `{"packets_sent": 10, "tx_success": 9, "rate": 90.0}` | Por envío |
| `domotics/{device}/status` | `{"isOn": true, "value": 0}` | Por comando |

---

## 🛠️ Técnicas de Desarrollo

### 1. Driver Simplificado vs Completo

El proyecto tiene **dos implementaciones** del driver MRF24J40:

| Driver | Archivo | Uso | MAC |
|--------|---------|-----|-----|
| **Simplificado** | `src/mrf24j40.cpp` / `mrf24j40.h` | main.cpp, Radio_t | 16 bits |
| **Completo** | `src/mrf24/mrf24j40.cpp` / `include/mrf24/mrf24j40.hpp` | Radio_t template | 64 bits |

- El driver simplificado se usa para la funcionalidad básica (init, send, poll)
- El driver completo (templates) permite direcciones de 64 bits y más control

### 2. Polling vs Interrupciones

El sistema usa **polling** en lugar de interrupciones hardware:
```cpp
radio.poll();   // Lee INTSTAT, procesa TX/RX si hay flags
```

### 3. Detección Automática de Librerías (Makefile)

```makefile
MOSQ_AVAILABLE := $(shell pkg-config --exists libmosquitto 2>/dev/null && echo "yes")
ifeq ($(MOSQ_AVAILABLE),yes)
    LIBS += -lmosquitto
    CXXFLAGS += -DENABLE_MQTT
endif
```

Define `ENABLE_MQTT`, `USE_QR`, `ENABLE_DATABASE` automáticamente según lo instalado.

### 4. LED Indicador

GPIO12 se usa como indicador visual:
```cpp
gpio.write(GPIO12, HIGH);   // TX iniciado
delayMs(100);
gpio.write(GPIO12, LOW);    // TX completado
```

### 5. Payload de 100 Bytes

El paquete TX se construye con datos incrementales:
```cpp
payload[i] = (msg_num + i) % 256;  // Datos de prueba
```

---

## 🐛 Debug

Descomentar en `config.hpp`:

```cpp
#define DBG             // Mensajes generales de debug
#define DBG_BUFFER      // Dump de buffers TX/RX en hex
#define DBG_SPI         // Mostrar transacciones SPI
#define DBG_GPIO        // Cambios de estado GPIO
```

---

## 📚 Documentación Relacionada

| Documento | Descripción |
|-----------|-------------|
| [README.md](README.md) | Documentación principal del transmisor |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | Diagrama de capas y flujos |
| [docs/API.md](docs/API.md) | Referencia de API: Mrf24j40, Mrf24j, MqttHandler, Spi_t |
| [docs/CONFIGURATION.md](docs/CONFIGURATION.md) | Opciones de compilación, defines, GPIO |
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
| `../scripts_tools/config_radio.sh` | Configurar interfaz de radio |
| `../scripts_tools/gpio.sh` | Configurar pines GPIO |
| `../scripts_tools/build_single_tx_rx.sh` | Compilar tx individual |
| `../scripts_tools/git_commit.sh` | Automatizar commits |

---

## ⚠️ Nota: Proyecto Legacy

El proyecto `mrf24_tx/` es **legacy** y será reemplazado por `mrf24_security/` (proyecto unificado).  
Actualmente se mantiene por compatibilidad y para pruebas de hardware.  
Las nuevas funcionalidades (roles, validación SHA-256, enrutamiento, menú interactivo)  
se implementan exclusivamente en `mrf24_security/`.
