#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include "mrf24j40.h"

static volatile bool running = true;
static Mrf24j40 radio;

void signalHandler(int sig) {
    (void)sig;
    printf("\n[INFO] Apagando...\n");
    running = false;
}

int main() {
    signal(SIGINT, signalHandler);
    
    printf("\n========================================\n");
    printf("       MRF24J40 RECEPTOR v3.0\n");
    printf("========================================\n\n");
    
    if (!radio.init(24)) {
        printf("[ERROR] No se pudo inicializar\n");
        return 1;
    }
    
    // Configurar direcciones
    radio.setPan(0xCAFE);
    radio.setShortAddress(0x6002);
    
    // Verificar configuración
    uint16_t pan = radio.getPan();
    uint16_t addr = radio.getShortAddress();
    
    printf("[CONFIGURACION]\n");
    printf("  PAN ID: 0x%04X %s\n", pan, pan == 0xCAFE ? "✅" : "❌");
    printf("  Mi direccion: 0x%04X %s\n", addr, addr == 0x6002 ? "✅" : "❌");
    printf("  Esperando paquetes...\n\n");
    
    int loop_count = 0;
    
    while (running) {
        radio.checkFlags();
        
        if (radio.hasRxPacket()) {
            uint8_t data[100];
            uint8_t len = radio.getRxLength();
            radio.getRxData(data);
            
            printf("\n┌─────────────────────────────────────────┐\n");
            printf("│         PAQUETE RECIBIDO               │\n");
            printf("├─────────────────────────────────────────┤\n");
            printf("│ Datos (%d bytes): ", len);
            for (int i = 0; i < len && i < 80; i++) {
                if (data[i] >= 32 && data[i] <= 126)
                    printf("%c", data[i]);
                else
                    printf(".");
            }
            printf("\n");
            printf("│ LQI: %d, RSSI: %d dBm\n", radio.getLQI(), -(int)radio.getRSSI());
            printf("└─────────────────────────────────────────┘\n");
            fflush(stdout);
        }
        
        loop_count++;
        if (loop_count % 100 == 0) {
            printf("."); fflush(stdout);
        }
        
        usleep(10000);
    }
    
    printf("\n\n[INFO] Programa terminado\n");
    return 0;
}
