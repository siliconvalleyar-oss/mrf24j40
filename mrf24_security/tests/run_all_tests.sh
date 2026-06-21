#!/bin/bash
# =============================================================================
# run_all_tests.sh - Compila y ejecuta todos los tests de whitelist
# =============================================================================
# Uso:
#   sudo ./tests/run_all_tests.sh          # Desde mrf24_security/
#   sudo ./run_all_tests.sh                # Desde tests/
#
# Requiere: make, sudo (para acceso a BCM2835 si hay radio conectada)
#
# @author  MRF24J40 Team
# @date    2026
# =============================================================================

set -euo pipefail

# Colores ANSI
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# ==============================================================================
# Detectar directorio raíz del proyecto
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd 2>/dev/null || echo "$SCRIPT_DIR")"

# Si estamos en tests/, ir al directorio del proyecto
if [[ "$(basename "$SCRIPT_DIR")" == "tests" ]]; then
    PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
fi

cd "$PROJECT_DIR"

if [[ ! -f Makefile ]]; then
    echo -e "${RED}Error: No se encontró Makefile en $PROJECT_DIR${NC}"
    echo "Ejecuta el script desde mrf24_security/ o mrf24_security/tests/"
    exit 1
fi

echo ""
echo -e "  ${BOLD}╔═══════════════════════════════════════════════════════════╗${NC}"
echo -e "  ${BOLD}║                                                           ║${NC}"
echo -e "  ${BOLD}║   MRF24J40 — SUITE COMPLETA DE TESTS                     ║${NC}"
echo -e "  ${BOLD}║                                                           ║${NC}"
echo -e "  ${BOLD}║   Directorio: ${CYAN}$PROJECT_DIR${NC}${BOLD}        ║${NC}"
echo -e "  ${BOLD}╚═══════════════════════════════════════════════════════════╝${NC}"
echo ""

# ==============================================================================
# Variables globales
# ==============================================================================

ALL_PASSED=true
PASS_COUNT=0
TOTAL_COUNT=0
FAIL_COUNT=0
TEST_BIN_DIR="$PROJECT_DIR/tests"

# Binarios de test
declare -a TEST_BINS=(
    "test_whitelist"
    "test_whitelist_scenario"
    "test_whitelist_persistence"
)

declare -a TEST_NAMES=(
    "UNIT TESTS (whitelist API + validateMessage)"
    "ESCENARIO REAL (simulación 2 nodos)"
    "PERSISTENCIA (save/load config.json)"
)

# ==============================================================================
# Función: ejecutar un test
# ==============================================================================

run_test() {
    local name="$1"
    local binary="$2"
    local binpath="$TEST_BIN_DIR/$binary"

    TOTAL_COUNT=$((TOTAL_COUNT + 1))

    echo ""
    echo -e "  ${BOLD}┌─ Test $TOTAL_COUNT: ${CYAN}$name${NC}${BOLD} ─${NC}"
    echo -e "  ${BOLD}│${NC}  Binario: ${YELLOW}$binary${NC}"
    echo -e "  ${BOLD}│${NC}"
    echo ""

    if [[ ! -f "$binpath" ]]; then
        echo -e "  ${BOLD}│${NC}  ${RED}✘ ERROR: Binario no encontrado${NC}"
        echo -e "  ${BOLD}│${NC}  Ejecuta 'make test' primero o compila con 'make test -j4'"
        echo ""
        echo -e "  ${BOLD}└${RED}✘  FALLÓ — binario ausente${NC}"
        echo ""
        ALL_PASSED=false
        FAIL_COUNT=$((FAIL_COUNT + 1))
        return 1
    fi

    # Ejecutar con sudo y capturar salida y código de retorno
    set +e
    OUTPUT=$(sudo "$binpath" 2>&1)
    EXIT_CODE=$?
    set -e

    # Mostrar salida
    echo "$OUTPUT"

    # Determinar resultado
    if [[ $EXIT_CODE -eq 0 ]]; then
        PASS_COUNT=$((PASS_COUNT + 1))
        echo ""
        echo -e "  ${BOLD}└${GREEN}✓  PASÓ${NC}${BOLD} — ${CYAN}$binary${NC}${BOLD} exit code: $EXIT_CODE${NC}"
    else
        ALL_PASSED=false
        FAIL_COUNT=$((FAIL_COUNT + 1))
        echo ""
        echo -e "  ${BOLD}└${RED}✘  FALLÓ — ${CYAN}$binary${NC}${RED} exit code: $EXIT_CODE${NC}"

        # Extraer resumen de fallos si existe
        FAIL_LINE=$(echo "$OUTPUT" | grep -i "FAIL\|FALLO\|fallo" | tail -5)
        if [[ -n "$FAIL_LINE" ]]; then
            echo -e "  ${RED}  Detalles:${NC}"
            echo "$FAIL_LINE" | while IFS= read -r line; do
                echo -e "  ${RED}  $line${NC}"
            done
        fi
    fi

    echo ""
}

