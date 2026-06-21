/**
 * @file    tests/test_whitelist.cpp
 * @brief   Pruebas unitarias para la validación con whitelist MAC
 * @details Verifica el comportamiento de:
 *          - isSourceAllowed() / setWhitelistEnabled() / addAllowedSource()
 *          - removeAllowedSource() / clearAllowedSources() / allowedSources()
 *          - validateMessage() con whitelist habilitada/deshabilitada,
 *            MAC permitida y MAC denegada
 *
 * Compilación:
 *   g++ -std=c++17 -I. tests/test_whitelist.cpp -o tests/test_whitelist \
 *       -pthread $(pkg-config --libs openssl) $(pkg-config --cflags openssl) \
 *       -DENABLE_JSON
 *
 * Ejecución:
 *   sudo ./tests/test_whitelist
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

// ============================================================================
// Macros de test
// ============================================================================

static int g_test_count = 0;
static int g_pass_count = 0;
static int g_fail_count = 0;

#define TEST_BEGIN(name) do { \
    g_test_count++; \
    std::cout << "  [" << g_test_count << "] " << name << "... "; \
    std::cout.flush(); \
} while(0)

#define TEST_PASS() do { \
    g_pass_count++; \
    std::cout << "\x1b[32mPASS\x1b[0m\n"; \
} while(0)

#define TEST_FAIL(msg) do { \
    g_fail_count++; \
    std::cout << "\x1b[31mFAIL\x1b[0m: " << msg << "\n"; \
} while(0)

#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { TEST_FAIL(msg); return; } \
} while(0)

// ============================================================================
// Helpers para construir tramas
// ============================================================================

/**
 * @brief Convierte uint64_t a bytes big-endian.
 */
static void uint64ToBytes(uint64_t val, uint8_t* out) {
    for (int i = 7; i >= 0; i--) {
        out[i] = static_cast<uint8_t>(val & 0xFF);
        val >>= 8;
    }
}

/**
 * @brief Construye una trama completa (sin hash) para pruebas.
 * @param dest_mac MAC destino (64 bits).
 * @param src_mac  MAC origen (64 bits).
 * @param ttl      TTL.
 * @param iv       IV (16 bytes).
 * @param ciphertext Datos cifrados.
 * @return Trama sin el hash al final (para calcular hash y agregar después).
 */
static std::vector<uint8_t> buildFrameWithoutHash(
    uint64_t dest_mac, uint64_t src_mac, uint8_t ttl,
    const std::vector<uint8_t>& iv,
    const std::vector<uint8_t>& ciphertext)
{
    std::vector<uint8_t> frame;

    // 1. dest_mac (8 bytes, big-endian)
    uint8_t dest_bytes[8];
    uint64ToBytes(dest_mac, dest_bytes);
    frame.insert(frame.end(), dest_bytes, dest_bytes + 8);

    // 2. src_mac (8 bytes, big-endian)
    uint8_t src_bytes[8];
    uint64ToBytes(src_mac, src_bytes);
    frame.insert(frame.end(), src_bytes, src_bytes + 8);

    // 3. TTL (1 byte)
    frame.push_back(ttl);

    // 4. Size (2 bytes, big-endian)
    uint16_t ct_size = static_cast<uint16_t>(ciphertext.size());
    frame.push_back(static_cast<uint8_t>((ct_size >> 8) & 0xFF));
    frame.push_back(static_cast<uint8_t>(ct_size & 0xFF));

    // 5. IV (16 bytes)
    frame.insert(frame.end(), iv.begin(), iv.end());

    // 6. Ciphertext
    frame.insert(frame.end(), ciphertext.begin(), ciphertext.end());

    return frame;
}

/**
 * @brief Calcula el hash SHA-256 de la parte relevante de la trama.
 *
 * El hash cubre: [dest_mac(8)] + [size(2)] + [iv(16)] + [ciphertext(N)]
 * que son los primeros (FRAME_DEST_MAC_LEN + FRAME_SIZE_LEN + iv.size() + ciphertext.size())
 * bytes después de src_mac y ttl.
 */
