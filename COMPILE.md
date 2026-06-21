# Guía de Compilación — MRF24J40 IoT

## Estructura del proyecto

```
mrf24j40/
├── mrf24_security/   → 🔐 Binario unificado (mrf24j40_iot)
│   ├── hal/          →   GPIO, SPI, I2C
│   ├── drivers/      →   MRF24J40, SSD1306, ST7789, QR
│   ├── services/     →   Crypto, FileSystem, Timer
│   └── application/  →   RadioManager, Menu
├── mrf24_tx/         → 🚀 Transmisor legacy (mrf24_transmitter)
├── mrf24_rx/         → 📡 Receptor legacy (mrf24_transmitter)
├── mrf24-dashboard/  → 🌐 Dashboard web interactivo
└── scripts_tools/    → Scripts de configuración
```

---

## 1. Compilación local (todo desde la raíz)

### Compilar todo
```bash
cd /home/joy/src/mrf24j40
make          # Compila mrf24_security + mrf24_tx + mrf24_rx
make -j4      # Compila en paralelo (4 núcleos)
```

### Compilar solo un subproyecto
```bash
make -C mrf24_security    # Solo el binario unificado
make -C mrf24_tx          # Solo el transmisor legacy
make -C mrf24_rx          # Solo el receptor legacy
```

### Compilar y ejecutar
```bash
make run-unificado   # Compila y ejecuta mrf24_security/bin/mrf24j40_iot
make run-tx          # Compila y ejecuta mrf24_tx/bin/mrf24_transmitter
make run-rx          # Compila y ejecuta mrf24_rx/bin/mrf24_transmitter
```

### Limpiar
```bash
make clean            # Limpia todos los subproyectos
make -C mrf24_tx clean  # Limpia solo un subproyecto
```

### Información de compilación
```bash
make info           # Muestra flags, librerías y fuentes de cada subproyecto
```

---

## 2. Gestión de ramas (branches)

### Ver en qué rama estás
```bash
git branch            # Muestra la rama actual (con *)
git branch -a         # Muestra todas las ramas, locales y remotas
git branch --show-current   # Solo el nombre de la rama actual
```

### Cambiar de rama
```bash
git switch release/V3        # Cambiar a la rama de desarrollo
git switch main               # Cambiar a la rama principal
git checkout release/V3       # Alternativa clásica a switch
```

### Verificar rama en remoto (Raspberry Pi)
```bash
ssh joy@raspberry.local "cd /home/joy/src/mrf24j40 && git branch --show-current"
```

### Traer una rama remota por primera vez
```bash
git fetch origin
git switch -c release/V3 origin/release/V3   # Crear y cambiar a rama local trackeando la remota
```

### Actualizar la rama local desde el remoto
```bash
git pull                    # Baja cambios de la rama actual (fast-forward)
git pull origin release/V3  # Baja cambios de una rama específica
```

### Confirmar que la rama local está sincronizada con el remoto
```bash
# En remoto:
ssh joy@raspberry.local "cd /home/joy/src/mrf24j40 && \
  echo 'Rama:'   && git branch --show-current && \
  echo 'Último:' && git log --oneline -1"
```

---

## 3. Compilación remota vía SSH (Raspberry Pi)

### Requisitos
- Acceso SSH configurado a la Raspberry Pi
- Claves SSH (recomendado) o passphrase
- Git configurado en el remoto
- Estar en la rama correcta (ver sección 2)

### Método 1: Script manual completo (asegurando rama correcta)
```bash
ssh joy@raspberry.local "cd /home/joy/src/mrf24j40 && \
  echo 'Rama actual:' && git branch --show-current && \
  git pull && \
  make -j4"
```

### Método 2: Cambiar rama + pull + compilar
```bash
ssh joy@raspberry.local "cd /home/joy/src/mrf24j40 && \
  git switch release/V3 && \
  git pull && \
  make clean && make -j4"
```

### Método 3: Compilar subproyectos individualmente
```bash
# Solo el binario unificado (mrf24j40_iot)
ssh joy@raspberry.local "cd /home/joy/src/mrf24j40/mrf24_security && make clean && make -j4"

# Solo el transmisor legacy
ssh joy@raspberry.local "cd /home/joy/src/mrf24j40/mrf24_tx && make clean && make -j4"

# Solo el receptor legacy
ssh joy@raspberry.local "cd /home/joy/src/mrf24j40/mrf24_rx && make clean && make -j4"
```

