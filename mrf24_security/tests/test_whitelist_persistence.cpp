/**
 * @file    tests/test_whitelist_persistence.cpp
 * @brief   Pruebas de persistencia: whitelist guardada y cargada desde JSON
 * @details Verifica el ciclo completo de guardado/carga de la whitelist
 *          a través de FileSystem_t, sin depender de hardware.
 *
 *          Pruebas:
 *          1. Guardar whitelist con MACs → cargar y verificar
 *          2. Guardar whitelist vacía → cargar lista vacía
 *          3. Guardar whitelist deshabilitada → cargar con enable=false
 *          4. Múltiples MACs en whitelist (3+)
 *          5. Formato MAC hexadecimal en JSON (16 dígitos, uppercase)
 *          6. Carga de JSON mal formado → valores por defecto
 *          7. Carga de archivo inexistente → valores por defecto
 *          8. Round-trip completo: modificar, guardar, recargar, verificar
 *          9. Preservación de otros campos al guardar whitelist
 *
 *          Ejecutar en la Pi:
 *            sudo ./tests/test_whitelist_persistence
 *
 * @author  MRF24J40 Team
 * @date    2026
 */

#include <services/filesystem.hpp>
#include <iostream>
#include <iomanip>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>

// ============================================================================
// ANSI colors
// ============================================================================
#define ANSI_RESET   "\x1b[0m"
#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_CYAN    "\x1b[36m"
#define ANSI_BOLD    "\x1b[1m"

// ============================================================================
// Test framework
// ============================================================================
static int g_test = 0;
static int g_pass = 0;
static int g_fail = 0;

#define TEST(name) do { \
    g_test++; \
    std::cout << "  [" << g_test << "] " << name << "... "; \
    std::cout.flush(); \
} while(0)

#define PASS() do { \
    g_pass++; \
    std::cout << ANSI_GREEN "PASS" ANSI_RESET "\n"; \
} while(0)

#define FAIL(msg) do { \
    g_fail++; \
    std::cout << ANSI_RED "FAIL" ANSI_RESET ": " << msg << "\n"; \
} while(0)

#define CHECK(cond, msg) do { \
    if (!(cond)) { FAIL(msg); return; } \
} while(0)

// ============================================================================
// Helpers
// ============================================================================

/** @brief Ruta del archivo de configuración temporal. */
static constexpr const char* TEST_CONFIG = "/tmp/test_whitelist_config.json";

/** @brief Limpia el archivo de configuración temporal. */
static void cleanConfig() {
    std::remove(TEST_CONFIG);
}

/** @brief MAC como string hex para logging. */
static std::string macHex(uint64_t mac) {
    char buf[20];
    snprintf(buf, sizeof(buf), "%016llX", (unsigned long long)mac);
    return buf;
}

/** @brief Lee el contenido del archivo JSON como string (para debug). */
static std::string readJsonFile() {
    std::ifstream f(TEST_CONFIG);
    if (!f) return "{FILE NOT FOUND}";
    std::string content((std::istreambuf_iterator<char>(f)),
                         std::istreambuf_iterator<char>());
    return content;
}

// ============================================================================
// Test 1: Guardar y cargar whitelist con 2 MACs
// ============================================================================

void test_save_and_load_basic() {
    TEST("Guardar whitelist con 2 MACs → cargar y verificar");

    cleanConfig();
    services::FileSystem_t fs(TEST_CONFIG);

    services::SystemConfig cfg;
    cfg.enable_whitelist = true;
    cfg.allowed_sources = {
        0x0000000000000002ULL,
        0x0000000000000003ULL
    };

    CHECK(fs.saveConfig(cfg), "saveConfig() should return true");

    auto loaded = fs.loadConfig();
    CHECK(loaded.enable_whitelist == true,
          "enable_whitelist should be true, got " +
          std::to_string(loaded.enable_whitelist));
    CHECK(loaded.allowed_sources.size() == 2,
          "allowed_sources.size() should be 2, got " +
          std::to_string(loaded.allowed_sources.size()));
    CHECK(loaded.allowed_sources[0] == 0x0000000000000002ULL,
          "Source[0] should be 0x02, got 0x" + macHex(loaded.allowed_sources[0]));
    CHECK(loaded.allowed_sources[1] == 0x0000000000000003ULL,
          "Source[1] should be 0x03, got 0x" + macHex(loaded.allowed_sources[1]));

    PASS();
}

