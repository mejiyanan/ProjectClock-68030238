#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_sntp.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"
#include "ssd1306.h"

// ================= Configurations =================
#define WIFI_SSID "A32"
#define WIFI_PASS "zxvj3842"

#define I2C_SDA_PIN  21
#define I2C_SCL_PIN  22
#define DHT_PIN      GPIO_NUM_4

// ปุ่ม 3 ปุ่มที่ใช้งานได้จริง
// BTN_MODE  (GPIO 18) — เปลี่ยนหน้า / ค้างเพื่อเข้าตั้งค่า / ยกเลิกตั้งค่า
// BTN_START (GPIO 19) — เข้าตั้งค่า → ยืนยันทีละ Step / Start-Pause Stopwatch
// BTN_DOWN  (GPIO 17) — ปรับค่าลง ใน Setting / Reset Stopwatch / เปลี่ยนหน้าถัดไป
#define BTN_MODE     GPIO_NUM_18
#define BTN_START    GPIO_NUM_19
#define BTN_DOWN     GPIO_NUM_17

// ================= Global States =================
static SSD1306_t oled;
static bool wifi_connected = false;
static bool time_synced = false;

static float current_temp = 28.5f;
static float current_humidity = 65.0f;
static bool sensor_has_data = false;

// 4 Pages:
// 0: หน้าหลัก (เวลา + อุณหภูมิ/ความชื้น)
// 1: หน้าจับเวลา (Stopwatch)
// 2: หน้าวันที่ (Date)
// 3: หน้าความชื้นและอุณหภูมิ (Temp & Humid)
static int current_page = 0;

// Setting Mode Variables
static bool is_setting_mode = false;
static int setting_step = 0; // 0: Hour, 1: Min, 2: Day, 3: Month, 4: Year
static struct tm edit_time;

// Stopwatch Variables
static bool sw_running = false;
static int64_t sw_start_time = 0;
static int64_t sw_elapsed_time = 0; // Microseconds

// Month & Day strings
static const char *MONTH_NAMES_SHORT[] = {
    "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
    "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

static const char *DAY_NAMES_SHORT[] = {
    "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
};

static const char *DAY_NAMES_FULL[] = {
    "SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY",
    "THURSDAY", "FRIDAY", "SATURDAY"
};

// ================= DHT Sensor Driver (Critical Section Protected) =================
static portMUX_TYPE dht_mux = portMUX_INITIALIZER_UNLOCKED;

static bool read_dht_sensor(float *temperature, float *humidity) {
    uint8_t data[5] = {0};

    // Send start signal
    gpio_set_direction(DHT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(20)); // Keep low for 20ms
    gpio_set_level(DHT_PIN, 1);
    ets_delay_us(30);

    // Switch to input mode
    gpio_set_direction(DHT_PIN, GPIO_MODE_INPUT);

    taskENTER_CRITICAL(&dht_mux);

    // Wait for DHT response (low ~80us then high ~80us)
    uint32_t timeout = 0;
    while (gpio_get_level(DHT_PIN) == 1) {
        if (++timeout > 120) { taskEXIT_CRITICAL(&dht_mux); return false; }
        ets_delay_us(1);
    }
    timeout = 0;
    while (gpio_get_level(DHT_PIN) == 0) {
        if (++timeout > 120) { taskEXIT_CRITICAL(&dht_mux); return false; }
        ets_delay_us(1);
    }
    timeout = 0;
    while (gpio_get_level(DHT_PIN) == 1) {
        if (++timeout > 120) { taskEXIT_CRITICAL(&dht_mux); return false; }
        ets_delay_us(1);
    }

    // Read 40 data bits
    for (int i = 0; i < 40; i++) {
        timeout = 0;
        while (gpio_get_level(DHT_PIN) == 0) {
            if (++timeout > 120) { taskEXIT_CRITICAL(&dht_mux); return false; }
            ets_delay_us(1);
        }
        // Bit length: ~26-28us = 0, ~70us = 1
        ets_delay_us(35);
        if (gpio_get_level(DHT_PIN) == 1) {
            data[i / 8] |= (1 << (7 - (i % 8)));
            timeout = 0;
            while (gpio_get_level(DHT_PIN) == 1) {
                if (++timeout > 120) { taskEXIT_CRITICAL(&dht_mux); return false; }
                ets_delay_us(1);
            }
        }
    }

    taskEXIT_CRITICAL(&dht_mux);

    // Verify Checksum
    uint8_t checksum = (data[0] + data[1] + data[2] + data[3]) & 0xFF;
    if (data[4] == checksum && (data[0] != 0 || data[2] != 0)) {
        // DHT22 calculation
        float h = ((data[0] << 8) | data[1]) / 10.0f;
        int16_t raw_t = ((data[2] & 0x7F) << 8) | data[3];
        if (data[2] & 0x80) raw_t = -raw_t;
        float t = raw_t / 10.0f;

        // Auto-detect DHT11 format if values are unrealistic for DHT22
        if (h > 100.0f || (data[1] == 0 && data[3] == 0 && data[0] <= 100)) {
            h = (float)data[0];
            t = (float)data[2];
        }

        if (h >= 0.0f && h <= 100.0f && t >= -40.0f && t <= 80.0f) {
            *humidity = h;
            *temperature = t;
            return true;
        }
    }
    return false;
}

static void dht_task(void *pvParam) {
    float t, h;
    while (1) {
        if (read_dht_sensor(&t, &h)) {
            current_temp = t;
            current_humidity = h;
            sensor_has_data = true;
        }
        vTaskDelay(pdMS_TO_TICKS(2500));
    }
}

// ================= WiFi & SNTP Event Handler =================
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        // Connected to AP
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_connected = false;
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        wifi_connected = true;
    }
}

