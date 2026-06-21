# TODO — MRF24J40 IoT System

## Estado actual
- **Último tag:** v2.0.2
- **Rama:** v2.0.0
- **Último commit:** `8359e7f` — eliminar stubs vacíos de mosquitto
- **Arquitectura target:** Raspberry Pi (ARM aarch64 / armv7l)

---

## 📋 Tareas ambas proyectos (mrf24_tx + mrf24_rx)

### Compilación — Fixes aplicados
- [x] Crear header `mrf24j40.h` (driver simplificado Mrf24j40)
- [x] Corregir rutas de fuentes en Makefiles (`oled/oled.cpp`, `fonts/font5x7.cpp`)
- [x] Eliminar `src/radio/data.hpp` (archivo vacío que shadoweaba a `include/radio/data.hpp`)
- [x] Agregar `using Mrf24j_t = Mrf24j;` en `mrf24j40.hpp`
- [x] Fix stray characters en `mrf24j40_template.cpp` (`*/` dentro de `/** */`)
- [x] Agregar declaración `send_template` en header `mrf24j40.hpp`
- [x] Fix constructor `GPIO::Gpio_t` — pasar `m_status`
- [x] Eliminar stubs vacíos de mosquitto (0 bytes)
- [x] Limpiar Makefiles (referencias a stubs eliminados)
- [ ] **Pendiente:** `Mrf24j::settingsSecurity()` — método llamado pero no declarado
- [ ] **Pendiente:** `Mrf24j::set_promiscuous()` es `protected` — Radio_t no puede accederlo
- [ ] **Pendiente:** Corregir `-Wignored-qualifiers` en `spi.hpp`, `gpio.hpp`
- [ ] **Pendiente:** Probar compilación completa en Raspberry Pi

### Documentación Doxygen
- [x] `main.cpp` (ambos proyectos)
- [x] `config/config.cpp`, `file/file.cpp`, `tyme/tyme.cpp`
- [x] `security/encrypt.cpp`, `security/decrypt.cpp`
- [x] `interrupt/interrupt.cpp`, `work/rfflush.cpp`
- [x] `display/epaper.cpp`, `qr/ff.cpp`, `spi/spi_dbg.cpp`
- [x] `oled/oled/oled.cpp`
- [ ] Headers `.hpp` restantes (`radio/`, `display/`, `gpio/`, `spi/`, `work/`, `mrf24/`)
- [ ] Archivos `.cpp` de `mrf24/` (`radio_trasnreceiver`, `mrf24j40_send`, `zigbee_packet_handler`, etc.)

### MQTT (Mosquitto)
- [x] Crear `mqtt_handler.hpp/.cpp` — Cliente MQTT con reconexión automática
- [x] Crear `mqtt_bridge.hpp/.cpp` — Puente radio ↔ MQTT
- [x] Actualizar Makefiles con libmosquitto y fuentes
- [ ] Actualizar `config.hpp` con constantes MQTT
- [ ] Actualizar `main.cpp` para integrar MQTT al inicio
- [ ] Probar suscripción y publicación en Raspberry Pi

### Estructura prompt.txt (arquitectura modular)
- [ ] Migrar a estructura `hal/`, `drivers/`, `services/`, `application/`
- [ ] Implementar driver completo MRF24J40 con modos (Router, End Device, Gateway, Nodo)
- [ ] Implementar cifrado AES-256-CBC + SHA-256
- [ ] Implementar menú interactivo con 9 opciones
- [ ] Implementar displays: SSD1306 (OLED) + ST7789 (TFT color)
- [ ] Implementar QR (generación + renderizado a OLED, TFT, PNG)
- [ ] Implementar sistema de archivos (config JSON + logs rotativos)

### Infraestructura
- [x] `VERSION.txt` con historial de versiones
- [x] `scripts_tools/auto_version.sh` para auto-bump
- [x] `Doxyfile` + target `make docs`
- [ ] `scripts_tools/install_mosquitto.sh` — script de instalación de dependencias MQTT

---

## 🔧 Proyecto mrf24_tx (Transmisor)

### Pendientes
- [ ] Verificar transmisión a dirección 0x0002 con ACK
- [ ] Integrar `MqttBridge` en main para publicar estado TX por MQTT
- [ ] Publicar estadísticas TX por MQTT periódicamente

---

## 🔧 Proyecto mrf24_rx (Receptor)

### Pendientes
- [ ] Verificar recepción y extracción de payload
- [ ] Traducir paquetes ZigBee recibidos a eventos MQTT
- [ ] Agregar soporte OLED y display de mensajes recibidos

---

## 🧪 Testing en Raspberry Pi

```bash
cd /home/joy/src/mrf24j40/ && git pull
cd mrf24_tx && make clean && make -j4
cd ../mrf24_rx && make clean && make -j4

# Ejecutar (requiere sudo)
sudo ./mrf24_tx/bin/mrf24_transmitter
sudo ./mrf24_rx/bin/mrf24_receiver
```
