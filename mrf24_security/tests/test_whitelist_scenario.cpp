/**
 * @file    tests/test_whitelist_scenario.cpp
 * @brief   Escenario real: whitelist con 2 nodos simulados
 * @details Simula una red con 2 nodos MRF24J40 donde la whitelist
 *          filtra mensajes por MAC origen. Se prueba:
 *          - Configuración whitelist desde menú (vía API)
 *          - Mensaje desde MAC autorizada → aceptado
 *          - Mensaje desde MAC no autorizada → rechazado con contador
 *          - Broadcast desde MAC whitelisted → permitido
 *          - Broadcast desde MAC no whitelisted → rechazado (seguridad)
 *          - Desactivar whitelist → todas las MACs permitidas
 *
 *          Ejecutar en la Pi:
 *            sudo ./tests/test_whitelist_scenario
 *
 * @author  MRF24J40 Team
 * @date    2026
 */

#include <application/radio_manager.hpp>
#include <services/crypto.hpp>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>

// ============================================================================
// Códigos de color ANSI para la consola
// ============================================================================
#define ANSI_RESET   "\x1b[0m"
#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_CYAN    "\x1b[36m"
#define ANSI_BOLD    "\x1b[1m"

// ============================================================================
// Contador de pasos
// ============================================================================
static int g_step = 0;
static int g_fails = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { \
        std::cout << "  " ANSI_RED "✘ ERROR" ANSI_RESET ": " << msg << "\n"; \
        g_fails++; \
    } \
} while(0)

#define STEP(name) do { \
    g_step++; \
    std::cout << "\n" ANSI_BOLD "  Paso " << g_step << ": " ANSI_CYAN << name << ANSI_RESET "\n"; \
    std::cout << "  " << std::string(55, '─') << "\n"; \
} while(0)

#define OK(msg) do { std::cout << "    " ANSI_GREEN "✓" ANSI_RESET " " << msg << "\n"; } while(0)
#define INFO(msg) do { std::cout << "    " ANSI_BLUE "ℹ" ANSI_RESET " " << msg << "\n"; } while(0)
#define WARN(msg) do { std::cout << "    " ANSI_YELLOW "⚠" ANSI_RESET " " << msg << "\n"; } while(0)

// ============================================================================
// Helpers para construir tramas
// ============================================================================

static void uint64ToBytes(uint64_t val, uint8_t* out) {
    for (int i = 7; i >= 0; i--) {
        out[i] = static_cast<uint8_t>(val & 0xFF);
        val >>= 8;
    }
}

static std::vector<uint8_t> buildFrameWithHash(
    services::Crypto_t& crypto,
    uint64_t dest_mac, uint64_t src_mac, uint8_t ttl,
    const std::vector<uint8_t>& iv,
    const std::vector<uint8_t>& ciphertext)
{
    std::vector<uint8_t> frame;

    // dest_mac (8 bytes)
    uint8_t dest_bytes[8];
    uint64ToBytes(dest_mac, dest_bytes);
    frame.insert(frame.end(), dest_bytes, dest_bytes + 8);

    // src_mac (8 bytes)
    uint8_t src_bytes[8];
    uint64ToBytes(src_mac, src_bytes);
    frame.insert(frame.end(), src_bytes, src_bytes + 8);

    // TTL (1 byte)
    frame.push_back(ttl);

    // Size (2 bytes)
    uint16_t ct_size = static_cast<uint16_t>(ciphertext.size());
    frame.push_back((ct_size >> 8) & 0xFF);
    frame.push_back(ct_size & 0xFF);

    // IV (16 bytes)
    frame.insert(frame.end(), iv.begin(), iv.end());

    // Ciphertext
    frame.insert(frame.end(), ciphertext.begin(), ciphertext.end());

    // Hash SHA-256: SHA256(dest_mac + size + iv + ciphertext)
    std::vector<uint8_t> hash_input;
    hash_input.insert(hash_input.end(), dest_bytes, dest_bytes + 8);
    hash_input.push_back((ct_size >> 8) & 0xFF);
    hash_input.push_back(ct_size & 0xFF);
    hash_input.insert(hash_input.end(), iv.begin(), iv.end());
    hash_input.insert(hash_input.end(), ciphertext.begin(), ciphertext.end());

    auto hash = crypto.sha256(hash_input);
    frame.insert(frame.end(), hash.begin(), hash.end());

    return frame;
}

