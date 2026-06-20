#!/bin/bash
# =============================================================================
# dev_tools.sh - Herramientas de desarrollo para MRF24J40
# =============================================================================
# Uso: ./dev_tools.sh <comando> [subcomando] [args...]
#
# Comandos principales:
#
#   gpio          Gestión de GPIO (Raspberry Pi)
#     set           Exportar y configurar pines GPIO
#     list          Listar estado de pines GPIO
#     output_low    Configurar todos los GPIO como salida LOW
#     info          Mostrar información de GPIO
#     chmod         Configurar permisos de GPIO
#     settings      Configurar pines específicos MRF24J40
#
#   mosquitto     Gestión del servicio Mosquitto
#     status        Ver estado del servicio
#     start         Iniciar servicio
#     stop          Detener servicio
#     restart       Reiniciar servicio
#     enable        Habilitar inicio automático
#
#   git           Utilidades Git
#     commit        Commit automático con timestamp
#     certificate   Configurar SSH key para GitHub
#     search <ref>  Buscar commit por fecha o hacer checkout
#
#   build         Compilar proyectos
#     tx            Compilar transmisor
#     rx            Compilar receptor
#     all           Compilar ambos
#     run           Ejecutar make run
#     repeat        Ejecutar make run 100 veces
#
#   radio         Configurar modo radio
#     tx            Configurar como transmisor
#     rx            Configurar como receptor
#
#   swap          Configurar swap (512MB)
#     create        Crear archivo swap
#     resize        Redimensionar swap vía dphys-swapfile
#
#   debug         Ejecutar debug con GDB
# =============================================================================

# NOTA: algunas funciones requieren sudo
# Uso: ./dev_tools.sh <comando> [subcomando]

ARCH=$(uname -m)
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
COMPONENT=$1
SUBCOMMAND=$2
shift 2 2>/dev/null || true

# -------------------------------------------------------
# GPIO: Pines reservados (SPI, UART, I2C, USB)
# -------------------------------------------------------
RESERVED_GPIO=(2 3 9 10 11 14 15 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 44 45 46 47)

is_reserved() {
    local pin=$1
    for reserved in "${RESERVED_GPIO[@]}"; do
        if [[ "$pin" -eq "$reserved" ]]; then
            return 0
        fi
    done
    return 1
}

# -------------------------------------------------------
# GPIO: Determinar rango de pines según arquitectura
# -------------------------------------------------------
gpio_get_range() {
    if [[ $ARCH == "armv7l" || $ARCH == "arm" ]]; then
        echo "0 53"
    else
        echo "0 53"
    fi
}

# -------------------------------------------------------
# GPIO: Exportar pines
# -------------------------------------------------------
gpio_set() {
    echo "--- Exportando pines GPIO ---"
    read INIT END <<< $(gpio_get_range)
    for PIN in $(seq $INIT $END); do
        if [[ -e /sys/class/gpio/gpio$PIN ]]; then
            echo "GPIO $PIN ya existe"
        else
            echo $PIN | sudo tee /sys/class/gpio/export > /dev/null 2>&1 || true
        fi
    done
    echo "✅ Pines GPIO exportados ($INIT..$END)"
}

# -------------------------------------------------------
# GPIO: Información
# -------------------------------------------------------
gpio_info() {
    echo "--- Información de GPIO ---"
    gpioinfo 2>/dev/null || echo "(gpioinfo no disponible)"
    echo ""
    ls -1l /sys/class/gpio 2>/dev/null || true
}

# -------------------------------------------------------
# GPIO: Listar
# -------------------------------------------------------
gpio_list() {
    echo "--- Listando GPIOs ---"
    read INIT END <<< $(gpio_get_range)
    
    if [[ $INIT -gt 500 ]]; then
        RESULT1=$(($INIT - 512))
        RESULT2=$(($END - 512))
        echo "Pines GPIO (offset 512): $RESULT1..$RESULT2"
        for PIN in $(seq $RESULT1 $RESULT2); do
            sudo raspi-gpio get $PIN 2>/dev/null || true
        done
    else
        echo "Pines GPIO: $INIT..$END"
        for PIN in $(seq $INIT $END); do
            sudo raspi-gpio get $PIN 2>/dev/null || true
        done
    fi
}