# ==============================================================================
# Fase 1: Compilar tests
# ==============================================================================

echo -e "  ${BOLD}┌──────────────────────────────────────────────────────────┐${NC}"
echo -e "  ${BOLD}│${NC}  ${YELLOW}FASE 1: COMPILAR TESTS${NC}"
echo -e "  ${BOLD}└──────────────────────────────────────────────────────────┘${NC}"
echo ""

echo -e "  Ejecutando: ${CYAN}make test -j4${NC}"
echo ""

set +e
BUILD_OUTPUT=$(make test -j4 2>&1)
BUILD_EXIT=$?
set -e

if [[ $BUILD_EXIT -ne 0 ]]; then
    echo -e "${RED}Error de compilación:${NC}"
    echo "$BUILD_OUTPUT"
    echo ""
    echo -e "  ${BOLD}${RED}═══════════════════════════════════════════════════════════${NC}"
    echo -e "  ${BOLD}${RED}  ✘  COMPILACIÓN FALLÓ (exit code: $BUILD_EXIT)${NC}"
    echo -e "  ${BOLD}${RED}═══════════════════════════════════════════════════════════${NC}"
    echo ""
    exit 2
fi

echo "$BUILD_OUTPUT" | grep -E "Test:|→|error:|warning:" || true
echo ""
echo -e "  ${GREEN}✓  Compilación exitosa${NC}"
echo ""

# ==============================================================================
# Fase 2: Ejecutar tests
# ==============================================================================

echo -e "  ${BOLD}┌──────────────────────────────────────────────────────────┐${NC}"
echo -e "  ${BOLD}│${NC}  ${YELLOW}FASE 2: EJECUTAR TESTS${NC}"
echo -e "  ${BOLD}└──────────────────────────────────────────────────────────┘${NC}"

for i in "${!TEST_BINS[@]}"; do
    run_test "${TEST_NAMES[$i]}" "${TEST_BINS[$i]}"
done

# ==============================================================================
# Fase 3: Resumen final
# ==============================================================================

echo ""
echo -e "  ${BOLD}╔═══════════════════════════════════════════════════════════╗${NC}"
echo -e "  ${BOLD}║${NC}                                                           ${BOLD}║${NC}"

if $ALL_PASSED && [[ $FAIL_COUNT -eq 0 ]]; then
    echo -e "  ${BOLD}║${NC}     ${GREEN}${BOLD}✓  TODOS LOS TESTS PASARON${NC}${BOLD}                        ║${NC}"
else
    echo -e "  ${BOLD}║${NC}     ${RED}${BOLD}✘  $FAIL_COUNT TEST(S) FALLARON${NC}${BOLD}                      ║${NC}"
fi

echo -e "  ${BOLD}║${NC}                                                           ${BOLD}║${NC}"
echo -e "  ${BOLD}║${NC}     Total:  ${CYAN}$TOTAL_COUNT${NC}                                     ${BOLD}║${NC}"
echo -e "  ${BOLD}║${NC}     PASÓ:   ${GREEN}$PASS_COUNT${NC}                                       ${BOLD}║${NC}"
echo -e "  ${BOLD}║${NC}     FALLÓ:  ${RED}$FAIL_COUNT${NC}                                       ${BOLD}║${NC}"
echo -e "  ${BOLD}║${NC}                                                           ${BOLD}║${NC}"

# Mostrar tabla resumen
echo -e "  ${BOLD}║${NC}  Tests:                                                ${BOLD}║${NC}"
for i in "${!TEST_BINS[@]}"; do
    local binpath="$TEST_BIN_DIR/${TEST_BINS[$i]}"
    if [[ -f "$binpath" ]]; then
        # Extraer resumen de la salida del test (últimas líneas relevantes)
        # Buscar en la salida capturada no es fácil aquí, mejor mostrar desde archivo
        echo -e "  ${BOLD}║${NC}    $((i+1)). ${TEST_NAMES[$i]}"
        echo -e "  ${BOLD}║${NC}       ${YELLOW}${TEST_BINS[$i]}${NC}                           ${BOLD}║${NC}"
    fi
done

echo -e "  ${BOLD}║${NC}                                                           ${BOLD}║${NC}"
echo -e "  ${BOLD}╚═══════════════════════════════════════════════════════════╝${NC}"
echo ""

if $ALL_PASSED && [[ $FAIL_COUNT -eq 0 ]]; then
    exit 0
else
    exit 1
fi
