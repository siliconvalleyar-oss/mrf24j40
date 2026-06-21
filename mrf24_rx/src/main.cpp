/**
 * @file    mrf24_rx/src/main.cpp
 * @brief   Punto de entrada del receptor MRF24J40.
 *
 * Escucha paquetes ZigBee entrantes, los muestra en consola con
 * timestamp, LQI y RSSI. Opcionalmente los despliega en un display
 * OLED SSD1306 vía I2C y los registra en un archivo CSV.
 * Incluye LED de actividad en GPIO12 y comandos interactivos.
 *
 * @author  MRF24J40 Team
 * @date    2026
 * @version 2.0.0
 */

#include <cstdio>
#include <cstring>
#include <csignal>
#include <unistd.h>
#include <ctime>
#include <chrono>
#include <fstream>
#include <sys/select.h>
#include <sys/time.h>
#include <bcm2835.h>
#include "mrf24j40.h"
#include "oled.hpp"

// ============================================================================
// Constantes de Red
// ============================================================================

/** @brief Dirección corta de este nodo (receptor). */
#define MY_ADDR     0x0002
/** @brief PAN ID de la red ZigBee. */
#define PAN_ID      0xCAFE
/** @brief Canal de radiofrecuencia (11-26). */
#define CHANNEL     20

// ============================================================================
// Configuración de LED (GPIO12, pin físico 32)
// ============================================================================

/** @brief GPIO del LED indicador de recepción. */
#define LED_GPIO    12
/** @brief Cantidad de parpadeos al recibir un paquete. */
#define BLINK_COUNT 3
/** @brief Duración de cada parpadeo en milisegundos. */
#define BLINK_DELAY_MS 80

// ============================================================================
// Variables Globales
// ============================================================================

/** @brief Bandera de ejecución; se pone a false en SIGINT para terminar. */
static volatile bool running = true;

/** @brief Instancia global del driver simplificado MRF24J40. */
static Mrf24j40 radio;

/** @brief Instancia del controlador OLED SSD1306. */
static OLED display;

/** @brief Archivo de log CSV para registrar paquetes recibidos. */
static std::ofstream logfile;

// ============================================================================
// Señales
// ============================================================================

/**
 * @brief Manejador de señales (SIGINT).
 * Establece running = false para terminar el bucle de recepción.
 */
void sig_handler(int) { running = false; }

// ============================================================================
// Funciones de Utilidad
// ============================================================================

/**
 * @brief Parpadeo del LED en GPIO12 con escritura digital.
 *
 * Enciende y apaga el LED @p count veces con @p delay_ms de
 * separación entre cada transición.
 *
 * @param count    Número de parpadeos.
 * @param delay_ms Duración de cada estado (HIGH/LOW) en ms.
 */
void blink_led(int count, int delay_ms) {
    for (int i = 0; i < count; i++) {
        bcm2835_gpio_write(LED_GPIO, HIGH);
        bcm2835_delay(delay_ms);
        bcm2835_gpio_write(LED_GPIO, LOW);
        if (i < count-1) bcm2835_delay(delay_ms);
    }
}

/**
 * @brief Imprime un timestamp con resolución de milisegundos.
 *
 * Formato: [HH:MM:SS.mmm]
 */
void print_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    struct tm* tm_info = localtime(&time_t);
    printf("[%02d:%02d:%02d.%03d] ", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec, (int)ms.count());
}

/**
 * @brief Imprime estadísticas de recepción.
 *
 * Muestra paquetes recibidos, y si hay datos, el promedio de
 * LQI (Link Quality Indicator) y RSSI (señal en dBm).
 */
void print_stats() {
    RadioStats stats;
    radio.getStats(stats);
    printf("\n=== ESTADÍSTICAS RX ===\n");
    printf("Paquetes recibidos: %u\n", stats.packets_received);
    if (stats.rx_count > 0) {
        printf("LQI promedio: %.1f, RSSI promedio: %.1f dBm\n",
               (float)stats.rx_lqi_sum/stats.rx_count,
               (float)stats.rx_rssi_sum/stats.rx_count);
    }
}

/**
 * @brief Actualiza la pantalla OLED con información del último paquete.
 *
 * Muestra: título "MRF24J40 RX", número de paquete, primeros 16
 * caracteres del payload, y métricas LQI/RSSI.
 *
 * @param packet_num Número secuencial del paquete.
 * @param payload    Cadena de caracteres imprimibles del payload.
 * @param len        Longitud total del payload.
 * @param lqi        Link Quality Indicator (0-255).
 * @param rssi       Received Signal Strength Indicator (dBm).
 */
void update_oled_display(int packet_num, const char* payload, int len, uint8_t lqi, int8_t rssi) {
    display.clear();
    display.draw_string(0, 0, "MRF24J40 RX", 1, true);
    char line2[32];
    snprintf(line2, sizeof(line2), "Paq #%d", packet_num);
    display.draw_string(0, 16, line2, 1, true);
    char line3[20];
    strncpy(line3, payload, 16);
    line3[16] = '\0';
    display.draw_string(0, 32, line3, 1, true);
    char line4[32];
    snprintf(line4, sizeof(line4), "LQI:%3d RSSI:%3ddBm", lqi, rssi);
    display.draw_string(0, 48, line4, 1, true);
    display.update();
}

/**
 * @brief Registra un paquete recibido en el archivo CSV.
 *
 * Formato: timestamp,packet_num,payload_hex,len,lqi,rssi
 *
 * @param packet_num Número secuencial del paquete.
 * @param buffer     Buffer con los datos raw recibidos.
 * @param len        Longitud de los datos.
 * @param lqi        Link Quality Indicator.
 * @param rssi       Received Signal Strength Indicator (dBm).
 */