static void sntp_sync_callback(struct timeval *tv) {
    time_synced = true;
}

static void wifi_sntp_init(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    // SNTP Configuration
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(1, "time.google.com");
    sntp_set_time_sync_notification_cb(sntp_sync_callback);
    esp_sntp_init();

    // Set Timezone to Thailand GMT+7
    setenv("TZ", "ICT-7", 1);
    tzset();
}

// ================= UI Screen Renderers =================

// SCREEN 1: หน้าหลัก (เวลา + อุณหภูมิ/ความชื้น)
static void render_page_home(SSD1306_t *dev, struct tm *timeinfo) {
    ui_draw_outer_frame(dev);

    // Header Text: "SAT, 08 NOV"
    char header_buf[32];
    snprintf(header_buf, sizeof(header_buf), "%s, %02d %s",
             DAY_NAMES_SHORT[timeinfo->tm_wday % 7],
             timeinfo->tm_mday,
             MONTH_NAMES_SHORT[timeinfo->tm_mon % 12]);
    ssd1306_draw_string_5x7(dev, 8, 5, header_buf, 1);

    // Header Icons
    ui_draw_wifi_icon(dev, 95, 5, wifi_connected);
    ui_draw_battery_icon(dev, 108, 5, 3);

    // Header Divider Line
    ssd1306_draw_fast_hline(dev, 6, 14, 116, 1);

    // Center Time: "HH:MM" (Large 14x19 Font)
    char time_str[16];
    snprintf(time_str, sizeof(time_str), "%02d:%02d", (int)timeinfo->tm_hour, (int)timeinfo->tm_min);
    int16_t cur_x = 31;
    for (int i = 0; time_str[i] != '\0'; i++) {
        int16_t w = ssd1306_draw_large_digit(dev, cur_x, 19, time_str[i], 1);
        cur_x += w + 2;
    }

    // Sparkle Stars
    ui_draw_star(dev, 12, 23, 7);  // Large 4-point star top-left
    ui_draw_star(dev, 18, 35, 4);  // Small star bottom-left
    ui_draw_star(dev, 114, 32, 5); // Medium star right

    // Bottom Separator Line with Star at Center
    ssd1306_draw_fast_hline(dev, 8, 46, 51, 1);
    ui_draw_star(dev, 64, 46, 7);
    ssd1306_draw_fast_hline(dev, 69, 46, 51, 1);

    // Footer: Thermo 28.5°C | Drop 65%
    ui_draw_thermo_icon(dev, 18, 49);
    char temp_buf[24];
    snprintf(temp_buf, sizeof(temp_buf), "%.1f°C", current_temp);
    ssd1306_draw_string_5x7(dev, 30, 52, temp_buf, 1);

    // Vertical Divider
    ssd1306_draw_fast_vline(dev, 66, 50, 9, 1);

    // Water Drop and Humidity
    ui_draw_drop_icon(dev, 73, 50);
    char hum_buf[24];
    snprintf(hum_buf, sizeof(hum_buf), "%.0f%%", current_humidity);
    ssd1306_draw_string_5x7(dev, 86, 52, hum_buf, 1);

    // Setting Mode Indicator
    if (is_setting_mode) {
        if (setting_step == 0) ssd1306_draw_fast_hline(dev, 31, 40, 30, 1); // Underline Hour
        if (setting_step == 1) ssd1306_draw_fast_hline(dev, 69, 40, 30, 1); // Underline Min
    }
}

