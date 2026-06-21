# TODO — MRF24J40 IoT System

## Estado actual
- **Último tag:** v2.0.2
- **Rama activa:** `release/V3` (nueva) ← `release/V2` (anteriormente `v2.0.0`)
- **Último commit:** `dc838f5` — implementar Mrf24j::settingsSecurity()
- **Arquitectura target:** Raspberry Pi (ARM aarch64 / armv7l)
- **Nuevo proyecto unificado:** `mrf24_security/` — HAL + Drivers + Services + Application

---

## ✅ Realizado

### Infraestructura y organización
- [x] Crear estructura `mrf24_security/` con `hal/`, `drivers/`, `services/`, `application/`
- [x] Mover código de primera plana a `mrf24_security/`
- [x] Crear `mrf24-dashboard/` — web interactiva con explorador, visor de código y dashboard animado
- [x] Makefile raíz con target `serve-dashboard` (python3 http.server)
- [x] `VERSION.txt` con historial de versiones
- [x] `scripts_tools/auto_version.sh` para auto-bump
- [x] `Doxyfile` + target `make docs`

### Drivers y comunicación
- [x] Crear header `mrf24j40.h` (driver simplificado Mrf24j40)
- [x] Agregar `using Mrf24j_t = Mrf24j;` en `mrf24j40.hpp`
- [x] Implementar `Mrf24j::settingsSecurity()` — deshabilita cifrado hardware
- [x] Mover `set_promiscuous()` de `protected` a `public` en `Mrf24j`
- [x] Eliminar stubs vacíos de mosquitto
- [x] Limpiar Makefiles (referencias a stubs eliminados)
- [x] Fix `#include <cstdint>` faltante en `mrf24j40_settings.hpp`

### MQTT (Mosquitto)
- [x] Crear `mqtt_handler.hpp/.cpp` — Cliente MQTT con reconexión automática
- [x] Crear `mqtt_bridge.hpp/.cpp` — Puente radio ↔ MQTT
- [x] Actualizar Makefiles con libmosquitto y fuentes

### Criptografía
- [x] Implementar `crypto.cpp/.hpp` (AES-256-CBC + SHA-256 con OpenSSL) en `mrf24_security/services/`

### Dashboard Web (`mrf24-dashboard/`)
- [x] Explorador de archivos con árbol colapsable y búsqueda en tiempo real
- [x] Visor de código con syntax highlighting (highlight.js)
- [x] Renderizado de Markdown (marked.js)
- [x] Dashboard con logo SVG animado (ondas de señal, dots volando)
- [x] Tarjetas de estadísticas con colores distintivos
- [x] Gráfico de tráfico en tiempo real (Canvas con requestAnimationFrame)
- [x] Diagrama de arquitectura SVG con capas y puntos animados
- [x] Barra de estado con LEDs parpadeantes
- [x] Tema oscuro terminal, responsive
- [x] Botones de acceso rápido (README, TODO, VERSION, MAIN, etc.)

### Compilación — Fixes aplicados
- [x] Crear header `mrf24j40.h` (driver simplificado Mrf24j40)
- [x] Corregir rutas de fuentes en Makefiles (`oled/oled.cpp`, `fonts/font5x7.cpp`)
- [x] Eliminar `src/radio/data.hpp` (archivo vacío que shadoweaba a `include/radio/data.hpp`)
- [x] Fix stray characters en `mrf24j40_template.cpp` (`*/` dentro de `/** */`)
- [x] Agregar declaración `send_template` en header `mrf24j40.hpp`
- [x] Fix constructor `GPIO::Gpio_t` — pasar `m_status`

### Documentación Doxygen
- [x] `main.cpp` (ambos proyectos)
- [x] `config/config.cpp`, `file/file.cpp`, `tyme/tyme.cpp`
- [x] `security/encrypt.cpp`, `security/decrypt.cpp`
- [x] `interrupt/interrupt.cpp`, `work/rfflush.cpp`
- [x] `display/epaper.cpp`, `qr/ff.cpp`, `spi/spi_dbg.cpp`
- [x] `oled/oled/oled.cpp`

---

## 📋 Pendiente — Nuevo Proyecto Unificado (`mrf24_security/`)

### Compilación
- [ ] **Pendiente:** Corregir `-Wignored-qualifiers` en `spi.hpp`, `gpio.hpp`
- [ ] **Pendiente:** Probar compilación completa en Raspberry Pi

### Estructura de archivos (completar implementación)
- [ ] `hal/gpio.hpp` / `hal/gpio.cpp`
- [ ] `hal/spi.hpp` / `hal/spi.cpp`
- [ ] `hal/i2c.hpp` / `hal/i2c.cpp`
- [ ] `drivers/mrf24j40.hpp` / `drivers/mrf24j40.cpp` — driver completo con modos
- [ ] `drivers/ssd1306.hpp` / `drivers/ssd1306.cpp` — OLED con herencia de graphics base
- [ ] `drivers/st7789.hpp` / `drivers/st7789.cpp` — TFT color RGB565
- [ ] `drivers/qr.hpp` / `drivers/qr.cpp` — generación y renderizado
- [ ] `drivers/graphics_base.hpp` — base para drivers gráficos
- [ ] `services/crypto.hpp` / `services/crypto.cpp` — AES-256-CBC + SHA-256 (ya implementado parcialmente)
- [ ] `services/filesystem.hpp` / `services/filesystem.cpp` — JSON + logs rotativos
- [ ] `services/timer.hpp` / `services/timer.cpp` — delays, timeouts
- [ ] `application/radio_manager.hpp` / `application/radio_manager.cpp` — orquestación, protocolo
- [ ] `application/menu.hpp` / `application/menu.cpp` — interfaz de usuario
- [ ] `main.cpp` — punto de entrada, inicialización y lanzamiento del menú

