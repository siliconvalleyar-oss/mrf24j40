# =============================================================================
# Makefile raíz — MRF24J40 IoT
# =============================================================================
# Delega la compilación a los 3 subproyectos: mrf24_tx, mrf24_rx, mrf24_security
# =============================================================================

SUB_PROJECTS := mrf24_security mrf24_tx mrf24_rx

.PHONY: all clean info serve-dashboard $(SUB_PROJECTS)

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

# Sirve el dashboard web localmente
serve-dashboard:
	@echo "  → Sirviendo mrf24-dashboard/ en http://localhost:8080"
	@echo "  → Presiona Ctrl+C para detener"
	@echo ""
	@cd mrf24-dashboard && python3 -m http.server 8080

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
