/**
 * @file    hal/i2c.hpp
 * @brief   Abstracción de I2C para Raspberry Pi
 * @details Proporciona comunicación I2C vía /dev/i2c-1 para conectar
 *          periféricos como el display OLED SSD1306.
 *
 * @author  MRF24J40 Team
 * @date    2026
 * @version 2.0.0
 */

#ifndef HAL_I2C_HPP
#define HAL_I2C_HPP

#include <cstdint>
#include <vector>
#include <memory>

namespace hal {

/**
 * @brief Driver de comunicación I2C.
 *
 * Gestiona la apertura del bus I2C y las transferencias con
 * esclavos. Soporta escritura de buffers y lectura de registros.
 *
 * Uso:
 * @code
 * auto i2c = hal::I2c_t(0x3C);  // Dirección del SSD1306
 * i2c.write({0x00, 0xAE});       // Enviar comando
 * i2c.write({0x40, 0xFF});       // Enviar datos
 * @endcode
 */
class I2c_t {
public:
    /**
     * @brief Constructor: abre el bus I2C y selecciona el esclavo.
     * @param slave_addr Dirección I2C del dispositivo esclavo (7 bits).
     * @param device     Ruta del bus I2C (default: /dev/i2c-1).
     * @throws std::runtime_error si no se puede abrir el bus.
     */
    explicit I2c_t(uint8_t slave_addr, const char* device = "/dev/i2c-1");

    /// Destructor: cierra el bus I2C.
    ~I2c_t();

    // No copiable
    I2c_t(const I2c_t&) = delete;
    I2c_t& operator=(const I2c_t&) = delete;

    /**
     * @brief Escribe datos en el dispositivo I2C.
     * @param data Buffer con los datos a enviar.
     * @return Número de bytes escritos, o -1 si error.
     */
    int write(const std::vector<uint8_t>& data);

    /**
     * @brief Escribe datos raw en el dispositivo I2C.
     * @param buf Puntero a los datos.
     * @param len Longitud en bytes.
     * @return Número de bytes escritos, o -1 si error.
     */
    int write(const uint8_t* buf, size_t len);

    /**
     * @brief Lee datos del dispositivo I2C.
     * @param buf  Buffer para almacenar los datos leídos.
     * @param len  Número de bytes a leer.
     * @return Número de bytes leídos, o -1 si error.
     */
    int read(uint8_t* buf, size_t len);

    /**
     * @brief Escribe un comando (byte con bit de comando = 0).
     * @param cmd Byte de comando.
     * @return 0 si éxito, -1 si error.
     */
    int writeCommand(uint8_t cmd);

    /**
     * @brief Escribe un dato (byte con bit de datos = 1).
     * @param data Byte de datos.
     * @return 0 si éxito, -1 si error.
     */
    int writeData(uint8_t data);

    /**
     * @brief Obtiene la dirección del esclavo.
     * @return Dirección I2C de 7 bits.
     */
    uint8_t slaveAddress() const { return m_slave; }

private:
    int     m_fd;    ///< File descriptor del bus I2C
    uint8_t m_slave; ///< Dirección del esclavo
};

} // namespace hal

#endif // HAL_I2C_HPP
