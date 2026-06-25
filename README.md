# 🌡️ Cold Chain Integrity Monitor

> Affordable IoT solution for real-time vaccine and perishable goods monitoring during last-mile rural delivery — built for India, priced for India.

![Platform](https://img.shields.io/badge/Platform-ESP32--C6-blue)
![Cost](https://img.shields.io/badge/Cost-%E2%82%B9500--600-green)
![Region](https://img.shields.io/badge/Region-Rural%20India-orange)
![License](https://img.shields.io/badge/License-MIT-lightgrey)

---

# 📌 Problem Statement

In rural India, vaccines and perishable goods are often transported using standard iceboxes with **no temperature monitoring**.

The most critical—and most overlooked—failure point is not inside refrigerated trucks but during **last-mile delivery**, where ASHA workers and healthcare staff repeatedly open and close vaccine boxes while traveling door-to-door.

Each opening causes a temperature fluctuation, yet these events are rarely monitored.

By the time a temperature breach is detected, vaccine potency may already be compromised.

Existing commercial cold-chain monitoring systems typically cost between **₹50,000 and ₹2,00,000**, making them unsuitable for widespread deployment in rural healthcare systems.

This project addresses that gap with a low-cost solution costing only **₹500–600**.

---

# 🚀 Solution

The **Cold Chain Integrity Monitor** is a compact IoT device that can be placed inside any standard vaccine carrier or cooler box without requiring hardware modifications.

The system continuously monitors temperature, predicts potential breaches before they occur, and immediately alerts field workers when intervention is required.

### Key Features

* Real-time temperature monitoring (1-second intervals)
* Predictive breach detection using temperature rise rate analysis
* Automatic cooling fan activation
* Audible buzzer alerts
* LCD-based live status display
* Cloud logging using Supabase
* Offline functionality for critical alerts
* Retrofit design for existing cold boxes

---

# 🛠 Hardware Components

| Component                        | Purpose                                             |
| -------------------------------- | --------------------------------------------------- |
| ESP32-C6 DevKit                  | Main microcontroller (Wi-Fi 6, RISC-V architecture) |
| DS18B20 Waterproof Sensor        | High-accuracy temperature sensing (±0.5°C)          |
| JHD162A LCD + HW-61 I2C Backpack | Live temperature and status display                 |
| 5V Brushless Fan (4010/5010)     | Active cooling response                             |
| TIP122 NPN Transistor            | Fan motor driver                                    |
| Active Buzzer                    | Audible alerts                                      |
| 1kΩ Resistors                    | Pull-up and transistor base current limiting        |

### Total BOM Cost

**₹500–600**

---

# 🔌 Pin Mapping

| Component                 | ESP32-C6 GPIO |
| ------------------------- | ------------- |
| DS18B20 Data              | GPIO4         |
| Buzzer                    | GPIO20        |
| Fan Control (TIP122 Base) | GPIO21        |
| LCD SDA                   | GPIO6         |
| LCD SCL                   | GPIO7         |

---

# 🔧 Wiring Connections

## DS18B20 Temperature Sensor

```text
VCC  → 3.3V
GND  → GND
DATA → GPIO4

1kΩ Pull-up Resistor:
GPIO4 ↔ 3.3V
```

## Buzzer

```text
Positive (+) → GPIO20
Negative (-) → GND
```

## Fan Driver Circuit

```text
GPIO21 → 1kΩ → TIP122 Base

TIP122 Collector → Fan Negative
TIP122 Emitter   → GND

Fan Positive → 5V
```

## LCD Display

```text
VCC → 5V
GND → GND
SDA → GPIO6
SCL → GPIO7
```

---

# 🏗 System Architecture

```text
Main Loop (~1 second)

│
├── Read DS18B20 Temperature
│     ├── SKIP ROM (0xCC)
│     ├── CONVERT T (0x44)
│     └── READ SCRATCHPAD (0xBE)
│
├── Calculate Rate of Rise
│     └── dT/dt (°C/min)
│
├── Evaluate State
│     ├── BREACH
│     │     temp > 8°C
│     │     → Fan ON
│     │     → Double Buzzer Alarm
│     │
│     ├── PRE-ALERT
│     │     rate > 0.5°C/min
│     │     → Warning Beep
│     │
│     └── OK
│           → Normal Operation
│
├── Update LCD
│
├── Serial Logging
│
└── Upload Data to Supabase
```

---

# 💻 Firmware Setup

## Prerequisites

### Software

* Arduino IDE 2.x
* ESP32 Board Package (Espressif v3.x)

### Libraries

* OneWireNg
* LiquidCrystal I2C
* Wire
* WiFi
* HTTPClient
* WiFiClientSecure

---

## ESP32 Board Manager URL

```text
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

---

## Arduino IDE Settings

```text
Board            : ESP32C6 Dev Module
USB CDC On Boot  : Enabled
Baud Rate        : 115200
```

---

# ⚙ Configuration

Update Wi-Fi credentials inside `coldchain_monitor.ino`:

```cpp
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";
```

To use your own Supabase instance:

```cpp
const char* SB_URL =
"https://YOUR_PROJECT.supabase.co/rest/v1/sensor_data";

const char* SB_KEY =
"YOUR_SUPABASE_ANON_KEY";
```

---

# 🗄 Supabase Database Schema

```sql
CREATE TABLE public.sensor_data (
    id BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    created_at TIMESTAMPTZ DEFAULT NOW(),
    temperature DOUBLE PRECISION
);
```

---

# 🔄 Operating Modes

## ✅ Normal Operation

Condition:

```text
Temperature < 8°C
```

Behavior:

* Fan OFF
* Buzzer OFF
* LCD displays current temperature
* Cloud updates every 30 seconds

---

## ⚠ Pre-Alert Mode

Condition:

```text
Rate of Rise > 0.5°C/min
```

Behavior:

* Single warning beep every 1.5 seconds
* LCD displays:

```text
WARN: Rising Fast
```

Provides approximately 3–5 minutes for corrective action before a breach occurs.

---

## 🚨 Breach Mode

Condition:

```text
Temperature > 8°C
```

Behavior:

* Double buzzer alarm pattern
* Cooling fan activated
* LCD displays:

```text
!! BREACH !!
```

* Every reading uploaded to Supabase

---

## ❌ Sensor Failure

Behavior:

* Continuous buzzer alarm
* LCD displays:

```text
SENSOR LOST!
```

---

# 📊 Accuracy Validation

The DS18B20 sensor was validated using a three-point calibration method.

| Test Condition   | Expected | Measured                 | Error |
| ---------------- | -------- | ------------------------ | ----- |
| Ice Water Bath   | 0°C      | 0.1°C                    | 0.1°C |
| Room Temperature | ~31°C    | 31.3°C                   | 0.3°C |
| Heat Source Test | Rising   | Rising + Alert Triggered | ✅     |

### Accuracy Comparison

| Standard                           | Accuracy |
| ---------------------------------- | -------- |
| DS18B20 Manufacturer Specification | ±0.5°C   |
| WHO Cold Chain Allowable Variance  | ±2°C     |
| System Observed Error              | ±0.3°C   |

The observed error is approximately **4× tighter than WHO requirements**.

---

# 📈 Comparison with Existing Solutions

| Solution                         | Cost           | Real-Time Alerts | Last-Mile Friendly | Retrofit Compatible |
| -------------------------------- | -------------- | ---------------- | ------------------ | ------------------- |
| Sensitech TempTale               | ₹4,000–12,000  | ❌                | ❌                  | ✅                   |
| Berlinger ELPRO                  | ₹16,000–40,000 | ✅                | ❌                  | ❌                   |
| Monnit Wireless                  | ₹6,500–16,000  | ✅                | ❌                  | ❌                   |
| **Cold Chain Integrity Monitor** | **₹500–600**   | **✅**            | **✅**              | **✅**               |

---

# 🛣 Future Roadmap

* [ ] PWM fan speed control
* [ ] Temperature-proportional cooling
* [ ] SIM800L GSM alert system
* [ ] 18650 battery backup with TP4056 charging
* [ ] React dashboard with Supabase integration
* [ ] Rugged enclosure design
* [ ] Pilot deployment with healthcare workers

---

# 👥 Team

Developed under:

**Innovation and Incubation Cell (IIC)**
**Build Club, MAKAUT**
*(Maulana Abul Kalam Azad University of Technology)*

---

# 📜 License

MIT License

Free to use, modify, distribute, and deploy.

> *If this project helps even one vaccine reach a child safely, it has achieved its purpose.*