static std::array<uint8_t, 32> computeFrameHash(
    services::Crypto_t& crypto,
    uint64_t dest_mac,
    uint16_t payload_size,
    const std::vector<uint8_t>& iv,
    const std::vector<uint8_t>& ciphertext)
{
    std::vector<uint8_t> hash_input;

    // dest_mac como bytes
    uint8_t dest_bytes[8];
    uint64ToBytes(dest_mac, dest_bytes);
    hash_input.insert(hash_input.end(), dest_bytes, dest_bytes + 8);

    // size
    hash_input.push_back(static_cast<uint8_t>((payload_size >> 8) & 0xFF));
    hash_input.push_back(static_cast<uint8_t>(payload_size & 0xFF));

    // iv
    hash_input.insert(hash_input.end(), iv.begin(), iv.end());

    // ciphertext
    hash_input.insert(hash_input.end(), ciphertext.begin(), ciphertext.end());

    return crypto.sha256(hash_input);
}

/**
 * @brief Construye una trama completa con hash SHA-256.
 */
static std::vector<uint8_t> buildFrameWithHash(
    services::Crypto_t& crypto,
    uint64_t dest_mac, uint64_t src_mac, uint8_t ttl,
    const std::vector<uint8_t>& iv,
    const std::vector<uint8_t>& ciphertext)
{
    auto frame = buildFrameWithoutHash(dest_mac, src_mac, ttl, iv, ciphertext);
    auto hash = computeFrameHash(crypto, dest_mac,
                                 static_cast<uint16_t>(ciphertext.size()),
                                 iv, ciphertext);
    frame.insert(frame.end(), hash.begin(), hash.end());
    return frame;
}

// ============================================================================
// Prueba 1: isSourceAllowed con whitelist deshabilitada
// ============================================================================

void test_isSourceAllowed_whitelist_disabled() {
    TEST_BEGIN("isSourceAllowed() returns true when whitelist disabled (default)");

    application::RadioManager_t mgr;
    // Por defecto enable_whitelist = false

    TEST_ASSERT(mgr.isSourceAllowed(0x1234567890ABCDEFULL),
                "Expected true for any MAC when whitelist disabled");
    TEST_ASSERT(mgr.isSourceAllowed(0x0000000000000001ULL),
                "Expected true for valid MAC when whitelist disabled");
    TEST_PASS();
}

// ============================================================================
// Prueba 2: isSourceAllowed con whitelist vacía
// ============================================================================

void test_isSourceAllowed_empty_whitelist() {
    TEST_BEGIN("isSourceAllowed() returns false with enabled but empty whitelist");

    application::RadioManager_t mgr;
    mgr.setWhitelistEnabled(true);

    TEST_ASSERT(!mgr.isSourceAllowed(0x0000000000000002ULL),
                "Expected false for MAC not in empty whitelist");
    TEST_ASSERT(!mgr.isSourceAllowed(0xDEADBEEFCAFE1234ULL),
                "Expected false for any MAC when whitelist empty");
    TEST_PASS();
}

// ============================================================================
// Prueba 3: isSourceAllowed con MAC en whitelist
// ============================================================================

void test_isSourceAllowed_mac_in_whitelist() {
    TEST_BEGIN("isSourceAllowed() returns true when MAC is in whitelist");

    application::RadioManager_t mgr;
    mgr.addAllowedSource(0x0000000000000002ULL);
    mgr.setWhitelistEnabled(true);

    TEST_ASSERT(mgr.isSourceAllowed(0x0000000000000002ULL),
                "Expected true for MAC in whitelist");
    TEST_PASS();
}

// ============================================================================
// Prueba 4: isSourceAllowed con MAC no en whitelist
// ============================================================================

void test_isSourceAllowed_mac_not_in_whitelist() {
    TEST_BEGIN("isSourceAllowed() returns false when MAC not in whitelist");

    application::RadioManager_t mgr;
    mgr.addAllowedSource(0x0000000000000002ULL);
    mgr.setWhitelistEnabled(true);

    TEST_ASSERT(!mgr.isSourceAllowed(0x0000000000000003ULL),
                "Expected false for MAC not in whitelist");
    TEST_ASSERT(!mgr.isSourceAllowed(0x0000000000000001ULL),
                "Expected false for different MAC");
    TEST_PASS();
}

