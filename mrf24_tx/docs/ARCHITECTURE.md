# Arquitectura — Transmisor MRF24J40

## Diagrama de Capas

```
┌───────────────────────────────────────────────────────┐
│                     main.cpp                           │
│  - Bucle principal                                     │
│  - Comandos interactivos (n, b, s, q)                 │
│  - LED indicador GPIO12                                │
│  - MQTT (MqttHandler + MqttBridge)                    │
└──────────────────────┬────────────────────────────────┘
                       │
┌──────────────────────▼────────────────────────────────┐
│          Radio_t (radio/radio.cpp)                     │
│  - Orquestación de TX                                  │
│  - Estadísticas                                        │
│  - Interfaz con Mrf24j                                 │
└──────────────────────┬────────────────────────────────┘
                       │
┌──────────────────────▼────────────────────────────────┐
│          Mrf24j (mrf24/mrf24j40.cpp)                   │
│  - Lógica ZigBee/IEEE 802.15.4                        │
│  - Envío de tramas (send, send64)                      │
│  - Manejo de interrupciones                            │
│  - Template genérico send_template()                   │
└──────────────────────┬────────────────────────────────┘
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

## Flujo de Transmisión

```
1. main.cpp llama a radio.send() o MqttBridge recibe comando MQTT
2. Radio_t.build() prepara payload de 100 bytes
3. Mrf24j::send() construye trama 802.15.4:
   - Header: 9 bytes (FCF + Seq + PAN + Dest + Src)
   - Payload: 100 bytes
   - FCS: 2 bytes (automático)
4. Escribe en TX Normal FIFO (0x300-0x3FF)
5. Activa TXNTRIG en TXNCON
6. Polling de INTSTAT hasta TXNIF
7. Lee TXSTAT para verificar éxito/retransmisiones
8. LED parpadea (GPIO12 HIGH/LOW)
9. Actualiza estadísticas
10. MqttBridge publica estado en MQTT
```

## Manejo de Interrupciones

```cpp
Mrf24j40::poll() {
    leer INTSTAT
    if (TXIF)  → handleTxIrq()  // TX completado
    if (RXIF)  → handleRxIrq()  // Paquete recibido
}
```

## Flujo MQTT

```
MqttHandler::begin()
  → mosquitto_loop_start() en thread separado
  → on_connect: subscribe a topics de comando
  → on_message: MqttBridge::processCommand(deviceId, command)

MqttBridge::processCommand("light_1", "on")
  → setGpio(pin, HIGH)
  → publishStatus("light_1", true, 0)
```

## Modos de Operación

### Normal
- Envía 1 paquete de 100 bytes cada 2000 ms
- Datos incrementales: `payload[i] = (msg_num + i) % 256`

### Burst
- Envía 10 paquetes rápidamente con 50 ms de separación
- Útil para pruebas de throughput y fiabilidad

### MQTT
- Recibe comandos vía MQTT y los traduce a GPIO
- Publica estadísticas TX periódicamente

## Registros Clave del MRF24J40

| Dirección | Nombre   | Función                        |
|-----------|----------|--------------------------------|
| 0x00      | RXMCR    | Configuración RX/promiscuo     |
| 0x01-0x02 | PANID    | Identificador de red           |
| 0x03-0x04 | SADRL/H  | Dirección corta                |
| 0x1B      | TXNCON   | Control de TX Normal FIFO      |
| 0x24      | TXSTAT   | Estado de TX                   |
| 0x2A      | SOFTRST  | Reset del módulo               |
| 0x31      | INTSTAT  | Estado de interrupciones       |
| 0x32      | INTCON   | Máscara de interrupciones      |
| 0x200     | RFCON0   | Selección de canal             |
| 0x210     | RSSI     | Indicador de intensidad señal  |