# -------------------------------------------------------
# GPIO: Configurar permisos
# -------------------------------------------------------
gpio_chmod() {
    echo "--- Configurando permisos GPIO ---"
    sudo chgrp gpio /sys/class/gpio/export 2>/dev/null || true
    sudo chgrp gpio /sys/class/gpio/unexport 2>/dev/null || true
    sudo chmod 660 /sys/class/gpio/export 2>/dev/null || true
    sudo chmod 660 /sys/class/gpio/unexport 2>/dev/null || true
    
    read INIT END <<< $(gpio_get_range)
    for PIN in $(seq $INIT $END); do
        if [[ -e /sys/class/gpio/gpio$PIN ]]; then
            sudo chgrp gpio /sys/class/gpio/gpio$PIN/value 2>/dev/null || true
            sudo chgrp gpio /sys/class/gpio/gpio$PIN/direction 2>/dev/null || true
            sudo chmod 660 /sys/class/gpio/gpio$PIN/value 2>/dev/null || true
            sudo chmod 660 /sys/class/gpio/gpio$PIN/direction 2>/dev/null || true
        fi
    done
    echo "✅ Permisos GPIO configurados"
    
    # Debug: mostrar estado
    echo ""
    echo "--- Debug GPIO ---"
    sudo ls -l /sys/class/gpio 2>/dev/null || true
    sudo cat /sys/kernel/debug/gpio 2>/dev/null || true
}

# -------------------------------------------------------
# GPIO: Configurar pines como salida LOW
# -------------------------------------------------------
gpio_output_low() {
    echo "--- Configurando GPIO como salida LOW ---"
    for pin in $(seq 0 27); do
        if ! is_reserved "$pin"; then
            sudo raspi-gpio set $pin op 2>/dev/null || true
            sudo raspi-gpio set $pin lo 2>/dev/null || true
        else
            echo "GPIO $pin está reservado - no modificado"
        fi
    done
    echo "✅ GPIOs configurados como salida LOW"
}

# -------------------------------------------------------
# GPIO: Configurar pines específicos MRF24J40
# -------------------------------------------------------
gpio_settings() {
    echo "--- Configurando pines MRF24J40 ---"
    
    if [[ $ARCH == "armv7l" || $ARCH == "arm" ]]; then
        PINS="16 19 20 21 22 23 26"
        echo "Sistema 32 bits detectado"
    else
        PINS="528 531 532 533 534 538"
        echo "Sistema 64 bits detectado"
    fi
    
    SET_PIN=ip
    STATUS_OUTPUT=pd
    
    for pin in $PINS; do
        if [[ $ARCH == "armv7l" || $ARCH == "arm" ]]; then
            sudo raspi-gpio set $pin $SET_PIN $STATUS_OUTPUT 2>/dev/null || true
            echo "GPIO $pin → input, pull-down"
        else
            resta=$(($pin - 512))
            sudo raspi-gpio set $resta $SET_PIN $STATUS_OUTPUT 2>/dev/null || true
            echo "GPIO $resta (físico $pin) → input, pull-down"
        fi
    done
    
    # Configurar GPIO 524 como test
    GPIO=524
    if [[ -e /sys/class/gpio/gpio$GPIO ]]; then
        echo "out" | sudo tee /sys/class/gpio/gpio$GPIO/direction > /dev/null
        echo "1" | sudo tee /sys/class/gpio/gpio$GPIO/value > /dev/null
        echo "0" | sudo tee /sys/class/gpio/gpio$GPIO/value > /dev/null
        echo "in" | sudo tee /sys/class/gpio/gpio$GPIO/direction > /dev/null
        echo "rising" | sudo tee /sys/class/gpio/gpio$GPIO/edge > /dev/null
        echo "falling" | sudo tee /sys/class/gpio/gpio$GPIO/edge > /dev/null
    fi
    
    echo "✅ Pines MRF24J40 configurados"
}

# -------------------------------------------------------
# GPIO: Menú principal
# -------------------------------------------------------
gpio_menu() {
    case $SUBCOMMAND in
        set)        gpio_set ;;
        list)       gpio_list ;;
        output_low) gpio_output_low ;;
        info)       gpio_info ;;
        chmod)      gpio_chmod ;;
        settings)   gpio_settings ;;
        *)
            echo "Uso: $0 gpio {set|list|output_low|info|chmod|settings}"
            echo ""
            echo "  set         Exportar pines GPIO"
            echo "  list        Listar estado de pines"
            echo "  output_low  Configurar como salida LOW"
            echo "  info        Mostrar información"
            echo "  chmod       Configurar permisos"
            echo "  settings    Configurar pines MRF24J40"
            exit 1
            ;;
    esac
}