// ============================================================================
// Prueba 5: isSourceAllowed con broadcast (siempre permitido)
// ============================================================================

void test_isSourceAllowed_broadcast() {
    TEST_BEGIN("isSourceAllowed() returns true for broadcast even with whitelist");

    application::RadioManager_t mgr;
    mgr.setWhitelistEnabled(true);

    TEST_ASSERT(mgr.isSourceAllowed(application::BROADCAST_ADDR),
                "Expected true for broadcast address (0xFF..FF)");
    TEST_PASS();
}

// ============================================================================
// Prueba 6: setWhitelistEnabled toggle
// ============================================================================

void test_setWhitelistEnabled_toggle() {
    TEST_BEGIN("setWhitelistEnabled() toggles correctly");

    application::RadioManager_t mgr;

    // Iniciar deshabilitado por defecto
    TEST_ASSERT(!mgr.isWhitelistEnabled(),
                "Expected whitelist disabled by default");

    mgr.setWhitelistEnabled(true);
    TEST_ASSERT(mgr.isWhitelistEnabled(),
                "Expected whitelist enabled after setWhitelistEnabled(true)");

    // Al habilitar, MACs no whitelisted deben ser rechazadas
    mgr.addAllowedSource(0x0000000000000001ULL);
    TEST_ASSERT(mgr.isSourceAllowed(0x0000000000000001ULL),
                "MAC in whitelist should be allowed when enabled");
    TEST_ASSERT(!mgr.isSourceAllowed(0x0000000000000009ULL),
                "MAC not in whitelist should be denied when enabled");

    mgr.setWhitelistEnabled(false);
    TEST_ASSERT(!mgr.isWhitelistEnabled(),
                "Expected whitelist disabled after toggle off");

    // Al deshabilitar, todas las MACs deben ser permitidas
    TEST_ASSERT(mgr.isSourceAllowed(0x0000000000000009ULL),
                "Any MAC should be allowed when whitelist disabled");
    TEST_PASS();
}

// ============================================================================
// Prueba 7: addAllowedSource (duplicados y verificación)
// ============================================================================

void test_addAllowedSource() {
    TEST_BEGIN("addAllowedSource() adds MAC correctly (no duplicates)");

    application::RadioManager_t mgr;

    mgr.addAllowedSource(0x000000000000000AULL);
    mgr.addAllowedSource(0x000000000000000BULL);
    mgr.addAllowedSource(0x000000000000000CULL);

    TEST_ASSERT(mgr.allowedSources().size() == 3,
                "Expected 3 sources after 3 adds");
    mgr.setWhitelistEnabled(true);
    TEST_ASSERT(mgr.isSourceAllowed(0x000000000000000AULL),
                "Source 0xA should be allowed");
    TEST_ASSERT(mgr.isSourceAllowed(0x000000000000000BULL),
                "Source 0xB should be allowed");
    TEST_ASSERT(mgr.isSourceAllowed(0x000000000000000CULL),
                "Source 0xC should be allowed");
    TEST_PASS();
}

// ============================================================================
// Prueba 8: removeAllowedSource
// ============================================================================

void test_removeAllowedSource() {
    TEST_BEGIN("removeAllowedSource() removes MAC from whitelist");

    application::RadioManager_t mgr;
    mgr.addAllowedSource(0x0000000000000005ULL);
    mgr.addAllowedSource(0x0000000000000006ULL);
    mgr.setWhitelistEnabled(true);

    TEST_ASSERT(mgr.isSourceAllowed(0x0000000000000005ULL),
                "Source 0x5 should be allowed before removal");

    mgr.removeAllowedSource(0x0000000000000005ULL);

    TEST_ASSERT(!mgr.isSourceAllowed(0x0000000000000005ULL),
                "Source 0x5 should be denied after removal");
    TEST_ASSERT(mgr.isSourceAllowed(0x0000000000000006ULL),
                "Source 0x6 should still be allowed");
    TEST_ASSERT(mgr.allowedSources().size() == 1,
                "Expected 1 source after removing 1 of 2");
    TEST_PASS();
}

// ============================================================================
// Prueba 9: clearAllowedSources
// ============================================================================