// SCREEN 2: หน้าจับเวลา (Stopwatch)
static void render_page_stopwatch(SSD1306_t *dev) {
    ui_draw_outer_frame(dev);

    // Header: Stopwatch Icon + "STOPWATCH", Play Icon, Battery
    ui_draw_stopwatch_icon(dev, 8, 4);
    ssd1306_draw_string_5x7(dev, 22, 5, "STOPWATCH", 1);

    // Top-right status play/pause icon
    if (sw_running) {
        // Small play triangle
        for (int py = 0; py < 7; py++) {
            int pw = (py <= 3) ? (py + 1) : (7 - py);
            ssd1306_draw_fast_hline(dev, 98, 5 + py, pw, 1);
        }
    } else {
        // Small pause icon
        ssd1306_draw_fast_vline(dev, 98, 5, 7, 1);
        ssd1306_draw_fast_vline(dev, 100, 5, 7, 1);
    }
    ui_draw_battery_icon(dev, 108, 5, 3);
    ssd1306_draw_fast_hline(dev, 6, 14, 116, 1);

    // Compute Stopwatch Time
    int64_t total_us = sw_elapsed_time;
    if (sw_running) {
        total_us += (esp_timer_get_time() - sw_start_time);
    }
    if (total_us < 0) total_us = 0;

    uint32_t total_seconds = (uint32_t)(total_us / 1000000ULL);
    uint32_t centis = (uint32_t)((total_us % 1000000ULL) / 10000ULL);
    uint32_t mins = total_seconds / 60;
    uint32_t secs = total_seconds % 60;
    uint32_t hours = mins / 60;

    char sw_buf[32];
    if (hours > 0) {
        snprintf(sw_buf, sizeof(sw_buf), "%02u:%02u:%02u", (unsigned int)(hours % 100), (unsigned int)(mins % 60), (unsigned int)secs);
    } else {
        snprintf(sw_buf, sizeof(sw_buf), "%02u:%02u:%02u", (unsigned int)(mins % 100), (unsigned int)secs, (unsigned int)centis);
    }

    // Centered Stopwatch Digits
    int16_t start_x = 24;
    for (int i = 0; sw_buf[i] != '\0'; i++) {
        if (sw_buf[i] == ':') {
            ssd1306_fill_rect(dev, start_x + 1, 26, 2, 2, 1);
            ssd1306_fill_rect(dev, start_x + 1, 33, 2, 2, 1);
            start_x += 5;
        } else {
            ssd1306_draw_large_digit(dev, start_x, 21, sw_buf[i], 1);
            start_x += 10;
        }
    }

    // Radiance / Speed Lines
    // Left:
    ssd1306_draw_line(dev, 14, 24, 8, 22, 1);
    ssd1306_draw_line(dev, 14, 29, 6, 29, 1);
    ssd1306_draw_line(dev, 14, 34, 8, 36, 1);
    // Right:
    ssd1306_draw_line(dev, 113, 24, 119, 22, 1);
    ssd1306_draw_line(dev, 113, 29, 121, 29, 1);
    ssd1306_draw_line(dev, 113, 34, 119, 36, 1);

    // Bottom Badges: [ ▶ START ]  [ 🔄 RESET ]
    // Button 1: START / PAUSE
    ssd1306_draw_round_rect(dev, 22, 47, 40, 11, 2, 1);
    if (!sw_running) {
        // Play triangle ▶
        for (int py = 0; py < 5; py++) {
            int pw = (py <= 2) ? (py + 1) : (5 - py);
            ssd1306_draw_fast_hline(dev, 28, 50 + py, pw, 1);
        }
        ssd1306_draw_string_5x7(dev, 35, 49, "START", 1);
    } else {
        // Pause bars ❚❚
        ssd1306_draw_fast_vline(dev, 27, 50, 5, 1);
        ssd1306_draw_fast_vline(dev, 29, 50, 5, 1);
        ssd1306_draw_string_5x7(dev, 34, 49, "PAUSE", 1);
    }

    // Button 2: RESET
    ssd1306_draw_round_rect(dev, 66, 47, 40, 11, 2, 1);
    // Refresh Icon 🔄
    ssd1306_draw_round_rect(dev, 71, 50, 5, 5, 1, 1);
    ssd1306_set_pixel(dev, 75, 49, 1);
    ssd1306_draw_string_5x7(dev, 78, 49, "RESET", 1);
}

