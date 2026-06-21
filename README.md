# MRF24J40 — Transmisor y Receptor ZigBee para Raspberry Pi

Proyecto C++ para comunicación inalámbrica usando el módulo **MRF24J40MA** en Raspberry Pi con soporte **MQTT**.

## 📦 Estructura del Proyecto

```
├── .gitignore
├── Makefile                  # Makefile principal
├── README.md
├── TODO.md                   # Tareas globales del sistema
├── VERSION.txt               # Historial de versiones
├── flow/                     # Diagramas de comunicación (DrawIO)
├── scripts_tools/            # Scripts de instalación y desarrollo
├── mrf24_tx/                 # 🚀 PROYECTO TRANSMISOR
│   ├── Makefile
│   ├── SKILL.md              # Referencia rápida
│   ├── docs/                 # Documentación (API, Arquitectura, Config)
│   ├── src/
│   │   ├── main.cpp
│   │   ├── mrf24j40.h        # Driver simplificado
│   │   ├── radio/            # Lógica de radio de alto nivel
│   │   ├── mrf24/            # Driver completo MRF24J40
│   │   ├── mosquitto/        # 🔸 MQTT (mqtt_handler, mqtt_bridge)
│   │   ├── config/           # Configuración
│   │   ├── spi/              # SPI
│   │   ├── gpio/             # GPIO
│   │   ├── oled/             # OLED SSD1306
│   │   ├── display/          # E-paper
│   │   ├── qr/               # QR
│   │   ├── security/         # AES
│   │   ├── file/             # Archivos y DB
│   │   ├── interrupt/        # Interrupciones
│   │   ├── tyme/             # Tiempo
│   │   └── work/             # Utilidades
│   └── include/
└── mrf24_rx/                 # 📡 PROYECTO RECEPTOR
    ├── Makefile
    ├── SKILL.md
    ├── docs/
    └── ... (misma estructura que mrf24_tx)
```

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
