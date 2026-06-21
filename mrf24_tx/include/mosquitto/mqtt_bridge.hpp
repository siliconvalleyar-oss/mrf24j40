/**
 * @file    mqtt_bridge.hpp
 * @brief   Puente entre radio MRF24J40 y MQTT
 * @details Orquesta la comunicación entre los eventos de la radio ZigBee
 *          y los mensajes MQTT. Traduce paquetes de radio recibidos a
 *          topics de estado MQTT, y ejecuta comandos MQTT sobre GPIO.
 *
 * @author  MRF24J40 Team
 * @date    2026
 * @version 2.0.2
 */

#pragma once

#include <mosquitto/mqtt_handler.hpp>
#include <memory>
#include <string>
#include <map>
#include <mutex>

// Forward declarations
class Mrf24j40;

/**
 * @brief Puente bidireccional entre radio MRF24J40 y MQTT
 *
 * Funcionalidades:
 * - Suscribe a topics de comando (domotics/light, etc.)
 * - Publica estado en topics de estado (domotics/light/status)
 * - Traduce paquetes ZigBee recibidos a eventos MQTT
 * - Ejecuta comandos MQTT sobre GPIO (LEDs, cerradura, ventilador)
 * - Reconexión automática al broker
 */
class MqttBridge {
public:
    /**
     * @brief Constructor
     * @param handler Referencia al manejador MQTT
     * @param radio   Puntero opcional al driver MRF24J40
     */
    MqttBridge(MqttHandler& handler, Mrf24j40* radio = nullptr);

    /** @brief Destructor */
    ~MqttBridge();

    // ========================================================================
    // Inicialización
    // ========================================================================

    /**
     * @brief Inicializa el bridge: suscribe a topics de comando
     * @return true si las suscripciones se realizaron correctamente
     */
    bool begin();

    /** @brief Detiene el bridge */
    void stop();

    /**
     * @brief Publica el estado de un dispositivo
     * @param deviceId Identificador del dispositivo
     * @param isOn     Estado encendido/apagado
     * @param value    Valor numérico opcional (0-100, etc.)
     */
    void publishStatus(const std::string& deviceId, bool isOn, int value = 0);

    /**
     * @brief Publica un valor de sensor
     * @param deviceId Identificador del sensor
     * @param value    Valor del sensor
     */
    void publishSensor(const std::string& deviceId, float value);

    // ========================================================================
    // Control de GPIO
    // ========================================================================

    /** @brief Enciende/apaga un GPIO */
    void setGpio(int pin, bool state);

    /** @brief Obtiene el estado de un GPIO */
    bool getGpio(int pin);

    // ========================================================================
    // Estado
    // ========================================================================

    /** @brief Verifica si el bridge está activo */
    bool isActive() const { return m_active.load(); }

    /** @brief Activa/desactiva el bridge */
    void setActive(bool active) { m_active = active; }

    /**
     * @brief Procesa un paquete recibido por radio y lo traduce a MQTT
     * @param data   Datos del paquete
     * @param len    Longitud del paquete
     * @param lqi    Link Quality Indicator
     * @param rssi   RSSI en dBm
     */
    void onRadioPacket(const uint8_t* data, size_t len, uint8_t lqi, int8_t rssi);

private:
    // ========================================================================
    // Callbacks internos
    // ========================================================================

    /** @brief Callback de mensaje MQTT entrante */
    void onMqttMessage(const std::string& topic, const std::string& payload);

    /** @brief Procesa un comando MQTT y actúa sobre GPIO/radio */
    void processCommand(const std::string& deviceId, const std::string& command, int value);

    /** @brief Publica el estado inicial de todos los dispositivos */
    void publishDiscovery();

    // ========================================================================
    // Topics de comando para suscripción
    // ========================================================================
    std::vector<std::string> m_commandTopics = {
        "domotics/light",
        "domotics/temperature",
        "domotics/fan",
        "domotics/lock",
        "domotics/curtain",
        "domotics/energy",
        "domotics/rgb",
        "rgb/set"
    };

    // ========================================================================
    // Estado de dispositivos
    // ========================================================================
    struct DeviceState {
        bool isOn{false};
        int  value{0};
    };
    std::map<std::string, DeviceState> m_devices;
    std::mutex                         m_devMutex;

    MqttHandler&       m_mqtt;         /**< Referencia al handler MQTT */
    Mrf24j40*          m_radio{nullptr}; /**< Puntero al driver de radio */
    std::atomic<bool>  m_active{true};   /**< Bridge activo/inactivo */
};
