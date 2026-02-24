<div align="center">
  <h1>🌟 Xiao ESP32-C6 Zigbee Projects</h1>
  <p>A collection of robust, Home Assistant-ready Zigbee node implementations for the Seeed Studio XIAO ESP32-C6 microcontroller.</p>
</div>

---

## 🚀 Overview

This repository contains three distinct, ready-to-flash Arduino projects that turn your **XIAO ESP32-C6** into powerful Zigbee devices. Whether you need a low-power remote control or an always-on multi-gang smart switch router, these firmware versions integrate instantly with Home Assistant (via Zigbee2MQTT or ZHA).

### 1. [1 Button / Light Node (No Sleep)](./1_Button_Light_Battery_NoSleep)
A simple always-on router device that registers as a Zigbee light bulb. Features instant two-way control (physical & virtual) and accurate battery voltage reporting.  
> ⚠️ **Note:** Requires a constant power supply (USB or dedicated 3.3V/5V circuit). It does not go to sleep, so standard batteries will drain quickly.

### 2. [3 Buttons / Lights Node (No Sleep)](./3_Button_Light_Battery_NoSleep)
An expanded always-on router device supporting 3 independent hardware buttons and 3 distinct Zigbee light endpoints. Enables external U.FL antenna support for extended range.  
> ⚠️ **Note:** Requires a constant power supply (USB or dedicated 3.3V/5V circuit) as it does not utilize deep sleep.

### 3. [3 Smart Buttons (With Deep Sleep)](./3_Smart_Button_Battery_WithSleep)
A deeply optimized, battery-powered **Sleepy End Device** remote control. 
- **Ultra-low power:** Stays in Deep Sleep and wakes instantly using RTC LP_GPIO pins.
- **Advanced Gestures:** Supports Short Press (Toggle), Long Press (Dimming), and Super Long Press (Factory Reset).
- **Automated Check-ins:** Wakes up periodically to report battery health to Home Assistant.

## 🛠️ Hardware Requirements

- **Microcontroller**: Seeed Studio XIAO ESP32-C6
- **Power Options**: 
  - 3.7V LiPo Battery (Highly recommended for the *With Sleep* remote version)
  - USB / Constant 3.3V-5V Power Supply module (Required for the *No Sleep* router versions)
- **Peripherals**: Momentary push buttons, status LEDs, and basic pull-down resistors (Refer to the individual `README.md` in each project folder for exact wiring diagrams and pinouts).

## 📥 Getting Started / Installation

1. Install the [Arduino IDE](https://www.arduino.cc/en/software) and ESP32 board manager core.
2. Select your board: **Tools > Board > Seeed Studio XIAO ESP32C6**.
3. Configure the Zigbee environment:
   - **Zigbee Mode:** `Zigbee ED (end device)`
   - **Partition Scheme:** `Zigbee 4MB with spiffs`
4. Open the desired project folder, double-click the `.ino` file, and click **Upload**.
5. Once flashed, set up your Zigbee coordinator to "Permit Join" and press the physical button on the board to pair!

---
*If you encounter issues or want to add features, feel free to open a Pull Request. Happy hacking!*
