#include <work/rfflush.hpp>
#include <display/color.hpp>

#include <iostream>
//#include <thread>
#include <memory>
#include <unistd.h>

namespace FFLUSH{



    void 
    Fflush_t::print(const std::string_view str_txt, int& rw,const int col) 
    { //row fila  // col : columna         
     //SET_COLOR(SET_COLOR_GRAY_TEXT);
    // move(row,col);
    
    std::cout << "\033[" << rw << ";" << col << "H"<<str_txt.data();
            //std::unique_lock<std::mutex> lock(m_mtx);
            // Mover el cursor a la ubicación de las coordenadas (row, col) y actualizar el valor
            //std::cout << "\033[" << row << ";" << col << "H" << str_txt.data();//<< std::flush;
            //lock.unlock();                
            //std::cout<<str_txt.data();
            rw++;
    return ;    
    }

    void 
    Fflush_t::insert(const std::string_view str_txt) { 
        message.emplace_back(str_txt);     
    }

    void 
    Fflush_t::print_all() 
    { 
    
    system("clear"); 
        int fil =0;
        for(const auto& msj : message){
            std::cout << "\033[" << fil << ";" << 0 << "H"<<msj;
            fil++;
        }
    }
 

int Fflush_t::funcThread() 
{
    // Limpia la pantalla y muestra "VALOR 1" y "VALOR 2" en ubicaciones específicas
   // std::cout << "\033[2J\033[HVALOR 1 :\nVALOR 2 :" << std::flush;

//std::cout << "\033[2J\033[H" << std::flush;

    // Inicia los hilos para actualizar los valores
    // std::thread thread1(updateValue, 1, 1000000, 2, 9);  // Fila 2, Columna 9
    // std::thread thread2(updateValue, 2, 2000000, 3, 9);  // Fila 3, Columna 9
// 
    // thread1.join();
    // thread2.join();
// 
    // std::cout << std::endl;
    return 0;
}


    
}


