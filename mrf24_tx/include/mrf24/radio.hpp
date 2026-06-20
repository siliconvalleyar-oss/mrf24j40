//radio.hpp

#pragma once
#include <mrf24/mrf24j40.hpp>
#include <gpio/gpio.hpp>
#include <file/database.hpp>
#include <config/config.hpp>
#include <work/work.hpp>
#include <work/data_analisis.hpp>
#include <qr/qr.hpp>

#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <vector>
#include <sstream>  // Para std::ostringstream
#include <iomanip>  // Para std::hex y std::setfill
#include <iostream>
#include <memory>


#ifdef USE_MAC_ADDRESS_LONG 
    #define MACADDR64
#elif defined (USE_MAC_ADDRESS_SHORT)
    #define MACADDR16
#endif

#ifdef USE_MRF24_RX
    #ifdef USE_OLED
    namespace OLED{
        struct Oled_t;
    }
    #endif
#endif

//#define POSITIOM_INIT_PRINTS 2

namespace MRF24J40{

   struct Radio_t
   {
        public:
            explicit            Radio_t();
                                ~Radio_t();
            void                verif(bool&);
            void                interrupt_routine();
            void                RunProccess(void);
            friend void         update();  
            //void                print_to_hex(uint64_t*);

        protected:            
            const std::vector<uint8_t> getVectorZigbee();

        private :
            unsigned long       last_time{0};
            unsigned long       tx_interval{1000}; 
            bool                status{false};
            bool                flag {false};            
        
        #ifdef ENABLE_INTERRUPT_MRF24 // rx
            std::unique_ptr<DATABASE::Database_t>   database{};
            //std::unique_ptr<WORK::Work_t>           fs{};                                    
        #else    
            std::unique_ptr<WORK::Work_t> qr{};
            struct DATA::packet_tx                  buffer_transmiter{};
        #endif                     
        std::unique_ptr<GPIO::Gpio_t> gpio{};                           
    };


    void update(std::string_view str_view);
    const uint8_t calculate_crc8(const uint8_t* data, size_t length);
            
    void handle_tx();
    void handle_rx();

}//end MRF24J40




