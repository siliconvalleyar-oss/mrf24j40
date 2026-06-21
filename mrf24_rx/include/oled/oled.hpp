#ifndef OLED_HPP
#define OLED_HPP

#include <string>
#include <cstdint>

#define OLED_ADDR 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_PAGES (OLED_HEIGHT / 8)

// Comandos SSD1306
#define OLED_CMD_DISPLAY_OFF 0xAE
#define OLED_CMD_DISPLAY_ON 0xAF
#define OLED_CMD_SET_DISPLAY_CLK_DIV 0xD5
#define OLED_CMD_SET_MULTIPLEX 0xA8
#define OLED_CMD_SET_DISPLAY_OFFSET 0xD3
#define OLED_CMD_SET_START_LINE 0x40
#define OLED_CMD_CHARGE_PUMP 0x8D
#define OLED_CMD_MEMORY_MODE 0x20
#define OLED_CMD_SET_COM_PINS 0xDA
#define OLED_CMD_SET_CONTRAST 0x81
#define OLED_CMD_SET_PRECHARGE 0xD9
#define OLED_CMD_SET_VCOM_DETECT 0xDB
#define OLED_CMD_DISPLAY_ALL_ON_RESUME 0xA4
#define OLED_CMD_NORMAL_DISPLAY 0xA6
#define OLED_CMD_INVERT_DISPLAY 0xA7
#define OLED_CMD_DEACTIVATE_SCROLL 0x2E
#define OLED_CMD_COLUMN_ADDR 0x21
#define OLED_CMD_PAGE_ADDR 0x22

class OLED {
private:
    int i2c_fd;
    uint8_t buffer[OLED_WIDTH * OLED_PAGES];
    static const uint8_t font5x7[][5];

    void write_command(uint8_t cmd);
    void write_command(uint8_t cmd, uint8_t data);
    void write_data(uint8_t data);

public:
    OLED();
    ~OLED();

    bool init();
    void clear();
    void update();
    
    void draw_pixel(uint8_t x, uint8_t y, bool color = true);
    void draw_char(uint8_t x, uint8_t y, char c, uint8_t size = 1, bool color = true);
    void draw_string(uint8_t x, uint8_t y, const char* str, uint8_t size = 1, bool color = true);
    void draw_string(uint8_t x, uint8_t y, const std::string& str, uint8_t size = 1, bool color = true);
    
    void set_contrast(uint8_t contrast);
    void set_power(bool on);
    void showInitScreen();
};

#endif