# -------------------------------------------------------
# MOSQUITTO: Gestión del servicio
# -------------------------------------------------------
mosquitto_menu() {
    case $SUBCOMMAND in
        status)
            sudo systemctl status mosquitto
            ;;
        start)
            sudo systemctl start mosquitto
            echo "✅ Mosquitto iniciado"
            ;;
        stop)
            sudo systemctl stop mosquitto
            echo "⏹  Mosquitto detenido"
            ;;
        restart)
            sudo systemctl restart mosquitto
            echo "🔄 Mosquitto reiniciado"
            ;;
        enable)
            sudo systemctl enable mosquitto
            echo "✅ Mosquitto habilitado al inicio"
            ;;
        *)
            echo "Uso: $0 mosquitto {status|start|stop|restart|enable}"
            exit 1
            ;;
    esac
}

# -------------------------------------------------------
# GIT: Commit automático con timestamp
# -------------------------------------------------------
git_commit() {
    BRANCH=${SUBCOMMAND:-main}
    timestamp=$(date +'%Y%m%d%H%M')
    commit_message="update $timestamp"
    
    echo "Mensaje: $commit_message"
    echo "Branch:  $BRANCH"
    
    git add .
    git commit -m "$commit_message"
    git push -u origin "$BRANCH"
    
    echo "✅ Commit y push completados"
}

# -------------------------------------------------------
# GIT: Configurar certificado SSH
# -------------------------------------------------------
git_certificate() {
    echo "--- Configurando SSH key para GitHub ---"
    read -p "Email GitHub: " MAIL
    
    ssh-keygen -t ed25519 -C "$MAIL"
    
    echo ""
    echo "=== TU CLAVE PÚBLICA (cópiala en GitHub Settings > SSH keys) ==="
    cat ~/.ssh/id_ed25519.pub
    echo ""
    echo "=============================================================="
    
    echo ""
    echo "Agregando al agente SSH..."
    eval "$(ssh-agent -s)"
    ssh-add ~/.ssh/id_ed25519
    
    echo ""
    echo "Probando conexión con GitHub..."
    ssh -T git@github.com || true
    
    git config --global user.email "$MAIL"
    echo "Configurado user.email: $MAIL"
}

# -------------------------------------------------------
# GIT: Buscar/checkout por fecha
# -------------------------------------------------------
git_search() {
    local REF=${SUBCOMMAND}
    
    if [[ $REF == "date" ]]; then
        SINCE="2023-01-01"
        UNTIL="2024-10-01"
        git log --since=$SINCE --until=$UNTIL
        exit 0
    fi
    
    if [[ -z $REF ]]; then
        echo "Uso: $0 git search <referencia_commit>"
        echo "     $0 git search date   (buscar por fecha)"
        exit 1
    fi
    
    git checkout $REF
    echo "✅ Checkout a $REF completado"
}

# -------------------------------------------------------
# GIT: Menú principal
# -------------------------------------------------------
git_menu() {
    case $SUBCOMMAND in
        commit)       shift 2; git_commit "$@" ;;
        certificate)  git_certificate ;;
        search)       shift 2; git_search "$@" ;;
        *)
            echo "Uso: $0 git {commit|certificate|search}"
            echo ""
            echo "  commit        Commit automático con timestamp"
            echo "  certificate   Configurar SSH key para GitHub"
            echo "  search <ref>  Buscar commit o hacer checkout"
            exit 1
            ;;
    esac
}

# -------------------------------------------------------
# BUILD: Compilar proyectos
# -------------------------------------------------------
build_tx() {
    echo "=== COMPILANDO TRANSMISOR ==="
    cd "$PROJECT_ROOT/mrf24_tx"
    make clean 2>/dev/null || true
    make
    echo "✅ Transmisor compilado"
}

build_rx() {
    echo "=== COMPILANDO RECEPTOR ==="
    cd "$PROJECT_ROOT/mrf24_rx"
    make clean 2>/dev/null || true
    make
    echo "✅ Receptor compilado"
}

