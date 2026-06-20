#include <mrf24/radio.hpp>
#include <mrf24/mrf24j40.hpp>
#include <mrf24/mrf24j40_template.tpp>
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
    
    std::unique_ptr<Mrf24j> zigbee = nullptr ;
    DATA::PACKET_RX buffer_receiver{};

extern uint8_t rx_buf[A_MAX_PHY_PACKET_SIZE];

    void 
    handle_tx() {
    #ifdef MRF24_TRANSMITER_ENABLE
    const auto status = zigbee->get_txinfo()->tx_ok;
        if (status) {
            std::cout<<"TX went ok, got ack \n";
        } else {
            std::cout<<"\nTX failed after \n";
            std::cout<<zigbee->get_txinfo()->retries;
            std::cout<<" retries\n";
        }
    #endif     
    }

#ifdef USE_MAC_ADDRESS_LONG

    void 
    handle_rx() {
        
        #ifdef MRF24_RECEIVER_ENABLE                
        auto  monitor{std::make_unique <FFLUSH::Fflush_t>()};

        std::ostringstream oss_zigbee{};        

        //detecto una interrupcion
        monitor->insert("received a packet ... ");

        // get_rxinfo()->frame_length devuelve un uint8_t
        const uint8_t frame_length = zigbee->get_rxinfo()->frame_length;

        // Usar std::ostringstream para construir el string en formato hexadecimal
        oss_zigbee << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(frame_length);

        // Mostrar el string resultante        
        monitor->insert(oss_zigbee.str() );

        oss_zigbee.str("");
        oss_zigbee.clear(); 
        //monitor->insert(" ");

        monitor->insert(" Packet data (PHY Payload) :" +  std::to_string( frame_length  ));
        monitor->insert(" ");
        if(zigbee->get_bufferPHY()){
            #ifdef DBG_PRINT_GET_INFO               

            for (uint8_t i = 0; i < frame_length; ++i)
                oss_zigbee << zigbee->get_rxbuf()[i];        

            monitor->insert(oss_zigbee.str());
            oss_zigbee.str("");   // Limpiar el contenido
            oss_zigbee.clear();   // Restablecer el estado

            #endif
        }            
            SET_COLOR(SET_COLOR_CYAN_TEXT);
            monitor->insert("ASCII data (relevant data) :");
            monitor->insert("data_length : " + std::to_string(zigbee->rx_datalength()) );        

        for (auto& byte : zigbee->get_rxinfo()->rx_data){
                if(byte!=0x00)oss_zigbee << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << ":";
            }
            monitor->insert("info_zigbee : " );
            monitor->insert( oss_zigbee.str());        
            oss_zigbee.str("");   // Limpiar el contenido
            oss_zigbee.clear();   // Restablecer el estado
            
        for (auto& byte : zigbee->get_rxinfo()->rx_data){                
                oss_zigbee << byte;
            }
 
            monitor->insert(" " );
            monitor->insert(" " );
            //monitor->insert("info_zigbee : " );
            //monitor->insert( oss_zigbee.str());        
            oss_zigbee.str("");   // Limpiar el contenido
            oss_zigbee.clear();   // Restablecer el estado
            
        #ifdef DBG_PRINT_GET_INFO                                     
        //std::memcpy (  &buffer_receiver , zigbee->get_rxbuf() , sizeof(rx_buf));                  
        std::memcpy (  &buffer_receiver , zigbee->get_rxbuf() , zigbee->rx_datalength() + BYTES_MHR); //haciendo ajustes recibe todos los paquetes                 

        const uint64_t mac_address_rx = (static_cast<uint64_t>(buffer_receiver.mac_msb_rx) << 32) | buffer_receiver.mac_lsb_rx;
        const uint64_t mac_address_tx = (static_cast<uint64_t>(buffer_receiver.mac_msb) << 32) | buffer_receiver.mac_lsb;
        monitor->insert (" " );            
        //  obtiene la direccion de mac seteada en el mrf24j40
        uint64_t mac_address_local;
        zigbee->mrf24j40_get_extended_mac_addr(&mac_address_local);
        //  compara la direccion de mac "slave" con la mac de "entrada"
        if(mac_address_local == mac_address_rx){
            monitor->insert ("mac recibida es aceptada" ); }
        else { //   muestra una direcion mac diferente a la configurada
            monitor->insert ("mac recibida no es aceptada" );}
            monitor->insert( "rx data_receiver->mac : 0x"         + hex_to_text( mac_address_rx )); 
            monitor->insert( "tx data_receiver->mac : 0x"         + hex_to_text( mac_address_tx )); 
            monitor->insert( "buffer_receiver->head : 0x"         + hex_to_text( buffer_receiver.head ));
            monitor->insert( "buffer_receiver->size : "         + std::to_string( buffer_receiver.size )); 
            monitor->insert( "buffer_receiver->panid : 0x"        + hex_to_text( buffer_receiver.panid ));
            monitor->insert( "buffer_receiver->checksum : 0x"     + hex_to_text( buffer_receiver.crc8 ));            

            std::string txt_tmp ;
            txt_tmp.assign(reinterpret_cast<const char*>(buffer_receiver.data), sizeof(buffer_receiver.data));
            monitor->insert( "data_receiver->data ( "   + std::to_string(sizeof(buffer_receiver.data))+ " ) : "      + txt_tmp );

            //  imprime mac adress local
            monitor->insert("get address mac : 0x"               + hex_to_text(mac_address_local));            
        #endif        
            RST_COLOR() ; 
            SET_COLOR(SET_COLOR_CYAN_TEXT);
            monitor->insert("LQI : " + std::to_string (zigbee->get_rxinfo()->lqi) );
            monitor->insert("RSSI : " + std::to_string(zigbee->get_rxinfo()->rssi) );                                                
            //std::string rx_b(rx_buf, rx_buf + sizeof(rx_buf));                       

        //imprime todo los datos obtenidos
        monitor->print_all();
        #endif
        RST_COLOR() ;           
        update(reinterpret_cast<const char*>(zigbee->get_rxinfo()->rx_data));    
    }//end rx

