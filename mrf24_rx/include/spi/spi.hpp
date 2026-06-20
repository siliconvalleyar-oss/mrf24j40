//codigo spi.hpp

#pragma once
#include <cstdint>
#include <memory>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <vector>

#define SPI_SPEED   100000 
#define MAX_BUFFER 256
namespace SPI{
  struct  Spi_t{

    explicit Spi_t();
      ~Spi_t();

    void init();
    void settings_spi();
    void spi_close();
    const uint8_t Transfer1bytes(const uint8_t cmd);
    const uint8_t Transfer2bytes(const uint16_t address);
    const uint8_t Transfer3bytes(const uint32_t address);
    void printDBGSpi();
    void msj_fail();  
    const uint32_t get_spi_speed();

  private:

    uint8_t m_tx_buffer[MAX_BUFFER]{0x00};
    uint8_t m_rx_buffer[MAX_BUFFER]{0x00};

    const uint32_t m_len_data { 32 };
    const uint32_t m_spi_speed { 0 };
    
    int m_fs{0};
    int m_ret{0};
    uint8_t looper{0};
    uint32_t scratch32{0};
    std::unique_ptr<struct spi_ioc_transfer >spi{nullptr};
  };

}//END namespace SPI_H