// SCREEN 3: หน้าวันที่ (Date)
static void render_page_date(SSD1306_t *dev, struct tm *timeinfo) {
    ui_draw_outer_frame(dev);

    // Header: Calendar Icon + "DATE", WiFi, Battery
    ui_draw_calendar_icon(dev, 8, 4);
    ssd1306_draw_string_5x7(dev, 22, 5, "DATE", 1);
    ui_draw_wifi_icon(dev, 95, 5, wifi_connected);
    ui_draw_battery_icon(dev, 108, 5, 3);
    ssd1306_draw_fast_hline(dev, 6, 14, 116, 1);

    // Subtitle: Day of Week "SATURDAY" (Centered)
    const char *day_full = DAY_NAMES_FULL[timeinfo->tm_wday % 7];
    int day_len = strlen(day_full);
    int day_x = (SSD1306_WIDTH - (day_len * 6)) / 2;
    ssd1306_draw_string_5x7(dev, day_x, 19, day_full, 1);

    // Main Big Date: "08 NOV 2025" (Centered)
    char date_buf[32];
    snprintf(date_buf, sizeof(date_buf), "%02d %s %04d",
             timeinfo->tm_mday,
             MONTH_NAMES_SHORT[timeinfo->tm_mon % 12],
             timeinfo->tm_year + 1900);
    int date_len = strlen(date_buf);
    int date_x = (SSD1306_WIDTH - (date_len * 8)) / 2;
    ssd1306_draw_medium_string(dev, date_x, 32, date_buf, 1);

    // Sparkle Stars Left & Right
    ui_draw_star(dev, 10, 26, 5);
    ui_draw_star(dev, 118, 26, 5);

    // Bottom Decorative Motif: • • • ✦ • • •
    ui_draw_star(dev, 64, 53, 7);
    int dots_left[] = {32, 40, 48, 56};
    for (int i = 0; i < 4; i++) {
        ssd1306_fill_rect(dev, dots_left[i], 52, 2, 2, 1);
    }
    int dots_right[] = {72, 80, 88, 96};
    for (int i = 0; i < 4; i++) {
        ssd1306_fill_rect(dev, dots_right[i], 52, 2, 2, 1);
    }

    // Setting Mode Indicator
    if (is_setting_mode) {
        if (setting_step == 2) ssd1306_draw_fast_hline(dev, date_x, 46, 18, 1); // Day
        if (setting_step == 3) ssd1306_draw_fast_hline(dev, date_x + 24, 46, 26, 1); // Month
        if (setting_step == 4) ssd1306_draw_fast_hline(dev, date_x + 56, 46, 34, 1); // Year
    }
}

