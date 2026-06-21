show_main_menu() {
    echo -e "\n${BLUE}=====================================${NC}"
    echo -e "${BLUE}       MENÚ INTERACTIVO GIT         ${NC}"
    echo -e "${BLUE}=====================================${NC}"
    echo -e " 1) ${GREEN}Fetch / Actualizar${NC}"
    echo -e " 2) ${GREEN}Listar TODAS las ramas${NC}"
    echo -e " 3) ${GREEN}Ver la rama más RECIENTE${NC}"
    echo -e " 4) ${GREEN}Cambiarse a una rama${NC}"
    echo -e " 5) ${GREEN}Pull${NC} en la rama actual"
    echo -e " 6) ${GREEN}Push${NC} en la rama actual"
    echo -e " 7) ${GREEN}Ver estado${NC}"
    echo -e " 8) ${YELLOW}Renombrar v2.0.0 → release/v2${NC}   # <--- NUEVA OPCIÓN"
    echo -e " 0) ${RED}Salir${NC}"
    echo -e "${BLUE}=====================================${NC}"
    read -p "Elige una opción (0-8): " choice   # Cambia el rango a 0-8
    echo
    case $choice in
        1) fetch_updates ;;
        2) list_branches ;;
        3) show_latest_branch ;;
        4) switch_branch ;;
        5) pull_current ;;
        6) push_current ;;
        7) show_status ;;
        8) rename_branch_to_release ;;   # <--- Llama a la nueva función
        0) echo "Saliendo..."; exit 0 ;;
        *) print_error "Opción no válida." ;;
    esac
}
