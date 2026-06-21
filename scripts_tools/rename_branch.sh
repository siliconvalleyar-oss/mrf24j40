# ============================================================================
# Renombrar rama específica: v2.0.0 -> release/v2
# ============================================================================

rename_branch_to_release() {
    local OLD_BRANCH="v2.0.0"
    local NEW_BRANCH="release/v2"

    echo -e "\n${BLUE}--- Renombrar rama ---${NC}"
    
    # 1. Verificar si la rama antigua existe localmente
    if ! git show-ref --verify --quiet refs/heads/"$OLD_BRANCH"; then
        print_error "La rama '$OLD_BRANCH' no existe localmente."
        return 1
    fi

    # 2. Verificar si la rama nueva YA existe (para evitar sobreescritura)
    if git show-ref --verify --quiet refs/heads/"$NEW_BRANCH"; then
        print_error "La rama '$NEW_BRANCH' ya existe. No se puede renombrar."
        return 1
    fi

    # 3. Preguntar confirmación
    echo -e "Vas a renombrar: ${YELLOW}$OLD_BRANCH${NC} → ${GREEN}$NEW_BRANCH${NC}"
    read -p "¿Estás seguro de continuar? (s/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Ss]$ ]]; then
        print_info "Renombrado cancelado."
        return 0
    fi

    # 4. Ejecutar el renombrado
    print_info "Renombrando '$OLD_BRANCH' a '$NEW_BRANCH' ..."
    if git branch -m "$OLD_BRANCH" "$NEW_BRANCH"; then
        print_success "Rama renombrada localmente a '$NEW_BRANCH'."

        # 5. Preguntar si quiere sincronizar con el remoto
        read -p "¿Quieres actualizar el remoto (borrar la vieja y subir la nueva)? (s/N): " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Ss]$ ]]; then
            print_info "Subiendo '$NEW_BRANCH' al remoto..."
            if git push -u origin "$NEW_BRANCH"; then
                print_success "Rama '$NEW_BRANCH' subida al remoto."
                
                print_info "Eliminando '$OLD_BRANCH' del remoto..."
                if git push origin --delete "$OLD_BRANCH"; then
                    print_success "Rama '$OLD_BRANCH' eliminada del remoto."
                else
                    print_error "No se pudo eliminar '$OLD_BRANCH' del remoto."
                fi
            else
                print_error "Fallo al subir '$NEW_BRANCH' al remoto."
            fi
        else
            print_info "No se actualizó el remoto. Recuerda hacerlo después."
        fi
    else
        print_error "El renombrado falló."
        return 1
    fi
}
