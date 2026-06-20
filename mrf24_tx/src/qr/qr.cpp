
#include <qr/qr.hpp>
#include <display/color.hpp>
#include <work/rfflush.hpp>
#include <vector>
#include <iostream>
#include <cstring>
//#include <tuple>

namespace QR{



    bool 
    Qr_t::create(const std::string_view& fname ) {
        auto monitor {std::make_unique <FFLUSH::Fflush_t>()};
        RST_COLOR() ;

        // Configuración del código QR
        QRcode* qr = QRcode_encodeString(fname.data(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);
        //auto qr = std::unique_ptr<QRcode>(QRcode_encodeString(fname.data(), 0, QR_ECLEVEL_L, QR_MODE_8, 1));
        
        // Imprime el código QR en la consola
       // std::cout << "\033[2J\033[H" << std::flush;
        
                SET_COLOR(SET_COLOR_WHITE_TEXT);
        std::cout << "\n";
        
        
        for (int y = 0; y < qr->width; y++) {
            for (int x = 0; x < qr->width; x++) {                
               std::cout << (qr->data[y * qr->width + x] & 1 ? "██" : "  ");                        
            }            
            std::cout << "\n";
        }
       
        // Libera la memoria
        // QRcode_free(qr.get()); 
        QRcode_free(qr); 
        return true;
    }
}