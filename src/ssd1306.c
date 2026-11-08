#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "esp_log.h"
#include "ssd1306.h"

#define OLED_I2C_ADDRESS 0x3C

// ================= Standard 5x7 ASCII Font Table (32 to 126) =================
static const uint8_t font5x7_data[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // 32 Space
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // 33 !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // 34 "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // 35 #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // 36 $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // 37 %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // 38 &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // 39 '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // 40 (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // 41 )
    {0x08, 0x2A, 0x1C, 0x2A, 0x08}, // 42 *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // 43 +
    {0x00, 0x50, 0x30, 0x00, 0x00}, // 44 ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // 45 -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // 46 .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // 47 /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 48 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 49 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 50 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 51 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 52 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 53 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 54 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 55 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 56 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 57 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // 58 :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // 59 ;
    {0x00, 0x08, 0x14, 0x22, 0x41}, // 60 <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // 61 =
    {0x41, 0x22, 0x14, 0x08, 0x00}, // 62 >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // 63 ?
    {0x32, 0x49, 0x79, 0x41, 0x3E}, // 64 @
    {0x7E, 0x11, 0x11, 0x11, 0x7E}, // 65 A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // 66 B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // 67 C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // 68 D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // 69 E
    {0x7F, 0x09, 0x09, 0x01, 0x01}, // 70 F
    {0x3E, 0x41, 0x41, 0x51, 0x32}, // 71 G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // 72 H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // 73 I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // 74 J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // 75 K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // 76 L
    {0x7F, 0x02, 0x04, 0x02, 0x7F}, // 77 M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // 78 N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 79 O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // 80 P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // 81 Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // 82 R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // 83 S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // 84 T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // 85 U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // 86 V
    {0x7F, 0x20, 0x18, 0x20, 0x7F}, // 87 W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // 88 X
    {0x03, 0x04, 0x78, 0x04, 0x03}, // 89 Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // 90 Z
    {0x00, 0x7F, 0x41, 0x41, 0x00}, // 91 [
    {0x02, 0x04, 0x08, 0x10, 0x20}, // 92 backslash
    {0x00, 0x41, 0x41, 0x7F, 0x00}, // 93 ]
    {0x04, 0x02, 0x01, 0x02, 0x04}, // 94 ^
    {0x40, 0x40, 0x40, 0x40, 0x40}, // 95 _
    {0x00, 0x01, 0x02, 0x04, 0x00}, // 96 `
    {0x20, 0x54, 0x54, 0x54, 0x78}, // 97 a
    {0x7F, 0x48, 0x44, 0x44, 0x38}, // 98 b
    {0x38, 0x44, 0x44, 0x44, 0x20}, // 99 c
    {0x38, 0x44, 0x44, 0x48, 0x7F}, // 100 d
    {0x38, 0x54, 0x54, 0x54, 0x18}, // 101 e
    {0x08, 0x7E, 0x09, 0x01, 0x02}, // 102 f
    {0x08, 0x14, 0x54, 0x54, 0x3C}, // 103 g
    {0x7F, 0x08, 0x04, 0x04, 0x78}, // 104 h
    {0x00, 0x44, 0x7D, 0x40, 0x00}, // 105 i
    {0x20, 0x40, 0x44, 0x3D, 0x00}, // 106 j
    {0x7F, 0x10, 0x28, 0x44, 0x00}, // 107 k
    {0x00, 0x41, 0x7F, 0x40, 0x00}, // 108 l
    {0x7C, 0x04, 0x18, 0x04, 0x78}, // 109 m
    {0x7C, 0x08, 0x04, 0x04, 0x78}, // 110 n
    {0x38, 0x44, 0x44, 0x44, 0x38}, // 111 o
    {0x7C, 0x14, 0x14, 0x14, 0x08}, // 112 p
    {0x08, 0x14, 0x14, 0x18, 0x7C}, // 113 q
    {0x7C, 0x08, 0x04, 0x04, 0x08}, // 114 r
    {0x48, 0x54, 0x54, 0x54, 0x20}, // 115 s
    {0x04, 0x3F, 0x44, 0x40, 0x20}, // 116 t
    {0x3C, 0x40, 0x40, 0x20, 0x7C}, // 117 u
    {0x1C, 0x20, 0x40, 0x20, 0x1C}, // 118 v
    {0x3C, 0x40, 0x30, 0x40, 0x3C}, // 119 w
    {0x44, 0x28, 0x10, 0x28, 0x44}, // 120 x
    {0x0C, 0x50, 0x50, 0x50, 0x3C}, // 121 y
    {0x44, 0x64, 0x54, 0x4C, 0x44}, // 122 z
    {0x00, 0x08, 0x36, 0x41, 0x00}, // 123 {
    {0x00, 0x00, 0x7F, 0x00, 0x00}, // 124 |
    {0x00, 0x41, 0x36, 0x08, 0x00}, // 125 }
    {0x08, 0x04, 0x08, 0x10, 0x08}, // 126 ~
};

