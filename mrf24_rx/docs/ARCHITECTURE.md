# Arquitectura — Receptor MRF24J40

## Diagrama de Capas

```
┌──────────────────────────────────────────────────────────────┐
│                     main.cpp                                  │
│  - Bucle principal RX                                        │
│  - Comandos interactivos (s, c, q)                           │
│  - LED indicador GPIO12                                      │
│  - Display OLED (opcional)                                   │
│  - Logging a archivo CSV                                     │
│  - MQTT (MqttHandler + MqttBridge)                           │
└──────────────────────┬───────────────────────────────────────┘
                       │
┌──────────────────────▼───────────────────────────────────────┐
│              Radio_t (radio/radio.cpp)                        │
│  - Orquestación de RX                                        │
│  - Estadísticas                                              │
│  - Interfaz con Mrf24j                                       │
└──────────────────────┬───────────────────────────────────────┘
                       │
┌──────────────────────▼───────────────────────────────────────┐
│           Mrf24j (mrf24/mrf24j40.cpp)                        │
│  - Lógica ZigBee/IEEE 802.15.4                              │
│  - Recepción de tramas                                       │
│  - Manejo de interrupciones                                  │
└──────────────────────┬───────────────────────────────────────┘
                       │
          ┌────────────┴────────────┐
          │                         │
┌─────────▼─────────┐   ┌──────────▼──────────┐
│   Spi_t (spi/spi)  │   │   MqttHandler        │
│   Comunicación SPI │   │   MQTT async         │
│                    │   │   MqttBridge         │
│                    │   │   radio ⟷ MQTT       │
└─────────┬─────────┘   └──────────┬──────────┘
          │                         │
   ┌──────▼──────┐           ┌─────▼─────┐
   │   BCM2835   │           │ Mosquitto │
   │  (hardware) │           │  (broker) │
   └─────────────┘           └───────────┘
```

## Flujo de Recepción

```
1. Bucle principal llama a radio.poll() continuamente
2. Mrf24j::poll() lee INTSTAT
3. Si RXIF está activo → handleRxIrq():
   a. Deshabilita RX (BBREG1 = 0x04)
   b. Lee frame_length del RX FIFO (0x300)
   c. Valida longitud (12-127 bytes)
   d. Extrae payload (bytes después del header)
   e. Lee LQI y RSSI del final del frame
   f. Marca rx_ready = true
   g. Limpia RX FIFO (RXFLUSH)
   h. Rehabilita RX (BBREG1 = 0x00)
4. main.cpp detecta hasPacket() == true
5. Lee datos con rxGet()
6. Parpadea LED (GPIO12)
7. Muestra en OLED (si disponible)
8. Escribe en log CSV
9. MqttBridge publica en MQTT (opcional)
```

## Flujo MQTT

```
Recepción de comando MQTT → processCommand():

"domotics/light_1/set" → {"command": "on"}
  → setGpio(pinLight1, HIGH)
  → publishStatus("light_1", true, 0)

"domotics/rgb/set" → {"command": "set", "value": 0xFF0000}
  → setGpio(pinRgbR, HIGH), setGpio(pinRgbG, LOW), ...
  → publishStatus("rgb_1", true, 0xFF0000)
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

### MQTT (mosquitto/)
- `mosquitto/mqtt_handler.cpp` — Cliente MQTT con reconexión automática
- `mosquitto/mqtt_bridge.cpp` — Puente radio ⟷ MQTT

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
| Tyme       | `tyme/tyme.cpp`                | Utilidades de tiempo             |

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