### MRF24J40 — Modos de operación
- [ ] Implementar modo **Router** — reenvío de paquetes entre nodos
- [ ] Implementar modo **End Device** — solo se comunica con su coordinador
- [ ] Implementar modo **Gateway** — puente entre ZigBee y red externa
- [ ] Implementar modo **Nodo** — comportamiento flexible (configurable)
- [ ] Inicialización con canal, PAN ID, dirección MAC (64 bits)
- [ ] Configuración de modo que afecta el comportamiento de reenvío

### Protocolo seguro (AES-256-CBC + SHA-256)
- [ ] **Envío:** cifrar payload con AES-256-CBC, IV aleatorio antepuesto al ciphertext
- [ ] **Envío:** calcular SHA-256 del ciphertext+IV y añadir al final (opcional)
- [ ] **Trama final:** `[dest_mac (8 bytes)] [size (2 bytes)] [iv (16 bytes)] [ciphertext] [hash (32 bytes)]`
- [ ] **Recepción:** verificar destino (comparar MAC local)
- [ ] **Recepción:** extraer IV y ciphertext, descifrar
- [ ] **Recepción:** verificar hash (si existe) para integridad
- [ ] **Recepción:** mostrar mensaje descifrado

### Displays
- [ ] **SSD1306 (OLED)** — fuentes 5x7, 7x8, 8x16
  - [ ] drawPixel, drawLine, drawRect, fillRect
  - [ ] drawCircle, drawString, clear, update
- [ ] **ST7789 (TFT color)** — resolución configurable, RGB565
  - [ ] fillScreen, drawPixel, drawLine, drawRect
  - [ ] fillRect, drawCircle, drawString, drawBitmap

### Códigos QR
- [ ] Generar matriz QR desde `std::string` (libqrencode)
- [ ] Renderizar en consola (ASCII)
- [ ] Renderizar en OLED (SSD1306)
- [ ] Renderizar en TFT (ST7789)
- [ ] Guardar como PNG (libpng)

### Menú interactivo (9 opciones)
- [ ] 1. Configurar modo MRF24J40 (Router/End Device/Gateway/Nodo)
- [ ] 2. Enviar mensaje cifrado (ingresar destino y texto)
- [ ] 3. Escuchar mensajes recibidos (modo receptor)
- [ ] 4. Generar QR desde texto ingresado
- [ ] 5. Probar OLED (mostrar texto, gráficos, QR)
- [ ] 6. Probar ST7789 (mostrar colores, formas, QR)
- [ ] 7. Ver/editar configuración JSON
- [ ] 8. Ver logs
- [ ] 9. Salir

### Sistema de archivos (JSON)
- [ ] Guardar configuración: PAN ID, canal, MAC propia, clave de cifrado, modo de operación
- [ ] Cargar configuración al inicio
- [ ] Guardar al modificar desde el menú

### Logging
- [ ] Registrar eventos con timestamp
- [ ] Registrar LQI y RSSI de paquetes recibidos
- [ ] Archivo rotativo (opcional)

### Documentación Doxygen (pendiente)
- [ ] Headers `.hpp` restantes (`radio/`, `display/`, `gpio/`, `spi/`, `work/`, `mrf24/`)
- [ ] Archivos `.cpp` de `mrf24/` (`radio_trasnreceiver`, `mrf24j40_send`, `zigbee_packet_handler`, etc.)

### Infraestructura
- [ ] `scripts_tools/install_mosquitto.sh` — script de instalación de dependencias MQTT
- [ ] Proveer `CMakeLists.txt` para compilar el proyecto unificado

---

## 🔧 Proyecto mrf24_tx (Transmisor — Legacy)

### Pendientes
- [ ] Verificar transmisión a dirección 0x0002 con ACK
- [ ] Integrar `MqttBridge` en main para publicar estado TX por MQTT
- [ ] Publicar estadísticas TX por MQTT periódicamente
- [ ] **Nota:** Este proyecto será reemplazado por `mrf24_security` cuando esté completo.

---

## 🔧 Proyecto mrf24_rx (Receptor — Legacy)

### Pendientes
- [ ] Verificar recepción y extracción de payload
- [ ] Traducir paquetes ZigBee recibidos a eventos MQTT
- [ ] Agregar soporte OLED y display de mensajes recibidos
- [ ] **Nota:** Este proyecto será reemplazado por `mrf24_security` cuando esté completo.

---

## 🧪 Testing en Raspberry Pi

```bash
cd /home/joy/src/mrf24j40/ && git pull

# Compilar proyectos legacy
cd mrf24_tx && make clean && make -j4
cd ../mrf24_rx && make clean && make -j4

# Compilar proyecto unificado (cuando esté listo)
cd ../mrf24_security && make clean && make -j4

# Ejecutar (requiere sudo)
sudo ./mrf24_tx/bin/mrf24_transmitter
sudo ./mrf24_rx/bin/mrf24_receiver
sudo ./mrf24_security/bin/mrf24_security  # cuando esté listo

# Servir dashboard web
make serve-dashboard
# o
cd mrf24-dashboard && python3 -m http.server 8080