#else

 void 
    handle_rx() {
        
        #ifdef MRF24_RECEIVER_ENABLE                
        auto  monitor{std::make_unique <FFLUSH::Fflush_t>()};

        std::ostringstream oss_zigbee{};        

        //detecto una interrupcion
        monitor->insert("received a packet ... ");

        // get_rxinfo()->frame_length devuelve un uint8_t
        const uint8_t frame_length = zigbee->get_rxinfo()->frame_length;

        // Usar std::ostringstream para construir el string en formato hexadecimal
        oss_zigbee << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(frame_length);

        // Mostrar el string resultante        
        monitor->insert(oss_zigbee.str() );

        oss_zigbee.str("");
        oss_zigbee.clear(); 
        monitor->insert(" ");

        monitor->insert(" Packet data (PHY Payload) :" +  std::to_string( frame_length  ));
        monitor->insert(" ");
        if(zigbee->get_bufferPHY()){
            #ifdef DBG_PRINT_GET_INFO               

            for (uint8_t i = 0; i < frame_length; ++i){
                //oss_zigbee <<std::hex<< zigbee->get_rxbuf()[i];//imprime en hexadecimal
                oss_zigbee << zigbee->get_rxbuf()[i];
            }
            monitor->insert(oss_zigbee.str());
            oss_zigbee.str("");   // Limpiar el contenido
            oss_zigbee.clear();   // Restablecer el estado
            #endif
        }            
            SET_COLOR(SET_COLOR_CYAN_TEXT);
            monitor->insert("ASCII data (relevant data) :");
            monitor->insert("data_length : " + std::to_string(zigbee->rx_datalength()) );        
        for (auto& byte : zigbee->get_rxinfo()->rx_data){
                if(byte!=0x00)oss_zigbee << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << ":";
            }
            monitor->insert("info_zigbee : " );
            monitor->insert( oss_zigbee.str());        
            oss_zigbee.str("");   // Limpiar el contenido
            oss_zigbee.clear();   // Restablecer el estado            
        for (auto& byte : zigbee->get_rxinfo()->rx_data){                
                oss_zigbee << byte;
            }
            
            monitor->insert(" " );
            monitor->insert("info_zigbee : " );
            monitor->insert( oss_zigbee.str());        
            oss_zigbee.str("");   // Limpiar el contenido
            oss_zigbee.clear();   // Restablecer el estado
            
        #ifdef DBG_PRINT_GET_INFO                                     
        std::memcpy (  &buffer_receiver , zigbee->get_rxbuf() , sizeof(rx_buf));                  

        const uint64_t mac_address_rx = (static_cast<uint64_t>(buffer_receiver.mac_msb_rx) << 32) | buffer_receiver.mac_lsb_rx;
            monitor->insert (" " );
            //compara la direccion de mac "slave" con la mac de "entrada"
        if(ADDRESS_LONG_SLAVE == mac_address_rx)
            {
            monitor->insert ("mac aceptada" ); 
            }
        else { //muestra una direcion mac diferente a la configurada
            monitor->insert ("mac no es aceptada" );}
            monitor->insert( "rx data_receiver->mac : "         + hex_to_text( mac_address_rx )); 

            std::string txt_tmp ;
            txt_tmp.assign(reinterpret_cast<const char*>(buffer_receiver.data), sizeof(buffer_receiver.data));
            monitor->insert( "data_receiver->data ( "   + std::to_string(sizeof(buffer_receiver.data))+ " ) : "      + txt_tmp );

            //obtiene la direccion de mac seteada en el mrf24j40
            uint64_t mac_address;
            zigbee->mrf24j40_get_extended_mac_addr(&mac_address);
            monitor->insert("get address mac: "               + hex_to_text(mac_address));            
        #endif        
            RST_COLOR() ; 
            SET_COLOR(SET_COLOR_CYAN_TEXT);
            monitor->insert("LQI : " + std::to_string (zigbee->get_rxinfo()->lqi) );
            monitor->insert("RSSI : " + std::to_string(zigbee->get_rxinfo()->rssi) );
            monitor->insert("Frame Length : " + std::to_string(zigbee->get_rxinfo()->frame_length) );
            
            monitor->insert( "sizeof - buffer_receiverRX : "  +  std::to_string(sizeof(buffer_receiver) ) );            
            monitor->insert( "sizeof - buffer_receiverRX.data : "  +  std::to_string(sizeof(buffer_receiver.data) ) );
            std::string rx_b(rx_buf, rx_buf + sizeof(rx_buf));            
            monitor->insert( "ZigeBee : "     +    rx_b  );
        //imprime todo los datos obtenidos
        monitor->print_all();
        #endif
        RST_COLOR() ;           
        update(reinterpret_cast<const char*>(zigbee->get_rxinfo()->rx_data));    
    }//end rx

#endif


}// end namespace