#include <bcm2835.h>
#include <iostream>
#include <csignal>

constexpr uint8_t INTERRUPT_PIN = RPI_V2_GPIO_P1_16;  // GPIO 23 en el pin físico 16
volatile int interrupt_count = 0;
volatile bool running = true;

// Función para manejar la interrupción
void interrupt_handler() {
    if (bcm2835_gpio_eds(INTERRUPT_PIN)) {
        // Limpia el flag de evento
        bcm2835_gpio_set_eds(INTERRUPT_PIN);
        
        interrupt_count++;
        std::cout << "Interrupción recibida: Mensaje " << interrupt_count << std::endl;
        
        if (interrupt_count >= 3) {
            running = false; // Detener el programa después de 3 interrupciones
        }
    }
}

// Función para manejar la señal SIGINT (Ctrl+C)
void sigint_handler(int sig) {
    running = false;
}

int interrupt_handler() {
    // Configurar la señal para poder terminar el programa con Ctrl+C
    signal(SIGINT, sigint_handler);

    if (!bcm2835_init()) {
        std::cerr << "Error al inicializar bcm2835. ¿Estás ejecutando como root?" << std::endl;
        return 1;
    }

    // Configurar el pin GPIO 23 como entrada
    bcm2835_gpio_fsel(INTERRUPT_PIN, BCM2835_GPIO_FSEL_INPT);
    
    // Habilitar pull-up interno para evitar valores flotantes
    bcm2835_gpio_set_pud(INTERRUPT_PIN, BCM2835_GPIO_PUD_UP);
    
    // Limpia cualquier interrupción previa pendiente
    bcm2835_gpio_set_eds(INTERRUPT_PIN);

    // Configurar para detectar flanco descendente (falling edge)
    bcm2835_gpio_fen(INTERRUPT_PIN);

    std::cout << "Esperando interrupciones en GPIO 23..." << std::endl;

    while (running) {
        // Verificar interrupciones
        interrupt_handler();

        // Pequeño retraso para evitar consumir demasiados recursos
        bcm2835_delay(100);
    }

    std::cout << "Programa terminado." << std::endl;
    bcm2835_close();
    return 0;
}
