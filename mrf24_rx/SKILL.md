# SKILL — Receptor MRF24J40 (`mrf24_rx/`)

Referencia rápida para trabajar con el proyecto receptor.

---

## 📦 Stack Tecnológico

| Componente       | Detalle                              |
|------------------|--------------------------------------|
| Lenguaje         | C++17                                |
| Compilador       | g++ / clang++                        |
| Librerías        | BCM2835 (GPIO/SPI)                   |
| Hardware         | MRF24J40MA, Raspberry Pi, LED, OLED  |
| Protocolo        | IEEE 802.15.4 (ZigBee PHY/MAC)       |
| Comunicación     | SPI Modo 0, 10 MHz                   |
| Logging          | Archivo CSV (`mrf24_receiver.log`)   |
| Display opcional | OLED SSD1306 (I²C)                   |
| DB opcional      | MySQL/MariaDB                         |

---

## 🚀 Comandos Esenciales

```bash
# Compilar
cd mrf24_rx && make

# Ejecutar (requiere sudo por SPI/GPIO)
sudo ./bin/mrf24_receiver

# Ver log de paquetes recibidos
tail -f mrf24_receiver.log

# Limpiar
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
| `src/mrf24j40.cpp`               | Driver simplificado MRF24J40       |
| `include/config/config.hpp`       | Configuración global               |
| `include/mrf24/mrf24j40.hpp`     | Driver completo (64-bit MAC)       |
| `include/mrf24/mrf24j40_cmd.hpp`  | Definición de registros            |
| `src/oled/oled/*.cpp`            | Driver OLED SSD1306                |
| `Makefile`                        | Compilación                        |

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
// Para cambiar canal (debe coincidir con TX):
#define CHANNEL 20

// Para cambiar direcciones:
#define ADDRESS     0x6002   // Dirección propia
#define ADDR_SLAVE  0x6001   // Dirección del transmisor
#define PAN_ID      0x1234   // Pan ID (debe coincidir con TX)

// Opcionales:
// #define USE_OLED            // Habilitar display OLED
// #define ENABLE_DATABASE     // Habilitar MySQL
// #define USE_QR              // Habilitar QR
```

## 📊 Estadísticas

El receptor reporta:
- Paquetes recibidos totales
- LQI promedio
- RSSI promedio (dBm)

## 📝 Formato del Log

```csv
#timestamp,packet_num,payload_hex,len,lqi,rssi
1712345678,1,00:01:02:03:...,100,200,-85
```

## 🔄 Flujo de RX

```
poll() → leer INTSTAT → si RXIF → leer FIFO → extraer payload + LQI/RSSI →
limpiar FIFO → LED blink → mostrar OLED → log CSV
```

## 🐛 Debug

Para más verbose, descomentar en config.hpp:
```cpp
#define DBG
#define DBG_BUFFER
#define DBG_SPI
#define DBG_DISPLAY_OLED
```

## 📚 Documentación Adicional

- [README.md](README.md)
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- [docs/API.md](docs/API.md)
- [docs/CONFIGURATION.md](docs/CONFIGURATION.md)
