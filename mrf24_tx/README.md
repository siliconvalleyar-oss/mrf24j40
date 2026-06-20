# MRF24J40 Transmisor — `mrf24_tx/`

Proyecto C++ para **transmitir** paquetes de datos usando el módulo **MRF24J40MA** (ZigBee/IEEE 802.15.4) en Raspberry Pi, con indicador LED en GPIO12.

---

## 📋 Requisitos

- Raspberry Pi (cualquier modelo con SPI)
- Módulo MRF24J40MA
- LED + resistencia (opcional, GPIO12)
- **Librería BCM2835** (para GPIO y SPI)

## 🔧 Compilación

```bash
cd mrf24_tx
make
```

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

## 🎮 Comandos en Tiempo Real

Durante la ejecución, puedes presionar:

| Tecla | Acción                     |
|-------|----------------------------|
| `n`   | Modo normal (envío cada 2s)|
| `b`   | Burst: 10 paquetes rápido  |
| `s`   | Mostrar estadísticas       |
| `q`   | Salir                      |

## 🧱 Estructura del Proyecto

```
mrf24_tx/
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
│   ├── oled/                 # Soporte opcional OLED SSD1306
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
│   ├── security/             # Encriptación AES
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
| LED TX | 12   | 32         | Indicador de transmisión |

## 📡 Protocolo

El transmisor usa **IEEE 802.15.4** con:

- **Direcciones cortas** (16 bits)
- **ACK solicitado** (TXNACKREQ)
- **Payload de 100 bytes** con datos incrementales
- **RSSI y LQI** añadidos al paquete
- **Modo promiscuo** para recibir todos los paquetes
- **CRC** válido requerido

## 📊 Estadísticas

El transmisor recolecta:
- Paquetes enviados totales
- Transmisiones exitosas/fallidas
- Retransmisiones totales
- Tasa de éxito (%)
- Estado de registros del MRF24J40

---

## 📚 Más Información

- [Arquitectura del Software](docs/ARCHITECTURE.md)
- [Referencia de la API](docs/API.md)
- [Guía de Configuración](docs/CONFIGURATION.md)
- [README principal (raíz del proyecto)](../README.md)