// SCREEN 4: หน้าความชื้นและอุณหภูมิ (Temp & Humid)
static void render_page_temp_humid(SSD1306_t *dev) {
    ui_draw_outer_frame(dev);

    // Header: Thermo Icon + "TEMP & HUMID", WiFi, Battery
    ui_draw_thermo_icon(dev, 8, 4);
    ssd1306_draw_string_5x7(dev, 20, 5, "TEMP & HUMID", 1);
    ui_draw_wifi_icon(dev, 95, 5, wifi_connected);
    ui_draw_battery_icon(dev, 108, 5, 3);
    ssd1306_draw_fast_hline(dev, 6, 14, 116, 1);

    // Vertical Dashed Center Divider (x = 64)
    for (int y = 17; y <= 45; y += 3) {
        ssd1306_draw_fast_vline(dev, 64, y, 2, 1);
    }

    // Left Section: TEMP
    ssd1306_draw_string_5x7(dev, 24, 19, "TEMP", 1);
    char temp_str[32];
    snprintf(temp_str, sizeof(temp_str), "%.1f°C", current_temp);
    ssd1306_draw_medium_string(dev, 16, 31, temp_str, 1);
    ssd1306_draw_fast_hline(dev, 20, 44, 28, 1);

    // Right Section: HUMIDITY
    ssd1306_draw_string_5x7(dev, 76, 19, "HUMIDITY", 1);
    char hum_str[32];
    snprintf(hum_str, sizeof(hum_str), "%.0f%%", current_humidity);
    ssd1306_draw_medium_string(dev, 80, 31, hum_str, 1);
    ui_draw_star(dev, 115, 33, 4); // Small sparkle star
    ssd1306_draw_fast_hline(dev, 76, 44, 28, 1);

    // Bottom Decorative Wavy Sine Curve with Center Star!
    ui_draw_wavy_bottom(dev);
}

