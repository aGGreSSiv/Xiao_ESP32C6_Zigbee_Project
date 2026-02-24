#ifndef ZIGBEE_MODE_ED
#error "Zigbee end device mode is not selected in Tools->Zigbee mode"
#endif

#include "Zigbee.h"

#define ZIGBEE_LIGHT_ENDPOINT 10
uint8_t led = LED_BUILTIN;
uint8_t button = 1;

uint32_t lastBatteryReportTime = 0;

ZigbeeLight zbLight = ZigbeeLight(ZIGBEE_LIGHT_ENDPOINT);

void setLED(bool value) {
  digitalWrite(led, !value);
}

void setup() {
  Serial.begin(115200);

  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
  pinMode(button, INPUT_PULLUP);
  pinMode(A0, INPUT);         // Configure A0 as ADC input

  zbLight.setManufacturerAndModel("Espressif", "ZBLightBulb");
  zbLight.onLightChange(setLED);

  // Set power source to battery (Percentage: 100, Voltage: 33 = 3.3V initial default)
  zbLight.setPowerSource(ZB_POWER_SOURCE_BATTERY, 100, 33);

  Serial.println("Adding ZigbeeLight endpoint to Zigbee Core");
  Zigbee.addEndpoint(&zbLight);

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
    if (dots % 50 == 0) Serial.println();
    delay(200);
  }
  Serial.println();
  Serial.println("Connected to Zigbee network!");
}

void loop() {
  // Battery reading every 60 seconds
  if (millis() - lastBatteryReportTime > 60000 || lastBatteryReportTime == 0) {
    if (lastBatteryReportTime == 0) lastBatteryReportTime = 1; // Prevent repeating 0
    else lastBatteryReportTime = millis();
    
    uint32_t Vbatt = 0;
    for(int i = 0; i < 16; i++) {
      Vbatt += analogReadMilliVolts(A0); // Read and accumulate ADC voltage
    }
    
    // ADC Calibration Factor: Adjust this to match your multimeter reading.
    // e.g., if multimeter says 4.19V and serial says 4.13V: 4.19 / 4.13 = 1.0145
    float calibration_factor = 1.0145; 
    float Vbattf = (2.0 * Vbatt / 16.0 / 1000.0) * calibration_factor;
    
    // Calculate battery percentage (Assuming 4.2V Max, 3.2V Min for LiPo/Li-ion)
    // If your battery is different (e.g., CR2450 3.0V), you can change the max_v and min_v values accordingly.
    float max_v = 4.2;
    float min_v = 3.2;
    float percentageReal = ((Vbattf - min_v) / (max_v - min_v)) * 100.0;
    if (percentageReal > 100.0) percentageReal = 100.0;
    if (percentageReal < 0.0) percentageReal = 0.0;
    uint8_t percentage = (uint8_t)percentageReal;
    
    uint8_t zigbee_voltage = (uint8_t)(Vbattf * 10.0); // Voltage in 100mV units
    
    zbLight.setBatteryVoltage(zigbee_voltage);
    zbLight.setBatteryPercentage(percentage);
    zbLight.reportBatteryPercentage(); // Force update to HA
    
    Serial.print("Battery Voltage: ");
    Serial.print(Vbattf, 3);
    Serial.print("V, Percentage: ");
    Serial.print(percentage);
    Serial.println("%");
  }

  if (digitalRead(button) == LOW) {
    delay(100);
    int startTime = millis();
    while (digitalRead(button) == LOW) {
      delay(50);
      if ((millis() - startTime) > 3000) {
        Serial.println("Resetting Zigbee to factory and rebooting in 1s.");
        delay(1000);
        Zigbee.factoryReset();
      }
    }
    zbLight.setLight(!zbLight.getLightState());
  }
  delay(100);

  
}