// ============================================================================
// Función para mostrar el estado de la whitelist
// ============================================================================

static void showWhitelistStatus(const application::RadioManager_t& mgr) {
    std::cout << "    ┌─ Estado de Whitelist ─────────────────┐\n";
    std::cout << "    │ Habilitada: " << (mgr.isWhitelistEnabled() ? ANSI_GREEN "SÍ" ANSI_RESET : ANSI_YELLOW "NO" ANSI_RESET) << "                         │\n";
    std::cout << "    │ MACs permitidas: " << mgr.allowedSources().size() << "                       │\n";
    for (size_t i = 0; i < mgr.allowedSources().size() && i < 4; i++) {
        char mac_str[20];
        snprintf(mac_str, sizeof(mac_str), "%016llX",
                 (unsigned long long)mgr.allowedSources()[i]);
        std::cout << "    │   " << (i+1) << ". " << mac_str << "          │\n";
    }
    if (mgr.allowedSources().size() > 4) {
        std::cout << "    │   ... y " << (mgr.allowedSources().size() - 4) << " más            │\n";
    }
    std::cout << "    └────────────────────────────────────────┘\n";
}

// ============================================================================
// Función para mostrar estadísticas de validación
// ============================================================================

static void showValidationStats(const application::RadioManager_t& mgr) {
    auto stats = mgr.validationStats();
    std::cout << "    ┌─ Estadísticas de Validación ───────────┐\n";
    std::cout << "    │ Validados:      " << std::setw(10) << stats.messages_validated << "              │\n";
    std::cout << "    │ Rechazados:     " << std::setw(10) << stats.messages_rejected << "              │\n";
    std::cout << "    │ Hash errors:    " << std::setw(10) << stats.hash_errors << "              │\n";
    std::cout << "    │ Fuentes deneg.: " << std::setw(10) << stats.source_denied << "              │\n";
    std::cout << "    │ TTL expirados:  " << std::setw(10) << stats.ttl_expired << "              │\n";
    std::cout << "    │ Reenviados:     " << std::setw(10) << stats.messages_forwarded << "              │\n";
    std::cout << "    └────────────────────────────────────────┘\n";
}

// ============================================================================
// Función para mostrar una MAC como string
// ============================================================================

static std::string macStr(uint64_t mac) {
    char buf[20];
    snprintf(buf, sizeof(buf), "%016llX", (unsigned long long)mac);
    return buf;
}