### Método 4: Compilar y ejecutar en remoto
```bash
ssh joy@raspberry.local "cd /home/joy/src/mrf24j40 && \
  git branch --show-current && \
  git pull && \
  make -j4 && \
  sudo ./mrf24_security/bin/mrf24j40_iot"
```

### Método 5: Compilar tx + rx secuencialmente
```bash
ssh joy@raspberry.local "cd /home/joy/src/mrf24j40 && \
  cd mrf24_tx && make -j4 && \
  cd ../mrf24_rx && make -j4"
```

---

## 4. Flujo completo de desarrollo remoto

### Push desde máquina local + pull y compilar en Pi

```bash
# 1. En máquina local: hacer cambios y pushear
git add -A
git commit -m "descripción de los cambios"
git push origin refs/heads/release/V3

# 2. En Pi: verificar rama, traer cambios y compilar
ssh joy@raspberry.local "cd /home/joy/src/mrf24j40 && \
  echo '=== Rama actual ===' && \
  git branch --show-current && \
  echo '=== Pull ===' && \
  git pull && \
  echo '=== Compilar ===' && \
  make clean && \
  make -j4"
```

### Script automatizado completo (con verificación de rama)
```bash
#!/bin/bash
# compile_remote.sh
USER="joy"
HOST="raspberry.local"
PROJECT_DIR="/home/$USER/src/mrf24j40"
BRANCH="release/V3"

echo "=== Verificar rama en remoto ==="
CURRENT_BRANCH=$(ssh $USER@$HOST "cd $PROJECT_DIR && git branch --show-current")
echo "Rama remota: $CURRENT_BRANCH"

if [ "$CURRENT_BRANCH" != "$BRANCH" ]; then
    echo "⚠️  La rama remota ($CURRENT_BRANCH) no es la esperada ($BRANCH)"
    echo "   Cambiando a $BRANCH..."
    ssh $USER@$HOST "cd $PROJECT_DIR && git switch $BRANCH"
fi

echo "=== Pull ==="
ssh $USER@$HOST "cd $PROJECT_DIR && git pull"

echo "=== Compilar mrf24_security ==="
ssh $USER@$HOST "cd $PROJECT_DIR/mrf24_security && make -j4"

echo "=== Compilar mrf24_tx ==="
ssh $USER@$HOST "cd $PROJECT_DIR/mrf24_tx && make -j4"

echo "=== Compilar mrf24_rx ==="
ssh $USER@$HOST "cd $PROJECT_DIR/mrf24_rx && make -j4"

echo "=== Listo ==="
```

---

## 4. Dashboard web

```bash
make serve-dashboard   # Inicia servidor HTTP en http://localhost:8080
```

Sirve el dashboard interactivo desde `mrf24-dashboard/`. Abrir en el navegador:

```
http://localhost:8080/mrf24-dashboard/index.html
```

---

## 5. Dependencias del sistema (Raspberry Pi OS)

```bash
# Librerías esenciales
sudo apt-get install libbcm2835-dev

# Librerías opcionales (se detectan automáticamente)
sudo apt-get install libqrencode-dev     # Generación de QR
sudo apt-get install libpng-dev          # Exportar QR a PNG
sudo apt-get install zlib1g-dev          # Compresión (PNG)
sudo apt-get install libmosquitto-dev    # MQTT
sudo apt-get install nlohmann-json-dev   # Configuración JSON

# OpenSSL (para cifrado AES-256, normalmente ya instalado)
sudo apt-get install libssl-dev
```

---

## 6. Solución de problemas comunes

| Error | Causa | Solución |
|-------|-------|----------|
| `bcm2835.h: No such file` | Falta libbcm2835-dev | `sudo apt-get install libbcm2835-dev` |
| `Cannot open /dev/spidev0.0` | SPI no habilitado | `sudo raspi-config` → Interface Options → SPI → Enable |
| `qrencode.h: No such file` | Falta libqrencode-dev | `sudo apt-get install libqrencode-dev` |
| `undefined reference to SHA256` | Falta libssl-dev | `sudo apt-get install libssl-dev` |
| `make: *** No rule to make target` | Archivo fuente faltante | Verificar que el archivo existe y está en git: `git ls-files` |
| `Permission denied` al ejecutar | Faltan permisos root | Ejecutar con `sudo` o agregar usuario al grupo SPI |
