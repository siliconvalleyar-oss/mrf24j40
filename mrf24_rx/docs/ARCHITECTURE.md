# Arquitectura — Receptor MRF24J40

## Diagrama de Capas

```
┌───────────────────────────────────────────────────────────┐
│                     main.cpp                               │
│  - Bucle principal RX                                     │
│  - Comandos interactivos (s, c, q)                        │
│  - LED indicador GPIO12                                   │
│  - Display OLED (opcional)                                │
│  - Logging a archivo CSV                                  │
└──────────────────────┬────────────────────────────────────┘
                       │
┌──────────────────────▼────────────────────────────────────┐
│              Radio_t (radio/radio.cpp)                     │
│  - Orquestación de RX                                     │
│  - Estadísticas                                           │
│  - Interfaz con Mrf24j                                    │
└──────────────────────┬────────────────────────────────────┘
                       │
┌──────────────────────▼────────────────────────────────────┐
│           Mrf24j (mrf24/mrf24j40.cpp)                     │
│  - Lógica ZigBee/IEEE 802.15.4                           │
│  - Recepción de tramas                                    │
│  - Manejo de interrupciones                               │
└──────────────────────┬────────────────────────────────────┘
                       │
┌──────────────────────▼────────────────────────────────────┐
│           Spi_t (spi/spi.cpp)                             │
│  - Comunicación SPI con MRF24J40                         │
│  - Transferencias de 1, 2 y 3 bytes                      │
└──────────────────────┬────────────────────────────────────┘
                       │
               ┌───────▼───────┐
               │   BCM2835     │
               │   (hardware)  │
               └───────────────┘
```

## Flujo de Recepción

```
1. Bucle principal llama a radio.poll() continuamente
2. Mrf24j40::poll() lee INTSTAT
3. Si INT_RXIF está activo → handleRxIrq():
   a. Deshabilita RX (BBREG1 = 0x04)
   b. Lee frame_length del RX FIFO (0x300)
   c. Valida longitud (12-127 bytes)
   d. Extrae payload (bytes después del header de 9 bytes)
   e. Lee LQI y RSSI del final del frame
   f. Marca rx_ready = true
   g. Limpia RX FIFO (RXFLUSH)
   h. Rehabilita RX (BBREG1 = 0x00)
4. main.cpp detecta hasPacket() == true
5. Lee datos con rxGet()
6. Parpadea LED (GPIO12)
7. Muestra en OLED (si disponible)
8. Escribe en log CSV
```

## Componentes

### Core Radio (radio/)
- `radio/radio.cpp` — Orquestación de la radio
- `radio/data.hpp` — Definiciones de datos (packet_rx, packet_tx)

### Driver MRF24J40 (mrf24/)
- `mrf24/mrf24j40.cpp` — Implementación principal del driver
- `mrf24/mrf24j40_send.cpp` — Envío de tramas (64 bits)
- `mrf24/mrf24j40_template.cpp` — Templates de envío
- `mrf24/zigbee_packet_handler.cpp` — Manejo de paquetes ZigBee
- `mrf24/radio.cpp` — Radio de alto nivel

### Módulos Auxiliares

| Módulo     | Archivos                        | Función                          |
|------------|--------------------------------|----------------------------------|
| SPI        | `spi/spi.cpp`                  | Comunicación SPI                 |
| GPIO       | `gpio/gpio.cpp`                | Control de GPIO                  |
| OLED       | `oled/oled/*.cpp`              | Display SSD1306 (I²C)            |
| Epaper     | `display/epaper.cpp`           | Display e-paper (opcional)       |
| QR         | `qr/qr.cpp, qr_img.cpp`        | Generación de códigos QR         |
| Seguridad  | `security/encrypt/decrypt`     | Encriptación AES                 |
| Archivos   | `file/file.cpp, database.cpp`  | Manejo de archivos y MySQL       |
| Interrupt  | `interrupt/interrupt.cpp`      | Manejo de interrupciones         |
| Tyme       | `tyme/tyme.cpp`                | Utilidades de tiempo             |

## Configuración Auto-detected (config.hpp)

La arquitectura determina automáticamente:

| Arquitectura | Define       | Comportamiento       |
|-------------|--------------|----------------------|
| ARM 32 bits | USE_MRF24_RX | Modo receptor        |
| ARM 64 bits | USE_MRF24_TX | Modo transmisor      |
| x86_64      | USE_MRF24_TX | Modo transmisor      |
| macOS       | USE_MRF24_RX | Modo receptor (test) |

## Flujo de Logging

```
paquete recibido
    → open("mrf24_receiver.log", append)
    → escribir: timestamp,packet_num,payload_hex,len,lqi,rssi
    → flush()
```

## Registros Clave del MRF24J40

| Dirección | Nombre   | Función                        |
|-----------|----------|--------------------------------|
| 0x00      | RXMCR    | Configuración RX/promiscuo     |
| 0x01-0x02 | PANID    | Identificador de red           |
| 0x03-0x04 | SADRL/H  | Dirección corta                |
| 0x0D      | RXFLUSH  | Limpiar RX FIFO                |
| 0x2A      | SOFTRST  | Reset del módulo               |
| 0x31      | INTSTAT  | Estado de interrupciones       |
| 0x32      | INTCON   | Máscara de interrupciones      |
| 0x36      | RFCTL    | Control de RF                  |
| 0x39      | BBREG1   | Control del receptor           |
| 0x200     | RFCON0   | Selección de canal             |
| 0x210     | RSSI     | Indicador de intensidad señal  |
