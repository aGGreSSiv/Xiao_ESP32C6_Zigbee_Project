# 3 Buttons / Lights Node with Battery (No Sleep)

This version expands heavily on the base "No Sleep" logic. It configures the XIAO ESP32-C6 as a three-channel Zigbee Light Node router device. It does not utilize any deep sleep functionality.

## Features
- **Always On**: Remains constantly connected to the Zigbee network, meaning it acts as an active router (depending on the Zigbee coordinator's mesh routing) and allows instant two-way communication.
- **Three Zigbee Light Endpoints**: Registers in Home Assistant (HA) as three independent "ZigbeeLight" entities (Endpoints 10, 11, and 12). 
- **Two-Way Control**: 
  - **Physical**: Pressing any of the three physical buttons toggles its corresponding light state and immediately updates HA.
  - **Virtual**: Toggling the light entities in the HA dashboard will immediately turn the corresponding physical LEDs on the ESP32 board on or off.
- **External Antenna Support:** Configures GPIO 3 (`LOW`) and GPIO 14 (`HIGH`) to automatically activate the U.FL external antenna connector for drastically better RF range.
- **Battery Monitoring**: Reads the LiPo battery voltage via the A0 analog pin every 60 seconds, calculates the percentage, and reports it to Home Assistant via the first endpoint.
- **Factory Reset**: Holding Button 1 for more than 3 seconds will perform a factory reset and reboot the device.

## Pin Definitions & Wiring
| Component | Pin / GPIO | Notes |
| :--- | :--- | :--- |
| **Button 1** | D1 / LP_GPIO 1 | Push button to GND. Controls Light 1. Hold 3s for Reset. |
| **Button 2** | D2 / LP_GPIO 2 | Push button to GND. Controls Light 2. |
| **Button 3** | D4 / LP_GPIO 4 | Push button to GND. Controls Light 3. |
| **LED 1** | LED_BUILTIN | Onboard LED. Tied to Light 1 state. |
| **LED 2** | TX / GPIO 16 | External LED. Tied to Light 2 state. Connect to GND via a small resistor. |
| **LED 3** | RX / GPIO 17 | External LED. Tied to Light 3 state. Connect to GND via a small resistor. |
| **Battery ADC** | A0 | Connected to battery voltage divider circuit. |
| **External Ant.** | D3 & GPIO 14 | Used internally by code (set LOW/HIGH) to enable the U.FL antenna. |

---
> [!WARNING]
> **Power Supply Required:** Because this firmware does not use deep sleep, it will drain a battery extremely fast. It is highly recommended to use a dedicated power supply circuit (e.g., constant 3.3V/5V supply or a high-capacity power bank module) or keep it plugged in via USB instead of relying on standard small batteries.
