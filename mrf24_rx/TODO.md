# TODO — Receptor MRF24J40 (`mrf24_rx/`)

Tareas pendientes, mejoras y issues conocidos del proyecto receptor.

---

## 🔴 Alta Prioridad

- [ ] **Fix `radio.cpp`**: implementar `Mrf24j::settingsSecurity()` o eliminarlo del constructor
- [ ] **Fix `radio.cpp`**: hacer `set_promiscuous()` público en `Mrf24j` o usar friend class
- [ ] **Probar compilación en Raspberry Pi** con todos los fixes aplicados

## 🟡 Media Prioridad

- [ ] **Integrar MQTT en main.cpp**: conectar `MqttHandler` + `MqttBridge` al bucle RX
- [ ] **Traducir paquetes ZigBee recibidos a eventos MQTT**
- [ ] **Overflow de RX FIFO**: si llegan paquetes rápido, el FIFO puede desbordarse
- [ ] **Validación de CRC**: `MRF24J40_ACCEPT_WRONG_CRC_PKT` — evaluar si es seguro en producción
- [ ] **Logging rotativo**: el archivo `mrf24_receiver.log` crece indefinidamente

## 🟢 Baja Prioridad

- [ ] **Refactor `printf` a logger**: el código mezcla printf, fprintf y std::cout
- [ ] **Agregar unit tests**: probar parsing de tramas ZigBee
- [ ] **Soporte para e-paper display**: hay código en `display/epaper.cpp` pero no integrado
- [ ] **Base de datos MySQL**: `ENABLE_DATABASE` comentado — verificar integración
- [ ] **Corregir `-Wignored-qualifiers`** en headers (`spi.hpp`, `gpio.hpp`, etc.)

## 🐛 Issues Conocidos

- [ ] **SPI a 10 MHz puede causar errores** en cables largos o protoboard
- [ ] **Selección automática TX/RX por arquitectura**: en Pi 64 bits compila como TX a menos que se fuerce en config.hpp
- [ ] **Display OLED**: la librería SSD1306_OLED_RPI debe instalarse por separado
- [ ] **Warning en `mrf24j40._microchip.hpp`**: emite `#warning` sobre tamaño máximo de payload

## 💡 Ideas

- [ ] Parsear y mostrar payload como texto ASCII (no solo hex)
- [ ] Filtrar por dirección MAC de origen en modo promiscuo
- [ ] Almacenar en SQLite en vez de MySQL (más liviano)
- [ ] Interfaz web para ver paquetes en tiempo real
- [ ] Modo bridge: recibir por radio y reenviar por WiFi/Ethernet
