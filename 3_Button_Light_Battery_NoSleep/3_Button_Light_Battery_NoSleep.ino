#ifndef ZIGBEE_MODE_ED
#error "Zigbee end device mode is not selected in Tools->Zigbee mode"
#endif

#include "Zigbee.h"

#define ZIGBEE_LIGHT_ENDPOINT_1 10
#define ZIGBEE_LIGHT_ENDPOINT_2 11
#define ZIGBEE_LIGHT_ENDPOINT_3 12

uint8_t button1 = 1; // LP_GPIO1 (D1)
uint8_t button2 = 2; // LP_GPIO2 (D2)
uint8_t button3 = 4; // LP_GPIO4 (D4)

uint32_t lastBatteryReportTime = 0;

ZigbeeLight zbLight1 = ZigbeeLight(ZIGBEE_LIGHT_ENDPOINT_1);
ZigbeeLight zbLight2 = ZigbeeLight(ZIGBEE_LIGHT_ENDPOINT_2);
ZigbeeLight zbLight3 = ZigbeeLight(ZIGBEE_LIGHT_ENDPOINT_3);

// Button states and timers for non-blocking logic
bool button1State = HIGH;
bool lastButton1State = HIGH;
uint32_t button1PressTime = 0;

bool button2State = HIGH;
bool lastButton2State = HIGH;
uint32_t button2PressTime = 0;

bool button3State = HIGH;
bool lastButton3State = HIGH;
uint32_t button3PressTime = 0;

uint8_t led1 = LED_BUILTIN; // Onboard LED
uint8_t led2 = 16;          // TX Pin (We can use it as a GPIO LED output)
uint8_t led3 = 17;          // RX Pin (We can use it as a GPIO LED output)

void setLED1(bool value) { digitalWrite(led1, !value); }
void setLED2(bool value) { digitalWrite(led2, !value); }
void setLED3(bool value) { digitalWrite(led3, !value); }

void setup() {
  Serial.begin(115200);

  // Configure XIAO ESP32-C6 for External U.FL Antenna
  pinMode(3, OUTPUT);
  pinMode(14, OUTPUT);
  digitalWrite(3, LOW);
  delay(10);
  digitalWrite(14, HIGH);
  Serial.println("External Antenna Enabled");

  pinMode(led1, OUTPUT);
  digitalWrite(led1, LOW);
  pinMode(led2, OUTPUT);
  digitalWrite(led2, LOW);
  pinMode(led3, OUTPUT);
  digitalWrite(led3, LOW);

  pinMode(button1, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);
  pinMode(button3, INPUT_PULLUP);

  pinMode(A0, INPUT); // Configure A0 as ADC input

  zbLight1.setManufacturerAndModel("Espressif", "ZBLightBulb1");
  zbLight1.onLightChange(setLED1);

  zbLight2.setManufacturerAndModel("Espressif", "ZBLightBulb2");
  zbLight2.onLightChange(setLED2);

  zbLight3.setManufacturerAndModel("Espressif", "ZBLightBulb3");
  zbLight3.onLightChange(setLED3);

  // Set power source to battery ONLY on the first endpoint (reporting once is
  // enough for the device)
  zbLight1.setPowerSource(ZB_POWER_SOURCE_BATTERY, 100, 33);

  Serial.println("Adding ZigbeeLight endpoints to Zigbee Core");
  Zigbee.addEndpoint(&zbLight1);
  Zigbee.addEndpoint(&zbLight2);
  Zigbee.addEndpoint(&zbLight3);

  // Fixed to Channel 11 (HA coordinator channel)
  Zigbee.setPrimaryChannelMask(1 << 11);
  Zigbee.setScanDuration(4);
  Zigbee.setTimeout(60000);

  if (!Zigbee.begin()) {
    Serial.println("Zigbee failed to start!");
    Serial.println("Rebooting...");
    ESP.restart();
  }

  Serial.println("Connecting to network...");
  int dots = 0;
  while (!Zigbee.connected()) {
    Serial.print(".");
    dots++;
    if (dots % 50 == 0)
      Serial.println();
    delay(200);
  }
  Serial.println();
  Serial.println("Connected to Zigbee network!");
}

