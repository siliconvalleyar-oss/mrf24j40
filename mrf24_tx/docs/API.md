# API Reference — Transmisor MRF24J40

## Clase `Mrf24j40` (driver simplificado en `src/mrf24j40.cpp`)

Driver de alto nivel para el módulo MRF24J40MA con interfaz simplificada.

### Constantes

```cpp
#define SPI_DEVICE   "/dev/spidev0.0"
#define SPI_SPEED_HZ 2000000   // 2 MHz
#define MAX_PAYLOAD  100       // Bytes máximos de payload
```

### Constructor / Destructor

```cpp
Mrf24j40();
~Mrf24j40();
```

### Inicialización

```cpp
bool init(uint8_t channel);    // Inicializa SPI, resetea módulo, configura RF y canal
bool selfTest();               // Verifica escritura/lectura del registro PAN ID
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

### Transmisión

```cpp
bool send(uint16_t dest_addr, uint16_t dest_pan, const uint8_t* data, uint8_t len);
bool sendString(uint16_t dest_addr, const char* str);
```

### Polling y Estado

```cpp
void poll();
bool txDone();
bool txSuccess();
uint8_t txRetries();
bool hasPacket();
```

### Estadísticas

```cpp
struct RadioStats {
    uint32_t packets_sent;
    uint32_t tx_success;
    uint32_t tx_fail;
    uint32_t tx_retries_total;
    uint32_t packets_received;
    uint32_t rx_lqi_sum;
    int32_t  rx_rssi_sum;
    uint32_t rx_count;
};

void getStats(RadioStats& stats);
void resetStats();
void printRegisters();
```

---

## Clase `Mrf24j` (namespace `MRF24J40`)

Driver completo con soporte de direcciones de 64 bits.

```cpp
void init();
void send(const uint64_t dest64, const std::vector<uint8_t> data);
void send64(const uint64_t dest64, const std::vector<uint8_t> data);
void send64(const uint64_t dest64, const DATA::packet_tx packet);
void interrupt_handler();
void set_pan(const uint16_t panid);
void address16_write(const uint16_t addr);
void address64_write(const uint64_t addr);
void set_promiscuous(const bool enable);
void set_channel(const uint8_t channel);
const bool check_flags(void (*rx_handler)(), void (*tx_handler)());
rx_info_t* get_rxinfo();
tx_info_t* get_txinfo();
uint8_t* get_rxbuf();
const int rx_datalength();
void set_bufferPHY(const bool enable);
bool get_bufferPHY();
void set_palna(const bool enable);
```

---

## Clase `MqttHandler` (namespace `MOSQUITTO`)

Cliente MQTT con reconexión automática y loop asíncrono.

```cpp
MqttHandler(const std::string& host, int port,
            const std::string& clientId = "mrf24j40_tx");

bool begin();                       // Conectar e iniciar loop thread
void stop();                        // Desconectar y detener loop
bool publish(const std::string& topic, const std::string& payload);
bool subscribe(const std::string& topic);
bool isConnected() const;
```

**Callbacks:**
```cpp
std::function<void(const std::string&, const std::string&)> onMessage;
std::function<void()> onConnect;
std::function<void()> onDisconnect;
```

---

## Clase `MqttBridge` (namespace `MOSQUITTO`)

Puente entre eventos de radio y topics MQTT.

```cpp
MqttBridge(MqttHandler& handler, Mrf24j40* radio = nullptr);

bool begin();
void stop();
void publishStatus(const std::string& deviceId, bool isOn, int value);
void publishDiscovery();             // Publica estado inicial de todos los dispositivos
void onRadioPacket(const uint8_t* data, uint8_t len);
```

**Dispositivos soportados:** `light_1`, `temperature_1`, `fan_1`, `lock_1`, `curtain_1`, `energy_1`, `rgb_1`.

---

## Estructura `Radio_t` (namespace `MRF24J40`)

Orquestación de alto nivel de la radio.

```cpp
void Start(bool& status);
void interrupt_routine();
const bool Run();
static void handle_tx();
static void handle_rx();
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
```
