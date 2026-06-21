# MRF24J40 — Transmisor y Receptor ZigBee para Raspberry Pi

Proyecto C++ para comunicación inalámbrica usando el módulo **MRF24J40MA** en Raspberry Pi con soporte **MQTT**.

## 📦 Estructura del Proyecto

```
├── .gitignore
├── Makefile                  # Makefile raíz (compila todo via subproyectos)
├── README.md
├── TODO.md                   # Tareas globales del sistema
├── VERSION.txt               # Historial de versiones
├── prompt.txt                # Prompt de funcionalidades pendientes
├── flow/                     # Diagramas de comunicación (DrawIO)
├── scripts_tools/            # Scripts de instalación y desarrollo
├── mrf24-dashboard/          # 🎛️ Dashboard web interactivo (HTML+CSS+JS)
├── mrf24_security/           # 🔐 PROYECTO UNIFICADO (primera plana)
│   ├── Makefile              # → bin/mrf24j40_iot
│   ├── main.cpp              # Punto de entrada unificado
│   ├── hal/                  # GPIO, SPI, I2C
│   ├── drivers/              # MRF24J40, SSD1306, ST7789, QR
│   ├── services/             # crypto, filesystem, timer
│   ├── application/          # radio_manager, menu
│   ├── security/             # aes.hpp (headers)
│   └── src/                  # encrypt.cpp, decrypt.cpp (stubs)
├── mrf24_tx/                 # 🚀 PROYECTO TRANSMISOR (legacy)
│   ├── Makefile
│   ├── SKILL.md
│   ├── docs/
│   ├── src/                  # main.cpp, radio/, mrf24/, mosquitto/, ...
│   └── include/              # headers legacy
└── mrf24_rx/                 # 📡 PROYECTO RECEPTOR (legacy)
    ├── Makefile
    ├── SKILL.md
    ├── docs/
    ├──    src/                  # main.cpp, radio/, mrf24/, display/, ...
    └── include/              # headers legacy
```

---

## 🎛️ Dashboard Web (`mrf24-dashboard/`)

Panel interactivo para explorar el proyecto, ver código con resaltado de sintaxis,
y monitorear estadísticas en tiempo real.

```bash
# Iniciar servidor
make serve-dashboard
# Abrir en el navegador: http://localhost:8080
```

**Características:**
- 🌳 Explorador de archivos con búsqueda
- 📜 Visor de código con syntax highlighting (highlight.js)
- 📄 Renderizado de Markdown (marked.js)
- 📊 Gráfico de tráfico de red en tiempo real (Canvas)
- 🏗️ Diagrama de arquitectura SVG animado
- 🎨 Tema oscuro tipo terminal

---

## 🚀 Proyecto Transmisor (`mrf24_tx/`)

Envía paquetes de datos a través del módulo MRF24J40MA usando protocolo ZigBee, con publicación de estado vía MQTT.

| Parámetro       | Valor  |
|-----------------|--------|
| PAN ID          | `0xCAFE` |
| Dirección propia | `0x6001` |
| Dirección destino | `0x6002` |
| Canal           | 24 |
| Intervalo       | 2000 ms |

**Compilar y ejecutar:**
```bash
cd mrf24_tx
make
sudo ./bin/mrf24_transmitter
```

---

## 📡 Proyecto Receptor (`mrf24_rx/`)

Recibe paquetes de datos del transmisor, los muestra y los traduce a eventos MQTT.

| Parámetro       | Valor  |
|-----------------|--------|
| PAN ID          | `0xCAFE` |
| Dirección propia | `0x6002` |
| Canal           | 24 |

**Compilar y ejecutar:**
```bash
cd mrf24_rx
make
sudo ./bin/mrf24_receiver
```

---

## 🔧 Dependencias

### Compilación

```bash
# Proyecto unificado (primera plana)
cd mrf24_security
make
sudo ./bin/mrf24j40_iot

# Proyectos legacy (transmisor y receptor)
cd mrf24_tx
make
sudo ./bin/mrf24_transmitter

cd mrf24_rx
make

# Todo desde la raíz
make all

# Dashboard web interactivo
make serve-dashboard
```

### Instalación Rápida