// ================= 8x12 Medium Font (for Date & Sensor) =================
// 12 rows of 8 bits each
static const uint8_t font_medium_0_9[10][12] = {
    // '0'
    {0x3C, 0x7E, 0xE7, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xE7, 0x7E, 0x3C},
    // '1'
    {0x18, 0x38, 0x78, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x7E},
    // '2'
    {0x3C, 0x7E, 0xC3, 0x03, 0x07, 0x0E, 0x1C, 0x38, 0x70, 0xE0, 0xFF, 0xFF},
    // '3'
    {0x3C, 0x7E, 0xC3, 0x03, 0x03, 0x1E, 0x1E, 0x03, 0x03, 0xC3, 0x7E, 0x3C},
    // '4'
    {0x06, 0x0E, 0x1E, 0x36, 0x66, 0xC6, 0xFF, 0xFF, 0x06, 0x06, 0x0F, 0x0F},
    // '5'
    {0xFF, 0xFF, 0xC0, 0xC0, 0xFE, 0x7F, 0x03, 0x03, 0x03, 0xC3, 0x7E, 0x3C},
    // '6'
    {0x3C, 0x7E, 0xC3, 0xC0, 0xC0, 0xFE, 0xFF, 0xC3, 0xC3, 0xC3, 0x7E, 0x3C},
    // '7'
    {0xFF, 0xFF, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x30, 0x60, 0x60, 0x60, 0x60},
    // '8'
    {0x3C, 0x7E, 0xC3, 0xC3, 0x7E, 0x3C, 0x7E, 0xC3, 0xC3, 0xC3, 0x7E, 0x3C},
    // '9'
    {0x3C, 0x7E, 0xC3, 0xC3, 0xC3, 0xFF, 0x7F, 0x03, 0x03, 0xC3, 0x7E, 0x3C},
};

