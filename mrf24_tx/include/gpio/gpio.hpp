// gpio.hpp

#pragma once

#include <string_view> 
#include <fstream>     
#include <string>
#include <iostream>
#include <cstdint>

#ifdef LIBRARIES_BCM2835
#define IN_INTERRUPT    RPI_GPIO_P1_16  // GPIO 23
#define OUT_INTERRUPT   RPI_GPIO_P1_12  // GPIO 18
#else
#define IN_INTERRUPT    23      // GPIO INTERRUPT 
#define OUT_INTERRUPT   12      // GPIO LED DBG    
#endif

#define READING_STEPS   2     
#define SYSFS_GPIO_PATH "/sys/class/gpio"
#define SYSFS_GPIO_EXPORT_FN "/export"
#define SYSFS_GPIO_UNEXPORT_FN "/unexport"
#define SYSFS_GPIO_VALUE "/value"
#define SYSFS_GPIO_DIRECTION "/direction"
#define SYSFS_GPIO_EDGE "/edge"

#define DIR_IN  "in"
#define DIR_OUT "out"

#define VALUE_HIGH "1"
#define VALUE_LOW  "0"

#define EDGE_RISING   "rising"
#define EDGE_FALLING  "falling"
#define POLL_TIMEOUT   10*1000

namespace GPIO {

    struct Gpio_t {
        explicit Gpio_t(bool& st);
        ~Gpio_t();

        const bool app(bool&);
        
    protected:
        int file_open_and_write_value(const std::string_view, const std::string_view);
        int gpio_export(int);
        int gpio_unexport(int);
        int gpio_set_direction(int, const std::string_view);
        int gpio_set_value(int, const std::string_view);
        int gpio_set_edge(int, const std::string_view);
        int gpio_get_fd_to_value(int);
        bool settings(int, const std::string_view, std::ifstream&);
        void CloseGpios();
        void set();
        
    private:
        bool m_state{false};
        int m_gpio_in_fd{0};
        int m_res{0};
        const int m_gpio_out{OUT_INTERRUPT};
        const int m_gpio_in{IN_INTERRUPT};
        std::ifstream filenameGpio;
    };

}
