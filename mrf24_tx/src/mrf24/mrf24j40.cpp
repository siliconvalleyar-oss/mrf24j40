#include <mrf24/mrf24j40_cmd.hpp>
#include <mrf24/mrf24j40_settings.hpp>
#include <mrf24/mrf24j40.hpp>
#include <tyme/tyme.hpp>
#include <config/config.hpp>
#include <work/data_analisis.hpp>
#include <spi/spi.hpp>


namespace MRF24J40{
            // aMaxPHYPacketSize = 127, from the 802.15.4-2006 standard.
    uint8_t rx_buf[A_MAX_PHY_PACKET_SIZE];// no es necesario declarar el static , ya que es static global 

    size_t ignoreBytes { 0 };   // bytes to ignore, some modules behaviour.
    bool bufPHY { false };      // flag to buffer all bytes in PHY Payload, or not
    rx_info_t rx_info{};
    tx_info_t tx_info{};
    RXMCR rxmcr{0x00};


    Mrf24j::Mrf24j()
    : prt_spi {std::make_unique<SPI::Spi_t>()} , m_bytes_nodata { m_bytes_MHR + m_bytes_FCS}
    {
        #ifdef DBG
            std::cout <<"Mrf24j( )\r\n";
        #endif
    }

    const uint8_t 
    Mrf24j::read_short(const uint8_t address) {
            // 0 top for short addressing, 0 bottom for read
        const uint8_t tmp = (address<<1 & 0b01111110);
        const uint8_t ret = prt_spi->Transfer2bytes(tmp); // envia 16 , los mas significativos en 0x00 , los menos significativos envia el comando
        return ret;
    }

    void 
    Mrf24j::write_short(const uint8_t address,const uint8_t data) {
    // 0 for top short address, 1 bottom for write
    const uint16_t lsb_tmp = ( (address<<1 & 0b01111110) | 0x01 ) | (data<<8);
        prt_spi->Transfer2bytes(lsb_tmp);
        return;
    }

    const uint8_t 
    Mrf24j::read_long(const uint16_t address) {
        const uint8_t lsb_address = (address >> 3 )& 0x7F;//0x7f
        const uint8_t msb_address = (address << 5) & 0xE0;//0xe0

        const uint32_t cmd = ( (0x80 | lsb_address) | (msb_address <<8) ) &  0x0000ffff;
       return prt_spi->Transfer3bytes(cmd);
    }

    void 
    Mrf24j::write_long(const uint16_t address,const uint8_t data) {
        const uint8_t lsb_address = (address >> 3) & 0x7F;
        const uint8_t msb_address = (address << 5) & 0xE0;
        const uint32_t cmd = ( (0x80 | lsb_address) | ( (msb_address | 0x10) << 8 ) | (data<<16) ) & 0xffffff;
        prt_spi->Transfer3bytes(cmd);
    }

    const uint16_t 
    Mrf24j::get_pan(void) {
        const uint8_t panh = read_short(MRF_PANIDH);
        return (panh << 8 | read_short(MRF_PANIDL));
    }

    void 
    Mrf24j::set_pan(const uint16_t panid) {
        write_short(MRF_PANIDH, (panid >> 8)& 0xff);
        write_short(MRF_PANIDL, panid & 0xff);
    }

    void 
    Mrf24j::address16_write(const uint16_t address) {
        write_short(MRF_SADRH, (address >> 8)& 0xff);
        write_short(MRF_SADRL, address & 0xff);
    }

    void 
    Mrf24j::address64_write(const uint64_t address){
        write_short(MRF_EADR7,(address>>56)&0xff);
        write_short(MRF_EADR6,(address>>48)&0xff);
        write_short(MRF_EADR5,(address>>40)&0xff);
        write_short(MRF_EADR4,(address>>32)&0xff);
        write_short(MRF_EADR3,(address>>24)&0xff);
        write_short(MRF_EADR2,(address>>16)&0xff);
        write_short(MRF_EADR1,(address>>8 )&0xff);
        write_short(MRF_EADR0,(address)&0xff);
    }

    const  uint16_t 
    Mrf24j::address16_read(void) {
        const uint8_t a16h = read_short(MRF_SADRH);
        return (a16h << 8 | read_short(MRF_SADRL));
    }

