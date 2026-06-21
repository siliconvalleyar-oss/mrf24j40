/**
 * @file    mqtt_handler.hpp
 * @brief   Cliente MQTT basado en libmosquitto
 * @details Encapsula la API de libmosquitto (C) en una clase C++ moderna
 *          con soporte para reconexión automática, callbacks seguros,
 *          suscripción a topics y publicación de mensajes JSON.
 *
 * @author  MRF24J40 Team
 * @date    2026
 * @version 2.0.2
 */

#pragma once

#include <string>
#include <functional>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>
#include <mosquitto.h>

/**
 * @brief Manejador de conexión MQTT
 *
 * Encapsula un cliente libmosquitto con:
 * - Conexión y reconexión automática al broker
 * - Callbacks de mensajes, conexión y desconexión
 * - Publicación en topics (QoS 1)
 * - Loop en hilo separado
 * - Logging interno de eventos
 */
class MqttHandler {
public:
    /**
     * @brief Constructor
     * @param host     Dirección del broker MQTT
     * @param port     Puerto del broker (default 1883)
     * @param clientId Identificador único del cliente
     * @param username Usuario (opcional, vacío = sin auth)
     * @param password Contraseña (opcional)
     */
    MqttHandler(const std::string& host, int port = 1883,
                const std::string& clientId = "mrf24j40",
                const std::string& username = "",
                const std::string& password = "");

    /** @brief Destructor: desconecta y libera recursos */
    ~MqttHandler();

    // No copiable ni movible
    MqttHandler(const MqttHandler&) = delete;
    MqttHandler& operator=(const MqttHandler&) = delete;

    /**
     * @brief Inicia la conexión al broker
     * @return true si la conexión se inició correctamente
     */
    bool begin();

    /**
     * @brief Detiene la conexión y el hilo de loop
     */
    void stop();

    /**
     * @brief Publica un mensaje en un topic
     * @param topic   Topic MQTT (ej. "domotics/light/status")
     * @param payload Contenido del mensaje (usualmente JSON)
     * @param qos     Calidad de servicio (0, 1, 2)
     * @return true si se encoló correctamente
     */
    bool publish(const std::string& topic, const std::string& payload, int qos = 1);

    /**
     * @brief Publica un mensaje (raw bytes)
     * @param topic Topic MQTT
     * @param data  Puntero a datos
     * @param len   Longitud de datos
     * @param qos   Calidad de servicio
     * @return true si se encoló correctamente
     */
    bool publishRaw(const std::string& topic, const uint8_t* data, size_t len, int qos = 1);

    /**
     * @brief Suscribe a un topic
     * @param topic Topic a suscribir
     * @param qos   Calidad de servicio
     * @return true si la suscripción se encoló
     */
    bool subscribe(const std::string& topic, int qos = 1);

    /**
     * @brief Desuscribe de un topic
     * @param topic Topic a desuscribir
     * @return true si se encoló correctamente
     */
    bool unsubscribe(const std::string& topic);

    /** @brief Verifica si está conectado al broker */
    bool isConnected() const { return m_connected.load(); }

    /** @brief Obtiene el host del broker */
    const std::string& getHost() const { return m_host; }

    /** @brief Obtiene el puerto del broker */
    int getPort() const { return m_port; }

    // ========================================================================
    // Callbacks configurables
    // ========================================================================

    /** @brief Callback al recibir un mensaje: (topic, payload) */
    std::function<void(const std::string&, const std::string&)> onMessage;

    /** @brief Callback al conectar */
    std::function<void()> onConnect;

    /** @brief Callback al desconectar */
    std::function<void()> onDisconnect;

private:
    // ========================================================================
    // Callbacks estáticos de libmosquitto
    // ========================================================================
    static void onConnectCb(struct mosquitto* mosq, void* obj, int rc);
    static void onDisconnectCb(struct mosquitto* mosq, void* obj, int rc);
    static void onMessageCb(struct mosquitto* mosq, void* obj,
                            const struct mosquitto_message* msg);

    /** @brief Bucle interno que llama mosquitto_loop() */
    void loopWorker();

    // ========================================================================
    // Logging interno
    // ========================================================================
    void log(const std::string& msg);

    std::string              m_host;         /**< Host del broker */
    int                      m_port;         /**< Puerto del broker */
    std::string              m_clientId;     /**< ID del cliente */
    std::string              m_username;     /**< Usuario */
    std::string              m_password;     /**< Contraseña */

    struct mosquitto*        m_mosq{nullptr}; /**< Puntero al mosquitto nativo */
    std::atomic<bool>        m_connected{false}; /**< Flag de conexión */
    std::atomic<bool>        m_running{false};   /**< Flag de ejecución */
    std::unique_ptr<std::thread> m_loopThread;   /**< Hilo de loop */
    mutable std::mutex       m_mutex;        /**< Mutex para operaciones */
};
