
#include <mrf24/mrf24j40_cmd.hpp>
#include <mrf24/mrf24j40_settings.hpp>
#include <mrf24/mrf24j40.hpp>
#include <tyme/tyme.hpp>
#include <config/config.hpp>
#include <work/data_analisis.hpp>
#include <spi/spi.hpp>



namespace MRF24J40{
extern size_t ignoreBytes;

    void 
    Mrf24j::send(const uint64_t mac_address_dest, const std::vector<uint8_t> vect) 
    {
        const auto size = vect.size();
        int incr = 0;
        write_long(++incr, m_bytes_MHR); // header length
        // +ignoreBytes is because some module seems to ignore 2 bytes after the header?!.
        // default: ignoreBytes = 0;
        //m_bytes_MHR 9 + 2 + 2 
        //size  = 16
        //ignoreBytes =2 

        write_long(++incr,  m_bytes_MHR + ignoreBytes + size);//

        // 0 | pan compression | ack | no security | no data pending | data frame[3 bits]
        write_long(++incr, 0b01100001); // first byte of Frame Control
        
        // 16 bit source, 802.15.4 (2003), 16 bit dest,
        write_long(++incr, 0b10001000); // second byte of frame control
        write_long(++incr, 1);  // sequence number 1

        const uint16_t panid = get_pan();
        #ifdef DBG_MRF
            printf("\npanid : 0x%X\n",panid);
        #endif

        
        
        write_long(++incr, panid & 0xff);  // dest panid
        write_long(++incr, panid >> 8);

        set_macaddress(incr, mac_address_dest );
        set_macaddress(incr, address64_read() );
  
        // All testing seems to indicate that the next two bytes are ignored.        
        //2 bytes on FCS appended by TXMAC
         incr+=ignoreBytes;

        for(const auto& byte : vect) write_long(++incr,byte);
        
        // ack on, and go!
        write_short(MRF_TXNCON, (1<<MRF_TXNACKREQ | 1<<MRF_TXNTRIG));
        mode_turbo();
    }

    void 
    Mrf24j::send64(const uint64_t dest64, const DATA::packet_tx packet_tx) {
        //const uint8_t len = strlen(packet_tx.data); // get the length of the char* array
        //const uint8_t len = strlen(packet_tx); // get the length of the char* array
        const size_t len =sizeof(packet_tx);// const uint8_t len =sizeof(packet_tx.data);
        int i = 0;
        write_long(i++, m_bytes_MHR); // header length

        // +ignoreBytes is because some module seems to ignore 2 bytes after the header?!.
        // default: ignoreBytes = 0;
        write_long(i++, m_bytes_MHR+ignoreBytes+len);//9 + 2 + tamaño del paquete

        // 0 | pan compression | ack | no security | no data pending | data frame[3 bits]
        write_long(i++, 0b01100001); // first byte of Frame Control
        // 16 bit source, 802.15.4 (2003), 16 bit dest,
        write_long(i++, 0b10001000); // second byte of frame control
        write_long(i++, 1);  // sequence number 1

        const uint16_t panid = get_pan();
        #ifdef DBG
            printf("\npanid: 0x%X\n",panid);
        #endif
        write_long(i++, panid >> 8);
        write_long(i++, panid & 0xff);  // dest panid
        
        //direccion de destino a enviar el mensaje
        set_macaddress(i, dest64 );

        //lee la direccion mac de 64 bits obtenida

        set_macaddress(i, address64_read() );

        #include <mrf24/mrf24j40._microchip.hpp>
        write_long(RFCTRL2,0x80);// que hace ?

        // All testing seems to indicate that the next two bytes are ignored.
        //2 bytes on FCS appended by TXMAC
        
        i+=ignoreBytes;//ignora 2 bytes

        //genera un paquete
        std::vector<uint8_t> vect(sizeof(packet_tx));
        std::memcpy(vect.data(), &packet_tx, sizeof(packet_tx)); // Copiar los datos de la estructura al vector

        for(const auto& byte : vect)write_long(i++,byte);
        // ack on, and go!
        write_short(MRF_TXNCON, (1<<MRF_TXNACKREQ | 1<<MRF_TXNTRIG));        
        mode_turbo();
    }
   

    void 
    Mrf24j::send64(const uint64_t dest64 ,const std::vector<uint8_t> vect) {                
        const auto len =vect.size();// const uint8_t len =sizeof(packet_tx.data);
        int i = 0;
        write_long(i++, m_bytes_MHR); // header length

        // +ignoreBytes is because some module seems to ignore 2 bytes after the header?!.
        // default: ignoreBytes = 2;
        // m_bytes_MHR 23
        // len packet = 69
        write_long(i++, m_bytes_MHR + ignoreBytes + len);//9 + 2 + tamaño del paquete

        // 0 | pan compression | ack | no security | no data pending | data frame[3 bits]
        write_long(i++, 0b01100001); // first byte of Frame Control
        // 16 bit source, 802.15.4 (2003), 16 bit dest,
        write_long(i++, 0b10001000); // second byte of frame control
        write_long(i++, 1);  // sequence number 1

        const uint16_t panid = get_pan();
        #ifdef DBG
            printf("\npanid: 0x%X\n",panid);
        #endif
        
        //send PANID
        write_long(i++, panid & 0xff);  // dest panid
        write_long(i++, panid >> 8);

        //direccion de destino a enviar el mensaje
        set_macaddress(i, dest64 );

        //lee la direccion mac de 64 bits obtenida

        set_macaddress(i, address64_read() );

        #include <mrf24/mrf24j40._microchip.hpp>
        write_long(RFCTRL2,0x80);// que hace ?

        // All testing seems to indicate that the next two bytes are ignored.
        //2 bytes on FCS appended by TXMAC
        
        i+=ignoreBytes;//ignora 2 bytes

        //genera un paquete

        for(const auto& byte : vect)write_long(i++,byte);
        // ack on, and go!
        write_short(MRF_TXNCON, (1<<MRF_TXNACKREQ | 1<<MRF_TXNTRIG));        
        mode_turbo();
    }
 
}