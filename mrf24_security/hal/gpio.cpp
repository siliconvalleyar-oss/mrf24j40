/**
 * @file    hal/gpio.cpp
 * @brief   Implementación del control de GPIO con BCM2835
 *
 * @author  MRF24J40 Team
 * @date    2026
 */

#include <hal/gpio.hpp>
#include <bcm2835.h>
#include <stdexcept>
#include <thread>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

namespace hal {

// ============================================================================
// Variables globales (contador de referencias para BCM2835)
// ============================================================================

static int g_bcm_refcount = 0;
static bool g_bcm_initialized = false;

// ============================================================================
// Funciones globales
// ============================================================================

bool gpioInit() {
    if (!g_bcm_initialized) {
        if (!bcm2835_init()) {
            return false;
        }
        g_bcm_initialized = true;
    }
    g_bcm_refcount++;
    return true;
}

void gpioClose() {
    if (g_bcm_refcount > 0) {
        g_bcm_refcount--;
    }
    if (g_bcm_refcount == 0 && g_bcm_initialized) {
        bcm2835_close();
        g_bcm_initialized = false;
    }
}

// ============================================================================
// Gpio_t
// ============================================================================

Gpio_t::Gpio_t(uint8_t pin, GpioDirection direction, GpioEdge edge)
    : m_pin(pin)
    , m_direction(direction)
    , m_fd(-1)
    , m_owned(true)
    , m_irq_running(false)
{
    initBcm2835();

    // Configurar dirección
    bcm2835_gpio_fsel(pin, (direction == GpioDirection::Output)
        ? BCM2835_GPIO_FSEL_OUTP
        : BCM2835_GPIO_FSEL_INPT);

    // Configurar pull-up/pull-down
    bcm2835_gpio_set_pud(pin, BCM2835_GPIO_PUD_OFF);

    // Valor inicial LOW para salidas
    if (direction == GpioDirection::Output) {
        bcm2835_gpio_write(pin, LOW);
    }
}

Gpio_t::~Gpio_t() {
    m_irq_running = false;
    if (m_irq_thread && m_irq_thread->joinable()) {
        m_irq_thread->join();
    }
    if (m_owned) {
        bcm2835_gpio_write(m_pin, LOW);
    }
}

Gpio_t::Gpio_t(Gpio_t&& other) noexcept
    : m_pin(other.m_pin)
    , m_direction(other.m_direction)
    , m_fd(other.m_fd)
    , m_owned(other.m_owned)
    , m_irq_running(false)
{
    other.m_owned = false;
    other.m_fd = -1;
}

Gpio_t& Gpio_t::operator=(Gpio_t&& other) noexcept {
    if (this != &other) {
        m_pin = other.m_pin;
        m_direction = other.m_direction;
        m_fd = other.m_fd;
        m_owned = other.m_owned;
        other.m_owned = false;
        other.m_fd = -1;
    }
    return *this;
}

void Gpio_t::write(GpioValue value) {
    if (m_direction != GpioDirection::Output) return;
    bcm2835_gpio_write(m_pin, (value == GpioValue::High) ? HIGH : LOW);
}

GpioValue Gpio_t::read() const {
    return (bcm2835_gpio_lev(m_pin) == HIGH) ? GpioValue::High : GpioValue::Low;
}

GpioValue Gpio_t::toggle() {
    GpioValue current = read();
    GpioValue next = (current == GpioValue::High) ? GpioValue::Low : GpioValue::High;
    write(next);
    return next;
}

void Gpio_t::onInterrupt(std::function<void()> callback) {
    if (!callback) return;

    // Usar sysfs para detección de flancos (BCM2835 no tiene API directa de IRQ)
    char path[64];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", m_pin);

    // Exportar si es necesario
    int exp_fd = open("/sys/class/gpio/export", O_WRONLY);
    if (exp_fd >= 0) {
        char pin_str[8];
        snprintf(pin_str, sizeof(pin_str), "%d", m_pin);
        write(exp_fd, pin_str, strlen(pin_str));
        close(exp_fd);
        usleep(100000); // Esperar a que sysfs cree los archivos
    }

    // Configurar dirección como entrada
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", m_pin);
    int dir_fd = open(path, O_WRONLY);
    if (dir_fd >= 0) {
        write(dir_fd, "in", 2);
        close(dir_fd);
    }

    // Configurar flanco
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/edge", m_pin);
    int edge_fd = open(path, O_WRONLY);
    if (edge_fd >= 0) {
        const char* edge_str = "both";
        if (m_direction == GpioDirection::Input) {
            // Configurar según el edge del constructor
        }
        write(edge_fd, edge_str, strlen(edge_str));
        close(edge_fd);
    }

    // Abrir value para poll()
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", m_pin);
    m_fd = open(path, O_RDONLY | O_NONBLOCK);
    if (m_fd < 0) return;

    // Iniciar hilo de monitoreo
    m_irq_running = true;
    m_irq_thread = std::make_unique<std::thread>([this, callback]() {
        struct pollfd pfd;
        pfd.fd = m_fd;
        pfd.events = POLLPRI;

        char buf[16];
        while (m_irq_running) {
            int ret = poll(&pfd, 1, 100); // timeout 100ms
            if (ret > 0 && (pfd.revents & POLLPRI)) {
                lseek(m_fd, 0, SEEK_SET);
                read(m_fd, buf, sizeof(buf));
                if (callback) callback();
            }
        }
    });
}

void Gpio_t::initBcm2835() {
    // Asegurar que BCM2835 esté inicializado
    if (!g_bcm_initialized) {
        if (!bcm2835_init()) {
            throw std::runtime_error("Failed to initialize BCM2835");
        }
        g_bcm_initialized = true;
        g_bcm_refcount++;
    }
}

} // namespace hal