// ============================================================================
// Test 2: Guardar whitelist vacía (habilitada pero sin MACs)
// ============================================================================

void test_empty_whitelist() {
    TEST("Guardar whitelist vacía → cargar lista vacía");

    cleanConfig();
    services::FileSystem_t fs(TEST_CONFIG);

    services::SystemConfig cfg;
    cfg.enable_whitelist = true;
    cfg.allowed_sources = {};  // Vacío explícitamente

    CHECK(fs.saveConfig(cfg), "saveConfig() should return true");

    auto loaded = fs.loadConfig();
    CHECK(loaded.enable_whitelist == true,
          "enable_whitelist should be true");
    CHECK(loaded.allowed_sources.empty(),
          "allowed_sources should be empty, got size " +
          std::to_string(loaded.allowed_sources.size()));

    PASS();
}

// ============================================================================
// Test 3: Guardar whitelist deshabilitada
// ============================================================================

void test_whitelist_disabled() {
    TEST("Guardar whitelist deshabilitada → enable_whitelist = false");

    cleanConfig();
    services::FileSystem_t fs(TEST_CONFIG);

    services::SystemConfig cfg;
    cfg.enable_whitelist = false;
    cfg.allowed_sources = {0x0000000000000002ULL};  // MACs no deberían importar

    CHECK(fs.saveConfig(cfg), "saveConfig() should return true");

    auto loaded = fs.loadConfig();
    CHECK(loaded.enable_whitelist == false,
          "enable_whitelist should be false");

    PASS();
}

// ============================================================================
// Test 4: Múltiples MACs en whitelist
// ============================================================================

void test_multiple_macs() {
    TEST("Guardar whitelist con 5 MACs → todas preservadas");

    cleanConfig();
    services::FileSystem_t fs(TEST_CONFIG);

    services::SystemConfig cfg;
    cfg.enable_whitelist = true;
    cfg.allowed_sources = {
        0x0000000000000001ULL,
        0x0000000000000002ULL,
        0x0000000000000003ULL,
        0x00000000000ABCDEULL,
        0xDEADBEEFCAFE1234ULL
    };

    CHECK(fs.saveConfig(cfg), "saveConfig() should return true");

    auto loaded = fs.loadConfig();
    CHECK(loaded.allowed_sources.size() == 5,
          "Expected 5 sources, got " + std::to_string(loaded.allowed_sources.size()));

    CHECK(loaded.allowed_sources[0] == 0x0000000000000001ULL, "Source[0] mismatch");
    CHECK(loaded.allowed_sources[1] == 0x0000000000000002ULL, "Source[1] mismatch");
    CHECK(loaded.allowed_sources[2] == 0x0000000000000003ULL, "Source[2] mismatch");
    CHECK(loaded.allowed_sources[3] == 0x00000000000ABCDEULL,
          "Source[3] should be 0x0000000000ABCDE, got 0x" +
          macHex(loaded.allowed_sources[3]));
    CHECK(loaded.allowed_sources[4] == 0xDEADBEEFCAFE1234ULL,
          "Source[4] should be 0xDEADBEEFCAFE1234, got 0x" +
          macHex(loaded.allowed_sources[4]));

    PASS();
}

// ============================================================================
// Test 5: Formato hexadecimal en JSON (16 dígitos, uppercase)
// ============================================================================

