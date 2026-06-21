/**
 * @file    services/crypto.cpp
 * @brief   Implementación del servicio criptográfico con OpenSSL
 *
 * @author  MRF24J40 Team
 * @date    2026
 */

#include <services/crypto.hpp>
#include <cstring>
#include <sstream>
#include <iomanip>

// OpenSSL headers
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/err.h>

namespace services {

// ============================================================================
// Constructor / Destructor
// ============================================================================

Crypto_t::Crypto_t(std::string_view passphrase) {
    deriveKey(passphrase);
}

Crypto_t::~Crypto_t() {
    // Limpiar clave de la memoria
    m_key.fill(0);
}

// ============================================================================
// Derivación de clave
// ============================================================================

void Crypto_t::deriveKey(std::string_view passphrase) {
    // Usar SHA-256 iterativo para derivar clave de la passphrase
    // En producción, usar PKCS5_PBKDF2_HMAC_SHA1 para mayor seguridad

    // Inicializar con SHA-256 de la passphrase
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, passphrase.data(), passphrase.size());
    SHA256_Final(m_key.data(), &ctx);

    // Iterar para fortalecer (1000 iteraciones)
    std::array<uint8_t, SHA256_HASH_LEN> temp;
    for (int i = 0; i < 1000; i++) {
        SHA256_Init(&ctx);
        SHA256_Update(&ctx, m_key.data(), m_key.size());
        SHA256_Final(temp.data(), &ctx);
        std::memcpy(m_key.data(), temp.data(), m_key.size());
    }
}

// ============================================================================
// Cifrado / Descifrado
// ============================================================================

CryptoResult Crypto_t::encrypt(const std::vector<uint8_t>& plaintext) {
    CryptoResult result;

    if (plaintext.empty()) {
        result.error_msg = "Plaintext is empty";
        return result;
    }

    // Generar IV aleatorio
    std::vector<uint8_t> iv(AES_IV_LEN);
    if (RAND_bytes(iv.data(), AES_IV_LEN) != 1) {
        result.error_msg = "Failed to generate random IV";
        return result;
    }

    // Crear contexto de cifrado
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        result.error_msg = "Failed to create cipher context";
        return result;
    }

    // Inicializar cifrado AES-256-CBC
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                           m_key.data(), iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        result.error_msg = ERR_error_string(ERR_get_error(), nullptr);
        return result;
    }

    // Cifrar
    std::vector<uint8_t> ciphertext(plaintext.size() + AES_IV_LEN);
    int out_len = 0;
    int tmp_len = 0;

    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &out_len,
                          plaintext.data(), plaintext.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        result.error_msg = "Encryption failed";
        return result;
    }

    // Finalizar (padding PKCS7)
    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + out_len, &tmp_len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        result.error_msg = "Encryption finalization failed";
        return result;
    }
    out_len += tmp_len;
    ciphertext.resize(out_len);
    EVP_CIPHER_CTX_free(ctx);

    // Resultado: IV + ciphertext
    result.data.reserve(AES_IV_LEN + ciphertext.size());
    result.data.insert(result.data.end(), iv.begin(), iv.end());
    result.data.insert(result.data.end(), ciphertext.begin(), ciphertext.end());
    result.success = true;

    return result;
}

CryptoResult Crypto_t::decrypt(const std::vector<uint8_t>& ciphertext_with_iv) {
    CryptoResult result;

    if (ciphertext_with_iv.size() <= AES_IV_LEN) {
        result.error_msg = "Data too short (missing IV)";
        return result;
    }

    // Extraer IV y ciphertext
    const uint8_t* iv = ciphertext_with_iv.data();
    const uint8_t* ciphertext = ciphertext_with_iv.data() + AES_IV_LEN;
    size_t ciphertext_len = ciphertext_with_iv.size() - AES_IV_LEN;

    // Crear contexto de descifrado
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        result.error_msg = "Failed to create cipher context";
        return result;
    }

    // Inicializar descifrado AES-256-CBC
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                           m_key.data(), iv) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        result.error_msg = ERR_error_string(ERR_get_error(), nullptr);
        return result;
    }

    // Descifrar
    std::vector<uint8_t> plaintext(ciphertext_len + AES_IV_LEN);
    int out_len = 0;
    int tmp_len = 0;

    if (EVP_DecryptUpdate(ctx, plaintext.data(), &out_len,
                          ciphertext, ciphertext_len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        result.error_msg = "Decryption failed";
        return result;
    }

    // Finalizar (remover padding PKCS7)
    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + out_len, &tmp_len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        result.error_msg = "Decryption finalization failed (wrong key?)";
        return result;
    }
    out_len += tmp_len;
    plaintext.resize(out_len);
    EVP_CIPHER_CTX_free(ctx);

    result.data = std::move(plaintext);
    result.success = true;
    return result;
}

// ============================================================================
// SHA-256
// ============================================================================

std::array<uint8_t, SHA256_HASH_LEN> Crypto_t::sha256(const std::vector<uint8_t>& data) {
    std::array<uint8_t, SHA256_HASH_LEN> hash{};
    SHA256(data.data(), data.size(), hash.data());
    return hash;
}

std::array<uint8_t, SHA256_HASH_LEN> Crypto_t::sha256(std::string_view str) {
    std::array<uint8_t, SHA256_HASH_LEN> hash{};
    SHA256(reinterpret_cast<const uint8_t*>(str.data()), str.size(), hash.data());
    return hash;
}

bool Crypto_t::verify(const std::vector<uint8_t>& data,
                       const std::array<uint8_t, SHA256_HASH_LEN>& hash) {
    auto computed = sha256(data);
    return computed == hash;
}

// ============================================================================
// Conversión hex
// ============================================================================

std::string Crypto_t::toHex(const std::vector<uint8_t>& data) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (auto b : data) {
        oss << std::setw(2) << static_cast<int>(b);
    }
    return oss.str();
}

std::vector<uint8_t> Crypto_t::fromHex(std::string_view hex) {
    std::vector<uint8_t> result;
    result.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        uint8_t byte = 0;
        for (int j = 0; j < 2; j++) {
            char c = hex[i + j];
            byte <<= 4;
            if (c >= '0' && c <= '9') byte |= (c - '0');
            else if (c >= 'a' && c <= 'f') byte |= (c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') byte |= (c - 'A' + 10);
        }
        result.push_back(byte);
    }
    return result;
}

// ============================================================================
// Gestión de clave
// ============================================================================

void Crypto_t::setPassphrase(std::string_view passphrase) {
    deriveKey(passphrase);
}

std::string Crypto_t::keyHex() const {
    return toHex(std::vector<uint8_t>(m_key.begin(), m_key.end()));
}

} // namespace services
