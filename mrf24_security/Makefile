# =============================================================================
# Makefile — MRF24J40 Security & Shared Infrastructure (mrf24_security)
# =============================================================================
# Compila el binario unificado mrf24j40_iot con radio, displays, crypto, QR
# =============================================================================

CXX       := g++
CXXFLAGS  := -std=c++17 -Wall -Wextra -O2 -pthread -DENABLE_JSON
LDFLAGS   := -pthread

# Directorios
SRC_DIR   := .
OBJ_DIR   := obj
BIN_DIR   := bin
INC_FLAGS := -I.

# -----------------------------------------------------------
# Detección de librerías
# -----------------------------------------------------------
LIBS := -lbcm2835

# OpenSSL (AES-256-CBC, SHA-256)
OPENSSL_LIBS := $(shell pkg-config --libs openssl 2>/dev/null || echo "-lcrypto -lssl")
LIBS += $(OPENSSL_LIBS)
CXXFLAGS += $(shell pkg-config --cflags openssl 2>/dev/null || echo "")

# libqrencode (QR)
QR_LIBS := $(shell pkg-config --libs libqrencode 2>/dev/null || echo "-lqrencode")
LIBS += $(QR_LIBS)

# libpng
PNG_LIBS := $(shell pkg-config --libs libpng 2>/dev/null || echo "-lpng -lz")
LIBS += $(PNG_LIBS)

# -----------------------------------------------------------
# Fuentes
# -----------------------------------------------------------
SRCS := \
    main.cpp \
    hal/gpio.cpp \
    hal/spi.cpp \
    hal/i2c.cpp \
    drivers/mrf24j40.cpp \
    drivers/ssd1306.cpp \
    drivers/st7789.cpp \
    drivers/qr.cpp \
    services/crypto.cpp \
    services/filesystem.cpp \
    services/timer.cpp \
    application/radio_manager.cpp \
    application/menu.cpp \
    src/encrypt.cpp \
    src/decrypt.cpp

OBJS := $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(SRCS))
DEPS := $(OBJS:.o=.d)

TARGET := $(BIN_DIR)/mrf24j40_iot

# -----------------------------------------------------------
# Reglas
# -----------------------------------------------------------
.PHONY: all directories clean info

all: directories $(TARGET)
	@echo ""
	@echo "  ╔══════════════════════════════════════════╗"
	@echo "  ║  MRF24J40 SECURITY — COMPILACIÓN OK     ║"
	@echo "  ║  Binario: ./$(TARGET)                    ║"
	@echo "  ╚══════════════════════════════════════════╝"

directories:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)
	@mkdir -p $(sort $(dir $(OBJS)))

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)
	@echo "  → Binario: $@"

$(OBJ_DIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INC_FLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

clean:
	@rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "  → Limpieza completada"

info:
	@echo "Compilador : $(CXX)"
	@echo "Flags      : $(CXXFLAGS)"
	@echo "Librerías  : $(LIBS)"
	@echo "Fuentes    : $(words $(SRCS)) archivos"
	@echo "Target     : $(TARGET)"