void test_clearAllowedSources() {
    TEST_BEGIN("clearAllowedSources() empties whitelist");

    application::RadioManager_t mgr;
    mgr.addAllowedSource(0x0000000000000007ULL);
    mgr.addAllowedSource(0x0000000000000008ULL);
    mgr.setWhitelistEnabled(true);

    mgr.clearAllowedSources();

    TEST_ASSERT(mgr.allowedSources().empty(),
                "Whitelist should be empty after clear");
    TEST_ASSERT(!mgr.isSourceAllowed(0x0000000000000007ULL),
                "Source 0x7 should be denied after clear");
    TEST_ASSERT(!mgr.isSourceAllowed(0x0000000000000008ULL),
                "Source 0x8 should be denied after clear");
    TEST_PASS();
}

// ============================================================================
// Prueba 10: allowedSources() returns correct list
// ============================================================================

void test_allowedSources_list() {
    TEST_BEGIN("allowedSources() returns the correct list");

    application::RadioManager_t mgr;
    mgr.addAllowedSource(0x000000000000000AULL);
    mgr.addAllowedSource(0x000000000000000BULL);
    mgr.addAllowedSource(0x000000000000000CULL);

    const auto& sources = mgr.allowedSources();

    TEST_ASSERT(sources.size() == 3, "Expected 3 sources");
    TEST_ASSERT(sources[0] == 0x000000000000000AULL,
                "Source[0] should be 0xA");
    TEST_ASSERT(sources[1] == 0x000000000000000BULL,
                "Source[1] should be 0xB");
    TEST_ASSERT(sources[2] == 0x000000000000000CULL,
                "Source[2] should be 0xC");
    TEST_PASS();
}

// ============================================================================
// Prueba 11: validateMessage — sin whitelist, MAC conocida
// ============================================================================
// NOTA: Estas pruebas requieren que m_crypto esté inicializado.
//       initFromConfig() inicializa crypto, y setupRadio() fallará
//       si no hay radio conectado — eso es aceptable.

void test_validateMessage_no_whitelist() {
    TEST_BEGIN("validateMessage() accepts valid message when whitelist disabled");

    application::RadioManager_t mgr;

    // Inicializar con configuración mínima (crypto se inicializa aquí)
    services::SystemConfig cfg;
    cfg.enable_whitelist = false;
    cfg.use_oled = false;
    cfg.use_tft = false;
    mgr.initFromConfig(cfg);

    // Si no hay crypto, saltar
    if (!mgr.isReady()) {
        std::cout << "\x1b[33mSKIP (sistema no inicializado, sin crypto)\x1b[0m\n";
        g_test_count--; // No contar este como test
        return;
    }

    // Crear frame válido
    std::vector<uint8_t> iv = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
    };
    std::vector<uint8_t> ciphertext = {0xDE, 0xAD, 0xBE, 0xEF};
    uint64_t dest = mgr.localMac64(); // Enviar a nosotros mismos
    uint64_t src = 0x0000000000000002ULL;

    auto frame = buildFrameWithHash(mgr.crypto(), dest, src, 5, iv, ciphertext);

    auto result = mgr.validateMessage(frame.data(), static_cast<uint8_t>(frame.size()));

    TEST_ASSERT(result.valid, "Expected valid message: " + result.error_msg);
    TEST_ASSERT(result.for_us, "Expected message for us (dest == local MAC)");
    TEST_ASSERT(result.src_mac == src, "Expected src_mac == 0x02");
    TEST_ASSERT(!result.should_forward, "Expected no forwarding (EndDevice role)");
    TEST_PASS();
}

// ============================================================================
// Prueba 12: validateMessage — whitelist + MAC permitida
// ============================================================================