void log_packet(int packet_num, const uint8_t* buffer, int len, uint8_t lqi, int8_t rssi) {
    if (!logfile.is_open()) return;
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    logfile << time_t << "," << packet_num << ",";
    for (int i = 0; i < len; i++) {
        logfile << std::hex << (int)buffer[i] << (i<len-1?":" : "");
    }
    logfile << std::dec << "," << len << "," << (int)lqi << "," << (int)rssi << "\n";
    logfile.flush();
}

// ============================================================================
// Punto de Entrada
// ============================================================================

/**
 * @brief   Punto de entrada principal del receptor.
 *
 * Inicializa BCM2835 y GPIO12, configura el MRF24J40 para recepción
 * en el canal y PAN ID especificados. Opcionalmente inicializa el
 * display OLED SSD1306.
 *
 * Bucle principal:
 * - Escanea continuamente el MRF24J40 en busca de paquetes.
 * - Cuando recibe un paquete: muestra timestamp, payload hex,
 *   LQI y RSSI; parpadea el LED; actualiza OLED y registra en CSV.
 * - Atiende comandos no bloqueantes por STDIN:
 *   - 's': Muestra estadísticas.
 *   - 'c': Limpia estadísticas.
 *   - 'q': Finaliza el programa.
 *
 * Al salir: muestra estadísticas finales, cierra el archivo de log,
 * muestra "Terminado" en OLED, apaga LED y libera BCM2835.
 *
 * @return 0 si la ejecución fue exitosa, 1 si hubo error.
 */
int main() {
    signal(SIGINT, sig_handler);

    // Inicializar bcm2835
    if (!bcm2835_init()) {
        fprintf(stderr, "Error: bcm2835_init falló. ¿Ejecutaste con sudo?\n");
        return 1;
    }

    // Configurar GPIO12 como salida digital
    bcm2835_gpio_fsel(LED_GPIO, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_write(LED_GPIO, LOW);
    printf("[LED] GPIO12 configurado como salida digital\n");

    printf("\n╔══════════════════════════════════════════╗\n");
    printf("║    MRF24J40 RECEPTOR BUFFER GRANDE     ║\n");
    printf("║         + LED (GPIO12)                 ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");

    // OLED opcional
    bool oled_ok = display.init();
    if (oled_ok) {
        display.showInitScreen();
        bcm2835_delay(500);
        display.clear();
        display.draw_string(0, 20, "MRF24J40 RX", 1, true);
        display.draw_string(0, 35, "Esperando...", 1, true);
        display.update();
    } else {
        printf("[WARN] OLED no detectado.\n");
    }

    if (!radio.init(CHANNEL)) {
        bcm2835_close();
        return 1;
    }

    radio.setPan(PAN_ID);
    radio.setShortAddress(MY_ADDR);
    radio.selfTest();

    printf("\n[CONFIG] PAN:0x%04X, Addr:0x%04X, Canal:%d, SPI:%u Hz\n",
           radio.getPan(), radio.getShortAddress(), CHANNEL, SPI_SPEED_HZ);

    logfile.open("mrf24_receiver.log", std::ios::app);
    if (logfile.is_open()) logfile << "#timestamp,packet_num,payload_hex,len,lqi,rssi\n";

    printf("\n[COMANDOS] s:stats, c:clear stats, q:salir\n\n");
    printf("[MAIN] Escuchando...\n");

    int packet_count = 0;
    uint8_t buffer[MAX_PAYLOAD];

    while (running) {
        radio.poll();

        // Comandos no bloqueantes via select()
        fd_set readfds;
        struct timeval tv = {0,0};
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        if (select(STDIN_FILENO+1, &readfds, NULL, NULL, &tv) > 0) {
            char cmd = getchar();
            if (cmd == 's') print_stats();
            else if (cmd == 'c') radio.resetStats();
            else if (cmd == 'q') running = false;
        }

        if (radio.hasPacket()) {
            uint8_t len = radio.rxLen();
            radio.rxGet(buffer);
            packet_count++;

            print_timestamp();
            printf("Paquete #%d (%d bytes):\n", packet_count, len);
            // Mostrar primeros 20 bytes y últimos 20 si es grande
            printf("  Datos: ");
            for (int i = 0; i < (len < 40 ? len : 20); i++) printf("%02X ", buffer[i]);
            if (len > 40) printf("... ");
            for (int i = (len > 40 ? len-20 : 0); i < len; i++) printf("%02X ", buffer[i]);
            printf("\n");
            printf("  LQI: %d/255, RSSI: %d dBm\n", radio.getLQI(), radio.getRSSI());

            // Parpadeo LED
            blink_led(BLINK_COUNT, BLINK_DELAY_MS);

            if (oled_ok) {
                // Mostrar primeros caracteres como texto (si son imprimibles)
                char text[17] = {0};
                int txtlen = len > 16 ? 16 : len;
                for (int i = 0; i < txtlen; i++) text[i] = (buffer[i] >= 32 && buffer[i] <= 126) ? buffer[i] : '.';
                update_oled_display(packet_count, text, len, radio.getLQI(), radio.getRSSI());
            }

            log_packet(packet_count, buffer, len, radio.getLQI(), radio.getRSSI());
        }

        usleep(10000);
    }

    print_stats();
    radio.printRegisters();
    if (logfile.is_open()) logfile.close();
    if (oled_ok) {
        display.clear();
        display.draw_string(0, 20, "Terminado", 1, true);
        display.update();
    }
    bcm2835_close();
    printf("\n[FIN] Total paquetes: %d\n", packet_count);
    return 0;
}
