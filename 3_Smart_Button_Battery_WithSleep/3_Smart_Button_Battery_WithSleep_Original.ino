#ifndef ZIGBEE_MODE_ED
#error "Zigbee end device mode is not selected in Tools->Zigbee mode"
#endif

#include "Zigbee.h"
#include "driver/gpio.h"

#define ZIGBEE_SWITCH_ENDPOINT_1 10
#define ZIGBEE_SWITCH_ENDPOINT_2 11
#define ZIGBEE_SWITCH_ENDPOINT_3 12

uint8_t button1 = 1; // LP_GPIO1 (D1)
uint8_t button2 = 2; // LP_GPIO2 (D2)
uint8_t button3 = 4; // LP_GPIO4 (D4) - Must be an LP_GPIO for RTC wakeup

uint8_t led1 = LED_BUILTIN; // Onboard LED
uint8_t led2 = 16;          // TX Pin (We can use it as a GPIO LED output)
uint8_t led3 = 17;          // RX Pin (We can use it as a GPIO LED output)

#define uS_TO_S_FACTOR                                                         \
  1000000ULL /* Conversion factor for micro seconds to seconds */
// Only wake up periodically to report battery, e.g., every 1 hour (3600
// seconds)
#define TIME_TO_SLEEP 3600

// We use ZigbeeColorDimmerSwitch so we can send toggle and step up/down
// (dimming) commands
ZigbeeColorDimmerSwitch zbSwitch1 =
    ZigbeeColorDimmerSwitch(ZIGBEE_SWITCH_ENDPOINT_1);
ZigbeeColorDimmerSwitch zbSwitch2 =
    ZigbeeColorDimmerSwitch(ZIGBEE_SWITCH_ENDPOINT_2);
ZigbeeColorDimmerSwitch zbSwitch3 =
    ZigbeeColorDimmerSwitch(ZIGBEE_SWITCH_ENDPOINT_3);

// Button variables for gesture detection
typedef struct {
  uint8_t pin;
  bool lastState;
  uint32_t pressTime;
  bool isPressing;
  bool actionTriggered; // True if long press action already triggered
  uint32_t lastDimTime; // For throttling dim step commands
  bool dimDirection;    // false = down, true = up
} ButtonState;

ButtonState btn1 = {button1, HIGH, 0, false, false, 0, false};
ButtonState btn2 = {button2, HIGH, 0, false, false, 0, false};
ButtonState btn3 = {button3, HIGH, 0, false, false, 0, false};

// Helper for LED feedback
void triggerLED(uint8_t ledPin) {
  digitalWrite(ledPin, LOW); // Turn ON (assuming sinking current/active low or
                             // onboard reverse logic; adjust if needed)
  delay(50);
  digitalWrite(ledPin, HIGH); // Turn OFF
}

// Global wake-up flag to prevent immediate sleep if buttons are being held
bool keepAwake = false;
uint32_t wakeTime = 0;

// Global variables for deep sleep wakeup detection
uint64_t globalWakeupPinMask = 0;
bool processedWakeup = false;
uint32_t lastActivityTime = 0;

