# ESP32 Smart OLED Clock & Environmental Monitor

> **รายวิชา:** Applied Microcontroller  
> **จัดทำโดย:** นางสาว เมจิยานันท์ กันยะ  
> **รหัสนักศึกษา:** 68030238  
> **สาขาวิชา:** เทคโนโลยีคอมพิวเตอร์    

---

## รายละเอียดโปรเจกต์
โปรเจกต์นาฬิกาอัจฉริยะมัลติฟังก์ชัน ควบคุมด้วยชิป **ESP32** แสดงผลผ่านหน้าจอ **OLED SSD1306** ซิงค์เวลาแม่นยำผ่าน **WiFi (NTP)** พร้อมระบบวัดอุณหภูมิและความชื้นด้วยเซ็นเซอร์ **DHT22** และมีโหมดจับเวลาในตัว

---

## อุปกรณ์ที่ใช้ในโปรเจกต์ (Hardware Components)

1. **ESP32 Development Board** (1 ตัว) — ชิปประมวลผลหลัก รองรับ WiFi ในตัว
2. **OLED Display 0.96" I2C (SSD1306)** (1 ตัว) — จอแสดงผลความละเอียด 128x64 Pixels
3. **DHT22 Temperature & Humidity Sensor** (1 ตัว) — เซ็นเซอร์วัดอุณหภูมิและความชื้น
4. **Tactile Push Buttons** (3 ปุ่ม) — ปุ่มกดสั่งการ
5. **Breadboard & Jumper Wires** — เบรดบอร์ดและสายต่อวงจร

---

## ฟีเจอร์หลัก (Key Features)

### 1. ระบบ 4 หน้าจอแสดงผล
* **Page 0 (Home Page):** แสดงเวลาเรียลไทม์ , วันที่ย่อ, วันในสัปดาห์, ค่าอุณหภูมิและความชื้นแบบสรุป พร้อมไอคอนสถานะ WiFi และแบตเตอรี่
* **Page 1 (Stopwatch):** ระบบจับเวลาความละเอียดสูง (นาที : วินาที : มิลลิวินาที) พร้อมปุ่มสั่งการ [START / PAUSE] และ [RESET]
* **Page 2 (Date Page):** แสดงชื่อวันเต็มตัวใหญ่ และวันที่เต็มรูปแบบ
* **Page 3 (Temp & Humid Page):** แยกฝั่งแสดงค่าอุณหภูมิ (°C) และความชื้น (%) แบบละเอียด

### 2. ระบบจัดการเวลา (Time Management)
* **NTP Auto-Sync:** เชื่อมต่อ WiFi และดึงเวลามาตรฐานประเทศไทย จาก Server อัตโนมัติเมื่อเริ่มเปิดเครื่อง
* **Manual Setting Mode:** ปรับตั้งเวลาและวันที่ด้วยตนเองผ่านปุ่มกดได้ (ชั่วโมง $\rightarrow$ นาที $\rightarrow$ วัน $\rightarrow$ เดือน $\rightarrow$ ปี) พร้อมระบบเส้นใต้ ระบุตำแหน่งที่กำลังแก้ไข

### 3. การควบคุมด้วยปุ่มกด 3 ปุ่ม (3-Button Controls)
* **BTN_MODE :**
  * *กดสั้น:* เปลี่ยนหน้าจอวนไปข้างหน้า (0 $\rightarrow$ 1 $\rightarrow$ 2 $\rightarrow$ 3)
  * *กดค้าง (>1.5 วินาที):* เข้าสู่โหมดตั้งเวลา ในหน้า 0 หรือ 2
  * *กดสั้นระหว่างตั้งค่า:* ยกเลิกการตั้งค่าและออกจากโหมดตั้งเวลา
* **BTN_START :**
  * *หน้า Stopwatch:* เริ่มจับเวลา (START) / หยุดชั่วคราว (PAUSE)
  * *หน้า Home/Date:* ยืนยันค่าการตั้งเวลาและเลื่อนไปตำแหน่งถัดไป
* **BTN_DOWN :**
  * *โหมดตั้งเวลา:* ปรับลดตัวเลข (มีระบบ Wrap-around วนกลับค่าสูงสุด)
  * *หน้า Stopwatch:* รีเซ็ตเวลาเป็น 00:00 (RESET)
  * *หน้าปกติ:* เปลี่ยนหน้าจอย้อนกลับ (0 $\rightarrow$ 3 $\rightarrow$ 2 $\rightarrow$ 1)

### ⚙️ 4. การจัดการระบบเบื้องหลัง (System Performance)
* **Non-blocking DHT Task:** แยก Task การอ่านค่าเซ็นเซอร์ทุกๆ 2.5 วินาที ไม่รบกวนการทำงานของการแสดงผลหน้าจอ
* **Critical Section Protection:** ป้องกันสัญญาณรบกวนขณะอ่านข้อมูลจาก DHT22 ด้วย `portMUX`
* **Smooth Frame Buffer:** รันหน้าจอผ่าน Memory Buffer ด้วยความเร็ว ~25-30 FPS ป้องกันปัญหาจอกระพริบและตัวหนังสือขาดหาย

---

## การต่อสายวงจร (Pinout Mapping)

| อุปกรณ์ | ปักขาอุปกรณ์ | GPIO ของ ESP32 |
| :--- | :--- | :--- |
| **OLED (SSD1306)** | SDA | GPIO 21 |
| | SCL | GPIO 22 |
| | VCC / GND | 3.3V / GND |
| **DHT22** | DATA | GPIO 4 |
| | VCC / GND | 3.3V (หรือ 5V) / GND |
| **BTN_MODE** | Pin A | GPIO 18 (อีกขาต่อ GND) |
| **BTN_START** | Pin A | GPIO 19 (อีกขาต่อ GND) |
| **BTN_DOWN** | Pin A | GPIO 17 (อีกขาต่อ GND) |

---

## การพัฒนาและเครื่องมือ (Development Tools)

* **Framework:** ESP-IDF (C Language)
* **IDE / Extension:** VS Code + PlatformIO
* **Drivers / Libraries:** Custom SSD1306 I2C Driver & Custom DHT22 Bit-banging
