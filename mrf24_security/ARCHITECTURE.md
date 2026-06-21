# Arquitectura — MRF24J40 IoT (mrf24_security)

## Versión 2.0.3 — Validación por Roles y Hash SHA-256

## Diagrama de Capas

```
┌──────────────────────────────────────────────────────────────────┐
│                       main.cpp                                    │
│   Punto de entrada, parseo de argumentos, inicialización         │
└──────────────────────────┬───────────────────────────────────────┘
                           │
┌──────────────────────────▼───────────────────────────────────────┐
│              Menu_t (application/menu.hpp/.cpp)                   │
│   Opciones: modo, enviar, recibir, QR, OLED, TFT, config,       │
│   logs, ROL, TABLA RUTAS, ESTADÍSTICAS VALIDACIÓN                │
└──────────────────────────┬───────────────────────────────────────┘
                           │
┌──────────────────────────▼───────────────────────────────────────┐
│          RadioManager_t (application/radio_manager.hpp/.cpp)     │
│                                                                   │
│   ╔══════════════════════════════════════════════════════════╗    │
│   ║  Protocolo Seguro (v2.0.3)                               ║    │
│   ║                                                          ║    │
│   ║  buildSecureMessage() → Trama con hash SHA-256          ║    │
│   ║  validateMessage()    → Verifica hash + TTL + destino    ║    │
│   ║  forwardMessage()     → Decrementa TTL y reenvía         ║    │
│   ║  setRole()            → EndDevice/Router/Coordinator/Mesh║    │
│   ║  addRoute/removeRoute → Tabla de enrutamiento            ║    │
│   ╚══════════════════════════════════════════════════════════╝    │
│                                                                   │
│   sendMessage() → encrypt → buildSecureMessage → m_radio->send() │
│   process()     → validateMessage → decrypt | forward            │
└────────────────────┬──────────────────┬──────────────────────────┘
                     │                  │
          ┌──────────▼────┐    ┌───────▼──────────┐
          │  Crypto_t     │    │  FileSystem_t    │
          │  AES-256-CBC  │    │  config.json     │
          │  SHA-256      │    │  mrf24j40.log    │
          │  (services/)  │    │  (services/)     │
          └───────────────┘    └──────────────────┘
                     │
          ┌──────────▼──────────────────────────────────────────────┐
          │              Mrf24j40_t (drivers/mrf24j40.hpp/.cpp)     │
          │   Driver IEEE 802.15.4 con modos EndDevice/Router/     │
          │   Gateway/Node. Polling TX/RX.                         │
          └──────────┬────────────────────────────────────────────┘
                     │
          ┌──────────▼────────────┐
          │  Spi_t (hal/spi.hpp)  │
          │  BCM2835 SPI Modo 0   │
          └───────────────────────┘
```

## Formato de Trama Segura (v2.0.3)

A partir de v2.0.3, todos los mensajes se envían con este formato de trama de aplicación:

```
┌────────────────────────────────────────────────────────────────┐
│  Trama segura (application payload, dentro de 802.15.4 frame)  │
├──────────┬─────────┬─────┬──────┬──────┬────────────┬─────────┤
│ dest_mac │ src_mac │ TTL │ size │  IV  │ ciphertext │  hash   │
│  8 bytes │ 8 bytes │  1  │  2   │  16  │    N       │   32    │
│          │         │     │      │      │            │         │
│          │         │     │      ├──────┴────────────┤         │
│          │         │     │      │   Cifrado (AES)   │         │
├──────────┴─────────┴─────┴──────┴───────────────────┴─────────┤
│  Hash SHA-256(dest_mac + size + iv + ciphertext)              │
└────────────────────────────────────────────────────────────────┘
```

- **Overhead total:** 67 bytes
- **Payload máximo cifrado:** 33 bytes (con MAX_PAYLOAD_LEN=100)
- **TTL máximo:** 10 saltos (broadcast: TTL=10)
- **Broadcast:** dest_mac = 0xFFFFFFFFFFFFFFFF

## Roles de Red

| Rol | ¿Reenvía? | Comportamiento |
|-----|-----------|---------------|
| **EndDevice** | ❌ No | Solo recibe mensajes dirigidos a él. Ignora los demás. |
| **Router** | ✅ Sí | Reenvía mensajes si tiene ruta en la tabla. Flood si es Coordinator. |
| **Coordinator** | ✅ Sí | Nodo raíz. Flood a todos los routers conocidos si no hay ruta. |
| **Mesh** | ✅ Sí | Router + puede actuar como EndDevice según topología. |

## Flujo de Recepción con Validación

```
1. poll() → hasPacket()
2. getData(buf, len) → payload crudo
3. validateMessage(buf, len)
   ├── ¿TTL == 0? → Descartar
   ├── ¿Hash inválido? → Descartar
   ├── ¿dest_mac == nosotros? → Descifrar y mostrar
   ├── ¿dest_mac == broadcast? → Descifrar y mostrar
   ├── ¿canForward() y dest_mac != nosotros? → Reenviar
   └── ¿EndDevice y no es para nosotros? → Ignorar
```

## Tabla de Enrutamiento

La tabla de rutas es un `std::vector<std::pair<uint64_t, uint64_t>>` que mapea
dirección MAC destino → siguiente salto. Se persiste en `config.json`:

```json
{
  "routing_table": [
    {"dest": "0000000000000002", "next": "0000000000000001"},
    {"dest": "0000000000000003", "next": "0000000000000001"}
  ]
}
```

## Dependencias

| Librería | Uso | Instalación |
|----------|-----|-------------|
| BCM2835 | GPIO + SPI | `sudo apt-get install libbcm2835-dev` |
| OpenSSL | AES-256-CBC + SHA-256 (EVP API) | `sudo apt-get install libssl-dev` |
| libqrencode | Generación QR | `sudo apt-get install libqrencode-dev` |
| libpng | Exportar QR a PNG | `sudo apt-get install libpng-dev` |
| nlohmann-json | Configuración JSON | `sudo apt-get install nlohmann-json-dev` |
| zlib | Compresión PNG | `sudo apt-get install zlib1g-dev` |

## Compilación

```bash
make                           # Compilar con -DENABLE_JSON
make compile-remote            # Push + SSH + pull + compilar en Pi
sudo ./bin/mrf24j40_iot --menu # Ejecutar menú interactivo
```

## Versiones

| Versión | Cambios |
|---------|---------|
| **v2.0.3** | Roles ZigBee, validación SHA-256, enrutamiento con TTL, menú extendido, nlohmann/json-3 para persistencia |
| **v2.0.2** | Faltaba fixes menores de includes |
| **v2.0.1** | Comentarios Doxygen |
| **v2.0.0** | Versión inicial de mrf24_security |