    //  lee la direccion mac de 64 bits
    const uint64_t 
    Mrf24j::address64_read(void){
        uint64_t address64 ;
        address64  = (read_short(MRF_EADR0));
        address64 |= (read_short(MRF_EADR1))<< 8;
        address64 |= static_cast<uint64_t>(read_short(MRF_EADR2))<<16;
        address64 |= static_cast<uint64_t>(read_short(MRF_EADR3))<<24;
        address64 |= static_cast<uint64_t>(read_short(MRF_EADR4))<<32;
        address64 |= static_cast<uint64_t>(read_short(MRF_EADR5))<<40;
        address64 |= static_cast<uint64_t>(read_short(MRF_EADR6))<<48;
        address64 |= static_cast<uint64_t>(read_short(MRF_EADR7))<<56;
    return  address64;
    }

    // 
    //  Simple send 16, with acks, not much of anything.. assumes src16 and local pan only.
    //  @param data
    //
    void 
    Mrf24j::set_interrupts(void) {
    //  interrupts for rx and tx normal complete
        write_short(MRF_INTCON, 0b11110110);
    }

    //
    //  use the 802.15.4 channel numbers..
    //
    void 
    Mrf24j::set_channel(const uint8_t channel) {
        write_long(MRF_RFCON0, (((channel - 11) << 4) | 0x03));
    }

    void 
    Mrf24j::init(void) {
    // //Seems a bit ridiculous when I use reset pin anyway
    // write_short(MRF_SOFTRST, 0x7); // from manual
    // while (read_short(MRF_SOFTRST) & 0x7 != 0) {
        // ; // wait for soft reset to finish
    // }
           delay(192); 
           #include <config/config.hpp>
           #ifdef RESET_MRF_SOFTWARE
            write_short(MRF_SOFTRST, 0x7);                          
           #endif
        write_short(MRF_PACON2, 0x98);  // – Initialize FIFOEN = 1 and TXONTS = 0x6.
        write_short(MRF_TXSTBL, 0x95);  // – Initialize RFSTBL = 0x9.

        write_long(MRF_RFCON0, 0x03);   // – Initialize RFOPT = 0x03.
        write_long(MRF_RFCON1, 0x01);   // – Initialize VCOOPT = 0x02.
        write_long(MRF_RFCON2, 0x80);   // – Enable PLL (PLLEN = 1).
        write_long(MRF_RFCON6, 0x90);   // – Initialize TXFIL = 1 and 20MRECVR = 1.
        write_long(MRF_RFCON7, 0x80);   // – Initialize SLPCLKSEL = 0x2 (100 kHz Internal oscillator).
        write_long(MRF_RFCON8, 0x10);   // – Initialize RFVCO = 1.
        write_long(MRF_SLPCON1,0x21);  // – Initialize CLKOUTEN = 1 and SLPCLKDIV = 0x01.

        //  Configuration for nonbeacon-enabled devices (see Section 3.8 “Beacon-Enabled and
        //  Nonbeacon-Enabled Networks”):
        write_short(MRF_BBREG2, 0x80);      // Set CCA mode to ED
        write_short(MRF_CCAEDTH, 0x60);     // – Set CCA ED threshold.
        write_short(MRF_BBREG6, 0x40);      // – Set appended RSSI value to RXFIFO.
        set_interrupts();
        set_channel(CHANNEL);                    //original es 12
        // max power is by default.. just leave it...
        // Set transmitter power - See “REGISTER 2-62: RF CONTROL 3 REGISTER (ADDRESS: 0x203)”.
        write_short(MRF_RFCTL, 0x04);       //  – Reset RF state machine.
        write_short(MRF_RFCTL, 0x00);       // part 2
        delay(192);                           // delay at least 192usec
    }


