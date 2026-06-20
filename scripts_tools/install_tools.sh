#!/bin/bash
# =============================================================================
# install_tools.sh - Instalación consolidada de dependencias para MRF24J40
# =============================================================================
# Uso: ./install_tools.sh [comando]
#
# Comandos:
#   all           Instalar todas las dependencias (por defecto)
#   basics        Dependencias básicas (qrencode, libpng, zlib, ssl)
#   bcm2835       Instalar librería BCM2835
#   mosquitto     Instalar Mosquitto MQTT broker y clientes
#   oled          Instalar librería SSD1306_OLED_RPI
#   database      Instalar MySQL/MariaDB
#   full          Instalar TODO (basics + bcm2835 + mosquitto + oled + database)
# =============================================================================

set +e
# NOTA: este script maneja errores internamente
# Uso: sudo ./install_tools.sh [comando]

ARCH=$(uname -m)

echo "=============================================="
echo "  install_tools.sh - MRF24J40 Dependencias"
echo "  Arquitectura detectada: $ARCH"
echo "=============================================="

# -------------------------------------------------------
# Función: instalar dependencias base
# -------------------------------------------------------
install_basics() {
    echo ""
    echo "--- Instalando dependencias básicas ---"
    
    sudo apt update -y
    sudo apt upgrade -y
    sudo apt install aptitude -y
    
    # Utilidades
    sudo aptitude install -y curl
    
    # QR
    sudo apt-get install qrencode libqrencode-dev -y
    
    # PNG / Zlib
    sudo apt-get install libpng-dev -y
    sudo apt-get install zlib1g-dev -y
    
    # Seguridad
    sudo apt-get install -y libssl-dev
    
    # Mosquitto libs
    sudo apt install libmosquitto-dev -y
    
    # MySQL C++ connector
    sudo apt-get install -y libmysqlcppconn-dev
    
    # Manejo especial para ARM 32 bits
    if [[ $ARCH == "armv7l" ]]; then
        echo "ARM 32 bits detectado - instalación específica de zlib/libpng"
        sudo apt install aptitude -y
        apt-cache policy zlib1g
        apt-cache policy zlib1g-dev
        sudo apt install zlib1g=1:1.2.11.dfsg-1+deb10u2 2>/dev/null || true
        sudo apt-get install zlib1g-dev --fix-missing
        sudo apt install libpng16-16 -y
        sudo aptitude install libpng-dev -y
        
        cd /tmp
        wget http://download.sourceforge.net/libpng/libpng-1.6.39.tar.gz
        tar -xzvf libpng-1.6.39.tar.gz
        cd libpng-1.6.39
        ./configure
        make
        sudo make install
        dpkg -l | grep libpng
    fi
    
    echo "--- Dependencias básicas instaladas ---"
}

# -------------------------------------------------------
# Función: instalar BCM2835
# -------------------------------------------------------
install_bcm2835() {
    echo ""
    echo "--- Instalando librería BCM2835 ---"
    
    if [[ $ARCH == "aarch64" ]]; then
        echo "ARM64 detectado - instalando libbcm2835-dev"
        sudo apt-get install -y libbcm2835-dev
    else
        VERSION="1.71"
        cd ~/Downloads 2>/dev/null || mkdir -p ~/Downloads && cd ~/Downloads
        
        wget "http://www.airspayce.com/mikem/bcm2835/bcm2835-${VERSION}.tar.gz"
        tar zxvf "bcm2835-${VERSION}.tar.gz"
        cd "bcm2835-${VERSION}"
        
        ./configure
        make
        sudo make check
        sudo make install
        
        rm -Rf ~/Downloads/bcm2835*
    fi
    
    ls /usr/local/include/bcm2835.h && echo "✅ BCM2835 instalado correctamente"
}

# -------------------------------------------------------
# Función: instalar Mosquitto
# -------------------------------------------------------
install_mosquitto() {
    echo ""
    echo "--- Instalando Mosquitto MQTT ---"
    
    sudo apt list --upgradable 2>/dev/null
    sudo apt-add-repository ppa:mosquitto-dev/mosquitto-ppa 2>/dev/null || true
    sudo apt update
    
    sudo apt install mosquitto mosquitto-clients -y
    
    sudo systemctl enable mosquitto
    sudo systemctl start mosquitto
    
    echo ""
    echo "✅ Mosquitto instalado"
    echo "   Config: /etc/mosquitto/mosquitto.conf"
    echo "   Para seguridad:"
    echo "     password_file /etc/mosquitto/passwd"
    echo "     allow_anonymous false"
    echo "   Crear usuario:"
    echo "     sudo mosquitto_passwd -c /etc/mosquitto/passwd myUser"
    echo "   Ver estado:"
    echo "     sudo systemctl status mosquitto"
}

# -------------------------------------------------------
# Función: instalar OLED (SSD1306)
# -------------------------------------------------------
install_oled() {
    echo ""
    echo "--- Instalando librería SSD1306_OLED_RPI ---"
    
    cd ~/src 2>/dev/null || mkdir -p ~/src && cd ~/src
    
    git clone https://github.com/gavinlyonsrepo/SSD1306_OLED_RPI.git
    
    cd SSD1306_OLED_RPI-1.6.1 2>/dev/null || cd SSD1306_OLED_RPI 2>/dev/null || { echo "No se encontró el directorio"; exit 1; }
    make
    sudo make install
    
    echo "✅ SSD1306_OLED_RPI instalado"
}

# -------------------------------------------------------
# Función: instalar base de datos (MySQL)
# -------------------------------------------------------
install_database() {
    echo ""
    echo "--- Instalando MySQL / MariaDB ---"
    
    sudo apt install libmariadb-dev-compat libmariadb-dev -y
    sudo apt install mysql-server -y
    
    echo "✅ Base de datos instalada"
    echo "   Para asegurar la instalación:"
    echo "     sudo mysql_secure_installation"
    echo "   Para acceder:"
    echo "     sudo mysql -u root -p"
}

# -------------------------------------------------------
# Función: instalar todo
# -------------------------------------------------------
install_all() {
    install_basics
    install_bcm2835
    install_mosquitto
    install_oled
    install_database
}

# -------------------------------------------------------
# MAIN
# -------------------------------------------------------
CMD=${1:-all}

case $CMD in
    all)
        install_all
        ;;
    basics)
        install_basics
        ;;
    bcm2835)
        install_bcm2835
        ;;
    mosquitto)
        install_mosquitto
        ;;
    oled)
        install_oled
        ;;
    database)
        install_database
        ;;
    full)
        install_all
        ;;
    *)
        echo "Uso: $0 {all|basics|bcm2835|mosquitto|oled|database}"
        echo ""
        echo "  all       Instalar dependencias básicas (por defecto)"
        echo "  basics    Solo dependencias básicas"
        echo "  bcm2835   Solo BCM2835"
        echo "  mosquitto Solo Mosquitto"
        echo "  oled      Solo SSD1306_OLED_RPI"
        echo "  database  Solo MySQL/MariaDB"
        exit 1
        ;;
esac

echo ""
echo "=============================================="
echo "  Comando '$CMD' completado"
echo "=============================================="
