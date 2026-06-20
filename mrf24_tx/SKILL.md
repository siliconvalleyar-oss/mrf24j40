# SKILL — Transmisor MRF24J40 (`mrf24_tx/`)

Referencia rápida para trabajar con el proyecto transmisor.

---

## 📦 Stack Tecnológico

| Componente       | Detalle                         |
|------------------|---------------------------------|
| Lenguaje         | C++17                           |
| Compilador       | g++ / clang++                   |
| Librerías        | BCM2835 (GPIO/SPI)              |
| Hardware         | MRF24J40MA, Raspberry Pi, LED   |
| Protocolo        | IEEE 802.15.4 (ZigBee PHY/MAC)  |
| Comunicación     | SPI Modo 0, 10 MHz              |
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
| `src/mrf24j40.cpp`            | Driver simplificado MRF24J40      |
| `include/config/config.hpp`    | Configuración global              |
| `include/mrf24/mrf24j40.hpp`  | Driver completo (64-bit MAC)      |
| `include/mrf24/mrf24j40_cmd.hpp` | Definición de registros        |
| `Makefile`                     | Compilación                       |

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
// Para cambiar canal (11-26):
#define CHANNEL 20

// Para cambiar direcciones:
#define ADDRESS     0x6001   // Dirección propia
#define ADDR_SLAVE  0x6002   // Dirección destino
#define PAN_ID      0x1234   // Pan ID (debe coincidir con RX)
```

## 📊 Estadísticas

El transmisor reporta:
- Paquetes enviados, éxito, fallos, retransmisiones
- Tasa de éxito (%)
- Estado de registros del MRF24J40

## 🔄 Flujo de TX

```
poll() → leer INTSTAT → si TXNIF → leer TXSTAT → éxito/fallo → LED blink
```

## 🐛 Debug

Para más verbose, descomentar en config.hpp:
```cpp
#define DBG
#define DBG_BUFFER
#define DBG_SPI
```

## 📚 Documentación Adicional

- [README.md](README.md)
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- [docs/API.md](docs/API.md)
- [docs/CONFIGURATION.md](docs/CONFIGURATION.md)
