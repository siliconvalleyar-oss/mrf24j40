# TODO — Transmisor MRF24J40 (`mrf24_tx/`)

Tareas pendientes, mejoras y issues conocidos del proyecto transmisor.

---

## 🔴 Alta Prioridad

- [ ] **Verificar Makefile**: referencia a `src/oled.cpp` y `src/font5x7.cpp` que no existen en `src/` raíz (están en `src/oled/`). La compilación puede fallar si esos archivos no están presentes.
- [ ] **Fix `mrf24j40.cpp` line 247**: revisar el `// TODO - we could check whether the flags are > 1 here, indicating data was lost?` — agregar chequeo de pérdida de datos en interrupciones.
- [ ] **Confirmar definición doble de `-lbcm2835`**: el Makefile tiene `LDFLAGS = -pthread -lbcm2835 -lbcm2835` (duplicado). Limpiar.

## 🟡 Media Prioridad

- [ ] **Revisar config.hpp duplicado**: `mrf24_tx/` y `mrf24_rx/` tienen `include/config/config.hpp` casi idénticos. Evaluar si se puede compartir o mantener sincronizados.
- [ ] **Manejo de errores SPI**: `src/spi/spi.cpp` solo imprime error con `fprintf(stderr)` pero no reintenta ni recupera. Agregar reintentos.
- [ ] **Timeout de TX**: en `main.cpp` el timeout de TX es de 500 iteraciones × 5ms = 2.5s. Considerar hacerlo configurable vía define.
- [ ] **Logging de errores**: no hay logging persistente de errores de transmisión (solo consola). Considerar agregar un archivo de log.
- [ ] **Modo sleep del MRF24J40**: el módulo soporta modo sleep de bajo consumo. No implementado actualmente.
- [ ] **Verificar polaridad de interrupción**: `INT_POLARITY_HIGH` está definido pero debe confirmarse con el hardware real.

## 🟢 Baja Prioridad

- [ ] **Refactor `printf` a `std::cout` o logger**: el código mezcla `printf` y `std::cout`. Unificar para mejor portabilidad.
- [ ] **Agregar unit tests**: probar funciones de lectura/escritura de registros, validación de checksum, etc.
- [ ] **Soporte para direcciones de 16 bits**: `USE_MAC_ADDRESS_SHORT` está comentado en `config.hpp`. Verificar que funcione correctamente.
- [ ] **Documentar valores de registros**: agregar comentarios en `mrf24j40_cmd.hpp` explicando el propósito de cada registro.
- [ ] **Separar config.hpp en archivos más pequeños**: actualmente es un archivo grande con muchas opciones. Separar por módulo.
- [ ] **Agregar flag `-Werror` al Makefile**: para evitar warnings no intencionales en compilación.

## 🐛 Issues Conocidos

- [ ] **SPI a 10 MHz puede causar errores** en cables largos o protoboard. Reducir a 1-5 MHz si hay problemas.
- [ ] **La selección automática TX/RX por arquitectura** puede ser confusa: el mismo código compila como TX o RX según si es 32/64 bits. Documentar mejor.
- [ ] **LED GPIO12**: se usa para TX, pero en algunos modelos de Raspberry Pi puede haber conflicto con otras funciones. Verificar.
- [ ] **Dependencia de BCM2835**: la librería no viene pre-instalada en Raspberry Pi OS moderno. Necesita instalación manual.

## 💡 Ideas

- [ ] Soporte para múltiples canales (salto de frecuencia)
- [ ] Encolar múltiples paquetes TX antes de enviar
- [ ] Modo broadcast (destino 0xFFFF)
- [ ] Integración con MQTT para monitoreo remoto
- [ ] Dashboard web con estadísticas en tiempo real
- [ ] Soporte para cifrado AES (hardware del MRF24J40)
