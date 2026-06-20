#!/bin/bash

# Colores para mejorar la legibilidad
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # Sin color

# --- Funciones auxiliares ---
print_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
print_success() { echo -e "${GREEN}[OK]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }

get_current_branch() {
    git branch --show-current 2>/dev/null || git rev-parse --abbrev-ref HEAD 2>/dev/null
}

# --- Funciones del menú ---

fetch_updates() {
    print_info "Obteniendo todas las ramas del remoto (fetch --all --prune)..."
    git fetch --all --prune
    if [ $? -eq 0 ]; then
        print_success "Fetch completado."
    else
        print_error "Error al hacer fetch. Revisa tu conexión o el remoto."
    fi
}

list_branches() {
    echo -e "\n${BLUE}--- Ramas LOCALES ---${NC}"
    git branch
    echo -e "\n${BLUE}--- Ramas REMOTAS (ordenadas por fecha, más nueva primero) ---${NC}"
    git for-each-ref --sort=-committerdate refs/remotes/origin/ --format='  %(refname:short) - %(committerdate:relative)'
}

show_latest_branch() {
    echo -e "\n${BLUE}--- Rama remota más RECIENTE (por fecha de commit) ---${NC}"
    latest=$(git for-each-ref --sort=-committerdate refs/remotes/origin/ --format='%(refname:short)' | grep -v "/HEAD" | head -1)
    latest_date=$(git for-each-ref --sort=-committerdate refs/remotes/origin/ --format='%(refname:short) - %(committerdate:relative)' | grep -v "/HEAD" | head -1)
    
    if [ -n "$latest" ]; then
        print_success "La rama más nueva es: $latest"
        echo "   ($latest_date)"
    else
        print_error "No se encontraron ramas remotas."
    fi
}

switch_branch() {
    echo -e "\n${BLUE}--- Selecciona una rama remota para cambiarte (nuevas primero) ---${NC}"
    # Crear un array con las ramas (excluyendo origin/HEAD)
    mapfile -t branches < <(git for-each-ref --sort=-committerdate refs/remotes/origin/ --format='%(refname:short)' | grep -v "/HEAD")
    
    if [ ${#branches[@]} -eq 0 ]; then
        print_error "No hay ramas remotas disponibles."
        return
    fi

    # Mostrar menú interactivo con 'select'
    PS3="Elige el número de la rama (o escribe 0 para cancelar): "
    select branch in "${branches[@]}" "Cancelar"; do
        if [ "$branch" = "Cancelar" ]; then
            print_info "Operación cancelada."
            return
        elif [ -n "$branch" ]; then
            local_name=${branch#origin/}  # Quita "origin/" para el nombre local
            print_info "Cambiando a la rama: $local_name"
            
            # Intenta crearla localmente desde remota, si ya existe solo se cambia
            git checkout -b "$local_name" "$branch" 2>/dev/null || git checkout "$local_name"
            
            if [ $? -eq 0 ]; then
                print_success "Ahora estás en la rama '$local_name'."
            else
                print_error "No se pudo cambiar de rama."
            fi
            return
        else
            print_error "Opción inválida."
        fi
    done
}

pull_current() {
    current=$(get_current_branch)
    if [ -z "$current" ] || [ "$current" = "HEAD" ]; then
        print_error "No estás en una rama (estás en detached HEAD)."
        return
    fi
    print_info "Haciendo pull de la rama actual '$current'..."
    git pull
    if [ $? -eq 0 ]; then
        print_success "Pull completado."
    else
        print_error "Pull falló. Resuelve los conflictos manualmente."
    fi
}

push_current() {
    current=$(get_current_branch)
    if [ -z "$current" ] || [ "$current" = "HEAD" ]; then
        print_error "No estás en una rama. No se puede hacer push."
        return
    fi

    print_warning "Antes de hacer push, es recomendable actualizar con los cambios remotos."
    read -p "¿Quieres hacer 'git pull' para actualizar '$current' antes de pushear? (s/n): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Ss]$ ]]; then
        pull_current
        # Preguntamos si a pesar de algún error quiere continuar
        read -p "¿Continuar con el push de todas formas? (s/n): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Ss]$ ]]; then
            print_info "Push cancelado."
            return
        fi
    else
        print_warning "Omitiendo pull. El push podría ser rechazado si el remoto tiene nuevos commits."
    fi

    print_info "Subiendo (push) la rama '$current' al remoto..."
    git push -u origin "$current"  # -u para establecer upstream si es primera vez
    if [ $? -eq 0 ]; then
        print_success "Push completado exitosamente."
    else
        print_error "Push falló. Si el remoto tiene cambios nuevos, haz pull primero."
    fi
}

show_status() {
    git status
}

# --- Menú principal ---
show_main_menu() {
    echo -e "\n${BLUE}=====================================${NC}"
    echo -e "${BLUE}       MENÚ INTERACTIVO GIT         ${NC}"
    echo -e "${BLUE}=====================================${NC}"
    echo -e " 1) ${GREEN}Fetch / Actualizar${NC} (git fetch --all --prune)"
    echo -e " 2) ${GREEN}Listar TODAS las ramas${NC} (ordenadas por fecha)"
    echo -e " 3) ${GREEN}Ver la rama más RECIENTE${NC} (último commit)"
    echo -e " 4) ${GREEN}Cambiarse a una rama${NC} (selección interactiva)"
    echo -e " 5) ${GREEN}Pull${NC} en la rama actual"
    echo -e " 6) ${GREEN}Push${NC} en la rama actual (¡con pull previo opcional!)"
    echo -e " 7) ${GREEN}Ver estado${NC} (git status)"
    echo -e " 0) ${RED}Salir${NC}"
    echo -e "${BLUE}=====================================${NC}"
    read -p "Elige una opción (0-7): " choice
    echo
    case $choice in
        1) fetch_updates ;;
        2) list_branches ;;
        3) show_latest_branch ;;
        4) switch_branch ;;
        5) pull_current ;;
        6) push_current ;;
        7) show_status ;;
        0) echo "Saliendo..."; exit 0 ;;
        *) print_error "Opción no válida. Intenta de nuevo." ;;
    esac
}

# --- Comprobación inicial: ¿estamos dentro de un repositorio Git? ---
if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    print_error "No estás dentro de un repositorio Git."
    print_info "Ejecuta este script dentro de la carpeta de tu proyecto."
    exit 1
fi

# --- Bucle infinito del menú ---
while true; do
    show_main_menu
    echo -e "\nPresiona Enter para volver al menú..."
    read
done

