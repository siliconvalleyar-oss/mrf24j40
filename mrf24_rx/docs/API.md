# API Reference — Receptor MRF24J40

## Clase `Mrf24j40` (driver simplificado)

```cpp
bool init(uint8_t channel);
void setPan(uint16_t pan);
void setShortAddress(uint16_t addr);
uint16_t getPan();
uint16_t getShortAddress();
void poll();
bool hasPacket();
uint8_t rxLen();
void rxGet(uint8_t* buf);
uint8_t getLQI();
int8_t getRSSI();
bool selfTest();
void printRegisters();
void flushRx();
```

### Estadísticas RX

```cpp
struct RadioStats {
    uint32_t packets_received;
    uint32_t rx_lqi_sum;
    int32_t  rx_rssi_sum;
    uint32_t rx_count;
};

void getStats(RadioStats& stats);
void resetStats();
```

---

## Clase `Mrf24j` (namespace `MRF24J40`)

Driver completo con soporte de direcciones de 64 bits.

```cpp
void init();
void interrupt_handler();
void set_pan(const uint16_t panid);
void address16_write(const uint16_t addr);
void address64_write(const uint64_t addr);
void set_promiscuous(const bool enable);
void set_channel(const uint8_t channel);
void rx_enable();
void rx_disable();
void rx_flush();
const bool check_flags(void (*rx_handler)(), void (*tx_handler)());
rx_info_t* get_rxinfo();
uint8_t* get_rxbuf();
const int rx_datalength();
void set_bufferPHY(const bool enable);
bool get_bufferPHY();
void set_palna(const bool enable);
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
    uint8_t tx_ok       : 1;
    uint8_t retries     : 2;
    uint8_t channel_busy : 1;
};
```

---

## Clase `MqttHandler` (namespace `MOSQUITTO`)

Cliente MQTT asíncrono.

```cpp
MqttHandler(const std::string& host, int port,
            const std::string& clientId = "mrf24j40_rx");

bool begin();
void stop();
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

Puente radio ⟷ MQTT para el receptor.

```cpp
MqttBridge(MqttHandler& handler, Mrf24j40* radio = nullptr);

bool begin();
void stop();
void publishStatus(const std::string& deviceId, bool isOn, int value);
void publishDiscovery();
void onRadioPacket(const uint8_t* data, uint8_t len);
```

**Topics de comando:**
- `domotics/{light,temperature,fan,lock,curtain,energy,rgb}/set`
- Formato: `{"command": "on"}` o `{"command": "set", "value": 50}`

**Topics de estado:**
- `domotics/{device}/status` — `{"isOn": true, "value": 0}`
- `domotics/zigbee/rx` — `{"data": "hex...", "rssi": -85, "lqi": 200}`

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

---

## Formato del Log CSV

```csv
#timestamp,packet_num,payload_hex,len,lqi,rssi
1712345678,1,00:01:02:03:04:...,100,200,-85
```
