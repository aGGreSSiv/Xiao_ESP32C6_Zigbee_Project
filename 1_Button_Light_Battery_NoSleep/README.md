# 1 Button / Light Node with Battery (No Sleep)

This version of the firmware configures the XIAO ESP32-C6 as an always-on Zigbee Light Node router device. It does not utilize any deep sleep functionality.

## Features
- **Always On**: Remains constantly connected to the Zigbee network, allowing instant two-way communication.
- **Zigbee Light Endpoint**: Registers in Home Assistant (HA) as a "ZigbeeLight". This means it appears just like a smart bulb.
- **Two-Way Control**: 
  - **Physical**: Pressing the physical button toggles the light state and immediately updates HA.
  - **Virtual**: Toggling the switch in the HA dashboard will immediately turn the physical LED on the board on/off.
- **Battery Monitoring**: Reads the LiPo battery voltage via the A0 analog pin every 60 seconds and reports the percentage and accurate voltage to Home Assistant.
- **Factory Reset**: Holding the button for more than 3 seconds will reset the Zigbee NVRAM network settings and reboot the device (Pairing mode).

## Pin Definitions & Wiring
| Component | Pin / GPIO | Notes |
| :--- | :--- | :--- |
| **Button 1** | D1 / GPIO 1 | Connect to push button (Internal Pull-up used). Grinds to GND. |
| **LED 1** | LED_BUILTIN | The onboard LED of the XIAO ESP32-C6. |
| **Battery ADC** | A0 | Connected to battery voltage divider circuit for reading battery levels. |

---
> [!WARNING]
> **Power Supply Required:** Because this firmware does not use deep sleep, it will drain a battery extremely fast. It is highly recommended to use a dedicated power supply circuit (e.g., constant 3.3V/5V supply or a high-capacity power bank module) or keep it plugged in via USB instead of relying on standard small batteries.
