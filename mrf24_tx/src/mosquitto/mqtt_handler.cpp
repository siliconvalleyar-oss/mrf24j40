/**
 * @file    mqtt_handler.cpp
 * @brief   Implementación del cliente MQTT
 * @details Implementa la clase MqttHandler con conexión, callbacks,
 *          publicación, suscripción y reconexión automática.
 *
 * @author  MRF24J40 Team
 * @date    2026
 * @version 2.0.2
 */

#include <mosquitto/mqtt_handler.hpp>
#include <iostream>
#include <cstring>
#include <mutex>

/** @brief Once flag para inicializar libmosquitto una sola vez (thread-safe) */
static std::once_flag g_mosquittoInitFlag;

// ============================================================================
// Constructor / Destructor
// ============================================================================

MqttHandler::MqttHandler(const std::string& host, int port,
                         const std::string& clientId,
                         const std::string& username,
                         const std::string& password)
    : m_host(host)
    , m_port(port)
    , m_clientId(clientId)
    , m_username(username)
    , m_password(password)
{
    // Inicializar librería mosquitto (thread-safe, una sola vez)
    std::call_once(g_mosquittoInitFlag, []{ mosquitto_lib_init(); });
}

MqttHandler::~MqttHandler()
{
    stop();
}

// ============================================================================
// Inicio / Parada
// ============================================================================

bool MqttHandler::begin()
{
    if (m_mosq) {
        log("MqttHandler: ya inicializado");
        return true;
    }

    // Crear instancia mosquitto
    m_mosq = mosquitto_new(m_clientId.c_str(), true, this);
    if (!m_mosq) {
        log("ERROR: no se pudo crear mosquitto instance");
        return false;
    }

    // Configurar callbacks
    mosquitto_connect_callback_set(m_mosq, onConnectCb);
    mosquitto_disconnect_callback_set(m_mosq, onDisconnectCb);
    mosquitto_message_callback_set(m_mosq, onMessageCb);

    // Autenticación opcional
    if (!m_username.empty()) {
        mosquitto_username_pw_set(m_mosq,
            m_username.c_str(),
            m_password.empty() ? nullptr : m_password.c_str());
    }

    // Configurar reconexión automática (5s, 30s max)
    mosquitto_reconnect_delay_set(m_mosq, 5, 30, true);

    // Conectar (asíncrono)
    int rc = mosquitto_connect_async(m_mosq, m_host.c_str(), m_port, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        log("ERROR: mosquitto_connect_async falló: " + std::to_string(rc));
        mosquitto_destroy(m_mosq);
        m_mosq = nullptr;
        return false;
    }

    // Iniciar hilo de loop
    m_running = true;
    m_loopThread = std::make_unique<std::thread>(&MqttHandler::loopWorker, this);

    log("MqttHandler: conexión iniciada a " + m_host + ":" + std::to_string(m_port));
    return true;
}

void MqttHandler::stop()
{
    m_running = false;

    if (m_loopThread && m_loopThread->joinable()) {
        m_loopThread->join();
        m_loopThread.reset();
    }

    if (m_mosq) {
        mosquitto_disconnect(m_mosq);
        mosquitto_destroy(m_mosq);
        m_mosq = nullptr;
    }

    m_connected = false;
    log("MqttHandler: detenido");
}

// ============================================================================
// Publicación
// ============================================================================

bool MqttHandler::publish(const std::string& topic, const std::string& payload, int qos)
{
    return publishRaw(topic,
        reinterpret_cast<const uint8_t*>(payload.data()),
        payload.size(), qos);
}

bool MqttHandler::publishRaw(const std::string& topic, const uint8_t* data,
                              size_t len, int qos)
{
    if (!m_mosq || !m_connected.load()) return false;

    std::lock_guard<std::mutex> lock(m_mutex);
    int rc = mosquitto_publish(m_mosq, nullptr, topic.c_str(),
                                len, data, qos, false);
    if (rc != MOSQ_ERR_SUCCESS) {
        log("ERROR: publish '" + topic + "' falló: " + std::to_string(rc));
        return false;
    }
    return true;
}

// ============================================================================
// Suscripción
// ============================================================================

bool MqttHandler::subscribe(const std::string& topic, int qos)
{
    if (!m_mosq) return false;

    std::lock_guard<std::mutex> lock(m_mutex);
    int rc = mosquitto_subscribe(m_mosq, nullptr, topic.c_str(), qos);
    if (rc != MOSQ_ERR_SUCCESS) {
        log("ERROR: subscribe '" + topic + "' falló: " + std::to_string(rc));
        return false;
    }
    log("Suscrito a: " + topic);
    return true;
}

bool MqttHandler::unsubscribe(const std::string& topic)
{
    if (!m_mosq) return false;

    std::lock_guard<std::mutex> lock(m_mutex);
    int rc = mosquitto_unsubscribe(m_mosq, nullptr, topic.c_str());
    if (rc != MOSQ_ERR_SUCCESS) {
        log("ERROR: unsubscribe '" + topic + "' falló: " + std::to_string(rc));
        return false;
    }
    log("Desuscrito de: " + topic);
    return true;
}

// ============================================================================
// Callbacks estáticos
// ============================================================================

void MqttHandler::onConnectCb(struct mosquitto* /*mosq*/, void* obj, int rc)
{
    auto* self = static_cast<MqttHandler*>(obj);
    if (rc == 0) {
        self->m_connected = true;
        self->log("Conectado al broker MQTT");
        if (self->onConnect) self->onConnect();
    } else {
        self->log("ERROR: conexión falló (rc=" + std::to_string(rc) + ")");
    }
}

void MqttHandler::onDisconnectCb(struct mosquitto* /*mosq*/, void* obj, int /*rc*/)
{
    auto* self = static_cast<MqttHandler*>(obj);
    self->m_connected = false;
    self->log("Desconectado del broker");
    if (self->onDisconnect) self->onDisconnect();
}

void MqttHandler::onMessageCb(struct mosquitto* /*mosq*/, void* obj,
                               const struct mosquitto_message* msg)
{
    auto* self = static_cast<MqttHandler*>(obj);
    if (!msg->payload || msg->payloadlen <= 0) return;

    std::string topic(msg->topic);
    std::string payload(static_cast<const char*>(msg->payload), msg->payloadlen);

    if (self->onMessage) {
        self->onMessage(topic, payload);
    }
}

// ============================================================================
// Loop interno
// ============================================================================

void MqttHandler::loopWorker()
{
    while (m_running.load()) {
        if (m_mosq) {
            mosquitto_loop(m_mosq, 50, 1);  // 50ms timeout, 1 socket
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

// ============================================================================
// Logging
// ============================================================================

void MqttHandler::log(const std::string& msg)
{
    std::cout << "[MQTT] " << msg << std::endl;
}