void setup() {
  Serial.begin(115200);

  // Configure XIAO ESP32-C6 for External U.FL Antenna
  // GPIO 3 (WIFI_ENABLE) must be LOW, and GPIO 14 (WIFI_ANT_CONFIG) must be
  // HIGH
  pinMode(3, OUTPUT);
  pinMode(14, OUTPUT);
  digitalWrite(3, LOW);
  delay(10);
  digitalWrite(14, HIGH);
  Serial.println("External Antenna Enabled");

  gpio_hold_dis((gpio_num_t)button1);
  gpio_hold_dis((gpio_num_t)button2);
  gpio_hold_dis((gpio_num_t)button3);

  // Determine if we woke up from deep sleep due to a button press
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1) {
    globalWakeupPinMask = esp_sleep_get_ext1_wakeup_status();
    Serial.printf("Woke up from EXT1 with mask: 0x%llX\n", globalWakeupPinMask);
  }

  wakeTime = millis();
  lastActivityTime = millis();

  pinMode(led1, OUTPUT);
  digitalWrite(led1, HIGH); // Wait state is HIGH (Off)
  pinMode(led2, OUTPUT);
  digitalWrite(led2, HIGH);
  pinMode(led3, OUTPUT);
  digitalWrite(led3, HIGH);

  pinMode(button1, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);
  pinMode(button3, INPUT_PULLUP);

  pinMode(A0, INPUT); // Configure A0 as ADC input

  zbSwitch1.setManufacturerAndModel("Espressif", "ZBSwitch1");
  zbSwitch2.setManufacturerAndModel("Espressif", "ZBSwitch2");
  zbSwitch3.setManufacturerAndModel("Espressif", "ZBSwitch3");

  // Optional: Allow switching multiple lights
  zbSwitch1.allowMultipleBinding(true);
  zbSwitch2.allowMultipleBinding(true);
  zbSwitch3.allowMultipleBinding(true);

  // Set power source to battery ONLY on the first endpoint
  zbSwitch1.setPowerSource(ZB_POWER_SOURCE_BATTERY, 100, 33);

  Serial.println("Adding ZigbeeSwitch endpoints to Zigbee Core");
  Zigbee.addEndpoint(&zbSwitch1);
  Zigbee.addEndpoint(&zbSwitch2);
  Zigbee.addEndpoint(&zbSwitch3);

  // Fixed to Channel 11 (HA coordinator channel)
  Zigbee.setPrimaryChannelMask(1 << 11);
  Zigbee.setScanDuration(4);
  Zigbee.setTimeout(60000);

  // Default Sleepy End Device Config
  esp_zb_cfg_t zigbeeConfig = ZIGBEE_DEFAULT_ED_CONFIG();
  zigbeeConfig.nwk_cfg.zed_cfg.keep_alive = 3000;

  if (!Zigbee.begin(&zigbeeConfig, false)) {
    Serial.println("Zigbee failed to start!");
    Serial.println("Rebooting...");
    ESP.restart();
  }

  // Set up deep sleep wakeups (Wake up when ANY of the 3 buttons goes LOW)
  // By using RTC pins (1, 2, 3) we can use ext1 wakeup which reliably keeps
  // pull-ups active during deep sleep
  uint64_t ext1_bitmask =
      (1ULL << button1) | (1ULL << button2) | (1ULL << button3);
  esp_sleep_enable_ext1_wakeup(ext1_bitmask, ESP_EXT1_WAKEUP_ANY_LOW);
  
  // Enable timer wakeup to report battery and keep connection alive
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  // Also keep RTC pins enabled during sleep
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

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
  uint32_t currentMillis = millis();

  // ----- Helper Lambda to Handle Button Logic -----
  auto handleButton = [&](ButtonState *btn, ZigbeeColorDimmerSwitch *sw,
                          uint8_t ledPin) {
    bool currentState = digitalRead(btn->pin);

    // If device just woke up due to THIS button, automatically override state
    // and send toggle command
    if (!processedWakeup && (globalWakeupPinMask & (1ULL << btn->pin))) {
      Serial.printf("Device woke up from Deep Sleep via Pin %d -> Toggle\n",
                    btn->pin);
      sw->lightToggle();
      triggerLED(ledPin);

      // Ensure we don't accidentally double-trigger if finger is still on
      // button
      btn->lastState = LOW;
      btn->isPressing = true;
      btn->pressTime = currentMillis;
      btn->actionTriggered = false;
      lastActivityTime = currentMillis;

      // We only process the wakeup pulse once per boot cycle
    } else if (currentState != btn->lastState) {
      lastActivityTime = currentMillis;
      if (currentState == LOW) {
        // Button just pressed
        btn->pressTime = currentMillis;
        btn->isPressing = true;
        btn->actionTriggered = false;
        triggerLED(ledPin);
      } else {
        // Button released
        btn->isPressing = false;
        // If released before long press threshold, and not already triggered
        if (!btn->actionTriggered && (currentMillis - btn->pressTime > 50)) {
          // Short press
          Serial.printf("Short Press on Pin %d -> Toggle\n", btn->pin);
          sw->lightToggle();
          triggerLED(ledPin);
        }
      }
    }

    // While button is held down (Long Press for Dimming)
    if (btn->isPressing && currentState == LOW) {
      lastActivityTime = currentMillis;           // Reset timeout while holding
      if (currentMillis - btn->pressTime > 500) { // 500ms hold threshold
        // If this is the very first moment we trigger long press, toggle dim
        // direction
        if (!btn->actionTriggered) {
          btn->dimDirection = !(btn->dimDirection);
        }
        btn->actionTriggered = true;

        if (currentMillis - btn->lastDimTime >
            200) { // Send dim tick every 200ms
          btn->lastDimTime = currentMillis;

          if (btn->dimDirection) {
            Serial.printf("Long Press on Pin %d -> Dim UP\n", btn->pin);
            sw->setLightLevelStep(ZIGBEE_LEVEL_STEP_UP, 10, 2);
          } else {
            Serial.printf("Long Press on Pin %d -> Dim DOWN\n", btn->pin);
            sw->setLightLevelStep(ZIGBEE_LEVEL_STEP_DOWN, 10, 2);
          }

          // Briefly blink LED to show dimming action
          digitalWrite(ledPin, LOW);
          delay(10);
          digitalWrite(ledPin, HIGH);
        }

        // Extremely long hold on Button 1 for Factory Reset
        if (btn->pin == button1 && currentMillis - btn->pressTime > 15000) {
          Serial.println("Factory Reset initiated!");
          delay(1000);
          Zigbee.factoryReset(false); // Reset without instant reboot
          esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
          esp_deep_sleep_start();
        }
      }
    }
    btn->lastState = currentState;
  };

  // Process all three buttons
  handleButton(&btn1, &zbSwitch1, led1);
  handleButton(&btn2, &zbSwitch2, led2);
  handleButton(&btn3, &zbSwitch3, led3);

  // Mark the global wakeup flag as processed so it doesn't fire continuously in
  // loop
  processedWakeup = true;

  // Measure and send battery periodically (if woken by timer, or before going
  // to sleep occasionally) For simplicity, we send it before we go to sleep
  static bool batteryReported = false;
  if (!keepAwake && !batteryReported) {
    uint32_t Vbatt = 0;
    for (int i = 0; i < 16; i++)
      Vbatt += analogReadMilliVolts(A0);
      
    // ADC Calibration Factor: Adjust this to match your multimeter reading.
    // e.g., if multimeter says 4.19V and serial says 4.13V: 4.19 / 4.13 = 1.0145
    float calibration_factor = 1.0145; 
    float Vbattf = (2.0 * Vbatt / 16.0 / 1000.0) * calibration_factor;

    float max_v = 4.2;
    float min_v = 3.2;
    float percentageReal = ((Vbattf - min_v) / (max_v - min_v)) * 100.0;
    if (percentageReal > 100.0)
      percentageReal = 100.0;
    if (percentageReal < 0.0)
      percentageReal = 0.0;

    zbSwitch1.setBatteryVoltage((uint8_t)(Vbattf * 10.0));
    zbSwitch1.setBatteryPercentage((uint8_t)percentageReal);
    zbSwitch1.reportBatteryPercentage(); // Force update to HA

    Serial.printf("Battery: %.2fV (%d%%)\n", Vbattf, (uint8_t)percentageReal);
    batteryReported = true;
  }

  bool isPaired = Zigbee.connected();
  uint32_t timeoutDuration =
      isPaired ? 10000 : 60000; // 10s if normal, 60s if pairing

  if (currentMillis - lastActivityTime < timeoutDuration) {
    keepAwake = true;
  } else {
    keepAwake = false;
  }

  // Go back to sleep if no buttons have activity
  if (!keepAwake) {
    gpio_hold_en((gpio_num_t)button1);
    gpio_hold_en((gpio_num_t)button2);
    gpio_hold_en((gpio_num_t)button3);

    // Wait slightly to ensure Zigbee packets are actually transmitted before
    // MCU dies
    delay(500);
    Serial.println("Sleeping...");
    esp_deep_sleep_start();
  }

  delay(10); // Loop delay
}