void test_json_format() {
    TEST("Formato MAC en JSON: 16 dígitos hexadecimal uppercase");

    cleanConfig();
    services::FileSystem_t fs(TEST_CONFIG);

    services::SystemConfig cfg;
    cfg.enable_whitelist = true;
    cfg.allowed_sources = {0xDEADBEEFCAFE1234ULL};

    CHECK(fs.saveConfig(cfg), "saveConfig() should return true");

    // Leer el archivo JSON directamente
    std::string json = readJsonFile();

    // Verificar que la MAC está en formato hex con 16 dígitos
    CHECK(json.find("DEADBEEFCAFE1234") != std::string::npos,
          "JSON should contain DEADBEEFCAFE1234 in hex. Content:\n" + json);

    // Verificar que NO está en formato decimal
    CHECK(json.find("16045690984833322036") == std::string::npos,
          "JSON should NOT contain decimal representation");

    // Verificar que enable_whitelist aparece como booleano
    CHECK(json.find("\"enable_whitelist\": true") != std::string::npos ||
          json.find("\"enable_whitelist\":true") != std::string::npos,
          "JSON should contain 'enable_whitelist': true");

    PASS();
}

// ============================================================================
// Test 6: Carga de JSON mal formado → valores por defecto
// ============================================================================

void test_corrupted_json() {
    TEST("Cargar JSON corrupto → valores por defecto (whitelist disabled)");

    cleanConfig();

    // Escribir JSON inválido
    {
        std::ofstream f(TEST_CONFIG);
        f << "{this is not valid json!!!}";
    }

    services::FileSystem_t fs(TEST_CONFIG);
    auto loaded = fs.loadConfig();

    CHECK(loaded.enable_whitelist == false,
          "enable_whitelist should be false for corrupted JSON");
    CHECK(loaded.allowed_sources.empty(),
          "allowed_sources should be empty for corrupted JSON");

    PASS();
}

// ============================================================================
// Test 7: Archivo inexistente → valores por defecto
// ============================================================================

void test_missing_file() {
    TEST("Cargar archivo inexistente → valores por defecto");

    cleanConfig();  // Asegurar que no existe

    services::FileSystem_t fs("/tmp/nonexistent_config_xyz.json");
    auto loaded = fs.loadConfig();

    CHECK(loaded.enable_whitelist == false,
          "enable_whitelist should be false for missing file");
    CHECK(loaded.allowed_sources.empty(),
          "allowed_sources should be empty for missing file");

    // Limpiar
    std::remove("/tmp/nonexistent_config_xyz.json");

    PASS();
}

// ============================================================================
// Test 8: Round-trip completo (modificar, guardar, recargar, verificar)
// ============================================================================

void test_round_trip() {
    TEST("Round-trip: guardar → cargar → modificar → guardar → cargar");

    cleanConfig();
    services::FileSystem_t fs(TEST_CONFIG);

    // --- Primera escritura ---
    services::SystemConfig cfg1;
    cfg1.enable_whitelist = true;
    cfg1.allowed_sources = {0x0000000000000002ULL};
    CHECK(fs.saveConfig(cfg1), "First saveConfig()");

    // --- Primera lectura ---
    auto loaded1 = fs.loadConfig();
    CHECK(loaded1.enable_whitelist, "After first save: whitelist should be enabled");
    CHECK(loaded1.allowed_sources.size() == 1, "After first save: 1 source expected");
    CHECK(loaded1.allowed_sources[0] == 0x0000000000000002ULL,
           "After first save: source should be 0x02");

    // --- Modificar y guardar ---
    loaded1.allowed_sources.push_back(0x0000000000000003ULL);
    loaded1.allowed_sources.push_back(0x0000000000000004ULL);
    CHECK(fs.saveConfig(loaded1), "Second saveConfig()");

    // --- Segunda lectura ---
    auto loaded2 = fs.loadConfig();
    CHECK(loaded2.allowed_sources.size() == 3,
          "After second save: expected 3 sources, got " +
          std::to_string(loaded2.allowed_sources.size()));
    CHECK(loaded2.allowed_sources[0] == 0x0000000000000002ULL,
          "Source[0] should be 0x02 after round-trip");
    CHECK(loaded2.allowed_sources[1] == 0x0000000000000003ULL,
          "Source[1] should be 0x03 after round-trip");
    CHECK(loaded2.allowed_sources[2] == 0x0000000000000004ULL,
          "Source[2] should be 0x04 after round-trip");

    // --- Deshabilitar y guardar ---
    loaded2.enable_whitelist = false;
    CHECK(fs.saveConfig(loaded2), "Third saveConfig()");

    // --- Tercera lectura ---
    auto loaded3 = fs.loadConfig();
    CHECK(loaded3.enable_whitelist == false,
          "After disabling: enable_whitelist should be false");
    // Las MACs deben preservarse aunque whitelist esté deshabilitada
    CHECK(loaded3.allowed_sources.size() == 3,
          "MACs should be preserved even when whitelist disabled");

    PASS();
}

