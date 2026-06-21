# 🧠 SKILL — Receptor MRF24J40 (`mrf24_rx/`)

Referencia rápida para trabajar con el proyecto receptor.

---

## 📦 Stack Tecnológico

| Componente       | Detalle                              |
|------------------|--------------------------------------|
| Lenguaje         | C++17                                |
| Compilador       | g++ / clang++                        |
| Librerías        | BCM2835 (GPIO/SPI), libmosquitto (MQTT) |
| Hardware         | MRF24J40MA, Raspberry Pi, LED, OLED  |
| Protocolo        | IEEE 802.15.4 (ZigBee PHY/MAC)       |
| Comunicación     | SPI Modo 0, 10 MHz                   |
| MQTT             | mqtt_handler + mqtt_bridge           |
| Logging          | Archivo CSV                          |
| Display opcional | OLED SSD1306 (I²C)                   |
| DB opcional      | MySQL/MariaDB                        |

---

## 🚀 Comandos Esenciales

```bash
cd mrf24_rx && make
sudo ./bin/mrf24_receiver
make clean
```

## 🎮 Comandos en Tiempo Real

| Tecla | Acción                          |
|-------|----------------------------------|
| `s`   | Mostrar estadísticas             |
| `c`   | Limpiar estadísticas             |
| `q`   | Salir                            |

## 📁 Archivos Clave

| Archivo                           | Propósito                          |
|-----------------------------------|------------------------------------|
| `src/main.cpp`                    | Punto de entrada, bucle RX         |
| `src/mrf24j40.h`                 | Driver simplificado MRF24J40       |
| `include/config/config.hpp`       | Configuración global               |
| `include/mrf24/mrf24j40.hpp`     | Driver completo (64-bit MAC)       |
| `src/mosquitto/mqtt_handler.*`    | 🔸 Cliente MQTT                    |
| `src/mosquitto/mqtt_bridge.*`     | 🔸 Puente radio ⟷ MQTT            |
| `src/oled/oled/*.cpp`            | Driver OLED SSD1306                |

## 🔌 GPIO

| GPIO | Función  | Pin Físico |
|------|----------|------------|
| 12   | LED RX   | 32         |
| 10   | MOSI     | 19         |
| 9    | MISO     | 21         |
| 11   | SCLK     | 23         |
| 8    | CS       | 24         |

## ⚙️ Configuración Rápida (config.hpp)

```cpp
#define CHANNEL     20     // Canal (debe coincidir con TX)
#define ADDRESS     0x6002 // Dirección propia
#define PAN_ID      0x1234 // Pan ID (debe coincidir con TX)

// Opcionales:
// #define USE_OLED            // Habilitar display OLED
// #define ENABLE_DATABASE     // Habilitar MySQL
// #define USE_QR              // Habilitar QR
```

## 📊 Estadísticas

- Paquetes recibidos totales
- LQI promedio
- RSSI promedio (dBm)

## 🔄 Flujo de RX

```
poll() → leer INTSTAT → si RXIF → leer FIFO → extraer payload + LQI/RSSI →
limpiar FIFO → LED blink → mostrar OLED → log CSV → publicar MQTT
```

## 🔸 MQTT

Si `libmosquitto` está instalado, el receptor puede publicar:

| Topic | Datos |
|-------|-------|
| `domotics/zigbee/rx` | `{"data": "hex...", "rssi": -85, "lqi": 200}` |
| `domotics/{device}/status` | Estado traducido de comandos |

## 🐛 Debug

```cpp
#define DBG
#define DBG_BUFFER
#define DBG_SPI
#define DBG_DISPLAY_OLED
```

## 📚 Documentación

- [README.md](README.md)
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- [docs/API.md](docs/API.md)
- [docs/CONFIGURATION.md](docs/CONFIGURATION.md)
