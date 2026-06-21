# TODO — Transmisor MRF24J40 (`mrf24_tx/`)

Tareas pendientes, mejoras y issues conocidos del proyecto transmisor.

---

## 🔴 Alta Prioridad

- [ ] **Fix `radio.cpp`**: implementar `Mrf24j::settingsSecurity()` o eliminarlo del constructor
- [ ] **Fix `radio.cpp`**: hacer `set_promiscuous()` público en `Mrf24j` o usar friend class
- [ ] **Probar compilación en Raspberry Pi** con todos los fixes aplicados

## 🟡 Media Prioridad

- [ ] **Integrar MQTT en main.cpp**: conectar `MqttHandler` + `MqttBridge` al bucle principal
- [ ] **Publicar estadísticas TX por MQTT** periódicamente
- [ ] **Revisar config.hpp duplicado**: `mrf24_tx/` y `mrf24_rx/` tienen copias casi idénticas
- [ ] **Manejo de errores SPI**: `src/spi/spi.cpp` solo imprime error, no reintenta
- [ ] **Logging de errores**: no hay logging persistente (solo consola)
- [ ] **Modo sleep del MRF24J40**: soportado por hardware pero no implementado

## 🟢 Baja Prioridad

- [ ] **Refactor `printf` a `std::cout` o logger**: el código mezcla ambos
- [ ] **Agregar unit tests**: probar lectura/escritura de registros, checksum
- [ ] **Soporte para direcciones de 16 bits**: `USE_MAC_ADDRESS_SHORT` comentado en config.hpp
- [ ] **Separar config.hpp en archivos más pequeños**: actualmente es monolítico
- [ ] **Corregir `-Wignored-qualifiers`** en headers (`spi.hpp`, `gpio.hpp`, etc.)

## 🐛 Issues Conocidos

- [ ] **SPI a 10 MHz puede causar errores** en cables largos o protoboard
- [ ] **LED GPIO12**: puede tener conflicto con otras funciones en algunos modelos de Pi
- [ ] **Dependencia de BCM2835**: no viene pre-instalada en Raspberry Pi OS moderno

## 💡 Ideas

- [ ] Soporte para múltiples canales (salto de frecuencia)
- [ ] Dashboard web con estadísticas en tiempo real
- [ ] Soporte para cifrado AES del MRF24J40