// ================= Main Application =================
void app_main(void) {
    // 1. Initialize Buttons (Pull-Up, Active LOW) — 3 ปุ่ม
    gpio_config_t btn_conf = {
        .pin_bit_mask = (1ULL << BTN_MODE) | (1ULL << BTN_START) | (1ULL << BTN_DOWN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&btn_conf);

    // 2. Initialize OLED Display (128x64 Fast I2C)
    ssd1306_init(&oled, I2C_SDA_PIN, I2C_SCL_PIN);
    ssd1306_clear(&oled);

    // Splash Screen
    ui_draw_outer_frame(&oled);
    ssd1306_draw_string_5x7(&oled, 24, 22, "CONNECTING...", 1);
    ssd1306_draw_string_5x7(&oled, 30, 34, "WIFI & NTP", 1);
    ssd1306_update(&oled);

    // 3. Initialize WiFi and SNTP
    wifi_sntp_init();

    // 4. Start Background DHT Reading Task
    xTaskCreate(dht_task, "dht_task", 2048, NULL, 5, NULL);

    // Button Debounce Variables — 3 ปุ่ม
    int last_mode = 1, last_start = 1, last_down = 1;
    int64_t mode_press_start = 0;

    while (1) {
        int cur_mode  = gpio_get_level(BTN_MODE);
        int cur_start = gpio_get_level(BTN_START);
        int cur_down  = gpio_get_level(BTN_DOWN);

        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);
        if (timeinfo.tm_year > 100) {
            time_synced = true;
        }

        // ================= Button Event Handling (3 ปุ่ม) =================
        //
        // BTN_MODE  (GPIO 18):
        //   - กดสั้น  → เปลี่ยนหน้า (เวียนไปข้างหน้า 0→1→2→3→0)
        //   - กดค้าง >1.5s → เข้า Setting Mode (ทำได้บนหน้า 0 หรือ 2)
        //   - ระหว่าง Setting → กดสั้น = ยกเลิก/ออก Setting ทันที
        //
        // BTN_START (GPIO 19):
        //   - ปกติ บนหน้า 0/2 → เข้า Setting Mode / ยืนยันไปยัง Step ถัดไป / บันทึก
        //   - บนหน้า 1 (Stopwatch) → Start / Pause
        //
        // BTN_DOWN  (GPIO 17):
        //   - ระหว่าง Setting → ลดค่าของ Field ที่กำลังตั้ง
        //              รวมถึง เพิ่มค่า ด้วยการกดหลายครั้ง (Wrap Around)
        //   - บนหน้า 1 (Stopwatch) → Reset
        //   - ปกติ → กดเปลี่ยนหน้า (เวียนไปข้างหลัง 0→3→2→1→0)
        // ========================================================================

        // 1. BTN_MODE: เปลี่ยนหน้า / Long press → Setting / กดสั้นระหว่าง Setting → ยกเลิก
        if (cur_mode == 0 && last_mode == 1) {
            mode_press_start = esp_timer_get_time();
        } else if (cur_mode == 1 && last_mode == 0) {
            int64_t duration_ms = (esp_timer_get_time() - mode_press_start) / 1000;
            if (is_setting_mode) {
                // กดสั้นระหว่าง Setting → ยกเลิกออกทันที (ไม่บันทึก)
                is_setting_mode = false;
            } else if (duration_ms > 1500 && (current_page == 0 || current_page == 2)) {
                // กดค้าง → เข้า Setting Mode
                is_setting_mode = true;
                setting_step = 0;
                edit_time = timeinfo;
            } else {
                // กดสั้น → เปลี่ยนหน้าไปข้างหน้า
                current_page = (current_page + 1) % 4;
            }
        }

        // 2. BTN_START: เข้า/ยืนยัน Setting ทีละ Step, Start-Pause Stopwatch
        if (cur_start == 0 && last_start == 1) {
            if (current_page == 1) {
                // หน้า Stopwatch: Toggle Start / Pause
                if (!sw_running) {
                    sw_running = true;
                    sw_start_time = esp_timer_get_time();
                } else {
                    sw_running = false;
                    sw_elapsed_time += (esp_timer_get_time() - sw_start_time);
                }
            } else if (current_page == 0 || current_page == 2) {
                // หน้า Home หรือ Date: เข้า Setting หรือยืนยัน Step ถัดไป
                if (!is_setting_mode) {
                    // เข้า Setting Mode
                    is_setting_mode = true;
                    setting_step = 0;
                    edit_time = timeinfo;
                } else {
                    setting_step++;
                    if (setting_step > 4) {
                        // ครบทุก Step → บันทึกเวลา
                        time_t t_new = mktime(&edit_time);
                        struct timeval tv = { .tv_sec = t_new, .tv_usec = 0 };
                        settimeofday(&tv, NULL);
                        is_setting_mode = false;
                    }
                }
            }
        }

        // 3. BTN_DOWN: ปรับค่าลง ใน Setting / Reset Stopwatch / เปลี่ยนหน้าย้อนกลับ
        if (cur_down == 0 && last_down == 1) {
            if (is_setting_mode) {
                // ลดค่า Field ที่กำลัง Setting (Wrap Around)
                if      (setting_step == 0) { edit_time.tm_hour--; if (edit_time.tm_hour < 0)  edit_time.tm_hour = 23; }
                else if (setting_step == 1) { edit_time.tm_min--;  if (edit_time.tm_min  < 0)  edit_time.tm_min  = 59; }
                else if (setting_step == 2) { edit_time.tm_mday--; if (edit_time.tm_mday < 1)  edit_time.tm_mday = 31; }
                else if (setting_step == 3) { edit_time.tm_mon--;  if (edit_time.tm_mon  < 0)  edit_time.tm_mon  = 11; }
                else if (setting_step == 4) { edit_time.tm_year--; }
            } else if (current_page == 1) {
                // หน้า Stopwatch: Reset
                sw_running = false;
                sw_elapsed_time = 0;
            } else {
                // ปกติ: เปลี่ยนหน้าย้อนกลับ
                current_page = (current_page + 3) % 4;
            }
        }

        last_mode  = cur_mode;
        last_start = cur_start;
        last_down  = cur_down;

        // ================= Screen Rendering =================
        ssd1306_clear(&oled);

        struct tm *disp_time = is_setting_mode ? &edit_time : &timeinfo;

        switch (current_page) {
            case 0:
                render_page_home(&oled, disp_time);
                break;
            case 1:
                render_page_stopwatch(&oled);
                break;
            case 2:
                render_page_date(&oled, disp_time);
                break;
            case 3:
                render_page_temp_humid(&oled);
                break;
            default:
                current_page = 0;
                break;
        }

        // Send Framebuffer to OLED Display
        ssd1306_update(&oled);

        // Smooth refresh rate (~25-30 FPS)
        vTaskDelay(pdMS_TO_TICKS(40));
    }
}