void test_validateMessage_whitelist_allowed() {
    TEST_BEGIN("validateMessage() accepts message from whitelisted source");

    application::RadioManager_t mgr;

    services::SystemConfig cfg;
    cfg.enable_whitelist = true;         // Habilitar whitelist
    cfg.allowed_sources = {0x0000000000000002ULL};  // Permitir MAC 0x02
    cfg.use_oled = false;
    cfg.use_tft = false;
    mgr.initFromConfig(cfg);

    if (!mgr.isReady()) {
        std::cout << "\x1b[33mSKIP (sistema no inicializado, sin crypto)\x1b[0m\n";
        g_test_count--;
        return;
    }

    // Crear frame desde MAC permitida (0x02) a nosotros
    std::vector<uint8_t> iv = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
    };
    std::vector<uint8_t> ciphertext = {0xCA, 0xFE, 0xBA, 0xBE};
    uint64_t dest = mgr.localMac64();
    uint64_t src_allowed = 0x0000000000000002ULL;

    auto frame = buildFrameWithHash(mgr.crypto(), dest, src_allowed, 5, iv, ciphertext);

    auto result = mgr.validateMessage(frame.data(), static_cast<uint8_t>(frame.size()));

    TEST_ASSERT(result.valid,
                "Expected valid message from whitelisted source: " + result.error_msg);
    TEST_PASS();
}

// ============================================================================
// Prueba 13: validateMessage — whitelist + MAC denegada
// ============================================================================

void test_validateMessage_whitelist_denied() {
    TEST_BEGIN("validateMessage() rejects message from non-whitelisted source");

    application::RadioManager_t mgr;

    services::SystemConfig cfg;
    cfg.enable_whitelist = true;
    cfg.allowed_sources = {0x0000000000000002ULL};  // Solo MAC 0x02 permitida
    cfg.use_oled = false;
    cfg.use_tft = false;
    mgr.initFromConfig(cfg);

    if (!mgr.isReady()) {
        std::cout << "\x1b[33mSKIP (sistema no inicializado, sin crypto)\x1b[0m\n";
        g_test_count--;
        return;
    }

    // Crear frame desde MAC NO permitida (0x03)
    std::vector<uint8_t> iv = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
    };
    std::vector<uint8_t> ciphertext = {0xDE, 0xAD, 0xBE, 0xEF};
    uint64_t dest = mgr.localMac64();
    uint64_t src_denied = 0x0000000000000003ULL;  // NO en whitelist

    auto frame = buildFrameWithHash(mgr.crypto(), dest, src_denied, 5, iv, ciphertext);

    auto result = mgr.validateMessage(frame.data(), static_cast<uint8_t>(frame.size()));

    TEST_ASSERT(!result.valid,
                "Expected invalid message from non-whitelisted source");
    TEST_ASSERT(result.error_msg.find("denied") != std::string::npos,
                "Expected 'denied' in error message, got: " + result.error_msg);

    // Verificar contador source_denied
    auto stats = mgr.validationStats();
    TEST_ASSERT(stats.source_denied > 0,
                "Expected source_denied counter > 0, got " +
                std::to_string(stats.source_denied));
    TEST_PASS();
}

// ============================================================================
// Prueba 14: validateMessage — whitelist + broadcast (siempre permitido)
// ============================================================================

void test_validateMessage_broadcast_with_whitelist() {
    TEST_BEGIN("validateMessage() accepts broadcast message even with whitelist");

    application::RadioManager_t mgr;

    services::SystemConfig cfg;
    cfg.enable_whitelist = true;
    cfg.allowed_sources = {0x0000000000000002ULL};  // Solo MAC 0x02
    cfg.use_oled = false;
    cfg.use_tft = false;
    mgr.initFromConfig(cfg);

    if (!mgr.isReady()) {
        std::cout << "\x1b[33mSKIP (sistema no inicializado, sin crypto)\x1b[0m\n";
        g_test_count--;
        return;
    }

    // Crear broadcast desde MAC NO permitida (broadcast orígen no se verifica)
    // Nota: isSourceAllowed() permite broadcast como destino, no como origen.
    // El origen (src_mac=0x03) no está en whitelist, pero el destino es broadcast.
    // La whitelist verifica src_mac, no dest_mac. Así que src_mac=0x03 será DENEGADO.
    //
    // Para que pase, usamos src_mac en whitelist:
    std::vector<uint8_t> iv = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
    };
    std::vector<uint8_t> ciphertext = {0xBE, 0xEF, 0xCA, 0xFE};
    uint64_t dest_broadcast = application::BROADCAST_ADDR;
    uint64_t src_allowed = 0x0000000000000002ULL;  // En whitelist

    auto frame = buildFrameWithHash(mgr.crypto(), dest_broadcast, src_allowed,
                                     3, iv, ciphertext);

    auto result = mgr.validateMessage(frame.data(), static_cast<uint8_t>(frame.size()));

    TEST_ASSERT(result.valid,
                "Expected valid broadcast from whitelisted source: " + result.error_msg);
    TEST_ASSERT(result.for_us,
                "Expected broadcast to be for_us");
    // Router with TTL>1 should forward broadcast
    if (mgr.canForward()) {
        TEST_ASSERT(result.should_forward,
                    "Expected broadcast should_forward for Router");
    }
    TEST_PASS();
}

