# MRF24J40 Receptor — `mrf24_rx/`

Proyecto C++ para **recibir** paquetes de datos usando el módulo **MRF24J40MA** (ZigBee/IEEE 802.15.4) en Raspberry Pi, con:
- Indicador LED en GPIO12
- Soporte opcional para display **OLED SSD1306**
- **Logging** a archivo CSV de todos los paquetes recibidos

---

## 📋 Requisitos

- Raspberry Pi (cualquier modelo con SPI)
- Módulo MRF24J40MA
- LED + resistencia (opcional, GPIO12)
- Display OLED SSD1306 (opcional, I²C)
- **Librería BCM2835** (para GPIO y SPI)

## 🔧 Compilación

```bash
cd mrf24_rx
make
```

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
| Log file         | `mrf24_receiver.log` | CSV de paquetes    |

## 🎮 Comandos en Tiempo Real

Durante la ejecución, puedes presionar:

| Tecla | Acción                     |
|-------|----------------------------|
| `s`   | Mostrar estadísticas RX    |
| `c`   | Limpiar/reiniciar stats    |
| `q`   | Salir                      |

## 🧱 Estructura del Proyecto

```
mrf24_rx/
├── Makefile                  # Compilación del proyecto
├── README.md                 # Esta documentación
├── docs/                     # Documentación adicional
│   ├── ARCHITECTURE.md       # Arquitectura del software
│   ├── API.md                # Referencia de la API
│   └── CONFIGURATION.md      # Guía de configuración
├── obj/                      # Objetos compilados (gitignored)
├── bin/                      # Binarios (gitignored)
├── src/                      # Código fuente
│   ├── main.cpp              # Punto de entrada
│   ├── mrf24j40.cpp          # Driver MRF24J40 simplificado
│   ├── mrf24j40.h            # Header del driver simplificado
│   ├── radio/                # Lógica de radio de alto nivel
│   │   ├── run.cpp
│   │   ├── radio.cpp
│   │   └── data.hpp
│   ├── mrf24/                # Driver MRF24J40 completo
│   │   ├── mrf24j40.cpp
│   │   ├── mrf24j40_send.cpp
│   │   ├── mrf24j40_template.cpp
│   │   ├── radio_trasnreceiver.cpp
│   │   ├── radio.cpp
│   │   └── zigbee_packet_handler.cpp
│   ├── config/
│   │   └── config.cpp
│   ├── gpio/
│   │   └── gpio.cpp
│   ├── spi/
│   │   ├── spi.cpp
│   │   └── spi_dbg.cpp
│   ├── oled/                 # Soporte OLED SSD1306
│   │   └── oled/
│   │       ├── SSD1306_OLED.cpp
│   │       ├── SSD1306_OLED_graphics.cpp
│   │       └── ...
│   ├── display/
│   │   └── epaper.cpp
│   ├── qr/                   # Generación de códigos QR
│   │   ├── qr.cpp
│   │   ├── qr_img.cpp
│   │   └── ff.cpp
│   ├── security/             # Desencriptación AES
│   │   ├── encrypt.cpp
│   │   └── decrypt.cpp
│   ├── file/                 # Manejo de archivos y DB
│   │   ├── file.cpp
│   │   └── database.cpp
│   ├── interrupt/
│   │   └── interrupt.cpp
│   ├── tyme/
│   │   └── tyme.cpp
│   └── work/
│       └── rfflush.cpp
└── include/                  # Headers
    ├── config/config.hpp     # Configuración global
    ├── mrf24/                # Headers del driver MRF24J40
    │   ├── mrf24j40.hpp
    │   ├── mrf24j40_cmd.hpp  # Definición de registros
    │   ├── mrf24j40_settings.hpp
    │   ├── mrf24j40_control_register.hpp
    │   ├── mrf24j40_template.tpp
    │   └── radio.hpp
    ├── spi/spi.hpp
    ├── gpio/gpio.hpp
    ├── radio/radio.hpp, run.hpp, data.hpp
    ├── oled/                 # Headers OLED
    ├── file/file.hpp, database.hpp
    ├── qr/qr.hpp
    ├── tyme/tyme.hpp
    ├── work/work.hpp, data_analisis.hpp, rfflush.hpp
    ├── display/color.hpp
    └── security/aes.hpp
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
| LED RX | 12   | 32         | Indicador de recepción   |

## 📡 Protocolo

El receptor usa **IEEE 802.15.4** con:

- **Direcciones cortas** (16 bits)
- **Payload variable** (hasta 100 bytes)
- **ACK automático** (deshabilitado vía config)
- **RSSI y LQI** extraídos de cada paquete
- **Modo promiscuo** para recibir todos los paquetes
- **CRC inválido aceptado** (configurable)

## 📊 Logging

El receptor genera un archivo `mrf24_receiver.log` con formato CSV:

```
#timestamp,packet_num,payload_hex,len,lqi,rssi
1712345678,1,00:01:02:03:...,100,200,-85
1712345679,2,04:05:06:07:...,100,210,-83
```

---

## 📚 Más Información

- [Arquitectura del Software](docs/ARCHITECTURE.md)
- [Referencia de la API](docs/API.md)
- [Guía de Configuración](docs/CONFIGURATION.md)
- [README principal (raíz del proyecto)](../README.md)
