#pragma once
#include <string_view>
#include <mutex>
#include <vector>
#include <string>


namespace FFLUSH{

struct Fflush_t
{
    
        Fflush_t()=default;
            //initscr(); 
        
        ~Fflush_t()=default;
        /* data */
        
        int         funcThread() ;
        void        print  (const std::string_view,int& ,const int) ;
        void        insert (const std::string_view) ;
        void        print_all();

    // Si quieres que se puedan modificar los mensajes externamente
    std::vector<std::string>& getAllMessages() { return message; }

    // Si no quieres que los mensajes sean modificables desde fuera de la clase
    const std::vector<std::string>& getAllMessages() const { return message; }
    
    private:
        std::mutex m_mtx;
        //inline static int row={0};
        std::vector<std::string>message{};

};



}