// ============================================================================
// Test 9: Preservación de otros campos al guardar whitelist
// ============================================================================

void test_preserves_other_fields() {
    TEST("Guardar whitelist no corrompe otros campos de configuración");

    cleanConfig();
    services::FileSystem_t fs(TEST_CONFIG);

    services::SystemConfig cfg;
    cfg.pan_id = 0xBEEF;
    cfg.channel = 25;
    cfg.passphrase = "test-passphrase-123";
    cfg.use_oled = false;
    cfg.use_tft = false;
    cfg.node_mode = 2;
    cfg.mac_address = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    cfg.enable_encryption = false;
    cfg.enable_hash = false;
    cfg.role = services::NodeRole::Router;
    cfg.enable_whitelist = true;
    cfg.allowed_sources = {0x0000000000000002ULL};

    CHECK(fs.saveConfig(cfg), "saveConfig() should return true");

    auto loaded = fs.loadConfig();

    CHECK(loaded.pan_id == 0xBEEF,
          "pan_id should be 0xBEEF, got 0x" +
          macHex(loaded.pan_id));
    CHECK(loaded.channel == 25,
          "channel should be 25, got " + std::to_string(loaded.channel));
    CHECK(loaded.passphrase == "test-passphrase-123",
          "passphrase mismatch");
    CHECK(loaded.use_oled == false, "use_oled should be false");
    CHECK(loaded.use_tft == false, "use_tft should be false");
    CHECK(loaded.node_mode == 2,
          "node_mode should be 2, got " + std::to_string(loaded.node_mode));
    CHECK(loaded.enable_encryption == false, "enable_encryption should be false");
    CHECK(loaded.enable_hash == false, "enable_hash should be false");
    CHECK(loaded.role == services::NodeRole::Router,
          "role should be Router");

    // Verificar MAC address
    for (int i = 0; i < 8; i++) {
        CHECK(loaded.mac_address[i] == cfg.mac_address[i],
              "mac_address[" + std::to_string(i) + "] mismatch");
    }

    // Verificar whitelist
    CHECK(loaded.enable_whitelist == true, "enable_whitelist should be true");
    CHECK(loaded.allowed_sources.size() == 1, "allowed_sources should have 1 entry");

    PASS();
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "  ╔══════════════════════════════════════════╗\n";
    std::cout << "  ║   WHITELIST PERSISTENCE TESTS (v2.1.0)  ║\n";
    std::cout << "  ╚══════════════════════════════════════════╝\n\n";

    std::cout << "  ── Persistencia en JSON ──────────────────\n\n";

    test_save_and_load_basic();
    test_empty_whitelist();
    test_whitelist_disabled();
    test_multiple_macs();
    test_json_format();
    test_corrupted_json();
    test_missing_file();
    test_round_trip();
    test_preserves_other_fields();

    // ========================================================================
    // Resultados
    // ========================================================================

    std::cout << "\n  ── Resultados ───────────────────────────\n";
    std::cout << "  Total: " << g_test << "\n";
    std::cout << "  PASS:  " << g_pass << "\n";
    std::cout << "  FAIL:  " << g_fail << "\n";

    if (g_fail > 0) {
        std::cout << "\n  " ANSI_RED "⚠  " << g_fail
                  << " prueba(s) fallaron" ANSI_RESET "\n\n";
    } else {
        std::cout << "\n  " ANSI_GREEN "✓  Todas las pruebas pasaron" ANSI_RESET "\n\n";
    }

    // Limpiar archivo temporal
    cleanConfig();

    return g_fail > 0 ? 1 : 0;
}
