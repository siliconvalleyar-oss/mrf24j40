# MRF24J40 Receptor — `mrf24_rx/`

Proyecto C++ para **recibir** paquetes de datos usando el módulo **MRF24J40MA** (ZigBee/IEEE 802.15.4) en Raspberry Pi, con soporte **MQTT**.

---

## 📋 Requisitos

- Raspberry Pi (cualquier modelo con SPI)
- Módulo MRF24J40MA
- LED + resistencia (opcional, GPIO12)
- **Librería BCM2835** (para GPIO y SPI)
- **libmosquitto-dev** (para MQTT, opcional)

## 🔧 Compilación

```bash
cd mrf24_rx
make
```

El Makefile detecta automáticamente las librerías instaladas (`libmosquitto`, `libqrencode`, `libpng`, `zlib`, MySQL).

## 🚀 Ejecución

```bash
sudo ./bin/mrf24_receiver
```

Requiere `sudo` por el acceso a `/dev/spidev0.0`, `bcm2835` y GPIO.

## ⚙️ Configuración por Defecto

| Parámetro        | Valor    | Descripción                 |
|------------------|----------|-----------------------------|
| Dirección propia | `0x0002` | Short address del RX        |
| PAN ID           | `0xCAFE` | Identificador de red        |
| Canal            | 20       | Canal IEEE 802.15.4 (11-26) |
| LED indicador    | GPIO12   | Parpadea al recibir paquete |
| Velocidad SPI    | 10 MHz   | `/dev/spidev0.0`            |

## 🧱 Estructura del Proyecto

```
mrf24_rx/
├── Makefile                  # Compilación con detección automática
├── SKILL.md                  # Referencia rápida
├── docs/
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
    ├── mosquitto/            # Headers MQTT
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

## 🔌 Conexiones GPIO

| Señal  | GPIO | Pin físico | Descripción              |
|--------|------|------------|--------------------------|
| MOSI   | 10   | 19         | SPI Master Out, Slave In |
| MISO   | 9    | 21         | SPI Master In, Slave Out |
| SCLK   | 11   | 23         | SPI Clock                |
| CS     | 8    | 24         | Chip Select (CE0)        |
| INT    | 25   | 22         | Interrupción             |
| WAKE   | 18   | 12         | Wake up                  |
| RESET  | 17   | 11         | Reset                    |
| LED RX | 12   | 32         | Indicador de recepción   |

## 🔸 MQTT

Si `libmosquitto` está instalado, el receptor incluye:

| Clase | Propósito |
|-------|-----------|
| `MqttHandler` | Cliente MQTT con reconexión automática |
| `MqttBridge`  | Puente radio ↔ MQTT, traducción de comandos |

**Topics de comando:**
- `domotics/{light,temperature,fan,lock,curtain,energy,rgb}/set`
- Formato: `{"command": "on", "value": 50}`

**Topics de estado:**
- `domotics/{device}/status` — Estado actual del dispositivo
- `domotics/zigbee/rx` — Payload raw de radio recibido

## 📡 Protocolo

IEEE 802.15.4 con direcciones cortas (16 bits), modo promiscuo, RSSI y LQI extraídos.

## 📊 Estadísticas

- Paquetes recibidos totales
- LQI promedio
- RSSI promedio (dBm)

## 📝 Logging

El receptor genera `mrf24_receiver.log` con formato CSV:

```
#timestamp,packet_num,payload_hex,len,lqi,rssi
1712345678,1,00:01:02:03:...,100,200,-85
```

---

## 📚 Más Información

- [Referencia rápida (SKILL.md)](SKILL.md)
- [Arquitectura del Software](docs/ARCHITECTURE.md)
- [Referencia de la API](docs/API.md)
- [Guía de Configuración](docs/CONFIGURATION.md)
- [README principal](../README.md)
