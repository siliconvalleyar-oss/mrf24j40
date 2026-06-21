# =============================================================================
# Makefile raíz — MRF24J40 IoT
# =============================================================================
# Delega la compilación a los 3 subproyectos: mrf24_tx, mrf24_rx, mrf24_security
# =============================================================================

SUB_PROJECTS := mrf24_security mrf24_tx mrf24_rx

.PHONY: all clean info serve-dashboard run-unificado run-tx run-rx compile-remote remote-push remote-pull compile-remote-pull-only $(SUB_PROJECTS)

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

# ---------------------------------------------------------------------------
# Compilación remota vía SSH (Raspberry Pi)
# ---------------------------------------------------------------------------
# Uso:
#   make compile-remote                      # usa valores por defecto
#   make compile-remote SSH_HOST=192.168.1.10 # IP personalizada
#   make compile-remote BRANCH=main           # rama específica
#
# Variables configurables:
#   SSH_USER  = joy               (usuario SSH)
#   SSH_HOST  = raspberry.local   (host/IP de la Pi)
#   SSH_DIR   = /home/joy/src/mrf24j40  (ruta del proyecto en la Pi)
#   BRANCH    = release/V3        (rama a compilar, vacío = usar la actual)
# ---------------------------------------------------------------------------

SSH_USER  ?= joy
SSH_HOST  ?= raspberry.local
SSH_DIR   ?= /home/joy/src/mrf24j40
BRANCH    ?= release/V3

.PHONY: compile-remote remote-push remote-pull

# Flujo completo: pushear local + pull remoto + compilar
compile-remote: remote-push remote-pull

# Pushear cambios locales al remoto
remote-push:
	@echo "  → Pusheando cambios locales..."
	git push origin refs/heads/$(BRANCH)

# Conectarse vía SSH, verificar rama, pull y compilar
remote-pull:
	@echo "  → Conectando a $(SSH_USER)@$(SSH_HOST)..."
	@ssh $(SSH_USER)@$(SSH_HOST) "cd $(SSH_DIR) && \
		echo '  ✓ Conectado' && \
		CURRENT_BRANCH=$$(git branch --show-current) && \
		echo "  Rama actual: $$CURRENT_BRANCH" && \
		if [ "$(BRANCH)" != "" ] && [ "$$CURRENT_BRANCH" != "$(BRANCH)" ]; then \
			echo "  → Cambiando a $(BRANCH)..."; \
			git switch $(BRANCH); \
		fi && \
		echo '  → Pull...' && \
		git pull && \
		echo '  → Compilando...' && \
		make clean && \
		make -j4"
	@echo "  ✅ Compilación remota completada"

# Solo compilar en remoto (sin push)
compile-remote-pull-only:
	@echo "  → Compilando en $(SSH_USER)@$(SSH_HOST)..."
	@ssh $(SSH_USER)@$(SSH_HOST) "cd $(SSH_DIR) && \
		git pull && \
		make -j4"
	@echo "  ✅ Compilación remota completada"

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
