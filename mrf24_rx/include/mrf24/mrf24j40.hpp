#pragma once
    #include <config/config.hpp>
    #include <spi/spi.hpp>

    #include <iostream>
    #include <memory>
    #include <cstring>
    #include <atomic>
    
    
namespace DATA{
        struct packet_tx;
    }

namespace MRF24J40{

    typedef struct _rx_info_t{
        uint8_t         frame_length         ;
        //max data length = (124 aMaxPHYPacketSize - 2 Frame control - 1 sequence number - 2 panid - 2 shortAddr Destination - 2 shortAddr Source - 2 FCS)  //2+2+2+2+1+2=11
        //max data length = (124 aMaxPHYPacketSize - 2 Frame control - 1 sequence number - 2 panid - 8 longAddr Destination - 8 longAddr Source - 2 FCS)    //8+8+2+2+1+2=23
        uint8_t         rx_data         [124]; //original 116 o 127-3-23
        uint8_t         lqi                  ;
        uint8_t         rssi                 ;//3+116 
    } rx_info_t;

    //
    //Based on the TXSTAT register, but "better"
    //
    typedef struct _tx_info_t{
        uint8_t         tx_ok           :1;
        uint8_t         retries         :2;
        uint8_t         channel_busy    :1;
    } tx_info_t;

    struct Mrf24j 
    {
        public:
            Mrf24j( );
            ~Mrf24j( );

            void                    init                    (void);
            void                    mrf24j40_init           (void);
            void                    send    (const uint64_t , const std::vector<uint8_t>) ;
            void                    send64  (const uint64_t , const std::vector<uint8_t> );
            void                    send64  (const uint64_t , const DATA::packet_tx );
            
            void                    interrupt_handler       (void);
            void                    set_pan                 (const uint16_t);               //panid
            void                    address16_write     (const uint16_t);
            void                    address64_write     (const uint64_t);
            void                    settings_mrf        (void);


    protected:
            const uint8_t       read_short          (const uint8_t);                //address
            const uint8_t       read_long           (const uint16_t);               //address
            void                write_short         (const uint8_t ,const uint8_t); //address ,data
            void                write_long          (const uint16_t,const uint8_t); //address ,data
            const uint16_t      get_pan             (void);
            
                           //address16
            
            const uint16_t      address16_read      (void);
            const uint64_t      address64_read      (void);
            void                set_interrupts      (void);

            //void set_promiscuous(__OBJC_BOOL_IS_BOOL );
            void                set_promiscuous     (const bool );  
            
            //Set the channel, using 802.15.4 channel numbers (11..26)            
            void                set_channel         (const uint8_t);
            void                rx_enable           (void);
            void                rx_disable          (void);
                                    

            void                pinMode             (const int,const bool);
            void                digitalWrite        (const int,const bool);
            void                delay               (const uint16_t);
            void                interrupts          (void);
            void                noInterrupts        (void);
        
        public:        
            
            //If you want to throw away rx data 
            void                rx_flush(void);
            rx_info_t *         get_rxinfo(void) ;
            tx_info_t *         get_txinfo(void) ;
            uint8_t *           get_rxbuf(void) ;
            const int           rx_datalength(void);
            void                set_ignoreBytes(const int );
            
            //
            // Set bufPHY flag to buffer all bytes in PHY Payload, or not
            // 
            void                 set_bufferPHY(const bool);
            bool                 get_bufferPHY(void);

             

            //
            //Set PA/LNA external control
            //
            void                    set_palna(const bool);


            const bool              check_flags(void (*rx_handler)(), void (*tx_handler)());
    protected:        
            void                    mode_turbo();
            void                    set_macaddress(int&,const uint64_t);
            void                    reset_rf_state_machine(void);
            void                    flush_rx_fifo(void);
            void                    mrf24j40_set_tx_power(uint8_t&);

        public:    
            void                    mrf24j40_get_extended_mac_addr(uint64_t *);
            void                    mrf24j40_get_short_mac_addr(uint16_t *);
        private:
            std::unique_ptr<SPI::Spi_t> prt_spi {};

                // essential for obtaining the data frame only
                
                // bytes_MHR = 2 Frame control + 1 sequence number + 2 panid + 2 shortAddr Destination + 2 shortAddr Source
                //const int m_bytes_MHR {9};// 
                const size_t m_bytes_MHR {SIZE_HEAD_PACKET_DATA};//9 para direcciones de 2 bytes de recepcion y 2 de transmision 

                const size_t m_bytes_FCS {2}; // FCS length = 2
                const size_t m_bytes_nodata { 0 }; // no_data bytes in PHY payload,  header length + FCS

                // Cambia el tipo de m_flag_got_rx a std::atomic
                std::atomic<uint8_t> m_flag_got_rx{};
                std::atomic<uint8_t> m_flag_got_tx{};
        };

}

