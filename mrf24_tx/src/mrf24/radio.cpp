//
//      radio.cpp
//

#include <mrf24/radio.hpp>
#include <mrf24/mrf24j40.hpp>
#include <mrf24/mrf24j40_template.tpp>
//#include <qr/qr.hpp>
#include <file/file.hpp>
#include <display/color.hpp>
#include <work/rfflush.hpp>
#ifdef USE_OLED
    #include <oled/oled.hpp>
#endif
#include <string_view>
#include <zlib.h>  // Para usar crc32
#include <string>
#include <cstdint>
#include <cstddef>

namespace MRF24J40{ 
extern std::unique_ptr<Mrf24j> zigbee ;
extern DATA::PACKET_RX buffer_receiver;

    Radio_t::Radio_t() 
    #ifdef ENABLE_INTERRUPT_MRF24
    :   status          (true)
        #ifdef USE_FS
        ,   fs              { std::make_unique<FILESYSTEM::File_t>() }
        #endif
        #ifdef ENABLE_DATABASE
    ,   database        { std::make_unique<DATABASE::Database_t>() }
        #endif
        #else
    :   status          (false)
        #ifdef USE_QR 
    ,   qr              { std::make_unique<QR::Qr_t>() }
        #endif
    #endif
    ,   gpio            { std::make_unique<GPIO::Gpio_t>(status) }
    {
        
        #ifdef ENABLE_INTERRUPT_MRF24
        
        #else
            #ifdef USR_QR
                qr->create(QR_CODE_SRT);
            #endif
        #endif
            
        #ifdef DBG_RADIO
        std::cout << "Size msj : ( "<<std::dec<<sizeof(MSJ)<<" )\n";
        #endif
        zigbee = std::make_unique<Mrf24j>();        
        zigbee->init();
        
        zigbee->set_pan(PAN_ID);
        
        // This is _our_ address
        #ifdef MACADDR16
            zigbee->address16_write(ADDRESS); 
        #elif defined (MACADDR64)
            zigbee->address64_write(ADDRESS_LONG);
        #endif

        // uncomment if you want to receive any packet on this channel
        //zigbee->set_promiscuous(true);
        zigbee->settings_mrf();
    
        // uncomment if you want to enable PA/LNA external control
        zigbee->set_palna(true);    // Enable PA/LNA on MRF24J40MB module.
    
        // uncomment if you want to buffer all PHY Payload
        zigbee->set_bufferPHY(true);

        //zigbee->interrupt_handler();//verifica si llegaron datos , solo por saber si quedo algo pendiente

        //attachInterrupt(0, interrupt_routine, CHANGE); // interrupt 0 equivalent to pin 2(INT0) on ATmega8/168/328
        flag=true;                        

    }

    void 
    Radio_t::RunProccess(void){
        system("clear"); 
        #ifdef MRF24_RECEIVER_ENABLE
            while(true)
        #endif
        {           
            gpio->app(flag);        //la primera vez flag es true y ap retorna un false
            interrupt_routine();    //es zigbee->interrupt_handler();
            verif(flag);            //si modulo recibe algo , flag es alto       
        }
    }


    //verificacion del flag,si recibe algo 
    void 
    Radio_t::verif(bool& flag) {
        flag = zigbee->check_flags(&handle_rx, &handle_tx); //checkea el flag , si recibio algo es true
        const unsigned long current_time = 100000;//1000000 original
        if (current_time - last_time > tx_interval) {
            last_time = current_time;
        #ifdef MRF24_TRANSMITER_ENABLE
            #ifdef DBG_RADIO
                #ifdef MACADDR64
                    std::cout<<"send msj 64() ... \n";
                #else
                    std::cout<<"send msj 16() ... \n";
                #endif
            #endif
                #ifdef MACADDR64
            zigbee->send64(ADDRESS_LONG_SLAVE,getVectorZigbee());            
            //zigbee->send64(ADDRESS_LONG_SLAVE,bufferTransReceiver);            
                #elif defined(MACADDR16)
            zigbee->send(ADDR_SLAVE, getVectorZigbee());                                
                #endif
        #endif
        }
    }

    void 
    Radio_t::interrupt_routine() {
        zigbee->interrupt_handler(); // mrf24 object interrupt routine
    }

    void 
    update(std::string_view str_view){
        
        const int positionAdvance{15};
        auto            fs          { std::make_unique<FILESYSTEM::File_t> () };
        #ifdef USE_QR
        auto            qr_img      { std::make_unique<QR::Qr_img_t>() };
        //auto            qr_oled      { std::make_unique<QR::QrOled_t>() };
        #endif
        auto            monitor     { std::make_unique <FFLUSH::Fflush_t>()};
        #ifdef USE_OLED
            static auto     oled        { std::make_unique<OLED::Oled_t>() };    //inicializar una sola vez 
        #endif
        const auto*     packet_data = reinterpret_cast<const char*>(str_view.data());
        
        std::string  packetData (packet_data += positionAdvance);
        packetData.resize(38);

        SET_COLOR(SET_COLOR_GRAY_TEXT);
    
        #ifdef USE_OLED
            oled->create(packetData.c_str());  
        #endif
        #ifdef USE_QR
        auto qr = std::make_unique<QR::QrOled_t>();

        //De momento no hace nada
        std::string packet_data2 = "1234567890";    
        std::vector<int> infoQrTmp; 
        qr->create_qr(packet_data2, infoQrTmp);
        monitor->insert( std::to_string(infoQrTmp.size()));
        std::cout << " Size info of Qr Buffer : " << infoQrTmp.size() << std::endl;    
        #endif
                        
        fs->create(packet_data);
        std::cout<<"\n";
        #ifdef USE_QR
            qr_img->create(packet_data);
    #endif
    return ;    
    }


    Radio_t::~Radio_t() {
        #ifdef DBG_RADIO
            std::cout<<"~Radio_t()\n";
        #endif
    }
}



