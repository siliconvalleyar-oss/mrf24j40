# API Reference — Transmisor MRF24J40

## Clase `Mrf24j40` (driver simplificado en `src/mrf24j40.cpp`)

Driver de alto nivel para el módulo MRF24J40MA con interfaz simplificada.

### Constantes

```cpp
#define SPI_DEVICE  "/dev/spidev0.0"
#define SPI_SPEED_HZ 10000000  // 10 MHz
#define MAX_PAYLOAD 100        // Bytes máximos de payload
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
// channel: 11-26 (IEEE 802.15.4)
// Retorna: true si éxito
```

### Configuración de Red

```cpp
void setPan(uint16_t pan);
// Configura PAN ID (identificador de red)

void setShortAddress(uint16_t addr);
// Configura dirección corta de 16 bits

uint16_t getPan();
// Retorna el PAN ID actual

uint16_t getShortAddress();
// Retorna la dirección corta actual

bool setChannel(uint8_t ch);
// Cambia el canal (11-26)

void setPromiscuous(bool enable);
// Habilita/deshabilita modo promiscuo
```

### Transmisión

```cpp
bool send(uint16_t dest_addr, uint16_t dest_pan, const uint8_t* data, uint8_t len);
// Envía un paquete a destino
// dest_addr: dirección corta destino
// dest_pan: PAN ID destino
// data: puntero a datos (máx MAX_PAYLOAD bytes)
// len: longitud de datos
// Retorna: true si se encoló correctamente

bool sendString(uint16_t dest_addr, const char* str);
// Envía un string a destino (wrapper de send)
```

### Polling y Estado

```cpp
void poll();
// Lee interrupciones y maneja TX/RX pendientes

bool txDone();
// Retorna true si la transmisión actual ha terminado

bool txSuccess();
// Retorna true si la última TX fue exitosa

uint8_t txRetries();
// Retorna número de retransmisiones de la última TX

bool hasPacket();
// Retorna true si hay un paquete RX disponible
```

### Recepción

```cpp
uint8_t rxLen();
// Retorna longitud del paquete RX recibido

void rxGet(uint8_t* buf);
// Obtiene los datos del paquete RX (copia a buf)

uint8_t getLQI();
// Retorna Link Quality Indicator (0-255)

int8_t getRSSI();
// Retorna RSSI en dBm
```

### Estadísticas

```cpp
struct RadioStats {
    uint32_t packets_sent;        // Paquetes enviados
    uint32_t tx_success;          // TX exitosas
    uint32_t tx_fail;             // TX fallidas
    uint32_t tx_retries_total;    // Retransmisiones totales
    uint32_t packets_received;    // Paquetes recibidos
    uint32_t rx_lqi_sum;          // Suma LQI
    int32_t  rx_rssi_sum;         // Suma RSSI
    uint32_t rx_count;            // Contador RX
};

void getStats(RadioStats& stats);
void resetStats();
```

### Diagnóstico

```cpp
bool selfTest();
// Verifica escritura/lectura del registro PAN ID
// Retorna: true si el módulo responde

void printRegisters();
// Imprime registros clave: SOFTRST, INTSTAT, TXSTAT, PANID, SADDR, RFCON2

void flushRx();
// Limpia el FIFO de recepción
```

---

## Clase `Mrf24j` (namespace `MRF24J40`, driver completo en `mrf24/mrf24j40.hpp`)

Driver completo para el MRF24J40 con soporte de direcciones de 64 bits.

### Métodos Públicos

```cpp
void init();
void mrf24j40_init();

void send(const uint64_t dest64, const std::vector<uint8_t> data);
void send64(const uint64_t dest64, const std::vector<uint8_t> data);
void send64(const uint64_t dest64, const DATA::packet_tx packet);

void interrupt_handler();
void set_pan(const uint16_t panid);
void address16_write(const uint16_t addr);
void address64_write(const uint64_t addr);
void settings_mrf();
void set_promiscuous(const bool enable);
void set_channel(const uint8_t channel);
void rx_enable();
void rx_disable();
void rx_flush();

rx_info_t* get_rxinfo();
tx_info_t* get_txinfo();
uint8_t* get_rxbuf();
const int rx_datalength();
void set_ignoreBytes(const int bytes);
void set_bufferPHY(const bool enable);
bool get_bufferPHY();
void set_palna(const bool enable);

const bool check_flags(void (*rx_handler)(), void (*tx_handler)());

void mrf24j40_get_extended_mac_addr(uint64_t* mac);
void mrf24j40_get_short_mac_addr(uint16_t* mac);
```

### Estructuras de Datos

```cpp
struct rx_info_t {
    uint8_t frame_length;
    uint8_t rx_data[124];
    uint8_t lqi;
    uint8_t rssi;
};

struct tx_info_t {
    uint8_t tx_ok      : 1;
    uint8_t retries    : 2;
    uint8_t channel_busy : 1;
};
```

---

## Clase `Spi_t` (namespace `SPI`, `spi/spi.hpp`)

Manejo de comunicación SPI.

```cpp
void init();
void settings_spi();
void spi_close();
const uint8_t Transfer1bytes(const uint8_t cmd);
const uint8_t Transfer2bytes(const uint16_t address);
const uint8_t Transfer3bytes(const uint32_t address);
void printDBGSpi();
void msj_fail();
const uint32_t get_spi_speed();
```

## Estructura `Radio_t` (namespace `MRF24J40`, `radio/radio.hpp`)

Orquestación de alto nivel de la radio.

```cpp
void Start(bool& status);
void interrupt_routine();
const bool Run();
void funcion(std::function<void(uint8_t*)> rx);
static void handle_tx();
static void handle_rx();
```
