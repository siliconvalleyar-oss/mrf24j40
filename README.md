# MRF24J40 - Transmisor y Receptor ZigBee para Raspberry Pi

Proyecto C++ para comunicación inalámbrica usando el módulo **MRF24J40MA** en Raspberry Pi.

## 📦 Estructura del Proyecto

```
├── .gitignore
├── Makefile                  # Makefile principal (proyecto completo)
├── README.md
├── flow/                     # Diagramas de comunicación (DrawIO)
├── scripts_tools/            # Scripts de instalación y desarrollo
│   ├── install_tools.sh      # [NUEVO] Instalación consolidada de dependencias
│   ├── dev_tools.sh          # [NUEVO] Herramientas de desarrollo unificadas
│   ├── commands_mosquitto.sh # Comandos de referencia MQTT
│   ├── rules.gdb             # Configuración de debug GDB
│   └── ... (scripts originales mantenidos por compatibilidad)
├── mrf24_tx/                 # 🚀 PROYECTO TRANSMISOR
│   ├── Makefile
│   ├── src/
│   │   ├── main.cpp          # Punto de entrada del transmisor
│   │   └── mrf24j40.cpp      # Driver MRF24J40 (SPI + ZigBee)
│   └── include/
└── mrf24_rx/                 # 📡 PROYECTO RECEPTOR
    ├── Makefile
    ├── src/
    │   ├── main.cpp           # Punto de entrada del receptor
    │   └── mrf24j40.cpp       # Driver MRF24J40 (SPI + ZigBee)
    └── include/
```

---

## 🚀 Proyecto Transmisor (`mrf24_tx/`)

Envia paquetes de datos a través del módulo MRF24J40MA usando protocolo ZigBee.

| Parámetro       | Valor  |
|-----------------|--------|
| PAN ID          | `0xCAFE` |
| Dirección propia | `0x6001` |
| Dirección destino | `0x6002` |
| Canal           | 24 |
| Intervalo       | 2000 ms |

**Compilar:**
```bash
cd mrf24_tx
make
```

**Ejecutar:**
```bash
sudo ./bin/mrf24_transmitter
```

---

## 📡 Proyecto Receptor (`mrf24_rx/`)

Recibe paquetes de datos del transmisor y muestra la información recibida.

| Parámetro       | Valor  |
|-----------------|--------|
| PAN ID          | `0xCAFE` |
| Dirección propia | `0x6002` |
| Canal           | 24 |

**Compilar:**
```bash
cd mrf24_rx
make
```

**Ejecutar:**
```bash
sudo ./bin/mrf24_receiver
```

---

## 🔧 Dependencias

### Instalación Rápida

```bash
# Instalar todo
./scripts_tools/install_tools.sh all

# O instalar por partes:
./scripts_tools/install_tools.sh basics     # Dependencias básicas
./scripts_tools/install_tools.sh bcm2835    # Librería BCM2835
./scripts_tools/install_tools.sh mosquitto  # Mosquitto MQTT
./scripts_tools/install_tools.sh oled       # SSD1306 OLED
./scripts_tools/install_tools.sh database   # MySQL/MariaDB
```

### Dependencias del Sistema

- **BCM2835** - Librería GPIO/SPI para Raspberry Pi
- **libpng-dev / zlib1g-dev** - Procesamiento de imágenes PNG
- **qrencode** - Generación de códigos QR
- **libmosquitto-dev** - Cliente MQTT
- **libmysqlcppconn-dev** - Conector MySQL/MariaDB
- **libssl-dev** - Criptografía
- **SSD1306_OLED_RPI** - Librería para display OLED (opcional)

---

## 🛠️ Herramientas de Desarrollo

### Script Unificado

```bash
# Ver ayuda
./scripts_tools/dev_tools.sh

# Gestión de GPIO
./scripts_tools/dev_tools.sh gpio settings   # Configurar pines MRF24J40
./scripts_tools/dev_tools.sh gpio list       # Listar estado de GPIOs
./scripts_tools/dev_tools.sh gpio output_low # Resetear GPIOs

# Servicio Mosquitto
./scripts_tools/dev_tools.sh mosquitto status
./scripts_tools/dev_tools.sh mosquitto start|stop|restart

# Compilar proyectos
./scripts_tools/dev_tools.sh build tx        # Compilar transmisor
./scripts_tools/dev_tools.sh build rx        # Compilar receptor
./scripts_tools/dev_tools.sh build all       # Compilar ambos

# Utilidades Git
./scripts_tools/dev_tools.sh git commit      # Commit automático
./scripts_tools/dev_tools.sh git certificate # Configurar SSH GitHub

# Configurar swap
./scripts_tools/dev_tools.sh swap create     # Crear swap 512MB

# Debug
./scripts_tools/dev_tools.sh debug           # GDB con reglas
```

### Scripts Originales

Los scripts originales se mantienen por compatibilidad. Se recomienda usar `install_tools.sh` y `dev_tools.sh`.

---

## 🔌 Configuración de GPIO

Pines utilizados por el MRF24J40:

| GPIO | Función  | Pin Físico |
|------|----------|------------|
| MOSI | SPI MOSI | 19         |
| MISO | SPI MISO | 21         |
| SCK  | SPI Clock| 23         |
| CS   | Chip Select | 24      |
| INT  | Interrupción | 18      |
| WAKE | Wake     | 22         |
| RESET| Reset    | 16         |

---

## 📡 Comunicación

### Verificación Rápida

**Terminal 1 (Receptor):**
```bash
sudo ./mrf24_rx/bin/mrf24_receiver
```

**Terminal 2 (Transmisor):**
```bash
sudo ./mrf24_tx/bin/mrf24_transmitter
```

### Configuración SSH Segura

```bash
ssh-keygen -t rsa
ssh-copy-id root@127.0.0.1
```

---

## 📊 Versiones

| Versión | Cambios |
|---------|---------|
| 1.4     | Seguridad, ruteo, capas, modo sleep |
| 1.3     | SPI/GPIO por BCM2835 |
| 1.2     | ACK, epaper, encriptación, router/coordinator/end device |
| 1.1     | Envío correcto con header, size, buffer, checksum |
| 1.0.1   | Versión inicial |

---

## 📖 Referencias

- [Microchip MRF24J40MA](http://www.microchip.com/wwwproducts/Devices.aspx?dDocName=en535967)
- [BCM2835 Library](http://www.airspayce.com/mikem/bcm2835/)
- [SSD1306_OLED_RPI](https://github.com/gavinlyonsrepo/SSD1306_OLED_RPI)
