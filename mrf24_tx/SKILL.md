# 🧠 SKILL — Transmisor MRF24J40 (`mrf24_tx/`)

Referencia rápida para trabajar con el proyecto transmisor.

---

## 📦 Stack Tecnológico

| Componente       | Detalle                         |
|------------------|---------------------------------|
| Lenguaje         | C++17                           |
| Compilador       | g++ / clang++                   |
| Librerías        | BCM2835 (GPIO/SPI), libmosquitto (MQTT) |
| Hardware         | MRF24J40MA, Raspberry Pi, LED   |
| Protocolo        | IEEE 802.15.4 (ZigBee PHY/MAC)  |
| Comunicación     | SPI Modo 0, 10 MHz              |
| MQTT             | mqtt_handler + mqtt_bridge      |
| Arquitecturas    | armv7l, aarch64, x86_64         |

---

## 🚀 Comandos Esenciales

```bash
# Compilar
cd mrf24_tx && make

# Ejecutar (requiere sudo por SPI/GPIO)
sudo ./bin/mrf24_transmitter

# Limpiar
make clean
```

## 🎮 Comandos en Tiempo Real

| Tecla | Acción                          |
|-------|----------------------------------|
| `n`   | Modo normal (envío cada 2s)      |
| `b`   | Burst: 10 paquetes rápidos       |
| `s`   | Mostrar estadísticas             |
| `q`   | Salir                            |

## 📁 Archivos Clave

| Archivo                        | Propósito                         |
|--------------------------------|-----------------------------------|
| `src/main.cpp`                 | Punto de entrada, bucle principal |
| `src/mrf24j40.h`              | Driver simplificado MRF24J40      |
| `src/mrf24j40.cpp`            | Implementación driver simplificado |
| `include/config/config.hpp`    | Configuración global              |
| `include/mrf24/mrf24j40.hpp`  | Driver completo (64-bit MAC)      |
| `src/mosquitto/mqtt_handler.*` | 🔸 Cliente MQTT                   |
| `src/mosquitto/mqtt_bridge.*`  | 🔸 Puente radio ⟷ MQTT           |
| `Makefile`                     | Compilación con detección automática |

## 🔌 GPIO

| GPIO | Función  | Pin Físico |
|------|----------|------------|
| 12   | LED TX   | 32         |
| 10   | MOSI     | 19         |
| 9    | MISO     | 21         |
| 11   | SCLK     | 23         |
| 8    | CS       | 24         |

## ⚙️ Configuración Rápida (config.hpp)

```cpp
#define CHANNEL     20     // Canal (11-26)
#define ADDRESS     0x6001 // Dirección propia
#define ADDR_SLAVE  0x6002 // Dirección destino
#define PAN_ID      0x1234 // Pan ID (debe coincidir con RX)
```

## 📊 Estadísticas

El transmisor reporta:
- Paquetes enviados, éxito, fallos, retransmisiones
- Tasa de éxito (%)
- Estado de registros del MRF24J40

## 🔄 Flujo de TX

```
poll() → leer INTSTAT → si TXNIF → leer TXSTAT → éxito/fallo → LED blink → publicar MQTT
```

## 🔸 MQTT

Si `libmosquitto` está instalado, el transmisor puede publicar:

| Topic | Datos |
|-------|-------|
| `domotics/zigbee/tx/status` | `{"packets_sent": 10, "tx_success": 9, "rate": 90.0}` |

## 🐛 Debug

Descomentar en `config.hpp` para más verbose:

```cpp
#define DBG
#define DBG_BUFFER
#define DBG_SPI
```

## 📚 Documentación

- [README.md](README.md)
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- [docs/API.md](docs/API.md)
- [docs/CONFIGURATION.md](docs/CONFIGURATION.md)
