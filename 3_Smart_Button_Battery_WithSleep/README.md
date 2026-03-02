# 3 Smart Buttons + Battery + Sleep

This version of the firmware configures the XIAO ESP32-C6 as a **Sleepy End Device** (battery optimized) acting as a Zigbee Color Dimmer Switch. This means it behaves like a smart wireless remote rather than a physical light bulb, sending control commands to your Zigbee coordinator (like Home Assistant).

## Features
- **Deep Sleep Optimization:** To maximize battery life, the MCU stays in Deep Sleep, waking up instantly only when a button is pressed. It also wakes up automatically every hour (3600s) to report the battery and notify the Zigbee network that it is still alive (Check-in).
- **LP_GPIO Ext1 Wakeup:** Utilizes the ESP32-C6's specific low-power RTC pins (LP_GPIOs) to ensure the internal pull-up resistors remain active during sleep without requiring external resistors.
- **Instant Response:** Reads the hardware wake-up status (`ESP_SLEEP_WAKEUP_EXT1`) upon booting so your very first button press that wakes the device immediately executes the command without being ignored.
- **Dynamic Awake Timeout:** Stays awake based on a 10-second activity timer. The timer resets every time a button is pressed or held. If no buttons are touched for 10 seconds, it returns to Deep Sleep. During the initial pairing process, this timeout extends to 60 seconds.
- **External Antenna Support:** Configures GPIO 3 (`LOW`) and GPIO 14 (`HIGH`) to automatically activate the U.FL external antenna connector for drastically better RF range.
- **Advanced Button Gestures:**
  - **Short Press:** Sends a "Toggle" command.
  - **Long Press:** Sends a "Dim Step Down" command every 200ms. If you release and hold again, the direction alternates (e.g., switches to "Dim Step Up").
  - **Super Long Press:** Holding Button 1 for > 15 seconds executes a Factory Reset and reboots the module into pairing mode.
- **Battery Reporting:** Reads LiPo voltage from A0 and correctly formats/sends to Home Assistant before going to sleep.
- **Visual Feedback:** Blinks corresponding LEDs automatically when buttons are used or when Zigbee commands are sent.

## Pin Definitions & Wiring
| Component | Pin / GPIO | Notes |
| :--- | :--- | :--- |
| **Button 1** | D1 / LP_GPIO 1 | Push button to GND. Controls `zbSwitch1`. Hold 15s for Reset. |
| **Button 2** | D2 / LP_GPIO 2 | Push button to GND. Controls `zbSwitch2`. |
| **Button 3** | D4 / LP_GPIO 4 | Push button to GND. Controls `zbSwitch3`. |
| **LED 1** | LED_BUILTIN | Onboard LED. Blinks on Button 1 activity. |
| **LED 2** | TX / GPIO 16 | External LED for Button 2 activity. Connect to GND via a small resistor. |
| **LED 3** | RX / GPIO 17 | External LED for Button 3 activity. Connect to GND via a small resistor. |
| **Battery ADC** | A0 | Connected to battery voltage divider. Use two **1 Megaohm** resistors (1M + 1M) for ultra-low power consumption and add a **100nF (0.1µF) ceramic capacitor** between A0 (ADC pin) and GND to stabilize the battery reading. |
| **External Ant.** | D3 & GPIO 14 | Used internally by code (set LOW/HIGH) to enable the U.FL antenna. |

---
*Note: Because this is a Sleepy End Device sending remote commands, you will need to map these "Toggle" and "Step" events to an actual smart bulb/relay inside Home Assistant Automations or Zigbee Settings (via Binding).*
