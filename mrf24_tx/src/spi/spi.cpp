//codigo spi.cpp

#include <spi/spi.hpp>
#include <config/config.hpp>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef SPI_BCM2835
#include <bcm2835.h>
#else
  #include <linux/ioctl.h>
  #include <linux/types.h>
  #include <linux/spi/spidev.h>
#endif

#ifdef SPI_BCM2835
namespace SPI {

Spi_t::Spi_t()
    : m_spi_speed(SPI_SPEED) {
    init();
}

void Spi_t::settings_spi() {
    // Configura el SPI
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST); // Orden de bits
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);              // Modo SPI
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_256); // Velocidad SPI
    bcm2835_spi_chipSelect(BCM2835_SPI_CS0);                 // Selección de chip
    bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW); // Polaridad CS
}

void Spi_t::init() {
    if (!bcm2835_init()) {
        std::cerr << "Error al inicializar bcm2835" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    if (!bcm2835_spi_begin()) {
        fprintf(stderr, "No se pudo inicializar SPI\n");
        bcm2835_close();
        return ;
    }
    settings_spi();
}

  const uint8_t Spi_t::Transfer1bytes(const uint8_t cmd) {
      bcm2835_spi_transfer(cmd); // Transferencia de 1 byte
      return m_rx_buffer[0];     // Retornar el valor recibido
  }

  const uint8_t Spi_t::Transfer2bytes(const uint16_t cmd) {
    //uint8_t buffer[2] = { static_cast<uint8_t>(cmd >> 8), static_cast<uint8_t>(cmd & 0xFF) ,0xff};
      uint8_t buffer[2] = { static_cast<uint8_t>(cmd & 0xFF) , static_cast<uint8_t>(cmd >> 8)};
      bcm2835_spi_transfern(reinterpret_cast<char *>(buffer), 2); // Transferencia de 2 bytes
      return buffer[1];
  }
  

  const uint8_t Spi_t::Transfer3bytes(const uint32_t cmd) {
      //uint8_t buffer[3] = { static_cast<uint8_t>(cmd >> 16), static_cast<uint8_t>((cmd >> 8) & 0xFF), static_cast<uint8_t>(cmd & 0xFF) };
      uint8_t buffer[3] = { static_cast<uint8_t>(cmd & 0xFF)  , static_cast<uint8_t>((cmd >> 8) & 0xFF) , static_cast<uint8_t>((cmd >> 16)& 0xFF)};
      bcm2835_spi_transfern(reinterpret_cast<char *>(buffer), 3); // Transferencia de 3 bytes
      return buffer[2];
  }

  void Spi_t::spi_close() {
      bcm2835_spi_end();
      bcm2835_close();
  }

  Spi_t::~Spi_t() {
      spi_close();
  }

} // namespace SPI

#else
#include <cstring>

#define SPI_DEVICE  "/dev/spidev0.0"

#define CMD_WRITE 0x2
#define CMD_READ 0x3

#define READ 0b011 //Read data from memory array beginning at selected address
#define WRITE 0b010 //Write data to memory array beginning at selected address
#define WRDI 0b100//Reset the write enable latch (disable write operations)
#define WREN 0b110//Set the write enable latch (enable write operations)
#define RDSR 0b101//Read STATUS
#define WRSR 0b001//Write STATUS register

namespace SPI {

  void Spi_t::settings_spi(){
      spi->tx_buf = (unsigned long)m_tx_buffer;
      spi->rx_buf = (unsigned long)m_rx_buffer;
      spi->bits_per_word = 0;
      spi->speed_hz = m_spi_speed;
      spi->delay_usecs = 1;
      spi->len = 3;        

          std::memset(m_tx_buffer,0x00,sizeof(m_tx_buffer));
          std::memset(m_rx_buffer,0xff,sizeof(m_rx_buffer));
    return;
  }

