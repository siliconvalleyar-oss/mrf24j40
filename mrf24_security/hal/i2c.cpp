/**
 * @file    hal/i2c.cpp
 * @brief   Implementación del driver I2C para Raspberry Pi
 *
 * @author  MRF24J40 Team
 * @date    2026
 */

#include <hal/i2c.hpp>
#include <stdexcept>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

namespace hal {

I2c_t::I2c_t(uint8_t slave_addr, const char* device)
    : m_fd(-1)
    , m_slave(slave_addr)
{
    m_fd = open(device, O_RDWR);
    if (m_fd < 0) {
        throw std::runtime_error(
            std::string("Cannot open I2C device: ") + device +
            " - " + strerror(errno));
    }

    if (ioctl(m_fd, I2C_SLAVE, slave_addr) < 0) {
        close(m_fd);
        m_fd = -1;
        throw std::runtime_error(
            "Cannot set I2C slave address 0x" +
            std::to_string(slave_addr));
    }
}

I2c_t::~I2c_t() {
    if (m_fd >= 0) {
        close(m_fd);
    }
}

int I2c_t::write(const std::vector<uint8_t>& data) {
    return write(data.data(), data.size());
}

int I2c_t::write(const uint8_t* buf, size_t len) {
    if (m_fd < 0 || !buf || len == 0) return -1;

    int ret = ::write(m_fd, buf, len);
    if (ret < 0) {
        return -1;
    }
    return ret;
}

int I2c_t::read(uint8_t* buf, size_t len) {
    if (m_fd < 0 || !buf || len == 0) return -1;

    int ret = ::read(m_fd, buf, len);
    if (ret < 0) {
        return -1;
    }
    return ret;
}

int I2c_t::writeCommand(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd};  // 0x00 = Co=0, D/C#=0 (comando)
    return write(buf, 2);
}

int I2c_t::writeData(uint8_t data) {
    uint8_t buf[2] = {0x40, data};  // 0x40 = Co=0, D/C#=1 (dato)
    return write(buf, 2);
}

} // namespace hal
