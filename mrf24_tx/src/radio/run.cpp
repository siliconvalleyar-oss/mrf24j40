/**
 * @file    run.cpp
 * @brief   Punto de entrada del bucle de ejecución de la radio
 * @details Proporciona la clase Run_t que inicia el procesamiento
 *          principal de la radio MRF24J40, creando una instancia
 *          de Radio_t y ejecutando su bucle en el contexto principal.
 *
 * @namespace RUN
 */

#include <radio/run.hpp>
#include <radio/radio.hpp>
#include <config/config.hpp>
#include <mrf24/mrf24j40.hpp>
#include <iostream>
#include <thread>
#include <vector>

// Variable externa para acceso al último mensaje recibido
namespace MRF24J40 {
    extern std::string msj_txt;
}

namespace RUN {

void Run_t::start()
{
    /**
     * @brief Inicia el bucle de procesamiento de la radio
     *
     * Crea una instancia de MRF24J40::Radio_t y ejecuta su bucle
     * principal. En modo RX (USE_MRF24_RX), el bucle es infinito
     * procesando paquetes entrantes. En modo TX, ejecuta ciclos
     * de transmisión periódica.
     *
     * @note Requiere permisos de root para acceso a SPI y GPIO.
     * @throws Captura excepciones genéricas mostrando "error :("
     */
    [[gnu::unused]] bool flag{true};
    system("clear");

    try {
        auto zigbee{std::make_unique<MRF24J40::Radio_t>()};

        #ifdef USE_MRF24_RX
            while (true)
        #endif
        {
            flag = zigbee->Run();
            #ifdef USE_MRF24_RX
                if (flag == true) {
                    // Flag activo: hay datos disponibles
                }
            #endif
        }
    } catch (...) {
        std::cerr << "\nerror :(\n";
    }
}

} // namespace RUN
