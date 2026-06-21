/**
 * @file    mqtt_bridge.cpp
 * @brief   Implementación del puente MQTT-Radio
 * @details Traduce eventos de radio en mensajes MQTT y viceversa.
 *          Controla GPIOs según comandos MQTT recibidos.
 *
 * @author  MRF24J40 Team
 * @date    2026
 * @version 2.0.2
 */

#include <mosquitto/mqtt_bridge.hpp>
#include <iostream>
#include <sstream>
#include <cstring>
#include <thread>
#include <memory>
#include <bcm2835.h>

// ============================================================================
// Logging
// ============================================================================

/** @brief Imprime mensaje de log del bridge */
static void log(const std::string& msg)
{
    std::cout << "[BRIDGE] " << msg << std::endl;
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

MqttBridge::MqttBridge(MqttHandler& handler, Mrf24j40* radio)
    : m_mqtt(handler)
    , m_radio(radio)
{
    // Vincular callback de mensaje MQTT
    m_mqtt.onMessage = [this](const std::string& topic, const std::string& payload) {
        onMqttMessage(topic, payload);
    };
}

MqttBridge::~MqttBridge()
{
    stop();
}

// ============================================================================
// Inicialización
// ============================================================================

bool MqttBridge::begin()
{
    log("Bridge iniciando...");

    // Suscribir a todos los topics de comando
    for (const auto& topic : m_commandTopics) {
        m_mqtt.subscribe(topic);
    }

    // Publicar estado inicial
    publishDiscovery();

    log("Bridge activo (" + std::to_string(m_commandTopics.size()) + " topics)");
    return true;
}

void MqttBridge::stop()
{
    m_active = false;
    log("Bridge detenido");
}

// ============================================================================
// Publicación de estado
// ============================================================================

void MqttBridge::publishStatus(const std::string& deviceId, bool isOn, int value)
{
    if (!m_active.load()) return;

    // Construir JSON manual (sin dependencia de nlohmann)
    std::ostringstream json;
    json << "{"
         << "\"deviceId\":\"" << deviceId << "\","
         << "\"isOn\":" << (isOn ? "true" : "false")
         << ",\"value\":" << value
         << "}";

    std::string topic = "domotics/" + deviceId.substr(0, deviceId.find('_')) + "/status";
    m_mqtt.publish(topic, json.str());

    // Actualizar estado local
    std::lock_guard<std::mutex> lock(m_devMutex);
    m_devices[deviceId] = {isOn, value};
}

void MqttBridge::publishSensor(const std::string& deviceId, float value)
{
    if (!m_active.load()) return;

    std::ostringstream json;
    json << "{"
         << "\"deviceId\":\"" << deviceId << "\","
         << "\"value\":" << value
         << "}";

    std::string topic = "domotics/" + deviceId.substr(0, deviceId.find('_')) + "/status";
    m_mqtt.publish(topic, json.str());
}

// ============================================================================
// Control de GPIO
// ============================================================================

void MqttBridge::setGpio(int pin, bool state)
{
    // Configurar pin como salida si no estaba
    bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_write(pin, state ? HIGH : LOW);

    std::cout << "[GPIO] Pin " << pin << " -> " << (state ? "HIGH" : "LOW") << std::endl;
}

bool MqttBridge::getGpio(int pin)
{
    uint8_t level = bcm2835_gpio_lev(pin);
    return (level == HIGH);
}

// ============================================================================
// Procesamiento de paquetes de radio
// ============================================================================

void MqttBridge::onRadioPacket(const uint8_t* data, size_t len, uint8_t lqi, int8_t rssi)
{
    if (!m_active.load() || !data || len == 0) return;

    // Convertir payload a string
    std::string payload(reinterpret_cast<const char*>(data),
                        std::min(len, size_t(128)));

    // Publicar en topic de radio
    std::ostringstream json;
    json << "{"
         << "\"source\":\"zigbee\","
         << "\"payload\":\"" << payload << "\","
         << "\"lqi\":" << int(lqi) << ","
         << "\"rssi\":" << int(rssi)
         << "}";

    m_mqtt.publish("domotics/zigbee/rx", json.str());

    std::cout << "[BRIDGE] Radio → MQTT: " << payload << std::endl;
}

// ============================================================================
// Callback de mensaje MQTT
// ============================================================================

void MqttBridge::onMqttMessage(const std::string& topic, const std::string& payload)
{
    std::cout << "[MQTT↓] " << topic << ": " << payload << std::endl;

    // Parsear JSON simple (sin dependencias)
    auto extractField = [](const std::string& json, const std::string& key) -> std::string {
        auto pos = json.find("\"" + key + "\"");
        if (pos == std::string::npos) return "";
        pos = json.find(':', pos + key.size() + 2);
        if (pos == std::string::npos) return "";
        pos++; // saltar ':'
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
        if (pos >= json.size()) return "";
        if (json[pos] == '"') {
            pos++;
            auto end = json.find('"', pos);
            if (end == std::string::npos) return "";
            return json.substr(pos, end - pos);
        }
        // Valor numérico
        auto end = json.find_first_of(",}\n\r", pos);
        if (end == std::string::npos) end = json.size();
        return json.substr(pos, end - pos);
    };

    std::string deviceId = extractField(payload, "deviceId");
    std::string command  = extractField(payload, "command");
    std::string valStr   = extractField(payload, "value");

    if (deviceId.empty() && command.empty()) {
        // Intentar formato simplificado: topic/rgb/set "R,G,B"
        if (topic == "rgb/set") {
            processCommand("rgb_1", payload, 0);
        }
        return;
    }

    int value = 0;
    if (!valStr.empty()) {
        try { value = std::stoi(valStr); } catch (...) {}
    }

    processCommand(deviceId, command, value);
}

// ============================================================================
// Procesamiento de comandos
// ============================================================================

void MqttBridge::processCommand(const std::string& deviceId,
                                 const std::string& command, int value)
{
    std::cout << "[CMD] " << deviceId << ": " << command
              << (value > 0 ? " (" + std::to_string(value) + ")" : "")
              << std::endl;

    bool isOn = (command == "on");

    // Mapear dispositivos a GPIOs (configurable)
    if (deviceId.find("light") != std::string::npos) {
        setGpio(18, isOn);  // GPIO18 = LED
        if (deviceId.find("rgb") != std::string::npos) {
            // Control RGB (WS2812) - placeholder
            std::cout << "[RGB] Color: " << command << std::endl;
        }
    } else if (deviceId.find("fan") != std::string::npos) {
        setGpio(23, isOn);  // GPIO23 = Ventilador
    } else if (deviceId.find("lock") != std::string::npos) {
        setGpio(24, isOn);  // GPIO24 = Cerradura
        if (isOn) {
            // Auto-bloqueo después de 5 segundos (usar shared_ptr para safety)
            auto self = std::make_shared<MqttBridge*>(this);
            std::thread([self]() {
                std::this_thread::sleep_for(std::chrono::seconds(5));
                if (self && *self) {
                    (*self)->setGpio(24, false);
                    (*self)->publishStatus("lock_1", false, 0);
                }
            }).detach();
        }
    } else if (deviceId.find("curtain") != std::string::npos) {
        // Cortina: value = 0-100 (% abierto)
        if (value >= 0 && value <= 100) {
            std::cout << "[CURTAIN] Posición: " << value << "%" << std::endl;
            // Usar servo/PWM aquí
        }
    } else if (deviceId.find("temperature") != std::string::npos) {
        // Setpoint de temperatura
        std::cout << "[TEMP] Setpoint: " << value << "°C" << std::endl;
    }

    // Publicar nuevo estado
    publishStatus(deviceId, isOn, value);
}

// ============================================================================
// Discovery
// ============================================================================

void MqttBridge::publishDiscovery()
{
    std::cout << "[BRIDGE] Publicando estado inicial..." << std::endl;

    // Publicar estado de todos los dispositivos en "off" inicial
    publishStatus("light_1", false, 0);
    publishStatus("light_rgb", false, 0);
    publishStatus("fan_1", false, 0);
    publishStatus("lock_1", false, 0);
    publishStatus("curtain_1", false, 100);

    // Publicar estado de sensores (placeholder)
    publishSensor("temperature_1", 22.5f);
    publishSensor("energy_1", 150.0f);
}


