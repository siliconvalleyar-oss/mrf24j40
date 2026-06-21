/**
 * @file    services/crypto.hpp
 * @brief   Servicio de cifrado AES-256-CBC y hash SHA-256
 * @details Proporciona cifrado y descifrado de payloads usando
 *          AES-256 en modo CBC con IV aleatorio. Incluye
 *          hash SHA-256 para verificación de integridad.
 *          Utiliza OpenSSL como backend criptográfico.
 *
 * @author  MRF24J40 Team
 * @date    2026
 * @version 2.0.0
 */

#ifndef SERVICES_CRYPTO_HPP
#define SERVICES_CRYPTO_HPP

#include <cstdint>
#include <vector>
#include <array>
#include <string>
#include <memory>

namespace services {

/** @brief Longitud de clave AES-256 (32 bytes). */
constexpr size_t AES256_KEY_LEN = 32;

/** @brief Longitud del IV para AES-CBC (16 bytes). */
constexpr size_t AES_IV_LEN = 16;

/** @brief Longitud del hash SHA-256 (32 bytes). */
constexpr size_t SHA256_HASH_LEN = 32;

/**
 * @brief Resultado de una operación de cifrado/descifrado.
 */
struct CryptoResult {
    bool success = false;           ///< true si la operación fue exitosa
    std::string error_msg;          ///< Mensaje de error (si falló)
    std::vector<uint8_t> data;     ///< Datos resultantes
};

/**
 * @brief Servicio de cifrado con AES-256-CBC y SHA-256.
 *
 * Gestiona la clave de cifrado (derivada de una passphrase) y
 * proporciona operaciones de cifrado, descifrado y hash.
 *
 * Uso:
 * @code
 * auto crypto = services::Crypto_t("mi-passphrase-secreta");
 *
 * // Cifrar
 * auto encrypted = crypto.encrypt({0x48, 0x65, 0x6C, 0x6C, 0x6F});
 *
 * // Descifrar
 * auto decrypted = crypto.decrypt(encrypted.data);
 *
 * // Hash
 * auto hash = crypto.sha256({0x48, 0x65, 0x6C, 0x6C, 0x6F});
 * @endcode
 */
class Crypto_t {
public:
    /**
     * @brief Constructor: deriva la clave AES-256 desde una passphrase.
     * @param passphrase Frase secreta para derivar la clave.
     *
     * La derivación usa PBKDF2 (si OpenSSL lo soporta) o un
     * hash SHA-256 iterado de la passphrase.
     */
    explicit Crypto_t(std::string_view passphrase);

    ~Crypto_t();

    // No copiable
    Crypto_t(const Crypto_t&) = delete;
    Crypto_t& operator=(const Crypto_t&) = delete;

    /**
     * @brief Cifra datos con AES-256-CBC.
     *
     * Genera un IV aleatorio de 16 bytes, cifra los datos y
     * devuelve IV + ciphertext concatenados.
     *
     * @param plaintext Datos a cifrar.
     * @return IV (16 bytes) + ciphertext.
     */
    CryptoResult encrypt(const std::vector<uint8_t>& plaintext);

    /**
     * @brief Descifra datos con AES-256-CBC.
     *
     * Espera IV (16 bytes) + ciphertext como entrada.
     *
     * @param ciphertext_with_iv IV (16 bytes) + ciphertext.
     * @return Datos descifrados.
     */
    CryptoResult decrypt(const std::vector<uint8_t>& ciphertext_with_iv);

    /**
     * @brief Calcula hash SHA-256 de los datos.
     * @param data Datos a hashear.
     * @return Hash de 32 bytes.
     */
    std::array<uint8_t, SHA256_HASH_LEN> sha256(const std::vector<uint8_t>& data);

    /**
     * @brief Calcula hash SHA-256 de un string.
     * @param str String a hashear.
     * @return Hash de 32 bytes.
     */
    std::array<uint8_t, SHA256_HASH_LEN> sha256(std::string_view str);

    /**
     * @brief Verifica que un hash corresponda a unos datos.
     * @param data Datos originales.
     * @param hash Hash a verificar.
     * @return true si el hash coincide.
     */
    bool verify(const std::vector<uint8_t>& data,
                const std::array<uint8_t, SHA256_HASH_LEN>& hash);

    /**
     * @brief Convierte datos binarios a string hexadecimal.
     */
    static std::string toHex(const std::vector<uint8_t>& data);

    /**
     * @brief Convierte string hexadecimal a datos binarios.
     */
    static std::vector<uint8_t> fromHex(std::string_view hex);

    /**
     * @brief Cambia la passphrase y re-deriva la clave.
     * @param passphrase Nueva frase secreta.
     */
    void setPassphrase(std::string_view passphrase);

    /** @brief Obtiene la clave actual como string hexadecimal. */
    std::string keyHex() const;

private:
    std::array<uint8_t, AES256_KEY_LEN> m_key;  ///< Clave AES-256

    /**
     * @brief Deriva una clave AES-256 desde una passphrase.
     * @param passphrase Frase secreta.
     */
    void deriveKey(std::string_view passphrase);
};

} // namespace services

#endif // SERVICES_CRYPTO_HPP