void loop() {
  // Battery reading every 60 seconds
  if (millis() - lastBatteryReportTime > 60000 || lastBatteryReportTime == 0) {
    if (lastBatteryReportTime == 0)
      lastBatteryReportTime = 1; // Prevent repeating 0
    else
      lastBatteryReportTime = millis();

    uint32_t Vbatt = 0;
    for (int i = 0; i < 16; i++) {
      Vbatt += analogReadMilliVolts(A0); // Read and accumulate ADC voltage
    }
    
    // ADC Calibration Factor: Adjust this to match your multimeter reading.
    // e.g., if multimeter says 4.19V and serial says 4.13V: 4.19 / 4.13 = 1.0145
    float calibration_factor = 1.0145;
    float Vbattf = (2.0 * Vbatt / 16.0 / 1000.0) * calibration_factor;

    // Calculate battery percentage (Assuming 4.2V Max, 3.2V Min for
    // LiPo/Li-ion) If your battery is different (e.g., CR2450 3.0V), you can change
    // the max_v and min_v values accordingly.
    float max_v = 4.2;
    float min_v = 3.2;
    float percentageReal = ((Vbattf - min_v) / (max_v - min_v)) * 100.0;
    if (percentageReal > 100.0)
      percentageReal = 100.0;
    if (percentageReal < 0.0)
      percentageReal = 0.0;
    uint8_t percentage = (uint8_t)percentageReal;

    uint8_t zigbee_voltage = (uint8_t)(Vbattf * 10.0); // Voltage in 100mV units

    zbLight1.setBatteryVoltage(zigbee_voltage);
    zbLight1.setBatteryPercentage(percentage);
    zbLight1.reportBatteryPercentage(); // Force update to HA

    Serial.print("Battery Voltage: ");
    Serial.print(Vbattf, 3);
    Serial.print("V, Percentage: ");
    Serial.print(percentage);
    Serial.println("%");
  }

  uint32_t currentMillis = millis();

  /* --- Button 1 Logic --- */
  bool currentButton1State = digitalRead(button1);
  if (currentButton1State != lastButton1State) {
    if (currentButton1State == LOW) { // Button 1 just pressed
      button1PressTime = currentMillis;
    } else { // Button 1 released
      if (currentMillis - button1PressTime > 50 &&
          currentMillis - button1PressTime < 3000) {
        // Short press (debounced, <3 sec)
        zbLight1.setLight(!zbLight1.getLightState());
      }
    }
  }
  // Check for long press while button is being held down
  if (currentButton1State == LOW && (currentMillis - button1PressTime > 3000)) {
    Serial.println("Resetting Zigbee to factory and rebooting in 1s.");
    delay(1000);
    Zigbee.factoryReset();
  }
  lastButton1State = currentButton1State;

  /* --- Button 2 Logic --- */
  bool currentButton2State = digitalRead(button2);
  if (currentButton2State != lastButton2State) {
    if (currentButton2State == LOW) { // Button 2 just pressed
      button2PressTime = currentMillis;
    } else { // Button 2 released
      if (currentMillis - button2PressTime > 50) {
        // Debounced press
        zbLight2.setLight(!zbLight2.getLightState());
      }
    }
  }
  lastButton2State = currentButton2State;

  /* --- Button 3 Logic --- */
  bool currentButton3State = digitalRead(button3);
  if (currentButton3State != lastButton3State) {
    if (currentButton3State == LOW) { // Button 3 just pressed
      button3PressTime = currentMillis;
    } else { // Button 3 released
      if (currentMillis - button3PressTime > 50) {
        // Debounced press
        zbLight3.setLight(!zbLight3.getLightState());
      }
    }
  }
  lastButton3State = currentButton3State;

  delay(10); // Small delay to prevent tight loop
}