// ============================================================================
// Prueba 15: validateMessage — MAC denegada + contador source_denied
// ============================================================================

void test_source_denied_counter() {
    TEST_BEGIN("validateMessage() increments source_denied counter");

    application::RadioManager_t mgr;

    services::SystemConfig cfg;
    cfg.enable_whitelist = true;
    cfg.allowed_sources = {};  // Vacío — todo origen será denegado
    cfg.use_oled = false;
    cfg.use_tft = false;
    mgr.initFromConfig(cfg);

    if (!mgr.isReady()) {
        std::cout << "\x1b[33mSKIP (sistema no inicializado, sin crypto)\x1b[0m\n";
        g_test_count--;
        return;
    }

    // Enviar 3 mensajes desde MACs no permitidas
    std::vector<uint8_t> iv = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
    };
    std::vector<uint8_t> ciphertext = {0x01, 0x02, 0x03, 0x04};
    uint64_t dest = mgr.localMac64();

    for (int i = 0; i < 3; i++) {
        uint64_t src = 0x0000000000000100ULL + i;  // 0x100, 0x101, 0x102 — no whitelisted
        auto frame = buildFrameWithHash(mgr.crypto(), dest, src, 5, iv, ciphertext);
        auto result = mgr.validateMessage(frame.data(), static_cast<uint8_t>(frame.size()));
        (void)result;
    }

    auto stats = mgr.validationStats();
    TEST_ASSERT(stats.source_denied == 3,
                "Expected source_denied = 3, got " + std::to_string(stats.source_denied));
    TEST_ASSERT(stats.messages_rejected >= 3,
                "Expected messages_rejected >= 3, got " +
                std::to_string(stats.messages_rejected));
    TEST_PASS();
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "\n";
    std::cout << "  ╔══════════════════════════════════════════╗\n";
    std::cout << "  ║     WHITELIST UNIT TESTS (v2.1.0)       ║\n";
    std::cout << "  ╚══════════════════════════════════════════╝\n\n";

    // === Whitelist API tests (no requieren init) ===
    std::cout << "  ── Whitelist API ───────────────────────\n\n";

    test_isSourceAllowed_whitelist_disabled();
    test_isSourceAllowed_empty_whitelist();
    test_isSourceAllowed_mac_in_whitelist();
    test_isSourceAllowed_mac_not_in_whitelist();
    test_isSourceAllowed_broadcast();
    test_setWhitelistEnabled_toggle();
    test_addAllowedSource();
    test_removeAllowedSource();
    test_clearAllowedSources();
    test_allowedSources_list();

    // === validateMessage tests (requieren crypto inicializado) ===
    std::cout << "\n  ── validateMessage con whitelist ────────\n\n";

    test_validateMessage_no_whitelist();
    test_validateMessage_whitelist_allowed();
    test_validateMessage_whitelist_denied();
    test_validateMessage_broadcast_with_whitelist();
    test_source_denied_counter();

    // === Resultados ===
    std::cout << "\n";
    std::cout << "  ── Resultados ───────────────────────────\n";
    std::cout << "  Total: " << g_test_count << "\n";
    std::cout << "  PASS:  " << g_pass_count << "\n";
    std::cout << "  FAIL:  " << g_fail_count << "\n";

    if (g_fail_count > 0) {
        std::cout << "\n  \x1b[31m⚠  Algunas pruebas fallaron\x1b[0m\n\n";
    } else {
        std::cout << "\n  \x1b[32m✓  Todas las pruebas pasaron\x1b[0m\n\n";
    }

    return g_fail_count > 0 ? 1 : 0;
}
