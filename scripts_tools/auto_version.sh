#!/bin/bash
#===============================================================================
# auto_version.sh - Auto-versionado para MRF24J40
#
# Uso:
#   ./auto_version.sh                   - Incrementa +0.0.1, modo interactivo
#   ./auto_version.sh patch             - Incrementa +0.0.1 (default)
#   ./auto_version.sh minor             - Incrementa +0.1.0
#   ./auto_version.sh major             - Incrementa +1.0.0
#   ./auto_version.sh --yes             - Modo automático (sin prompts)
#   ./auto_version.sh --dry-run         - Muestra lo que haría sin ejecutar
#   ./auto_version.sh --set 2.1.0       - Fija una versión específica
#   ./auto_version.sh --install-hook    - Instala como pre-push hook de git
#   ./auto_version.sh --help            - Muestra esta ayuda
#
# Automatiza: bump de versión → update VERSION.txt → commit → tag → push
#===============================================================================

set -euo pipefail

# --- Configuración ---
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSION_FILE="${PROJECT_ROOT}/VERSION.txt"
DRY_RUN=false
AUTO_YES=false
SET_VERSION=""
BUMP_TYPE="patch"
INSTALL_HOOK=false

# --- Colores ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

info()  { echo -e "${GREEN}[INFO]${NC}  $1"; }
warn()  { echo -e "${YELLOW}[WARN]${NC}  $1"; }
error() { echo -e "${RED}[ERROR]${NC} $1"; }

# --- Parseo de argumentos ---
while [[ $# -gt 0 ]]; do
    case "$1" in
        --yes|-y)           AUTO_YES=true; shift ;;
        --dry-run)          DRY_RUN=true; shift ;;
        --install-hook)     INSTALL_HOOK=true; shift ;;
        --set)              SET_VERSION="${2:-}"; shift 2 ;;
        --help|-h)          head -20 "$0" | grep -E "^#" | sed 's/^#//'; exit 0 ;;
        patch|minor|major)  BUMP_TYPE="$1"; shift ;;
        *)                  error "Argumento desconocido: $1"; exit 1 ;;
    esac
done

# === INSTALL HOOK ============================================================
if [[ "$INSTALL_HOOK" == true ]]; then
    HOOK_DIR="${PROJECT_ROOT}/.git/hooks"
    HOOK_FILE="${HOOK_DIR}/pre-push"

    if [[ "$DRY_RUN" == true ]]; then
        info "[DRY-RUN] Instalaría hook en: $HOOK_FILE"
        exit 0
    fi

    mkdir -p "$HOOK_DIR"
    cat > "$HOOK_FILE" << 'HOOK'
#!/bin/bash
# pre-push hook - Auto-versionado antes de cada push
# Instalado por scripts_tools/auto_version.sh
set -euo pipefail

# Solo ejecutar en ramas de versión (vX.Y.Z), no en main ni otras
BRANCH="$(git rev-parse --abbrev-ref HEAD)"
if [[ ! "$BRANCH" =~ ^v[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    exit 0
fi

SCRIPT="$(git rev-parse --show-toplevel)/scripts_tools/auto_version.sh"
if [[ -x "$SCRIPT" ]]; then
    exec "$SCRIPT" patch --yes
fi
HOOK
    chmod +x "$HOOK_FILE"
    info "✅ Hook pre-push instalado en: $HOOK_FILE"
    info "   Se ejecutará 'auto_version.sh patch --yes' antes de cada git push."
    exit 0
fi

# === FUNCIONES ================================================================

# Obtener el último tag vX.Y.Z del repositorio
get_latest_tag() {
    local tag
    tag=$(git -C "$PROJECT_ROOT" tag --list 'v*' --sort=-v:refname 2>/dev/null | head -1)
    echo "${tag:-v0.0.0}"
}

# Parsear vX.Y.Z a componentes numéricos
parse_version() {
    local version="${1#v}"
    IFS='.' read -r major minor patch <<< "${version}"
    major=${major:-0}; minor=${minor:-0}; patch=${patch:-0}
}

# Incrementar versión según bump type
bump_version() {
    local mj="$1" mn="$2" pt="$3" bump="$4"
    case "$bump" in
        patch) pt=$((pt + 1)) ;;
        minor) mn=$((mn + 1)); pt=0 ;;
        major) mj=$((mj + 1)); mn=0; pt=0 ;;
    esac
    echo "v${mj}.${mn}.${pt}"
}

# Actualizar VERSION.txt con la nueva versión
update_version_file() {
    local new_version="$1"
    local commit_msg="${2:-Actualización automática}"
    local clean="${new_version#v}"

    if [[ "$DRY_RUN" == true ]]; then
        info "[DRY-RUN] Actualizaría VERSION.txt → $new_version"
        return
    fi

    if [[ -f "$VERSION_FILE" ]]; then
        # Reescribir el archivo completo para evitar problemas de sed
    local tmp_file
    tmp_file=$(mktemp "${TMPDIR:-/tmp}/version-XXXXXX")
        local in_header=true
        local in_history=false

        while IFS= read -r line; do
            if [[ "$in_header" == true ]]; then
                if [[ "$line" =~ ^Versión: ]]; then
                    echo "Versión: ${clean}" >> "$tmp_file"
                elif [[ "$line" =~ ^Tag: ]]; then
                    echo "Tag:     ${new_version}" >> "$tmp_file"
                elif [[ "$line" =~ ^Historial ]]; then
                    echo "$line" >> "$tmp_file"
                    echo "  ${new_version}  - ${commit_msg}" >> "$tmp_file"
                    in_header=false
                else
                    echo "$line" >> "$tmp_file"
                fi
            else
                echo "$line" >> "$tmp_file"
            fi
        done < "$VERSION_FILE"

        mv "$tmp_file" "$VERSION_FILE"
    else
        cat > "$VERSION_FILE" << EOF
MRF24J40 - Transmisor/Receptor ZigBee para Raspberry Pi
=======================================================

Versión: ${clean}
Tag:     ${new_version}

Historial de versiones:
  ${new_version}  - ${commit_msg}
EOF
    fi

    info "VERSION.txt actualizado → $new_version"
}

