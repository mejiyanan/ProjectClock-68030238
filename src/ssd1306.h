#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/i2c.h"

#define SSD1306_WIDTH   128
#define SSD1306_HEIGHT  64
#define SSD1306_BUFFER_SIZE (SSD1306_WIDTH * SSD1306_HEIGHT / 8) // 1024 bytes

typedef struct {
    uint8_t _address;
    uint8_t _width;
    uint8_t _height;
    uint8_t buffer[SSD1306_BUFFER_SIZE];
} SSD1306_t;

// Driver & Screen Management
void ssd1306_init(SSD1306_t *dev, int sda_pin, int scl_pin);
void ssd1306_clear(SSD1306_t *dev);
void ssd1306_update(SSD1306_t *dev);
void ssd1306_contrast(SSD1306_t *dev, uint8_t contrast);

// Core 2D Graphics Primitives
void ssd1306_set_pixel(SSD1306_t *dev, int16_t x, int16_t y, uint8_t color);
uint8_t ssd1306_get_pixel(SSD1306_t *dev, int16_t x, int16_t y);
void ssd1306_draw_line(SSD1306_t *dev, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color);
void ssd1306_draw_fast_hline(SSD1306_t *dev, int16_t x, int16_t y, int16_t w, uint8_t color);
void ssd1306_draw_fast_vline(SSD1306_t *dev, int16_t x, int16_t y, int16_t h, uint8_t color);
void ssd1306_draw_rect(SSD1306_t *dev, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
void ssd1306_fill_rect(SSD1306_t *dev, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
void ssd1306_draw_round_rect(SSD1306_t *dev, int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint8_t color);
void ssd1306_draw_bitmap(SSD1306_t *dev, int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint8_t color);

// Text & Font Rendering
void ssd1306_draw_char_5x7(SSD1306_t *dev, int16_t x, int16_t y, char c, uint8_t color);
void ssd1306_draw_string_5x7(SSD1306_t *dev, int16_t x, int16_t y, const char *str, uint8_t color);
int16_t ssd1306_draw_medium_char(SSD1306_t *dev, int16_t x, int16_t y, char c, uint8_t color);
void ssd1306_draw_medium_string(SSD1306_t *dev, int16_t x, int16_t y, const char *str, uint8_t color);
int16_t ssd1306_draw_large_digit(SSD1306_t *dev, int16_t x, int16_t y, char c, uint8_t color);

// Specialized Retro UI Components Matching User Reference
void ui_draw_outer_frame(SSD1306_t *dev);
void ui_draw_wifi_icon(SSD1306_t *dev, int16_t x, int16_t y, bool connected);
void ui_draw_battery_icon(SSD1306_t *dev, int16_t x, int16_t y, uint8_t level);
void ui_draw_star(SSD1306_t *dev, int16_t cx, int16_t cy, uint8_t size);
void ui_draw_thermo_icon(SSD1306_t *dev, int16_t x, int16_t y);
void ui_draw_drop_icon(SSD1306_t *dev, int16_t x, int16_t y);
void ui_draw_calendar_icon(SSD1306_t *dev, int16_t x, int16_t y);
void ui_draw_stopwatch_icon(SSD1306_t *dev, int16_t x, int16_t y);
void ui_draw_wavy_bottom(SSD1306_t *dev);

#endif // SSD1306_H