```bash
# Todo en uno
./scripts_tools/install_tools.sh all

# O por partes:
./scripts_tools/install_tools.sh basics     # Dependencias básicas
./scripts_tools/install_tools.sh bcm2835    # Librería BCM2835
./scripts_tools/install_tools.sh mosquitto  # Mosquitto MQTT
./scripts_tools/install_tools.sh oled       # SSD1306 OLED
./scripts_tools/install_tools.sh database   # MySQL/MariaDB
```

### Dependencias del Sistema

- **BCM2835** — GPIO/SPI para Raspberry Pi
- **libmosquitto-dev** — Cliente MQTT (nuevo en v2.0.2)
- **libpng-dev / zlib1g-dev** — Procesamiento de imágenes
- **qrencode** — Generación de QR
- **libmysqlcppconn-dev** — MySQL (opcional)
- **libssl-dev** — Criptografía
- **SSD1306_OLED_RPI** — OLED (opcional)

---

## 🔌 Configuración de GPIO

Pines utilizados por el MRF24J40:

| GPIO | Función  | Pin Físico |
|------|----------|------------|
| MOSI | SPI MOSI | 19         |
| MISO | SPI MISO | 21         |
| SCK  | SPI Clock| 23         |
| CS   | Chip Select | 24      |
| INT  | Interrupción | 18      |
| WAKE | Wake     | 22         |
| RESET| Reset    | 16         |

---

## 📡 MQTT (Mosquitto)

Los proyectos ahora incluyen un **puente radio ⟷ MQTT** (solo compila si `libmosquitto` está instalado).

### Topics

| Topic | Dirección | Formato |
|-------|-----------|---------|
| `domotics/{device}/set` | RX ← MQTT | `{"command": "on"}` |
| `domotics/{device}/status` | RX → MQTT | `{"isOn": true, "value": 0}` |
| `domotics/zigbee/rx` | RX → MQTT | `{"data": "hex..."}` |

### Clases implementadas

| Clase | Archivo | Propósito |
|-------|---------|-----------|
| `MqttHandler` | `mosquitto/mqtt_handler.hpp` | Conexión async, reconexión automática |
| `MqttBridge` | `mosquitto/mqtt_bridge.hpp` | Traduce comandos MQTT ↔ GPIO y radio |

---

## 🛠️ Herramientas de Desarrollo

```bash
./scripts_tools/dev_tools.sh                 # Ver ayuda
./scripts_tools/dev_tools.sh build tx        # Compilar transmisor
./scripts_tools/dev_tools.sh build rx        # Compilar receptor
./scripts_tools/dev_tools.sh gpio settings   # Configurar pines
./scripts_tools/dev_tools.sh mosquitto status # Estado MQTT
```

---

## 📚 Documentación por Proyecto

| Proyecto | README | Skill | API | Arquitectura | Configuración |
|----------|--------|-------|-----|-------------|---------------|
| Transmisor | [`mrf24_tx/README.md`](mrf24_tx/README.md) | [`SKILL.md`](mrf24_tx/SKILL.md) | [`docs/API.md`](mrf24_tx/docs/API.md) | [`docs/ARCHITECTURE.md`](mrf24_tx/docs/ARCHITECTURE.md) | [`docs/CONFIGURATION.md`](mrf24_tx/docs/CONFIGURATION.md) |
| Receptor | [`mrf24_rx/README.md`](mrf24_rx/README.md) | [`SKILL.md`](mrf24_rx/SKILL.md) | [`docs/API.md`](mrf24_rx/docs/API.md) | [`docs/ARCHITECTURE.md`](mrf24_rx/docs/ARCHITECTURE.md) | [`docs/CONFIGURATION.md`](mrf24_rx/docs/CONFIGURATION.md) |

---

## 📊 Versiones

| Versión | Cambios |
|---------|---------|
| **v2.0.2** | MQTT (mqtt_handler, mqtt_bridge), fix Makefiles, header mrf24j40.h, TODO.md, eliminar stubs vacíos |
| **v2.0.1** | Comentarios Doxygen en todos los .cpp |
| **v2.0.0** | Reestructuración: mrf24_tx/ y mrf24_rx/, Makefile con detección automática de librerías |
| 1.4 | Seguridad, ruteo, capas, modo sleep |
| 1.3 | SPI/GPIO por BCM2835 |
| 1.2 | ACK, epaper, encriptación, router/coordinator/end device |
| 1.1 | Envío correcto con header, size, buffer, checksum |
| 1.0.1 | Versión inicial |
