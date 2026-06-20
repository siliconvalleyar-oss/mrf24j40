# TODO — Receptor MRF24J40 (`mrf24_rx/`)

Tareas pendientes, mejoras y issues conocidos del proyecto receptor.

---

## 🔴 Alta Prioridad

- [ ] **Verificar Makefile**: referencia a `src/oled.cpp` y `src/font5x7.cpp` que no existen en `src/` raíz (están en `src/oled/`). La compilación puede fallar si esos archivos no están presentes.
- [ ] **Fix `mrf24j40.cpp` line 247**: revisar el `// TODO - we could check whether the flags are > 1 here, indicating data was lost?` — agregar chequeo de pérdida de datos en interrupciones.
- [ ] **Confirmar definición doble de `-lbcm2835`**: el Makefile tiene `LDFLAGS = -pthread -lbcm2835 -lbcm2835` (duplicado). Limpiar.

## 🟡 Media Prioridad

- [ ] **Revisar config.hpp duplicado**: `mrf24_tx/` y `mrf24_rx/` tienen `include/config/config.hpp` casi idénticos. Evaluar si se puede compartir o mantener sincronizados.
- [ ] **Manejo de errores SPI**: `src/spi/spi.cpp` solo imprime error pero no reintenta. Agregar reintentos y recuperación.
- [ ] **Overflow de RX FIFO**: si llegan paquetes más rápido de lo que se procesan, el FIFO puede desbordarse. Implementar detección de overflow.
- [ ] **Validación de CRC**: actualmente está configurado `MRF24J40_ACCEPT_WRONG_CRC_PKT`. Evaluar si se debe rechazar CRC inválido en producción.
- [ ] **Logging rotativo**: el archivo `mrf24_receiver.log` crece indefinidamente. Implementar rotación por tamaño o fecha.

## 🟢 Baja Prioridad

- [ ] **Refactor `printf` a logger**: el código mezcla `printf`, `fprintf` y `std::cout`. Unificar.
- [ ] **Agregar unit tests**: probar parsing de tramas ZigBee, validación de RX, etc.
- [ ] **Modo promiscuo vs modo coordinador**: documentar mejor la diferencia y casos de uso.
- [ ] **Soporte para e-paper display**: hay código en `display/epaper.cpp` pero no integrado en el flujo principal.
- [ ] **Base de datos MySQL**: `ENABLE_DATABASE` está comentado. Verificar que la integración con MySQL funcione correctamente.
- [ ] **Generación de QR**: hay código para QR pero no integrado en el flujo RX (solo en TX). Evaluar si tiene sentido en RX.
- [ ] **Agregar flag `-Werror` al Makefile**: para evitar warnings no intencionales.

## 🐛 Issues Conocidos

- [ ] **SPI a 10 MHz puede causar errores** en cables largos o protoboard. Reducir a 1-5 MHz si hay problemas.
- [ ] **La selección automática TX/RX por arquitectura**: en Raspberry Pi 64 bits, el receptor compila como transmisor a menos que se fuerce manualmente en `config.hpp`.
- [ ] **LED GPIO12**: funciona, pero en algunas placas puede haber conflicto. Verificar.
- [ ] **Dependencia de BCM2835**: no viene pre-instalada en Raspberry Pi OS moderno. Instalar con `./scripts_tools/install_tools.sh bcm2835`.
- [ ] **Display OLED**: la librería SSD1306_OLED_RPI debe instalarse por separado y puede no compilar en arquitecturas 64 bits.
- [ ] **Warning en compilación**: el archivo `include/mrf24/mrf24j40._microchip.hpp` emite `#warning` sobre tamaño máximo de payload.

## 💡 Ideas

- [ ] Parsear y mostrar payload como texto ASCII (no solo hex)
- [ ] Filtrar por dirección MAC de origen en modo promiscuo
- [ ] Alarma/configuración por RSSI mínimo
- [ ] Almacenar en base de datos SQLite en vez de MySQL (más liviano)
- [ ] Interfaz web para ver paquetes en tiempo real
- [ ] Soporte para múltiples displays (OLED + e-paper simultáneo)
- [ ] Modo bridge: recibir por radio y reenviar por WiFi/Ethernet