build_all() {
    build_tx
    build_rx
    echo "✅ Ambos proyectos compilados"
}

build_run() {
    echo "=== EJECUTANDO make run ==="
    cd "$PROJECT_ROOT"
    make run
}

build_repeat() {
    echo "=== EJECUTANDO make run x100 ==="
    for ((i=1; i<=100; i++)); do
        echo "--- Iteración $i ---"
        cd "$PROJECT_ROOT"
        make run
        echo "----------------"
    done
}

build_menu() {
    case $SUBCOMMAND in
        tx)      build_tx ;;
        rx)      build_rx ;;
        all)     build_all ;;
        run)     build_run ;;
        repeat)  build_repeat ;;
        *)
            echo "Uso: $0 build {tx|rx|all|run|repeat}"
            echo ""
            echo "  tx       Compilar transmisor"
            echo "  rx       Compilar receptor"
            echo "  all      Compilar ambos"
            echo "  run      Ejecutar make run"
            echo "  repeat   Ejecutar make run x100"
            exit 1
            ;;
    esac
}

# -------------------------------------------------------
# RADIO: Configurar modo (tx/rx)
# -------------------------------------------------------
radio_configure() {
    local MODE=$SUBCOMMAND
    
    if [[ -z $MODE ]]; then
        echo "Uso: $0 radio {tx|rx}"
        exit 1
    fi
    
    echo ""
    echo "⚠️  Nota: La configuración TX/RX se define automáticamente"
    echo "   según la arquitectura en los archivos:"
    echo "   - mrf24_tx/include/config/config.hpp"
    echo "   - mrf24_rx/include/config/config.hpp"
    echo ""
    echo "   ARM 32 bits   → USE_MRF24_RX (receptor)"
    echo "   ARM 64 bits   → USE_MRF24_TX (transmisor)"
    echo "   x86_64        → USE_MRF24_TX (transmisor)"
    echo ""
    echo "   Para cambiar manualmente, edita los defines en config.hpp"
    echo ""
    
    case $MODE in
        tx)
            echo "Configurando Makefile raíz como TRANSMISOR..."
            if [[ -f "$PROJECT_ROOT/Makefile" ]]; then
                cd "$PROJECT_ROOT"
                # Ajustar librería OLED según arquitectura
                if [[ $ARCH == "x86_64" || $ARCH == "aarch64" ]]; then
                    sed -i 's,LIBS += -lSSD1306_OLED_RPI,#LIBS += -lSSD1306_OLED_RPI ,g' Makefile 2>/dev/null || true
                fi
                echo "✅ Makefile raíz actualizado"
            else
                echo "ℹ️  No se encontró Makefile raíz, los proyectos son independientes"
            fi
            ;;
        rx)
            echo "Configurando Makefile raíz como RECEPTOR..."
            if [[ -f "$PROJECT_ROOT/Makefile" ]]; then
                cd "$PROJECT_ROOT"
                if [[ $ARCH == "armv7l" || $ARCH == "i386" || $ARCH == "i686" ]]; then
                    sed -i 's,#LIBS += -lSSD1306_OLED_RPI,LIBS += -lSSD1306_OLED_RPI,g' Makefile 2>/dev/null || true
                fi
                echo "✅ Makefile raíz actualizado"
            else
                echo "ℹ️  No se encontró Makefile raíz, los proyectos son independientes"
            fi
            ;;
        *)
            echo "Uso: $0 radio {tx|rx}"
            exit 1
            ;;
    esac
    
    echo "Configuración de radio completada."
}

# -------------------------------------------------------
# SWAP: Crear archivo swap
# -------------------------------------------------------
swap_create() {
    if [ "$EUID" -ne 0 ]; then
        echo "Por favor, ejecuta con sudo."
        exit 1
    fi
    
    SWAP_SIZE_MB=512
    SWAP_FILE="/swapfile"
    
    echo "--- Creando swap de ${SWAP_SIZE_MB}MB ---"
    
    swapoff $SWAP_FILE 2>/dev/null || true
    rm -f $SWAP_FILE
    
    dd if=/dev/zero of=$SWAP_FILE bs=1M count=$SWAP_SIZE_MB
    chmod 600 $SWAP_FILE
    mkswap $SWAP_FILE
    swapon $SWAP_FILE
    
    if ! grep -q "$SWAP_FILE" /etc/fstab; then
        echo "$SWAP_FILE none swap sw 0 0" >> /etc/fstab
        echo "✅ Swap agregado a /etc/fstab"
    fi
    
    echo "✅ Swap configurado:"
    swapon --show
    free -h
}

