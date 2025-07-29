# Smart Heater – ESP32-Based Temperature Controller

This project is a smart electric heater controller built using an ESP32 microcontroller. It features a web-based interface, OTA (Over-the-Air) firmware updates, automatic and manual temperature setpoint control, night mode, and a built-in PID algorithm for precise temperature regulation. The system uses a DHT21 sensor for temperature and humidity monitoring, displays data on an OLED screen, and can be adjusted remotely via Wi-Fi.

---

## 🛠 Features

- **PID temperature control**
- **DHT21 sensor** for temperature and humidity
- **OLED display (128x32)** for real-time feedback
- **Manual analog input** for adjusting temperature setpoint
- **Wi-Fi connectivity** with OTA updates and web interface
- **Night mode scheduling** based on time settings
- **EEPROM** storage for persistent setpoint configuration
- **Built-in safety features**, including over-temperature protection and error handling

---

## ⚙️ Hardware Requirements

- ESP32 board
- DHT21 temperature/humidity sensor
- SSD1306 OLED display (I2C, 128x32)
- Analog input potentiometer (optional for manual control)
- Relay or MOSFET output driver (connected to GPIO 4 for heater control)
- Push button or digital input (optional, GPIO 35 for manual mode trigger)

---

## 🧪 Libraries Required

- `DHT.h`
- `Ticker.h`
- `Wire.h`
- `Adafruit_GFX.h`
- `Adafruit_SSD1306.h`
- `WiFi.h`
- `ESPmDNS.h`
- `WiFiUdp.h`
- `ArduinoOTA.h`
- `WebServer.h`
- `EEPROM.h`
- `Time.h`

Install these libraries via the Arduino Library Manager or manually from GitHub/Adafruit repositories.

---

## 🌐 Web Interface

Access the ESP32 through its local IP to:
- View current temperature, humidity, and setpoint
- Modify the setpoint remotely
- Configure start/end hours for night mode
- Toggle night mode

---

## 🔐 OTA (Over-The-Air) Updates

The firmware supports OTA updates for convenient wireless reprogramming.

- Default OTA hostname: `SMART-HEATER`
- Default OTA password: `admin`

---

## 🔁 PID Control Parameters

- **Kp** = 0.5
- **Ki** = 0.005
- **Max Integral** = 0.40
- **Safety Temperature Cutoff** = 30°C

The heater output is controlled by a basic PI controller that activates the relay when PID output exceeds a threshold.

---

## 🕓 Night Mode

The system includes time-based behavior with support for:
- Start and end hour settings
- Automatic switching into "Night Mode" (different display behavior or heater behavior)

Time is synchronized via NTP (`pool.ntp.org`), with support for GMT offset and daylight saving.

---

## 🧠 EEPROM Storage

- Stores the latest setpoint value to retain configuration across restarts.
- EEPROM address used: `0`

---

## 📷 Display Behavior

- Displays temperature and humidity alternately every few seconds.
- Visual setpoint indication during manual adjustment.
- Custom animations during startup and setpoint changes.

---

## 🧩 Pin Mapping

| Component       | Pin         |
|----------------|-------------|
| DHT21 Sensor   | GPIO 5      |
| Heater Output  | GPIO 4      |
| Manual Input   | GPIO 35 (ADC) |
| OLED Display   | I2C (Default SDA/SCL) |

---

## 🧪 Safety & Error Handling

- Restarts ESP32 after extended sensor errors.
- Stops heater when sensor is invalid or over-temperature detected.
- LED blinking and internal flags notify of issues.

---

## 🏁 Getting Started

1. Flash the code via USB using the Arduino IDE or PlatformIO.
2. Connect to your Wi-Fi by editing `ssid` and `password` in the code.
3. Access the device via the IP shown in the Serial Monitor.
4. Configure setpoint, night mode hours, or update firmware via web interface or OTA.

---

## 📌 Notes

- The analog read is used for adjusting setpoint between **18°C and 30°C**.
- The screen is updated every **15 ms** for smooth feedback.
- The internal timers handle all critical tasks without using `delay()`.

---

## 📜 License

This project is released under the MIT License.

---

## 👨‍💻 Author

Developed by Marcelo Stüpp  
Date: 09/02/2024  
Version: 1.0

---

