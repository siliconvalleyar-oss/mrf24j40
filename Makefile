#===============================================================================
# Makefile - Sistema MRF24J40 IoT (versión unificada)
#===============================================================================
# Compilación del sistema completo: radio, displays, crypto, QR, menú
#===============================================================================

# --- Compilador y flags ---
CXX       ?= g++
CXXFLAGS  ?= -O2 -std=c++17
CXXFLAGS  += -Wall -Wextra -Wpedantic -MMD -MP
CXXFLAGS  += -I. -Iinclude

LDFLAGS   = -pthread -lbcm2835

# --- Archivos fuente ---
SRCS_HAL     = hal/gpio.cpp hal/spi.cpp hal/i2c.cpp
SRCS_DRIVERS = drivers/ssd1306.cpp drivers/st7789.cpp drivers/qr.cpp \
               drivers/mrf24j40.cpp
SRCS_SERVICES = services/crypto.cpp services/filesystem.cpp services/timer.cpp
SRCS_APP     = application/radio_manager.cpp application/menu.cpp

SRCS = main.cpp $(SRCS_HAL) $(SRCS_DRIVERS) $(SRCS_SERVICES) $(SRCS_APP)
OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

TARGET = mrf24j40_iot

# --- Detección automática de librerías opcionales ---

# OpenSSL (AES-256-CBC, SHA-256)
ifneq ($(shell pkg-config --exists openssl 2>/dev/null && echo yes),)
    CXXFLAGS += -DENABLE_OPENSSL
    LDFLAGS  += $(shell pkg-config --cflags --libs openssl 2>/dev/null)
else
    # Fallback: buscar libcrypto directamente
    ifneq ($(shell ldconfig -p 2>/dev/null | grep -q libcrypto; echo $$?),1)
        LDFLAGS += -lcrypto -lssl
        CXXFLAGS += -DENABLE_OPENSSL
    endif
endif

# nlohmann/json (configuración JSON)
ifneq ($(shell pkg-config --exists nlohmann_json 2>/dev/null && echo yes),)
    CXXFLAGS += -DENABLE_JSON
    CXXFLAGS += $(shell pkg-config --cflags nlohmann_json 2>/dev/null)
else
    # Buscar header directamente
    JSON_HEADER := /usr/include/nlohmann/json.hpp
    ifeq ($(wildcard $(JSON_HEADER)),)
        $(warning nlohmann/json no encontrado. La configuración JSON usará valores por defecto.)
    else
        CXXFLAGS += -DENABLE_JSON
    endif
endif

# libqrencode (generación de QR)
ifneq ($(shell pkg-config --exists libqrencode 2>/dev/null && echo yes),)
    CXXFLAGS += -DENABLE_QRENCODE
    LDFLAGS  += $(shell pkg-config --cflags --libs libqrencode 2>/dev/null)
else
    LDFLAGS  += -lqrencode
    CXXFLAGS += -DENABLE_QRENCODE
endif

# libpng (exportar QR a PNG)
ifneq ($(shell pkg-config --exists libpng 2>/dev/null && echo yes),)
    LDFLAGS  += $(shell pkg-config --cflags --libs libpng 2>/dev/null)
else
    LDFLAGS  += -lpng -lz
endif

# BCM2835 (GPIO nativo)
ifneq ($(shell pkg-config --exists libbcm2835 2>/dev/null && echo yes),)
    CXXFLAGS += $(shell pkg-config --cflags libbcm2835 2>/dev/null)
else
    CXXFLAGS += -I/usr/local/include
endif

# --- Targets ---

.PHONY: all clean clean-docs info help docs docs-html docs-init

all: $(TARGET)

# Link
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo ""
	@echo "=========================================="
	@echo "  ✅ $(TARGET) compilado correctamente"
	@echo "=========================================="