  void Spi_t::init(){
  m_fs = open(SPI_DEVICE, O_RDWR);
  if(m_fs < 0) {
      msj_fail();
      exit(EXIT_FAILURE);
    }
    m_ret = ioctl(m_fs, SPI_IOC_RD_MODE, &scratch32);
  if(m_ret != 0) {
        msj_fail();
        if(m_fs)close(m_fs);
        exit(EXIT_FAILURE);
    }
    scratch32 |= SPI_MODE_0;
    m_ret = ioctl(m_fs, SPI_IOC_WR_MODE, &scratch32);   //SPI_IOC_WR_MODE32
  if(m_ret != 0) {
      msj_fail();
      close(m_fs);
      exit(EXIT_FAILURE);
    }
    m_ret = ioctl(m_fs, SPI_IOC_RD_MAX_SPEED_HZ, &scratch32);
  if(m_ret != 0) {
      close(m_fs);
      exit(EXIT_FAILURE);
    }
      scratch32 = m_spi_speed;
      m_ret = ioctl(m_fs, SPI_IOC_WR_MAX_SPEED_HZ, &scratch32);

      if(m_ret != 0) {
          msj_fail();
          close(m_fs);
          exit(EXIT_FAILURE);
      }
      return;
  }

  const uint32_t Spi_t::get_spi_speed(){
    return m_spi_speed;
  }

  const uint8_t Spi_t::Transfer1bytes(const uint8_t cmd){
      if (m_fs < 0) {
        std::cerr << "SPI device not open." << std::endl;
        return -1;
      }
        std::memset(m_rx_buffer, 0xff,sizeof(m_tx_buffer));
        std::memset(m_tx_buffer, 0xff,sizeof(m_rx_buffer));
        std::memset(spi.get(), 0, sizeof(struct spi_ioc_transfer));  // Limpiar la estructura a la que apunta spi
        spi->len = 1;
        m_tx_buffer[0] = cmd;
        spi->tx_buf = reinterpret_cast <unsigned long> (m_tx_buffer);
        spi->rx_buf = reinterpret_cast <unsigned long> (m_rx_buffer);
        spi->speed_hz = get_spi_speed();
        spi->bits_per_word = 8;
        spi->cs_change = 0;
        spi->delay_usecs = 0;

        int ret = ioctl(m_fs, SPI_IOC_MESSAGE(1), spi.get());
        if (ret < 0) {
            std::cerr << "Error en Transfer1bytes: " << strerror(errno) << std::endl;
            return -1;
        }
        return 0;
    }//end Transfer1bytes

  const uint8_t Spi_t::Transfer2bytes(const uint16_t cmd){
      spi->len = sizeof(cmd);
      m_rx_buffer[0]=m_rx_buffer[1]=0xff;
      m_rx_buffer[2]=m_rx_buffer[3]=0x00;
      memcpy(m_tx_buffer, &cmd, sizeof(cmd));
      m_ret = ioctl(m_fs, SPI_IOC_MESSAGE(1), spi.get());
      if((cmd>>8&0xff)==0x00)
          printDBGSpi(); 
        //if(ret != 0) return rx_buffer[1];  
    return m_rx_buffer[1];
    }

  const uint8_t Spi_t::Transfer3bytes(const uint32_t cmd){
    spi->len = 3;
    m_rx_buffer[0]=m_rx_buffer[1]=m_rx_buffer[2]==0xff;
    m_rx_buffer[3]=0x00;
    memcpy(m_tx_buffer, &cmd, sizeof(cmd));
    m_ret = ioctl(m_fs, SPI_IOC_MESSAGE(1), spi.get());
      if((cmd>>16&0xff)==0x00) 
        printDBGSpi();
        //if(ret != 0) return rx_buffer[2];       
    return m_rx_buffer[2];
    }

  void Spi_t::spi_close(){
          if(m_fs)close(m_fs);
        return;
      }

  Spi_t::Spi_t()
      :      
        m_tx_buffer { 0x00 },
        m_rx_buffer { 0x00 },
        m_spi_speed { SPI_SPEED }, 
        spi       { std::make_unique<struct spi_ioc_transfer >() } 
      {
            #ifdef DBG_SPI
                std::cout<<"Spi()\n";
            #endif
            settings_spi();   
            init(); 
          return;
      }

  Spi_t::~Spi_t(){
        spi_close();
        #ifdef DBG_SPI
            std::cout<<"~Spi()\n";
        #endif
        exit(EXIT_SUCCESS);
      }

}//end namespace SPI_H
#endif