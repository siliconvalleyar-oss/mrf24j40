#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include "mrf24j40.h"

static volatile bool running = true;
static Mrf24j40 radio;
static int counter = 0;

void signalHandler(int sig) {
    (void)sig;
    printf("\n[INFO] Apagando...\n");
    running = false;
}

int main() {
    signal(SIGINT, signalHandler);
    
    printf("\n========================================\n");
    printf("      MRF24J40 TRANSMISOR v3.0\n");
    printf("========================================\n\n");
    
    if (!radio.init(24)) {
        printf("[ERROR] No se pudo inicializar\n");
        return 1;
    }
    
    // Configurar direcciones
    radio.setPan(0xCAFE);
    radio.setShortAddress(0x6001);
    
    // Verificar configuración
    uint16_t pan = radio.getPan();
    uint16_t addr = radio.getShortAddress();
    
    printf("\n[CONFIGURACION]\n");
    printf("  PAN ID: 0x%04X %s\n", pan, pan == 0xCAFE ? "✅" : "❌");
    printf("  Mi direccion: 0x%04X %s\n", addr, addr == 0x6001 ? "✅" : "❌");
    printf("  Enviando a: 0x6002\n");
    printf("  Intervalo: 2000 ms\n\n");
    
    if (pan != 0xCAFE || addr != 0x6001) {
        printf("[ERROR] Configuracion fallida!\n");
        return 1;
    }
    
    while (running) {
        char message[64];
        snprintf(message, sizeof(message), "MSG%d", counter++);
        
        printf("[TX] Enviando: %s\n", message);
        fflush(stdout);
        
        if (radio.sendString(0x6002, message)) {
            int timeout = 100;
            while (timeout-- && !radio.wasTxSuccessful() && running) {
                radio.checkFlags();
                usleep(10000);
            }
            
            if (radio.wasTxSuccessful()) {
                printf("[TX] ✅ OK (retries: %d)\n", radio.getTxRetries());
            } else {
                printf("[TX] ❌ FALLO\n");
            }
        } else {
            printf("[TX] ❌ Error\n");
        }
        fflush(stdout);
        
        for (int i = 0; i < 20 && running; i++) {
            usleep(100000);
            radio.checkFlags();
        }
    }
    
    printf("\n[INFO] Programa terminado\n");
    return 0;
}