# Compilación
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Información de compilación
info:
	@echo "=========================================="
	@echo "  MRF24J40 IoT - Información"
	@echo "=========================================="
	@echo "  Compilador:       $(CXX)"
	@echo "  Flags:            $(CXXFLAGS)"
	@echo "  Linker:           $(LDFLAGS)"
	@echo ""
	@echo "  Librerías detectadas:"
	@if $(CXX) $(CXXFLAGS) -E -P - < /dev/null 2>/dev/null; then \
		echo "  - OpenSSL:         $(shell pkg-config --exists openssl && echo '✅' || echo '❌')"; \
		echo "  - nlohmann/json:   $(shell pkg-config --exists nlohmann_json && echo '✅' || echo '❌')"; \
		echo "  - libqrencode:     $(shell pkg-config --exists libqrencode && echo '✅' || echo '❌')"; \
		echo "  - libpng:          $(shell pkg-config --exists libpng && echo '✅' || echo '❌')"; \
		echo "  - BCM2835:         $(shell pkg-config --exists libbcm2835 && echo '✅' || echo '❌')"; \
	fi
	@echo ""
	@echo "  Archivos fuente:   $(words $(SRCS))"
	@echo "  Target:            $(TARGET)"
	@echo "=========================================="

# Limpieza
clean:
	@rm -f $(OBJS) $(DEPS) $(TARGET)
	@echo "🧹 Limpieza completada."

# Limpieza de documentación
clean-docs:
	@rm -rf docs/doxygen
	@echo "🧹 Documentación Doxygen eliminada."

# Generar documentación Doxygen
docs:
	@echo "=========================================="
	@echo "  Generando documentación Doxygen..."
	@echo "=========================================="
	@command -v doxygen >/dev/null 2>&1 || { \
		echo "❌  doxygen no está instalado."; \
		echo "    Instalálo con: sudo apt-get install doxygen graphviz"; \
		exit 1; \
	}
	doxygen Doxyfile
	@echo ""
	@echo "=========================================="
	@echo "  ✅ Documentación generada en docs/doxygen/"
	@echo "  📄 HTML: docs/doxygen/html/index.html"
	@if command -v pdflatex >/dev/null 2>&1; then \
		echo "  📄 PDF:  docs/doxygen/latex/refman.pdf"; \
	else \
		echo "  ⚠  PDF no generado (pdflatex no instalado)."; \
		echo "     Para incluír PDF: sudo apt-get install texlive-latex-base"; \
	fi
	@echo "=========================================="

# Documentación solo HTML (más rápida, sin pdflatex)
docs-html:
	@command -v doxygen >/dev/null 2>&1 || { \
		echo "❌ doxygen no está instalado."; \
		echo "    Instalálo con: sudo apt-get install doxygen graphviz"; \
		exit 1; \
	}
	@sed -i 's/^GENERATE_LATEX.*= YES/GENERATE_LATEX = NO/' Doxyfile
	doxygen Doxyfile
	@echo ""
	@echo "✅ Documentación HTML generada en docs/doxygen/html/"

# Regenerar Doxyfile por defecto
docs-init:
	@echo "Regenerando Doxyfile..."
	@doxygen -g Doxyfile 2>/dev/null
	@echo "✅ Doxyfile regenerado. Personalizá las opciones según el proyecto."

# Ayuda
help:
	@echo "Uso: make [target]"
	@echo ""
	@echo "Targets:"
	@echo "  all        Compila el sistema completo (default)"
	@echo "  docs       Genera doc. Doxygen (HTML + PDF si pdflatex instalado)"
	@echo "  docs-html  Genera solo HTML (más rápido, sin pdflatex)"
	@echo "  docs-init  Regenera archivo Doxyfile por defecto"
	@echo "  info       Muestra información de compilación"
	@echo "  clean      Limpia archivos objeto y binarios"
	@echo "  clean-docs Limpia documentación generada"
	@echo "  help       Muestra esta ayuda"
	@echo ""
	@echo "Variables:"
	@echo "  CXX        Compilador C++ (default: g++)"
	@echo "  CXXFLAGS   Flags adicionales (default: -O2 -std=c++17)"

# Incluir dependencias automáticas
-include $(DEPS)