# -------------------------------------------------------
# SWAP: Redimensionar vía dphys-swapfile
# -------------------------------------------------------
swap_resize() {
    if [ "$(id -u)" -ne 0 ]; then
        echo "Este script debe ejecutarse como root. Usa sudo." >&2
        exit 1
    fi
    
    SWAP_FILE="/etc/dphys-swapfile"
    
    if [ ! -f "$SWAP_FILE" ]; then
        echo "El archivo $SWAP_FILE no existe." >&2
        echo "Usa 'swap create' para swap directo." >&2
        exit 1
    fi
    
    echo "--- Redimensionando swap vía dphys-swapfile ---"
    cp "$SWAP_FILE" "$SWAP_FILE.bak" 2>/dev/null || true
    sed -i 's/CONF_SWAPSIZE=.*/CONF_SWAPSIZE=512/' "$SWAP_FILE"
    systemctl restart dphys-swapfile
    
    echo "✅ Swap redimensionado:"
    free -h
}

swap_menu() {
    case $SUBCOMMAND in
        create)  swap_create ;;
        resize)  swap_resize ;;
        *)
            echo "Uso: $0 swap {create|resize}"
            echo ""
            echo "  create   Crear/redimensionar archivo swap (512MB)"
            echo "  resize   Redimensionar vía dphys-swapfile (512MB)"
            exit 1
            ;;
    esac
}

# -------------------------------------------------------
# DEBUG: Ejecutar GDB
# -------------------------------------------------------
debug_gdb() {
    local RULES_FILE="$SCRIPT_DIR/rules.gdb"
    
    # Buscar binarios disponibles
    local APP_PATH=""
    for candidate in "$PROJECT_ROOT/bin/mrf24_app" \
                     "$PROJECT_ROOT/mrf24_tx/bin/mrf24_transmitter" \
                     "$PROJECT_ROOT/mrf24_rx/bin/mrf24_receiver"; do
        if [[ -f "$candidate" ]]; then
            APP_PATH="$candidate"
            break
        fi
    done
    
    if [ ! -f "$RULES_FILE" ]; then
        echo "❌ No se encuentra $RULES_FILE"
        exit 1
    fi
    
    if [ -z "$APP_PATH" ]; then
        echo "⚠️  No se encontró ningún binario compilado."
        echo "   Compila primero:"
        echo "     $0 build all"
        echo "   O directamente:"
        echo "     cd mrf24_tx && make"
        echo "     cd mrf24_rx && make"
        exit 1
    fi
    
    echo "--- Iniciando GDB con: $(basename "$APP_PATH") ---"
    sudo gdb -x "$RULES_FILE" "$APP_PATH"
}

# =============================================================================
# MAIN
# =============================================================================

if [[ -z $COMPONENT ]]; then
    echo "MRF24J40 - Herramientas de Desarrollo"
    echo "=============================================="
    echo "Uso: $0 <comando> [subcomando]"
    echo ""
    echo "Comandos:"
    echo "  gpio       Gestión de GPIO"
    echo "  mosquitto  Gestión del servicio Mosquitto"
    echo "  git        Utilidades Git"
    echo "  build      Compilar proyectos"
    echo "  radio      Configurar modo radio (tx/rx)"
    echo "  swap       Configurar swap"
    echo "  debug      Debug con GDB"
    echo ""
    echo "Ejemplos:"
    echo "  $0 gpio settings"
    echo "  $0 gpio list"
    echo "  $0 mosquitto status"
    echo "  $0 build all"
    echo "  $0 git commit"
    echo "  $0 swap create"
    echo "  $0 debug"
    exit 0
fi

case $COMPONENT in
    gpio)       gpio_menu ;;
    mosquitto)  mosquitto_menu ;;
    git)        git_menu ;;
    build)      build_menu ;;
    radio)      radio_configure ;;
    swap)       swap_menu ;;
    debug)      debug_gdb ;;
    *)
        echo "Comando desconocido: $COMPONENT"
        echo "Usa: $0 sin argumentos para ver la ayuda"
        exit 1
        ;;
esac
