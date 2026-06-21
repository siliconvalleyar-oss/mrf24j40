# MRF24J40 Transmisor — `mrf24_tx/`

Proyecto C++ para **transmitir** paquetes de datos usando el módulo **MRF24J40MA** (ZigBee/IEEE 802.15.4) en Raspberry Pi, con soporte **MQTT**.

---

## 📋 Requisitos

- Raspberry Pi (cualquier modelo con SPI)
- Módulo MRF24J40MA
- LED + resistencia (opcional, GPIO12)
- **Librería BCM2835** (para GPIO y SPI)
- **libmosquitto-dev** (para MQTT, opcional)

## 🔧 Compilación

```bash
cd mrf24_tx
make
```

El Makefile detecta automáticamente las librerías instaladas (`libmosquitto`, `libqrencode`, `libpng`, `zlib`, MySQL).

## 🚀 Ejecución

```bash
sudo ./bin/mrf24_transmitter
```

Requiere `sudo` por el acceso a `/dev/spidev0.0` y `bcm2835`.

## ⚙️ Configuración por Defecto

| Parámetro       | Valor    | Descripción                |
|-----------------|----------|----------------------------|
| Dirección propia| `0x0001` | Short address del TX       |
| Dirección destino| `0x0002`| Short address del RX       |
| PAN ID          | `0xCAFE` | Identificador de red       |
| Canal           | 20       | Canal IEEE 802.15.4 (11-26)|
| Intervalo TX    | 2000 ms  | Envío automático           |
| LED indicador   | GPIO12   | Parpadea al transmitir     |
| Velocidad SPI   | 10 MHz   | `/dev/spidev0.0`           |

## 🧱 Estructura del Proyecto

```
mrf24_tx/
├── Makefile                  # Compilación con detección automática de librerías
├── SKILL.md                  # Referencia rápida
├── docs/                     # Documentación adicional
│   ├── ARCHITECTURE.md
│   ├── API.md
│   └── CONFIGURATION.md
├── src/
│   ├── main.cpp              # Punto de entrada
│   ├── mrf24j40.h            # Header del driver simplificado
│   ├── mrf24j40.cpp          # Driver MRF24J40 simplificado
│   ├── radio/                # Lógica de radio de alto nivel
│   ├── mrf24/                # Driver MRF24J40 completo
│   ├── mosquitto/            # 🔸 MQTT handler + bridge
│   ├── config/               # Configuración
│   ├── gpio/                 # GPIO
│   ├── spi/                  # SPI
│   ├── oled/                 # OLED SSD1306
│   ├── display/              # E-paper
│   ├── qr/                   # QR
│   ├── security/             # AES
│   ├── file/                 # Archivos y DB
│   ├── interrupt/            # Interrupciones
│   ├── tyme/                 # Tiempo
│   └── work/                 # Utilidades
└── include/
    ├── config/config.hpp     # Configuración global
    ├── mrf24/                # Headers del driver MRF24J40
    ├── mosquitto/            # Headers MQTT (mqtt_handler, mqtt_bridge)
    ├── spi/spi.hpp
    ├── gpio/gpio.hpp
    ├── radio/
    ├── oled/
    ├── file/
    ├── qr/
    ├── tyme/
    ├── work/
    ├── display/
    └── security/
```

## 🔌 Conexiones GPIO (MRF24J40 ↔ Raspberry Pi)

| Señal  | GPIO | Pin físico | Descripción              |
|--------|------|------------|--------------------------|
| MOSI   | 10   | 19         | SPI Master Out, Slave In |
| MISO   | 9    | 21         | SPI Master In, Slave Out |
| SCLK   | 11   | 23         | SPI Clock                |
| CS     | 8    | 24         | Chip Select (CE0)        |
| INT    | 25   | 22         | Interrupción             |
| WAKE   | 18   | 12         | Wake up                  |
| RESET  | 17   | 11         | Reset                    |
| LED TX | 12   | 32         | Indicador de transmisión |

## 🔸 MQTT

Si `libmosquitto` está instalado, el transmisor incluye:

| Clase | Propósito |
|-------|-----------|
| `MqttHandler` | Cliente MQTT con reconexión automática y loop async |
| `MqttBridge`  | Puente entre eventos de radio y topics MQTT |

**Topics:**
- `domotics/zigbee/tx/status` — Estadísticas TX periódicas
- `domotics/{device}/status` — Estado de dispositivos

## 📡 Protocolo

IEEE 802.15.4 con direcciones cortas (16 bits), ACK solicitado, payload de 100 bytes, modo promiscuo.

## 📊 Estadísticas

- Paquetes enviados totales
- Transmisiones exitosas/fallidas
- Retransmisiones totales
- Tasa de éxito (%)

---

## 📚 Más Información

- [Referencia rápida (SKILL.md)](SKILL.md)
- [Arquitectura del Software](docs/ARCHITECTURE.md)
- [Referencia de la API](docs/API.md)
- [Guía de Configuración](docs/CONFIGURATION.md)
- [README principal](../README.md)
