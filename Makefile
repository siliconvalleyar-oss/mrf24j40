# =============================================================================
# Makefile raíz — MRF24J40 IoT
# =============================================================================
# Delega la compilación a los 3 subproyectos: mrf24_tx, mrf24_rx, mrf24_security
# =============================================================================

SUB_PROJECTS := mrf24_security mrf24_tx mrf24_rx

.PHONY: all clean info serve-dashboard run-unificado run-tx run-rx $(SUB_PROJECTS)

all: $(SUB_PROJECTS)

# Construir cada subproyecto
$(SUB_PROJECTS):
	$(MAKE) -C $@

# Limpieza en todos los subproyectos
clean:
	@for dir in $(SUB_PROJECTS); do \
		$(MAKE) -C $$dir clean; \
	done
	@echo "  → Todos los subproyectos limpiados"

# Compilar y ejecutar el proyecto unificado
run-unificado: mrf24_security
	@echo "  → Ejecutando proyecto unificado (requiere sudo)..."
	sudo ./mrf24_security/bin/mrf24j40_iot

# Compilar y ejecutar el transmisor legacy
run-tx: mrf24_tx
	@echo "  → Ejecutando transmisor (requiere sudo)..."
	sudo ./mrf24_tx/bin/mrf24_transmitter

# Compilar y ejecutar el receptor legacy
run-rx: mrf24_rx
	@echo "  → Ejecutando receptor (requiere sudo)..."
	sudo ./mrf24_rx/bin/mrf24_transmitter

# Sirve el dashboard web localmente (desde la raíz del proyecto)
serve-dashboard:
	@echo "  → Dashboard: http://localhost:8080/mrf24-dashboard/"
	@echo "  → Presiona Ctrl+C para detener"
	@echo ""
	@python3 -m http.server 8080

# Información de cada subproyecto
info:
	@echo "=========================================="
	@echo "  MRF24J40 IoT — Info de subproyectos"
	@echo "=========================================="
	@for dir in $(SUB_PROJECTS); do \
		echo ""; \
		echo "--- $$dir ---"; \
		$(MAKE) -C $$dir info 2>/dev/null || true; \
	done