// ================= 14x19 Large Font (for Time & Stopwatch) =================
// 19 rows of 16-bit integers
static const uint16_t font_large_digits[10][19] = {
    // 0
    {0x0FF0, 0x1FF8, 0x3FFC, 0x3C3C, 0x3C3C, 0x3C3C, 0x3C3C, 0x3C3C, 0x3C3C, 0x3C3C,
     0x3C3C, 0x3C3C, 0x3C3C, 0x3C3C, 0x3C3C, 0x3FFC, 0x1FF8, 0x0FF0, 0x0000},
    // 1
    {0x0180, 0x0380, 0x0780, 0x0F80, 0x1F80, 0x0180, 0x0180, 0x0180, 0x0180, 0x0180,
     0x0180, 0x0180, 0x0180, 0x0180, 0x0180, 0x0FF8, 0x0FF8, 0x0FF8, 0x0000},
    // 2
    {0x0FF0, 0x1FF8, 0x3FFC, 0x3C3C, 0x003C, 0x003C, 0x0078, 0x00F0, 0x01E0, 0x03C0,
     0x0780, 0x0F00, 0x1E00, 0x3C00, 0x3FFF, 0x3FFF, 0x3FFF, 0x3FFF, 0x0000},
    // 3
    {0x0FF0, 0x1FF8, 0x3FFC, 0x3C3C, 0x003C, 0x0078, 0x01F0, 0x0FE0, 0x0FE0, 0x01F0,
     0x0078, 0x003C, 0x003C, 0x3C3C, 0x3FFC, 0x1FF8, 0x0FF0, 0x0000, 0x0000},
    // 4
    {0x0038, 0x0078, 0x00F8, 0x01F8, 0x03B8, 0x0738, 0x0E38, 0x1C38, 0x3838, 0x3FFF,
     0x3FFF, 0x3FFF, 0x0038, 0x0038, 0x0038, 0x007C, 0x007C, 0x0000, 0x0000},
    // 5
    {0x3FFF, 0x3FFF, 0x3FFF, 0x3C00, 0x3C00, 0x3FE0, 0x3FF8, 0x3FFC, 0x003C, 0x003C,
     0x003C, 0x003C, 0x3C3C, 0x3FFC, 0x1FF8, 0x0FF0, 0x0000, 0x0000, 0x0000},
    // 6
    {0x07F0, 0x1FF8, 0x3FFC, 0x3C00, 0x3C00, 0x3FE0, 0x3FF8, 0x3C3C, 0x3C3C, 0x3C3C,
     0x3C3C, 0x3C3C, 0x3C3C, 0x3FFC, 0x1FF8, 0x0FF0, 0x0000, 0x0000, 0x0000},
    // 7
    {0x3FFF, 0x3FFF, 0x3FFF, 0x003C, 0x0078, 0x00F0, 0x01E0, 0x03C0, 0x0780, 0x0F00,
     0x0F00, 0x1E00, 0x1E00, 0x3C00, 0x3C00, 0x3C00, 0x0000, 0x0000, 0x0000},
    // 8
    {0x0FF0, 0x1FF8, 0x3FFC, 0x3C3C, 0x3C3C, 0x1FF8, 0x0FF0, 0x1FF8, 0x3FFC, 0x3C3C,
     0x3C3C, 0x3C3C, 0x3C3C, 0x3FFC, 0x1FF8, 0x0FF0, 0x0000, 0x0000, 0x0000},
    // 9
    {0x0FF0, 0x1FF8, 0x3FFC, 0x3C3C, 0x3C3C, 0x3C3C, 0x3FFC, 0x1FFC, 0x003C, 0x003C,
     0x003C, 0x003C, 0x3C3C, 0x3FFC, 0x1FF8, 0x0FF0, 0x0000, 0x0000, 0x0000},
};

static esp_err_t ssd1306_send_cmd(SSD1306_t *dev, uint8_t cmd) {
    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, (dev->_address << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd_handle, 0x00, true); // Control byte: Command
    i2c_master_write_byte(cmd_handle, cmd, true);
    i2c_master_stop(cmd_handle);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd_handle, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd_handle);
    return ret;
}

void ssd1306_init(SSD1306_t *dev, int sda_pin, int scl_pin) {
    dev->_address = OLED_I2C_ADDRESS;
    dev->_width = SSD1306_WIDTH;
    dev->_height = SSD1306_HEIGHT;
    memset(dev->buffer, 0, SSD1306_BUFFER_SIZE);

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = scl_pin,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);

    // SSD1306 Initialization sequence
    ssd1306_send_cmd(dev, 0xAE); // Display OFF
    ssd1306_send_cmd(dev, 0xD5); ssd1306_send_cmd(dev, 0x80); // Clock divide ratio
    ssd1306_send_cmd(dev, 0xA8); ssd1306_send_cmd(dev, 0x3F); // Multiplex (64 - 1)
    ssd1306_send_cmd(dev, 0xD3); ssd1306_send_cmd(dev, 0x00); // Display offset 0
    ssd1306_send_cmd(dev, 0x40); // Start line 0
    ssd1306_send_cmd(dev, 0x8D); ssd1306_send_cmd(dev, 0x14); // Enable charge pump
    ssd1306_send_cmd(dev, 0x20); ssd1306_send_cmd(dev, 0x00); // Horizontal addressing mode
    ssd1306_send_cmd(dev, 0xA1); // Segment remap
    ssd1306_send_cmd(dev, 0xC8); // COM scan remap
    ssd1306_send_cmd(dev, 0xDA); ssd1306_send_cmd(dev, 0x12); // COM pins config
    ssd1306_send_cmd(dev, 0x81); ssd1306_send_cmd(dev, 0xCF); // Contrast
    ssd1306_send_cmd(dev, 0xD9); ssd1306_send_cmd(dev, 0xF1); // Pre-charge
    ssd1306_send_cmd(dev, 0xDB); ssd1306_send_cmd(dev, 0x40); // VCOMH deselect
    ssd1306_send_cmd(dev, 0xA4); // Output follows RAM
    ssd1306_send_cmd(dev, 0xA6); // Normal display (not inverted)
    ssd1306_send_cmd(dev, 0xAF); // Display ON

    ssd1306_clear(dev);
    ssd1306_update(dev);
}

