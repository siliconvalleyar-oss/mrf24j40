# API Reference — Receptor MRF24J40

## Clase `Mrf24j40` (driver simplificado en `src/mrf24j40.cpp`)

Driver de alto nivel para el módulo MRF24J40MA con interfaz simplificada.

### Constantes

```cpp
#define SPI_DEVICE   "/dev/spidev0.0"
#define SPI_SPEED_HZ 10000000  // 10 MHz
#define MAX_PAYLOAD  100
```

### Constructor / Destructor

```cpp
Mrf24j40();
~Mrf24j40();
```

### Inicialización

```cpp
bool init(uint8_t channel);
// Inicializa SPI, resetea el módulo, configura RF y canal
```

### Configuración de Red

```cpp
void setPan(uint16_t pan);
void setShortAddress(uint16_t addr);
uint16_t getPan();
uint16_t getShortAddress();
bool setChannel(uint8_t ch);
void setPromiscuous(bool enable);
```

### Polling

```cpp
void poll();
// Procesa interrupciones TX/RX pendientes
// Debe llamarse frecuentemente en el bucle principal
```

### Recepción

```cpp
bool hasPacket();
// Retorna true si hay un paquete disponible

uint8_t rxLen();
// Retorna la longitud del payload recibido

void rxGet(uint8_t* buf);
// Copia el payload recibido a buf (MAX_PAYLOAD bytes)
// Resetea el flag rx_ready

uint8_t getLQI();
// Retorna Link Quality Indicator (0-255)

int8_t getRSSI();
// Retorna RSSI en dBm (ej: -85)
```

### Estadísticas RX

```cpp
struct RadioStats {
    uint32_t packets_received;  // Paquetes recibidos
    uint32_t rx_lqi_sum;        // Suma de LQI para promedio
    int32_t  rx_rssi_sum;       // Suma de RSSI para promedio
    uint32_t rx_count;          // Contador de lecturas
};

void getStats(RadioStats& stats);
void resetStats();
```

### Diagnóstico

```cpp
bool selfTest();
void printRegisters();
void flushRx();
```

---

## Clase `OLED` (display/display.cpp)

Manejo del display OLED SSD1306 vía I²C.

```cpp
bool init();
// Inicializa comunicación I²C y configura el display

void clear();
// Limpia el buffer del display

void draw_string(int x, int y, const char* str, int size, bool color);
// Dibuja texto en posición (x, y)

void update();
// Envía el buffer al display físico

void showInitScreen();
// Muestra pantalla de inicio
```

---

## Clase `Mrf24j` (namespace `MRF24J40`)

Driver completo del MRF24J40 con soporte de direcciones de 64 bits. Misma API que en el transmisor.

### Recepción

```cpp
rx_info_t* get_rxinfo();
// Retorna puntero a estructura con datos del RX

uint8_t* get_rxbuf();
// Retorna puntero al buffer RX

const int rx_datalength();
// Retorna longitud de datos RX

void rx_flush();
// Limpia el FIFO de recepción

void rx_enable();
void rx_disable();
```

### Configuración

```cpp
void set_pan(const uint16_t panid);
void address16_write(const uint16_t addr);
void address64_write(const uint64_t addr);
void set_promiscuous(const bool enable);
void set_channel(const uint8_t channel);
void settings_mrf();
void set_ignoreBytes(const int bytes);
void set_bufferPHY(const bool enable);
void set_palna(const bool enable);
```

### Interrupciones

```cpp
void interrupt_handler();
void interrupts();
void noInterrupts();

const bool check_flags(void (*rx_handler)(), void (*tx_handler)());
```

---

## Clase `Spi_t` (namespace `SPI`)

```cpp
void init();
void settings_spi();
void spi_close();
const uint8_t Transfer1bytes(const uint8_t cmd);
const uint8_t Transfer2bytes(const uint16_t address);
const uint8_t Transfer3bytes(const uint32_t address);
const uint32_t get_spi_speed();
```

---

## Estructuras de Datos

### rx_info_t
```cpp
struct rx_info_t {
    uint8_t frame_length;     // Longitud del frame
    uint8_t rx_data[124];     // Datos recibidos (máx 124 bytes)
    uint8_t lqi;              // Link Quality Indicator
    uint8_t rssi;             // RSSI raw
};
```

### tx_info_t
```cpp
struct tx_info_t {
    uint8_t tx_ok       : 1;  // TX exitosa
    uint8_t retries     : 2;  // Número de retransmisiones
    uint8_t channel_busy : 1; // Canal ocupado
};
```

### RadioStats
```cpp
struct RadioStats {
    uint32_t packets_received;  // Total paquetes recibidos
    uint32_t rx_lqi_sum;        // Suma acumulada de LQI
    int32_t  rx_rssi_sum;       // Suma acumulada de RSSI (dBm)
    uint32_t rx_count;          // Número de muestras
};
```

## Formato del Log CSV

```csv
#timestamp,packet_num,payload_hex,len,lqi,rssi
1712345678,1,00:01:02:03:04:...,100,200,-85
1712345680,2,AA:BB:CC:DD:EE:...,100,210,-83
```

Donde:
- `timestamp`: Unix timestamp (segundos)
- `packet_num`: Número secuencial de paquete
- `payload_hex`: Payload en hexadecimal separado por `:`
- `len`: Longitud del payload en bytes
- `lqi`: Link Quality Indicator (0-255)
- `rssi`: RSSI en dBm