# Verificar si hay cambios sin commitear (staged y unstaged)
check_dirty_tree() {
    local has_changes=false
    if ! git -C "$PROJECT_ROOT" diff --quiet HEAD 2>/dev/null; then
        has_changes=true
    fi
    if ! git -C "$PROJECT_ROOT" diff --cached --quiet 2>/dev/null; then
        has_changes=true
    fi
    $has_changes && return 0 || return 1
}

# Preguntar al usuario (solo en modo interactivo)
ask() {
    local prompt="$1" default="$2"
    if [[ "$AUTO_YES" == true ]]; then
        echo "$default"
        return 0
    fi
    if [[ ! -t 0 ]]; then
        echo "$default"
        return 0
    fi
    local reply
    read -rp "$prompt " reply
    if [[ -z "$reply" ]]; then
        echo "$default"
    else
        echo "$reply"
    fi
}

# ==============================================================================
# --- MAIN ---
# ==============================================================================

cd "$PROJECT_ROOT"

info "Proyecto: $(basename "$PROJECT_ROOT")"
info "Rama:     $(git branch --show-current)"
echo ""

# --- 1. Determinar versión ---
if [[ -n "$SET_VERSION" ]]; then
    NEW_VERSION="v${SET_VERSION#v}"
    info "Versión fijada: $NEW_VERSION"
else
    LATEST_TAG=$(get_latest_tag)
    parse_version "$LATEST_TAG"
    info "Último tag: $LATEST_TAG → v${major}.${minor}.${patch}"
    NEW_VERSION=$(bump_version "$major" "$minor" "$patch" "$BUMP_TYPE")
    info "Nueva versión: $NEW_VERSION (bump: $BUMP_TYPE)"
fi
echo ""

# --- 2. Mensaje de commit ---
DEFAULT_MSG="v$(echo "$NEW_VERSION" | sed 's/^v//'): actualización automática"
COMMIT_MSG=$(ask "Mensaje del commit (Enter: '$DEFAULT_MSG'):" "$DEFAULT_MSG")
echo ""

# --- 3. Verificar cambios sin commitear ---
if check_dirty_tree; then
    warn "Hay cambios sin commitear en el repositorio:"
    git -C "$PROJECT_ROOT" status --short
    echo ""
    REPLY=$(ask "¿Commitear estos cambios junto con la nueva versión? [S/n]:" "S")
    if [[ "$REPLY" =~ ^[Nn] ]]; then
        error "Abortando. Commitée o stashée los cambios primero."
        exit 1
    fi
fi

# --- 4. Actualizar VERSION.txt ---
update_version_file "$NEW_VERSION" "$COMMIT_MSG"
echo ""

# --- 5. Git add ---
if [[ "$DRY_RUN" == true ]]; then
    info "[DRY-RUN] git add -A"
else
    git add -A
    info "✅ Archivos staged."
fi
echo ""

# --- 6. Git commit ---
if [[ "$DRY_RUN" == true ]]; then
    info "[DRY-RUN] git commit -m \"$COMMIT_MSG\""
else
    if git diff --cached --quiet 2>/dev/null; then
        warn "No hay cambios para commitear. Solo se creará el tag."
    else
        git commit -m "$COMMIT_MSG"
        info "✅ Commit creado."
    fi
fi
echo ""

# --- 7. Git tag ---
if [[ "$DRY_RUN" == true ]]; then
    info "[DRY-RUN] git tag -a $NEW_VERSION -m \"$COMMIT_MSG\""
else
    if git tag --list "$NEW_VERSION" | grep -q "$NEW_VERSION"; then
        warn "El tag $NEW_VERSION ya existe. Eliminalo con: git tag -d $NEW_VERSION && git push origin :refs/tags/$NEW_VERSION"
    else
        git tag -a "$NEW_VERSION" -m "$COMMIT_MSG"
        info "✅ Tag creado: $NEW_VERSION"
    fi
fi
echo ""

# --- 8. Git push ---
if [[ "$DRY_RUN" == true ]]; then
    info "[DRY-RUN] git push origin <branch> + git push origin tag"
else
    BRANCH=$(git branch --show-current)

    if git push origin "$BRANCH"; then
        info "✅ Branch '$BRANCH' pusheada."
    else
        warn "Push de branch falló. Puede haber cambios en el remoto."
        warn "   → Ejecutá: git pull --rebase origin $BRANCH"
    fi

    if git push origin "refs/tags/$NEW_VERSION"; then
        info "✅ Tag $NEW_VERSION pusheado."
    else
        warn "Push del tag falló."
    fi
fi
echo ""

# --- Resumen final ---
echo -e "${GREEN}========================================${NC}"
info "Versión: $NEW_VERSION"
info "Tag:     $NEW_VERSION"
if [[ "$DRY_RUN" == true ]]; then
    info "Modo:    DRY-RUN (no se ejecutó nada)"
fi
echo -e "${GREEN}========================================${NC}"