    //
    //  Call this from within an interrupt handler connected to the MRFs output
    //  interrupt pin.  It handles reading in any data from the module, and letting it
    //  continue working.
    //  Only the most recent data is ever kept.
    //                    
    void Mrf24j::interrupt_handler(void) {
        const uint8_t last_interrupt = read_short(MRF_INTSTAT);

        if (last_interrupt & MRF_I_RXIF) {
            m_flag_got_rx.fetch_add(1, std::memory_order_relaxed);

            // Desactivar interrupciones y deshabilitar RX temporalmente
            noInterrupts();
            rx_disable();

            // Leer longitud del frame desde el registro de inicio del RX FIFO
            const size_t frame_length = read_long(0x300);

    #ifdef USE_MAC_ADDRESS_LONG
            // Caso en el que `USE_MAC_ADDRESS_LONG` está definido
            if (bufPHY) {
                int rb_ptr = 0;
                for (size_t i = 0; i < frame_length; ++i) {
                    rx_buf[++rb_ptr] = read_long(0x301 + i);
                }
            }
    #else
            // Caso en el que `USE_MAC_ADDRESS_LONG` NO está definido
            if (MAX_PACKET_TX < frame_length) {
                if (bufPHY) {
                    int rb_ptr = 0;
                    for (int i = 0; i < frame_length; ++i) {
                        rx_buf[++rb_ptr] = read_long(0x301 + i);
                    }
                } else {
                    write_short(MRF_RXFLUSH, 0x01);
                }
                write_short(MRF_BBREG1, 0x00);
            } else {
                write_short(MRF_RXFLUSH, 0x01);
            }
    #endif

    #ifdef ENABLE_SECURITY
            write_short(WRITE_SECCR0, 0x80);
            write_short(WRITE_RXFLUSH, 0x01);
    #endif
            // Bufferizar datos específicos en rx_info
            int rd_ptr = 0;
            for (int i = 0; i < frame_length; ++i) {
                rx_info.rx_data[++rd_ptr] = read_long(0x301 + m_bytes_MHR + i);
            }

            // Completar información en `rx_info`
            rx_info.frame_length = frame_length;
            rx_info.lqi = read_long(0x301 + frame_length);
            rx_info.rssi = read_long(0x301 + frame_length + 1);

            // Rehabilitar RX e interrupciones
            rx_enable();
            interrupts();
        }

        if (last_interrupt & MRF_I_TXNIF) {
            m_flag_got_tx.fetch_add(1, std::memory_order_relaxed);
            const uint8_t tmp = read_short(MRF_TXSTAT);
            tx_info.tx_ok = !(tmp & ~(1 << TXNSTAT));
            tx_info.retries = tmp >> 6;
            tx_info.channel_busy = (tmp & (1 << CCAFAIL));
        }
    }

    //
    //  Call this function periodically, it will invoke your nominated handlers
    //
    const bool 
    Mrf24j::check_flags(void (*rx_handler)(), void (*tx_handler)()){
    // TODO - we could check whether the flags are > 1 here, indicating data was lost?
        if (m_flag_got_rx) {
            m_flag_got_rx = 0;
            #ifdef DBG
                std::cout<< "recibe algo \n";
            #endif
            rx_handler();
            return true;// si recibe algo , habilita 
        }
        if (m_flag_got_tx) {
            m_flag_got_tx = 0;
            #ifdef DBG_MRF
                std::cout<< "transmite algo \n";
            #endif
            tx_handler();
            return false;// si transmite el modulo , no habilita flag
        }
        return false;
    }

    //
    //  Set RX mode to promiscuous, or normal
    //
    void 
    Mrf24j::set_promiscuous(const bool enabled) {
        if (enabled) {
            write_short(MRF_RXMCR, 0x01);
        } else {
            write_short(MRF_RXMCR, 0x00);
        }
    }    

    //  configuracion MRF24J40MA
    void 
    Mrf24j::settings_mrf(void){
        #ifdef COORDINATOR
        rxmcr.PANCOORD=true;
        #else
        rxmcr.PANCOORD=true;
        #endif

        #ifdef ROUTER
        rxmcr.COORD=false;//original false
        #else 
        rxmcr.COORD=false;
        #endif

        #ifdef PROMISCUE
        rxmcr.PROMI=true;
        #else
        rxmcr.PROMI=true;
        #endif        
        
        #ifdef DBG_MRF
            printf("*reinterpret_cast : 0x%x\n",*reinterpret_cast<uint8_t*>(&rxmcr));
        #endif
        write_short(MRF_RXMCR, *reinterpret_cast<uint8_t*>(&rxmcr));
        return;
    }


    rx_info_t* 
    Mrf24j::get_rxinfo(void) {
        return &rx_info;
    }

    tx_info_t* 
    Mrf24j::get_txinfo(void) {
        return &tx_info;
    }

    uint8_t* 
    Mrf24j::get_rxbuf(void) {
        return rx_buf;
    }

    const int 
    Mrf24j::rx_datalength(void) {
        return rx_info.frame_length - m_bytes_nodata;
    }

    void 
    Mrf24j::set_ignoreBytes(const int ib) {
        // some modules behaviour
        ignoreBytes = ib;
    }

    //
    //Set bufPHY flag to buffer all bytes in PHY Payload, or not
    //
    void 
    Mrf24j::set_bufferPHY(const bool bp) {
        bufPHY = bp;
    }

    bool 
    Mrf24j::get_bufferPHY(void) {
        return bufPHY;
    }

    //
    //Set PA/LNA external control
    //
    void 
    Mrf24j::set_palna(const bool enabled) {
        if (enabled) {
            write_long(MRF_TESTMODE, 0x07); // Enable PA/LNA on MRF24J40MB module.
        }else{
            //write_long(MRF_TESTMODE, 0x00); // Disable PA/LNA on MRF24J40MB module.//original
            write_long(MRF_TESTMODE, 0x08); // Disable PA/LNA on MRF24J40MB module.
        }
    }

