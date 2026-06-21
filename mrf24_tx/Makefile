# =============================================================================
# Makefile — MRF24J40 Transmisor (mrf24_tx)
# =============================================================================
# Compilador y flags base
CXX       := g++
CXXFLAGS  := -std=c++17 -Wall -Wextra -O2 -pthread
LDFLAGS   := -pthread

# Directorios
SRC_DIR   := src
OBJ_DIR   := obj
BIN_DIR   := bin
INC_DIRS  := src include
INC_FLAGS := $(addprefix -I, $(INC_DIRS))

# -----------------------------------------------------------
# Detección de arquitectura
# -----------------------------------------------------------
ARCH := $(shell uname -m)
$(info [BUILD] Arquitectura detectada: $(ARCH))

# -----------------------------------------------------------
# Librerías base
# -----------------------------------------------------------
LIBS :=

# BCM2835 (esencial para GPIO/SPI/I2C)
BCM2835_LIB := $(shell pkg-config --libs bcm2835 2>/dev/null || echo "-lbcm2835")
LIBS += $(BCM2835_LIB)

# -----------------------------------------------------------
# Librerías opcionales (se detectan automáticamente)
# -----------------------------------------------------------

# MySQL Connector/C++
MYSQL_AVAILABLE := $(shell pkg-config --exists mysqlcppconn 2>/dev/null && echo "yes" || echo "no")
ifeq ($(MYSQL_AVAILABLE),yes)
    $(info [BUILD] MySQL Connector detectado)
    LIBS += -lmysqlcppconn
    CXXFLAGS += -DENABLE_DATABASE
else
    $(info [BUILD] MySQL Connector NO detectado — funcionalidad de BD deshabilitada)
endif

# libqrencode (QR)
QR_AVAILABLE := $(shell pkg-config --exists libqrencode 2>/dev/null && echo "yes" || echo "no")
ifeq ($(QR_AVAILABLE),yes)
    $(info [BUILD] libqrencode detectado)
    LIBS += -lqrencode
    CXXFLAGS += -DUSE_QR
else
    $(info [BUILD] libqrencode NO detectado — generación de QR deshabilitada)
endif

# libpng
PNG_AVAILABLE := $(shell pkg-config --exists libpng 2>/dev/null && echo "yes" || echo "no")
ifeq ($(PNG_AVAILABLE),yes)
    $(info [BUILD] libpng detectado)
    LIBS += -lpng
else
    $(info [BUILD] libpng NO detectado)
endif

# zlib
Z_AVAILABLE := $(shell pkg-config --exists zlib 2>/dev/null && echo "yes" || echo "no")
ifeq ($(Z_AVAILABLE),yes)
    $(info [BUILD] zlib detectado)
    LIBS += -lz
else
    $(info [BUILD] zlib NO detectado)
endif

# libmosquitto (MQTT)
MOSQ_AVAILABLE := $(shell pkg-config --exists libmosquitto 2>/dev/null && echo "yes" || echo "no")
ifeq ($(MOSQ_AVAILABLE),yes)
    $(info [BUILD] libmosquitto detectado)
    LIBS += -lmosquitto
    CXXFLAGS += -DENABLE_MQTT
else
    $(info [BUILD] libmosquitto NO detectado — MQTT deshabilitado)
    # Fallback: intentar linkear directamente
    MOSQ_LIB := $(shell ldconfig -p 2>/dev/null | grep libmosquitto.so | head -1 | awk '{print $$NF}')
    ifneq ($(strip $(MOSQ_LIB)),)
        $(info [BUILD] libmosquitto encontrado via ldconfig)
        LIBS += -lmosquitto
        CXXFLAGS += -DENABLE_MQTT
    endif
endif

# -----------------------------------------------------------
# NOTA: mrf24_security ahora es un proyecto independiente (binario).
# El código de seguridad está en mrf24_security/ como parte del binario unificado.
# -----------------------------------------------------------

# -----------------------------------------------------------
# Fuentes: todos los .cpp del proyecto
# -----------------------------------------------------------
SRCS := \
    $(SRC_DIR)/main.cpp \
    $(SRC_DIR)/config/config.cpp \
    $(SRC_DIR)/spi/spi.cpp \
    $(SRC_DIR)/spi/spi_dbg.cpp \
    $(SRC_DIR)/gpio/gpio.cpp \
    $(SRC_DIR)/mrf24/mrf24j40.cpp \
    $(SRC_DIR)/file/file.cpp \
    $(SRC_DIR)/file/database.cpp \
    $(SRC_DIR)/qr/qr.cpp \
    $(SRC_DIR)/qr/qr_img.cpp \
    $(SRC_DIR)/qr/ff.cpp \
    $(SRC_DIR)/tyme/tyme.cpp \
    $(SRC_DIR)/interrupt/interrupt.cpp \
    $(SRC_DIR)/work/rfflush.cpp \
    $(SRC_DIR)/display/epaper.cpp \
    $(SRC_DIR)/oled/oled/SSD1306_OLED.cpp \
    $(SRC_DIR)/oled/oled/SSD1306_OLED_graphics.cpp \
    $(SRC_DIR)/oled/oled/SSD1306_OLED_Print.cpp \
    $(SRC_DIR)/oled/oled/SSD1306_OLED_font.cpp \
    $(SRC_DIR)/oled/oled/oled.cpp \
    $(SRC_DIR)/mosquitto/mqtt_handler.cpp \
    $(SRC_DIR)/mosquitto/mqtt_bridge.cpp

OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))
DEPS := $(OBJS:.o=.d)

# Nombre del binario de salida
TARGET := $(BIN_DIR)/mrf24_transmitter

# -----------------------------------------------------------
# Reglas
# -----------------------------------------------------------
.PHONY: all directories clean info

all: directories $(TARGET)
	@echo ""
	@echo "  ╔══════════════════════════════════════════╗"
	@echo "  ║  MRF24J40 TRANSMISOR — COMPILACIÓN OK   ║"
	@echo "  ║  Ejecutar: sudo ./$(TARGET)              ║"
	@echo "  ╚══════════════════════════════════════════╝

# Directorios de salida
directories:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)
	@mkdir -p $(sort $(dir $(OBJS)))

# Linkeo final
$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)
	@echo "  → Binario: $@"

# Compilación de objetos
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INC_FLAGS) -MMD -MP -c $< -o $@

# Incluir dependencias automáticas
-include $(DEPS)

# Limpieza
clean:
	@rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "  → Limpieza completada"

# Información de compilación
info:
	@echo "Arquitectura : $(ARCH)"
	@echo "Compilador   : $(CXX)"
	@echo "Flags C++    : $(CXXFLAGS)"
	@echo "Librerías    : $(LIBS)"
	@echo "Fuentes      : $(words $(SRCS)) archivos"
	@echo "Target       : $(TARGET)"
