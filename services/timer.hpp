/**
 * @file    services/timer.hpp
 * @brief   Servicio de temporización con chrono de C++17
 * @details Proporciona delays, timeouts y medición de intervalos
 *          usando std::chrono de alto rendimiento.
 *
 * @author  MRF24J40 Team
 * @date    2026
 * @version 2.0.0
 */

#ifndef SERVICES_TIMER_HPP
#define SERVICES_TIMER_HPP

#include <chrono>
#include <thread>

namespace services {

/**
 * @brief Servicio de temporización de alta precisión.
 *
 * Uso:
 * @code
 * // Delay bloqueante
 * services::Timer_t::delayMs(100);
 *
 * // Timeout no bloqueante
 * auto t = services::Timer_t::now();
 * while (services::Timer_t::elapsedMs(t) < 5000) {
 *     poll();
 * }
 * @endcode
 */
class Timer_t {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    /**
     * @brief Obtiene el timestamp actual.
     */
    static TimePoint now() { return Clock::now(); }

    /**
     * @brief Delay bloqueante en milisegundos.
     * @param ms Milisegundos a esperar.
     */
    static void delayMs(uint32_t ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    /**
     * @brief Delay bloqueante en microsegundos.
     * @param us Microsegundos a esperar.
     */
    static void delayUs(uint32_t us) {
        std::this_thread::sleep_for(std::chrono::microseconds(us));
    }

    /**
     * @brief Milisegundos transcurridos desde @p start.
     * @param start Tiempo de inicio (obtenido con now()).
     * @return Milisegundos transcurridos.
     */
    static uint64_t elapsedMs(TimePoint start) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now() - start).count();
    }

    /**
     * @brief Microsegundos transcurridos desde @p start.
     */
    static uint64_t elapsedUs(TimePoint start) {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            Clock::now() - start).count();
    }

    /**
     * @brief Verifica si ha transcurrido un timeout.
     * @param start   Tiempo de inicio.
     * @param timeout_ms Timeout en milisegundos.
     * @return true si el timeout ha expirado.
     */
    static bool isTimeout(TimePoint start, uint32_t timeout_ms) {
        return elapsedMs(start) >= timeout_ms;
    }

    /**
     * @brief Constructor: registra el tiempo de inicio.
     */
    Timer_t() : m_start(now()) {}

    /**
     * @brief Reinicia el temporizador al tiempo actual.
     */
    void reset() { m_start = now(); }

    /**
     * @brief Milisegundos transcurridos desde la creación o último reset.
     */
    uint64_t elapsed() const {
        return elapsedMs(m_start);
    }

    /**
     * @brief Verifica si ha transcurrido un timeout desde el inicio.
     */
    bool timeout(uint32_t ms) const {
        return isTimeout(m_start, ms);
    }

private:
    TimePoint m_start;
};

} // namespace services

#endif // SERVICES_TIMER_HPP