    void 
    Mrf24j::rx_flush(void) {
        write_short(MRF_RXFLUSH, 0x01);
    }

    void 
    Mrf24j::rx_disable(void) {
        write_short(MRF_BBREG1, 0x04);  // RXDECINV - disable receiver
    }

    void 
    Mrf24j::rx_enable(void) {
        write_short(MRF_BBREG1, 0x00);  // RXDECINV - enable receiver
    }

    void 
    Mrf24j::pinMode(const int i,const bool b){
    }

    void 
    Mrf24j::digitalWrite(const int i,const bool b){
    }

    void 
    Mrf24j::delay(const uint16_t t){
        TYME::Time_t time ;
        time.delay_ms(t);
    }

    void 
    Mrf24j::interrupts(){
        set_interrupts();//verificar 
    }

    void 
    Mrf24j::noInterrupts(){
    }

    Mrf24j::~Mrf24j( ){
        #ifdef DBG_MRF
            std::cout <<"~Mrf24j( )\r\n";
        #endif
    }

    void 
    Mrf24j::mode_turbo(){
        // Define TURBO_MODE if more bandwidth is required
        // to enable radio to operate to TX/RX maximum 
        // 625Kbps
        #ifdef TURBO_MODE        
            write_short(WRITE_BBREG0, 0x01);
            write_short(WRITE_BBREG3, 0x38);
            write_short(WRITE_BBREG4, 0x5C);            
            write_short(WRITE_RFCTL,0x04);
            write_short(WRITE_RFCTL,0x00);    
        #endif          
    }

    void 
    Mrf24j::set_macaddress( int& i , const uint64_t mac_adress ){                
            write_long(i++, mac_adress  & 0xff ); // uint64_t
            write_long(i++, (mac_adress >> 8  ) & 0xff); 
        if(sizeof(mac_adress)>2)
        {
            #ifdef DBG_MRF
                std::cout <<"es un mac de 64 bytes\n";
            #endif
            write_long(i++, (mac_adress >> 16 ) & 0xff); 
            write_long(i++, (mac_adress >> 24 ) & 0xff); 
            write_long(i++, (mac_adress >> 32 ) & 0xff); 
            write_long(i++, (mac_adress >> 40 ) & 0xff); 
            write_long(i++, (mac_adress >> 48 ) & 0xff); 
            write_long(i++, (mac_adress >> 56 ) & 0xff); 
                    
        }
            
            
    }

    void
    Mrf24j::flush_rx_fifo(void){
      write_short(MRF_RXFLUSH, read_short(MRF_RXFLUSH) | 0b00000001);
    }

    //static void
    void
    Mrf24j::reset_rf_state_machine(void){
    //
    //Reset RF state machine
    //
      const uint8_t rfctl = read_short(MRF_RFCTL);

      write_short(MRF_RFCTL, rfctl | 0b00000100);
      write_short(MRF_RFCTL, rfctl & 0b11111011);    
      //TYME::delay_us(2500);
    }

    
    void 
    Mrf24j::mrf24j40_get_extended_mac_addr(uint64_t *address){
        uint8_t* addr_ptr = reinterpret_cast<uint8_t*>(address);
        addr_ptr[7] = read_short(MRF_EADR7);
        addr_ptr[6] = read_short(MRF_EADR6);
        addr_ptr[5] = read_short(MRF_EADR5);
        addr_ptr[4] = read_short(MRF_EADR4);
        addr_ptr[3] = read_short(MRF_EADR3);
        addr_ptr[2] = read_short(MRF_EADR2);
        addr_ptr[1] = read_short(MRF_EADR1);
        addr_ptr[0] = read_short(MRF_EADR0);
    }

    void 
    Mrf24j::mrf24j40_get_short_mac_addr(uint16_t *address) {
        // Leer los valores de las direcciones corta desde los registros
        uint16_t low_addr = read_short(MRF_SADRL);
        uint16_t high_addr = read_short(MRF_SADRH);

        // Combinar los dos valores en un solo uint16_t
        *address = (high_addr << 8) | low_addr;  // Almacena la dirección combinada en 'address'
    }
    
    void
    Mrf24j::mrf24j40_set_tx_power(uint8_t& pwr){
      //write_long(MRF_RFCON3, pwr);
      pwr = read_long(MRF_RFCON3);
    }

}//END namespace MRF24