void ssd1306_clear(SSD1306_t *dev) {
    memset(dev->buffer, 0x00, SSD1306_BUFFER_SIZE);
}

void ssd1306_update(SSD1306_t *dev) {
    // Set column address range 0..127
    ssd1306_send_cmd(dev, 0x21);
    ssd1306_send_cmd(dev, 0x00);
    ssd1306_send_cmd(dev, 0x7F);

    // Set page address range 0..7
    ssd1306_send_cmd(dev, 0x22);
    ssd1306_send_cmd(dev, 0x00);
    ssd1306_send_cmd(dev, 0x07);

    // Stream the 1024 bytes in chunks of 64 bytes via I2C
    for (int offset = 0; offset < SSD1306_BUFFER_SIZE; offset += 64) {
        i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
        i2c_master_start(cmd_handle);
        i2c_master_write_byte(cmd_handle, (dev->_address << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write_byte(cmd_handle, 0x40, true); // Control byte: Data
        i2c_master_write(cmd_handle, &dev->buffer[offset], 64, true);
        i2c_master_stop(cmd_handle);
        i2c_master_cmd_begin(I2C_NUM_0, cmd_handle, pdMS_TO_TICKS(100));
        i2c_cmd_link_delete(cmd_handle);
    }
}

void ssd1306_contrast(SSD1306_t *dev, uint8_t contrast) {
    ssd1306_send_cmd(dev, 0x81);
    ssd1306_send_cmd(dev, contrast);
}

void ssd1306_set_pixel(SSD1306_t *dev, int16_t x, int16_t y, uint8_t color) {
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) return;
    int index = x + (y / 8) * SSD1306_WIDTH;
    if (color) {
        dev->buffer[index] |= (1 << (y % 8));
    } else {
        dev->buffer[index] &= ~(1 << (y % 8));
    }
}

uint8_t ssd1306_get_pixel(SSD1306_t *dev, int16_t x, int16_t y) {
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) return 0;
    int index = x + (y / 8) * SSD1306_WIDTH;
    return (dev->buffer[index] & (1 << (y % 8))) ? 1 : 0;
}

