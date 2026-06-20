# Arquitectura — Transmisor MRF24J40

## Diagrama de Capas

```
┌────────────────────────────────────────────────────┐
│                   main.cpp                          │
│  - Bucle principal                                  │
│  - Comandos interactivos (n, b, s, q)              │
│  - LED indicador GPIO12                             │
└──────────────────────┬─────────────────────────────┘
                       │
┌──────────────────────▼─────────────────────────────┐
│          Radio_t (radio/radio.cpp)                  │
│  - Orquestación de TX                               │
│  - Estadísticas                                     │
│  - Interfaz con Mrf24j                              │
└──────────────────────┬─────────────────────────────┘
                       │
┌──────────────────────▼─────────────────────────────┐
│          Mrf24j (mrf24/mrf24j40.cpp)                │
│  - Lógica ZigBee/IEEE 802.15.4                     │
│  - Envío de tramas (send, send64)                   │
│  - Manejo de interrupciones                         │
└──────────────────────┬─────────────────────────────┘
                       │
┌──────────────────────▼─────────────────────────────┐
│          Spi_t (spi/spi.cpp)                        │
│  - Comunicación SPI con MRF24J40                   │
│  - Transferencias de 1, 2 y 3 bytes                │
└──────────────────────┬─────────────────────────────┘
                       │
               ┌───────▼───────┐
               │   BCM2835     │
               │   (hardware)  │
               └───────────────┘
```

## Flujo de Transmisión

```
1. main.cpp llama a radio.send()
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
```

## Manejo de Interrupciones

```cpp
Mrf24j40::poll() {
    leer INTSTAT
    if (TXIF)  → handleTxIrq()  // TX completado
    if (RXIF)  → handleRxIrq()  // Paquete recibido (modo promiscuo)
}
```

## Modos de Operación

### Normal
- Envía 1 paquete de 100 bytes cada 2000 ms
- Datos incrementales: `payload[i] = (msg_num + i) % 256`

### Burst
- Envía 10 paquetes rápidamente con 50 ms de separación
- Incrementa contador interno `burst_count`
- Útil para pruebas de throughput y fiabilidad

### Estadísticas
- Muestra tabla con paquetes enviados, éxito/fallo, retransmisiones
- Accesible vía tecla `s`

## Configuración Auto-detected (config.hpp)

La arquitectura determina automáticamente:

| Arquitectura | Define       | Comportamiento       |
|-------------|--------------|----------------------|
| ARM 32 bits | USE_MRF24_RX | Modo receptor        |
| ARM 64 bits | USE_MRF24_TX | Modo transmisor      |
| x86_64      | USE_MRF24_TX | Modo transmisor      |
| macOS       | USE_MRF24_RX | Modo receptor (test) |

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