// ============================================================================
// Main — Escenario real
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "  ╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "  ║                                                           ║\n";
    std::cout << "  ║   " ANSI_BOLD "ESCENARIO REAL: WHITELIST DE MACs" ANSI_RESET "                  ║\n";
    std::cout << "  ║                                                           ║\n";
    std::cout << "  ║   Red con 2 nodos MRF24J40                              ║\n";
    std::cout << "  ║   Nodo A: 00:00:00:00:00:00:00:01  (nosotros)           ║\n";
    std::cout << "  ║   Nodo B: 00:00:00:00:00:00:00:02  (autorizado)         ║\n";
    std::cout << "  ║   Nodo C: 00:00:00:00:00:00:00:03  (no autorizado)      ║\n";
    std::cout << "  ║                                                           ║\n";
    std::cout << "  ╚═══════════════════════════════════════════════════════════╝\n";

    // ========================================================================
    // Escenario: Red IoT con whitelist de seguridad
    // ========================================================================
    //
    // Contexto: Una red de sensores donde solo ciertos nodos pueden
    // enviar mensajes. El administrador configura una whitelist con
    // las MACs de los nodos autorizados.
    //
    // Nodos:
    //   A (0x01) - Coordinador de red, recibe mensajes de todos
    //   B (0x02) - Sensor de temperatura, autorizado para enviar
    //   C (0x03) - Dispositivo desconocido, NO autorizado
    //
    // ========================================================================

    // ========================================================================
    // Paso 1: Inicializar sistema
    // ========================================================================

    STEP("Inicializar sistema con configuración limpia");

    application::RadioManager_t mgr;

    services::SystemConfig cfg;
    cfg.enable_whitelist = false;   // Empezar sin whitelist
    cfg.use_oled = false;
    cfg.use_tft = false;
    cfg.log_to_file = false;        // Sin logging a archivo para esta prueba
    mgr.initFromConfig(cfg);

    if (!mgr.isReady()) {
        std::cerr << "\n  " ANSI_RED "ERROR FATAL" ANSI_RESET
                  << ": No se pudo inicializar el sistema (crypto no disponible?)\n";
        std::cerr << "  Ejecutar con: sudo ./tests/test_whitelist_scenario\n";
        return 1;
    }

    std::string local_mac_str = macStr(mgr.localMac64());
    OK("Sistema inicializado. MAC local: " << ANSI_CYAN << local_mac_str << ANSI_RESET);
    OK("Rol: EndDevice (no reenvía mensajes)");
    OK("Cifrado: habilitado");
    showValidationStats(mgr);

    // ========================================================================
    // Paso 2: Inicialmente SIN whitelist — todos los nodos pueden comunicarse
    // ========================================================================

    STEP("Sin whitelist: todos los nodos pueden enviar mensajes");

    std::vector<uint8_t> iv = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
    };
    std::vector<uint8_t> payload = {0x48, 0x65, 0x6C, 0x6C, 0x6F};  // "Hello"

    uint64_t nodo_A = mgr.localMac64();
    uint64_t nodo_B = 0x0000000000000002ULL;
    uint64_t nodo_C = 0x0000000000000003ULL;

    // Mensaje desde Nodo B (autorizado en el futuro)
    {
        auto frame = buildFrameWithHash(mgr.crypto(), nodo_A, nodo_B, 5, iv, payload);
        auto result = mgr.validateMessage(frame.data(), static_cast<uint8_t>(frame.size()));
        CHECK(result.valid, "Mensaje desde Nodo B debería ser válido sin whitelist");
        if (result.valid) OK("Nodo B → A: " ANSI_GREEN "ACEPTADO" ANSI_RESET " (whitelist deshabilitada)");
        else WARN("Nodo B → A: RECHAZADO: " + result.error_msg);
    }

    // Mensaje desde Nodo C (no autorizado en el futuro)
    {
        auto frame = buildFrameWithHash(mgr.crypto(), nodo_A, nodo_C, 5, iv, payload);
        auto result = mgr.validateMessage(frame.data(), static_cast<uint8_t>(frame.size()));
        CHECK(result.valid, "Mensaje desde Nodo C debería ser válido sin whitelist");
        if (result.valid) OK("Nodo C → A: " ANSI_GREEN "ACEPTADO" ANSI_RESET " (whitelist deshabilitada)");
        else WARN("Nodo C → A: RECHAZADO: " + result.error_msg);
    }

    showValidationStats(mgr);

    // ========================================================================
    // Paso 3: Activar whitelist — solo Nodo B autorizado
    // ========================================================================

    STEP("Activar whitelist y agregar Nodo B (0x02) como única fuente autorizada");

    INFO("Agregando Nodo B a la whitelist...");
    mgr.addAllowedSource(nodo_B);
    INFO("Activando whitelist...");
    mgr.setWhitelistEnabled(true);

    showWhitelistStatus(mgr);

    CHECK(mgr.isWhitelistEnabled(), "Whitelist debería estar habilitada");
    CHECK(mgr.allowedSources().size() == 1, "Debería haber exactamente 1 MAC en whitelist");

    if (mgr.isWhitelistEnabled()) OK("Whitelist " ANSI_GREEN "ACTIVADA" ANSI_RESET);
    showValidationStats(mgr);

    // ========================================================================
    // Paso 4: Nodo B (autorizado) envía un mensaje → debe ser aceptado
    // ========================================================================

    STEP("Nodo B (0x02, autorizado) envía mensaje → debe ser ACEPTADO");

    {
        auto frame = buildFrameWithHash(mgr.crypto(), nodo_A, nodo_B, 5, iv, payload);
        auto result = mgr.validateMessage(frame.data(), static_cast<uint8_t>(frame.size()));

        CHECK(result.valid, "Mensaje desde Nodo B (whitelisted) debería pasar validación");
        CHECK(!result.error_msg.empty() == !result.valid,
              "Mensaje válido no debería tener error");

        if (result.valid) {
            OK("Nodo B → A: " ANSI_GREEN "ACEPTADO" ANSI_RESET);
            OK("Hash: OK | Origen: " + macStr(nodo_B) + " (en whitelist)");
        } else {
            WARN("Nodo B → A: RECHAZADO: " + result.error_msg);
        }
    }

    showValidationStats(mgr);

    // ========================================================================
    // Paso 5: Nodo C (no autorizado) envía un mensaje → debe ser RECHAZADO
    // ========================================================================

    STEP("Nodo C (0x03, NO autorizado) envía mensaje → debe ser RECHAZADO");

    {
        auto frame = buildFrameWithHash(mgr.crypto(), nodo_A, nodo_C, 5, iv, payload);
        auto result = mgr.validateMessage(frame.data(), static_cast<uint8_t>(frame.size()));

        CHECK(!result.valid, "Mensaje desde Nodo C (no whitelisted) debería ser rechazado");
        CHECK(result.error_msg.find("denied") != std::string::npos,
              "Error debería mencionar 'denied'");

        if (!result.valid) {
            OK("Nodo C → A: " ANSI_RED "RECHAZADO" ANSI_RESET);
            INFO("Motivo: " + result.error_msg);
        } else {
            WARN("Nodo C → A: ACEPTADO (inesperado)");
        }
    }

    showValidationStats(mgr);

    // ========================================================================
    // Paso 6: Verificar contador source_denied
    // ========================================================================

    STEP("Verificar contadores de validación");

    auto stats = mgr.validationStats();
    CHECK(stats.source_denied == 1,
          "source_denied debería ser 1 (1 mensaje de Nodo C rechazado)");
    CHECK(stats.messages_rejected >= 1,
          "messages_rejected debería ser >= 1");
    CHECK(stats.messages_validated >= 1,
          "messages_validated debería ser >= 1 (Nodo B aceptado)");

    INFO("Resumen de comunicaciones:");
    INFO("  Nodo B (autorizado):   " ANSI_GREEN "1 mensaje aceptado" ANSI_RESET);
    INFO("  Nodo C (no autorizado): " ANSI_RED "1 mensaje rechazado" ANSI_RESET);
    INFO("  source_denied = " + std::to_string(stats.source_denied));

    showValidationStats(mgr);

    // ========================================================================
    // Paso 7: Broadcast desde Nodo B (autorizado) — debe ser ACEPTADO
    // ========================================================================
    // NOTA: La whitelist verifica el ORIGEN (src_mac), no el destino.
    //       Un broadcast es válido solo si la MAC origen está autorizada.

    STEP("Broadcast desde Nodo B (0x02, whitelisted) → debe ser ACEPTADO");

    {
        uint64_t broadcast = application::BROADCAST_ADDR;
        auto frame = buildFrameWithHash(mgr.crypto(), broadcast, nodo_B, 3, iv, payload);
        auto result = mgr.validateMessage(frame.data(), static_cast<uint8_t>(frame.size()));

        CHECK(result.valid,
              "Broadcast desde Nodo B (whitelisted) debería ser aceptado");
        CHECK(result.for_us,
              "Broadcast debería marcarse como for_us");

        if (result.valid) {
            OK("Broadcast B → Todos: " ANSI_GREEN "ACEPTADO" ANSI_RESET);
            INFO("Origen whitelisted + destino broadcast = permitido");
            INFO("for_us=Y, should_forward=" << (result.should_forward ? "Y" : "N"));
        } else {
            WARN("Broadcast B → Todos: RECHAZADO: " + result.error_msg);
        }
    }

    showValidationStats(mgr);

    // ========================================================================
    // Paso 8: Broadcast desde Nodo C (no autorizado) — debe ser RECHAZADO
    // ========================================================================
    // La whitelist verifica src_mac, no dest_mac. Un broadcast desde MAC no
    // autorizada es rechazado por seguridad: el nodo no es fuente conocida.

    STEP("Broadcast desde Nodo C (0x03, NO whitelisted) → debe ser RECHAZADO");

    {
        uint64_t broadcast = application::BROADCAST_ADDR;
        auto frame = buildFrameWithHash(mgr.crypto(), broadcast, nodo_C, 3, iv, payload);
        auto result = mgr.validateMessage(frame.data(), static_cast<uint8_t>(frame.size()));

        CHECK(!result.valid,
              "Broadcast desde Nodo C (no whitelisted) debería ser rechazado");
        CHECK(result.error_msg.find("denied") != std::string::npos,
              "Error debería decir 'denied'");

        if (!result.valid) {
            OK("Broadcast C → Todos: " ANSI_RED "RECHAZADO" ANSI_RESET);
            INFO("Motivo: " + result.error_msg);
            INFO("La whitelist filtra por ORIGEN: C no está autorizado");
        } else {
            WARN("Broadcast C → Todos: ACEPTADO (inesperado)");
        }
    }

    showValidationStats(mgr);

    stats = mgr.validationStats();
    CHECK(stats.source_denied == 2,
          "source_denied debería ser 2 (C unicast denegado + C broadcast denegado)");

    // ========================================================================
    // Paso 9: Desactivar whitelist — todos vuelven a ser aceptados
    // ========================================================================

    STEP("Desactivar whitelist — Nodo C vuelve a poder enviar");

    INFO("Desactivando whitelist...");
    mgr.setWhitelistEnabled(false);

    CHECK(!mgr.isWhitelistEnabled(), "Whitelist debería estar deshabilitada");
    OK("Whitelist " ANSI_YELLOW "DESACTIVADA" ANSI_RESET);

    // Nodo C ahora debería ser aceptado
    {
        auto frame = buildFrameWithHash(mgr.crypto(), nodo_A, nodo_C, 5, iv, payload);
        auto result = mgr.validateMessage(frame.data(), static_cast<uint8_t>(frame.size()));

        CHECK(result.valid, "Nodo C debería ser aceptado con whitelist desactivada");

        if (result.valid) {
            OK("Nodo C → A (whitelist OFF): " ANSI_GREEN "ACEPTADO" ANSI_RESET);
        } else {
            WARN("Nodo C → A (whitelist OFF): RECHAZADO: " + result.error_msg);
        }
    }

    showValidationStats(mgr);

    // ========================================================================
    // Paso 10: Reactivar whitelist con múltiples MACs
    // ========================================================================

    STEP("Reactivar whitelist con ambos nodos (B y C) autorizados");

    INFO("Agregando Nodo C a la whitelist...");
    mgr.addAllowedSource(nodo_C);
    INFO("Reactivando whitelist...");
    mgr.setWhitelistEnabled(true);

    showWhitelistStatus(mgr);

    // Ambos nodos deben ser aceptados ahora
    {
        auto frame_b = buildFrameWithHash(mgr.crypto(), nodo_A, nodo_B, 5, iv, payload);
        auto result_b = mgr.validateMessage(frame_b.data(), static_cast<uint8_t>(frame_b.size()));

        auto frame_c = buildFrameWithHash(mgr.crypto(), nodo_A, nodo_C, 5, iv, payload);
        auto result_c = mgr.validateMessage(frame_c.data(), static_cast<uint8_t>(frame_c.size()));

        CHECK(result_b.valid, "Nodo B debería ser aceptado (en whitelist)");
        CHECK(result_c.valid, "Nodo C debería ser aceptado (agregado a whitelist)");

        if (result_b.valid && result_c.valid) {
            OK("Nodo B → A (whitelist ON): " ANSI_GREEN "ACEPTADO" ANSI_RESET);
            OK("Nodo C → A (whitelist ON): " ANSI_GREEN "ACEPTADO" ANSI_RESET);
            INFO("Ambos nodos autorizados en la whitelist");
        }
    }

    // ========================================================================
    // Paso 11: Limpiar whitelist — todos denegados
    // ========================================================================

    STEP("Limpiar whitelist — todos los orígenes denegados");

    INFO("Limpiando whitelist...");
    mgr.clearAllowedSources();
    CHECK(mgr.allowedSources().empty(), "Whitelist debería estar vacía después de clear");

    showWhitelistStatus(mgr);

    // Ahora incluso Nodo B debe ser rechazado (whitelist activa pero vacía)
    {
        auto frame = buildFrameWithHash(mgr.crypto(), nodo_A, nodo_B, 5, iv, payload);
        auto result = mgr.validateMessage(frame.data(), static_cast<uint8_t>(frame.size()));

        CHECK(!result.valid, "Nodo B debería ser rechazado (whitelist vacía)");

        if (!result.valid) {
            OK("Nodo B → A (whitelist vacía): " ANSI_RED "RECHAZADO" ANSI_RESET);
            INFO("Whitelist vacía = ningún origen permitido");
        }
    }

    showValidationStats(mgr);

    // ========================================================================
    // Resumen final
    // ========================================================================

    std::cout << "\n";
    std::cout << "  ╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "  ║                                                           ║\n";

    if (g_fails == 0) {
        std::cout << "  ║   " ANSI_GREEN ANSI_BOLD "✓  WHITELIST FUNCIONA CORRECTAMENTE" ANSI_RESET "              ║\n";
    } else {
        std::cout << "  ║   " ANSI_RED ANSI_BOLD "✘  WHITELIST - " << g_fails << " FALLOS" ANSI_RESET << "                    ║\n";
    }

    std::cout << "  ║                                                           ║\n";
    std::cout << "  ║   Resumen del escenario:                                ║\n";
    std::cout << "  ║                                                           ║\n";
    std::cout << "  ║   " << ANSI_CYAN " Sin whitelist:" ANSI_RESET "                                     ║\n";
    std::cout << "  ║     - Nodo B (0x02): ACEPTADO                           ║\n";
    std::cout << "  ║     - Nodo C (0x03): ACEPTADO                           ║\n";
    std::cout << "  ║                                                           ║\n";
    std::cout << "  ║   " ANSI_CYAN " Whitelist ON (solo 0x02):" ANSI_RESET "                             ║\n";
    std::cout << "  ║     - Nodo B (0x02): ACEPTADO    ← en whitelist         ║\n";
    std::cout << "  ║     - Nodo C (0x03): RECHAZADO   ← source_denied++      ║\n";
    std::cout << "  ║                                                           ║\n";
    std::cout << "  ║   " ANSI_CYAN " Broadcast (0xFF..FF):" ANSI_RESET "                             ║\n";
    std::cout << "  ║     - Desde B (whitelisted): ACEPTADO                   ║\n";
    std::cout << "  ║     - Desde C (no whitelist): RECHAZADO (src no auth)   ║\n";
    std::cout << "  ║     - La whitelist filtra por ORIGEN, no por destino    ║\n";
    std::cout << "  ║                                                           ║\n";
    std::cout << "  ║   " ANSI_CYAN " Whitelist OFF:" ANSI_RESET "                                    ║\n";
    std::cout << "  ║     - Nodo C (0x03): ACEPTADO (vuelve a funcionar)      ║\n";
    std::cout << "  ║                                                           ║\n";
    std::cout << "  ╚═══════════════════════════════════════════════════════════╝\n";

    std::cout << "\n  Pruebas completadas: " << (g_fails == 0 ? ANSI_GREEN "0 fallos" ANSI_RESET : ANSI_RED + std::to_string(g_fails) + " fallos" ANSI_RESET) << "\n\n";

    return g_fails > 0 ? 1 : 0;
}