void ssd1306_draw_line(SSD1306_t *dev, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color) {
    int16_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int16_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy, e2;
    while (1) {
        ssd1306_set_pixel(dev, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void ssd1306_draw_fast_hline(SSD1306_t *dev, int16_t x, int16_t y, int16_t w, uint8_t color) {
    for (int16_t i = x; i < x + w; i++) {
        ssd1306_set_pixel(dev, i, y, color);
    }
}

void ssd1306_draw_fast_vline(SSD1306_t *dev, int16_t x, int16_t y, int16_t h, uint8_t color) {
    for (int16_t i = y; i < y + h; i++) {
        ssd1306_set_pixel(dev, x, i, color);
    }
}

void ssd1306_draw_rect(SSD1306_t *dev, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) {
    ssd1306_draw_fast_hline(dev, x, y, w, color);
    ssd1306_draw_fast_hline(dev, x, y + h - 1, w, color);
    ssd1306_draw_fast_vline(dev, x, y, h, color);
    ssd1306_draw_fast_vline(dev, x + w - 1, y, h, color);
}

void ssd1306_fill_rect(SSD1306_t *dev, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color) {
    for (int16_t i = x; i < x + w; i++) {
        for (int16_t j = y; j < y + h; j++) {
            ssd1306_set_pixel(dev, i, j, color);
        }
    }
}

void ssd1306_draw_round_rect(SSD1306_t *dev, int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint8_t color) {
    ssd1306_draw_fast_hline(dev, x + r, y, w - 2 * r, color);
    ssd1306_draw_fast_hline(dev, x + r, y + h - 1, w - 2 * r, color);
    ssd1306_draw_fast_vline(dev, x, y + r, h - 2 * r, color);
    ssd1306_draw_fast_vline(dev, x + w - 1, y + r, h - 2 * r, color);
    // Simple 2-pixel corner roundness
    ssd1306_set_pixel(dev, x + 1, y + 1, color);
    ssd1306_set_pixel(dev, x + w - 2, y + 1, color);
    ssd1306_set_pixel(dev, x + 1, y + h - 2, color);
    ssd1306_set_pixel(dev, x + w - 2, y + h - 2, color);
}

void ssd1306_draw_bitmap(SSD1306_t *dev, int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h, uint8_t color) {
    for (int16_t j = 0; j < h; j++) {
        for (int16_t i = 0; i < w; i++) {
            int byte_idx = (j * ((w + 7) / 8)) + (i / 8);
            int bit_idx = 7 - (i % 8);
            if ((bitmap[byte_idx] >> bit_idx) & 1) {
                ssd1306_set_pixel(dev, x + i, y + j, color);
            }
        }
    }
}

void ssd1306_draw_char_5x7(SSD1306_t *dev, int16_t x, int16_t y, char c, uint8_t color) {
    // Support degree symbol mapping
    if ((uint8_t)c == 0xB0 || (uint8_t)c == 0xDF) {
        // Degree symbol circle 3x3
        ssd1306_set_pixel(dev, x + 1, y, color);
        ssd1306_set_pixel(dev, x, y + 1, color);
        ssd1306_set_pixel(dev, x + 2, y + 1, color);
        ssd1306_set_pixel(dev, x + 1, y + 2, color);
        return;
    }
    if (c < 32 || c > 126) c = ' ';
    const uint8_t *glyph = font5x7_data[(int)(c - 32)];
    for (int col = 0; col < 5; col++) {
        uint8_t bits = glyph[col];
        for (int row = 0; row < 7; row++) {
            if ((bits >> row) & 1) {
                ssd1306_set_pixel(dev, x + col, y + row, color);
            }
        }
    }
}

void ssd1306_draw_string_5x7(SSD1306_t *dev, int16_t x, int16_t y, const char *str, uint8_t color) {
    int16_t cur_x = x;
    while (*str) {
        if ((uint8_t)*str == 0xC2 || (uint8_t)*str == 0xC3) {
            // Skip UTF-8 prefix byte
            str++;
            continue;
        }
        ssd1306_draw_char_5x7(dev, cur_x, y, *str, color);
        cur_x += 6;
        str++;
    }
}

int16_t ssd1306_draw_medium_char(SSD1306_t *dev, int16_t x, int16_t y, char c, uint8_t color) {
    if (c >= '0' && c <= '9') {
        const uint8_t *glyph = font_medium_0_9[c - '0'];
        for (int row = 0; row < 12; row++) {
            uint8_t bits = glyph[row];
            for (int col = 0; col < 8; col++) {
                if ((bits >> (7 - col)) & 1) {
                    ssd1306_set_pixel(dev, x + col, y + row, color);
                }
            }
        }
        return 8;
    } else if (c == '.') {
        // Dot at bottom
        ssd1306_set_pixel(dev, x + 1, y + 10, color);
        ssd1306_set_pixel(dev, x + 2, y + 10, color);
        ssd1306_set_pixel(dev, x + 1, y + 11, color);
        ssd1306_set_pixel(dev, x + 2, y + 11, color);
        return 4;
    } else if (c == ':' || c == ' ') {
        return 5;
    } else {
        // Fallback to double-size 5x7
        for (int col = 0; col < 5; col++) {
            uint8_t bits = font5x7_data[(c >= 32 && c <= 126) ? (c - 32) : 0][col];
            for (int row = 0; row < 7; row++) {
                if ((bits >> row) & 1) {
                    ssd1306_set_pixel(dev, x + col, y + row + 2, color);
                    ssd1306_set_pixel(dev, x + col + 1, y + row + 2, color);
                }
            }
        }
        return 7;
    }
}

void ssd1306_draw_medium_string(SSD1306_t *dev, int16_t x, int16_t y, const char *str, uint8_t color) {
    int16_t cur_x = x;
    while (*str) {
        if ((uint8_t)*str == 0xC2 || (uint8_t)*str == 0xC3) { str++; continue; }
        cur_x += ssd1306_draw_medium_char(dev, cur_x, y, *str, color);
        str++;
    }
}

int16_t ssd1306_draw_large_digit(SSD1306_t *dev, int16_t x, int16_t y, char c, uint8_t color) {
    if (c >= '0' && c <= '9') {
        const uint16_t *glyph = font_large_digits[c - '0'];
        for (int row = 0; row < 19; row++) {
            uint16_t bits = glyph[row];
            for (int col = 0; col < 14; col++) {
                if ((bits >> (13 - col)) & 1) {
                    ssd1306_set_pixel(dev, x + col, y + row, color);
                }
            }
        }
        return 14;
    } else if (c == ':') {
        // Colon: two 4x4 squares
        ssd1306_fill_rect(dev, x + 1, y + 4, 3, 3, color);
        ssd1306_fill_rect(dev, x + 1, y + 12, 3, 3, color);
        return 6;
    }
    return 10;
}

// ================= Specialized Retro UI Components =================

void ui_draw_outer_frame(SSD1306_t *dev) {
    // Outer rounded rectangle
    ssd1306_draw_fast_hline(dev, 6, 2, 116, 1);
    ssd1306_draw_fast_hline(dev, 6, 61, 116, 1);
    ssd1306_draw_fast_vline(dev, 2, 6, 52, 1);
    ssd1306_draw_fast_vline(dev, 125, 6, 52, 1);

    // Corner curves
    ssd1306_set_pixel(dev, 3, 5, 1); ssd1306_set_pixel(dev, 3, 4, 1);
    ssd1306_set_pixel(dev, 4, 3, 1); ssd1306_set_pixel(dev, 5, 3, 1);

    ssd1306_set_pixel(dev, 124, 5, 1); ssd1306_set_pixel(dev, 124, 4, 1);
    ssd1306_set_pixel(dev, 123, 3, 1); ssd1306_set_pixel(dev, 122, 3, 1);

    ssd1306_set_pixel(dev, 3, 58, 1); ssd1306_set_pixel(dev, 3, 59, 1);
    ssd1306_set_pixel(dev, 4, 60, 1); ssd1306_set_pixel(dev, 5, 60, 1);

    ssd1306_set_pixel(dev, 124, 58, 1); ssd1306_set_pixel(dev, 124, 59, 1);
    ssd1306_set_pixel(dev, 123, 60, 1); ssd1306_set_pixel(dev, 122, 60, 1);

    // 3-dot diagonal step in each corner (2x2 square pixels)
    // Bottom-Left
    for (int i = 0; i < 3; i++) {
        int dx = 5 + i * 3, dy = 55 + i * 2;
        ssd1306_set_pixel(dev, dx, dy, 1); ssd1306_set_pixel(dev, dx + 1, dy, 1);
        ssd1306_set_pixel(dev, dx, dy + 1, 1); ssd1306_set_pixel(dev, dx + 1, dy + 1, 1);
    }
    // Bottom-Right
    for (int i = 0; i < 3; i++) {
        int dx = 121 - i * 3, dy = 55 + i * 2;
        ssd1306_set_pixel(dev, dx, dy, 1); ssd1306_set_pixel(dev, dx - 1, dy, 1);
        ssd1306_set_pixel(dev, dx, dy + 1, 1); ssd1306_set_pixel(dev, dx - 1, dy + 1, 1);
    }
    // Top-Left
    for (int i = 0; i < 3; i++) {
        int dx = 5 + i * 3, dy = 7 - i * 2;
        ssd1306_set_pixel(dev, dx, dy, 1); ssd1306_set_pixel(dev, dx + 1, dy, 1);
        ssd1306_set_pixel(dev, dx, dy + 1, 1); ssd1306_set_pixel(dev, dx + 1, dy + 1, 1);
    }
    // Top-Right
    for (int i = 0; i < 3; i++) {
        int dx = 121 - i * 3, dy = 7 - i * 2;
        ssd1306_set_pixel(dev, dx, dy, 1); ssd1306_set_pixel(dev, dx - 1, dy, 1);
        ssd1306_set_pixel(dev, dx, dy + 1, 1); ssd1306_set_pixel(dev, dx - 1, dy + 1, 1);
    }
}

void ui_draw_wifi_icon(SSD1306_t *dev, int16_t x, int16_t y, bool connected) {
    if (connected) {
        // Outer arc
        ssd1306_draw_fast_hline(dev, x + 2, y, 6, 1);
        ssd1306_set_pixel(dev, x + 1, y + 1, 1); ssd1306_set_pixel(dev, x + 8, y + 1, 1);
        // Middle arc
        ssd1306_draw_fast_hline(dev, x + 3, y + 2, 4, 1);
        ssd1306_set_pixel(dev, x + 2, y + 3, 1); ssd1306_set_pixel(dev, x + 7, y + 3, 1);
        // Center dot
        ssd1306_set_pixel(dev, x + 4, y + 5, 1); ssd1306_set_pixel(dev, x + 5, y + 5, 1);
        ssd1306_set_pixel(dev, x + 4, y + 6, 1); ssd1306_set_pixel(dev, x + 5, y + 6, 1);
    } else {
        // Disconnected: base dot + slash
        ssd1306_set_pixel(dev, x + 4, y + 5, 1); ssd1306_set_pixel(dev, x + 5, y + 5, 1);
        ssd1306_draw_line(dev, x + 1, y + 6, x + 8, y, 1);
    }
}

void ui_draw_battery_icon(SSD1306_t *dev, int16_t x, int16_t y, uint8_t level) {
    // Battery outer frame
    ssd1306_draw_rect(dev, x, y, 12, 7, 1);
    // Terminal nub on right
    ssd1306_draw_fast_vline(dev, x + 12, y + 2, 3, 1);
    // Inner fill bars
    if (level >= 1) ssd1306_draw_fast_vline(dev, x + 2, y + 2, 3, 1);
    if (level >= 1) ssd1306_draw_fast_vline(dev, x + 3, y + 2, 3, 1);
    if (level >= 2) ssd1306_draw_fast_vline(dev, x + 5, y + 2, 3, 1);
    if (level >= 2) ssd1306_draw_fast_vline(dev, x + 6, y + 2, 3, 1);
    if (level >= 3) ssd1306_draw_fast_vline(dev, x + 8, y + 2, 3, 1);
    if (level >= 3) ssd1306_draw_fast_vline(dev, x + 9, y + 2, 3, 1);
}

void ui_draw_star(SSD1306_t *dev, int16_t cx, int16_t cy, uint8_t size) {
    if (size >= 7) {
        for (int r = -3; r <= 3; r++) {
            int w = 7 - 2 * abs(r);
            ssd1306_draw_fast_hline(dev, cx - w / 2, cy + r, w, 1);
        }
    } else if (size >= 5) {
        for (int r = -2; r <= 2; r++) {
            int w = 5 - 2 * abs(r);
            ssd1306_draw_fast_hline(dev, cx - w / 2, cy + r, w, 1);
        }
    } else {
        ssd1306_set_pixel(dev, cx, cy - 1, 1);
        ssd1306_draw_fast_hline(dev, cx - 1, cy, 3, 1);
        ssd1306_set_pixel(dev, cx, cy + 1, 1);
    }
}

void ui_draw_thermo_icon(SSD1306_t *dev, int16_t x, int16_t y) {
    // Top tube
    ssd1306_draw_fast_hline(dev, x + 2, y, 4, 1);
    ssd1306_draw_fast_vline(dev, x + 2, y + 1, 5, 1);
    ssd1306_draw_fast_vline(dev, x + 5, y + 1, 5, 1);
    // Inner mercury column
    ssd1306_draw_fast_vline(dev, x + 3, y + 2, 4, 1);
    ssd1306_draw_fast_vline(dev, x + 4, y + 2, 4, 1);
    // Bottom round bulb
    ssd1306_draw_fast_hline(dev, x + 1, y + 6, 6, 1);
    ssd1306_fill_rect(dev, x, y + 7, 8, 3, 1);
    ssd1306_draw_fast_hline(dev, x + 1, y + 10, 6, 1);
}

void ui_draw_drop_icon(SSD1306_t *dev, int16_t x, int16_t y) {
    ssd1306_set_pixel(dev, x + 4, y, 1);
    ssd1306_draw_fast_hline(dev, x + 3, y + 1, 3, 1);
    ssd1306_draw_fast_hline(dev, x + 2, y + 2, 5, 1);
    ssd1306_draw_fast_hline(dev, x + 1, y + 3, 7, 1);
    ssd1306_fill_rect(dev, x, y + 4, 9, 3, 1);
    // Inner hollow highlight
    ssd1306_set_pixel(dev, x + 2, y + 4, 0);
    ssd1306_set_pixel(dev, x + 3, y + 4, 0);
    ssd1306_draw_fast_hline(dev, x + 1, y + 7, 7, 1);
    ssd1306_draw_fast_hline(dev, x + 2, y + 8, 5, 1);
}

void ui_draw_calendar_icon(SSD1306_t *dev, int16_t x, int16_t y) {
    // Top spiral rings
    ssd1306_set_pixel(dev, x + 1, y, 1);
    ssd1306_set_pixel(dev, x + 3, y, 1);
    ssd1306_set_pixel(dev, x + 5, y, 1);
    ssd1306_set_pixel(dev, x + 7, y, 1);
    // Header bar
    ssd1306_draw_fast_hline(dev, x, y + 1, 9, 1);
    ssd1306_draw_fast_hline(dev, x, y + 2, 9, 1);
    // Outer border
    ssd1306_draw_rect(dev, x, y + 1, 9, 8, 1);
    // Grid dots
    ssd1306_set_pixel(dev, x + 2, y + 4, 1); ssd1306_set_pixel(dev, x + 4, y + 4, 1); ssd1306_set_pixel(dev, x + 6, y + 4, 1);
    ssd1306_set_pixel(dev, x + 2, y + 6, 1); ssd1306_set_pixel(dev, x + 4, y + 6, 1); ssd1306_set_pixel(dev, x + 6, y + 6, 1);
}

void ui_draw_stopwatch_icon(SSD1306_t *dev, int16_t x, int16_t y) {
    // Top button
    ssd1306_draw_fast_hline(dev, x + 3, y, 4, 1);
    ssd1306_draw_fast_hline(dev, x + 3, y + 1, 4, 1);
    // Outer ring
    ssd1306_draw_round_rect(dev, x, y + 2, 10, 9, 3, 1);
    // Hands inside
    ssd1306_draw_fast_vline(dev, x + 4, y + 4, 3, 1);
    ssd1306_draw_fast_hline(dev, x + 4, y + 6, 3, 1);
}

void ui_draw_wavy_bottom(SSD1306_t *dev) {
    // Center star
    ui_draw_star(dev, 64, 54, 5);
    // Left wave: x = 14..58
    for (int x = 14; x <= 58; x++) {
        int wy = (int)(54 + 3.0 * sin((x - 14) * 0.28));
        ssd1306_set_pixel(dev, x, wy, 1);
        ssd1306_set_pixel(dev, x, wy + 1, 1);
    }
    // Right wave: x = 70..114
    for (int x = 70; x <= 114; x++) {
        int wy = (int)(54 + 3.0 * sin((x - 70) * 0.28));
        ssd1306_set_pixel(dev, x, wy, 1);
        ssd1306_set_pixel(dev, x, wy + 1, 1);
    }
}
