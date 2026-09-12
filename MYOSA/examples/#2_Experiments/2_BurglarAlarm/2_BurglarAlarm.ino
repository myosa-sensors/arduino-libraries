/*
  This code is developed under the MYOSA (LearnTheEasyWay) initiative of MakeSense EduTech and Pegasus Automation.
  Code has been derived from internet sources and component datasheets.
  Existing readily-available libraries would have been used "AS IS" and modified for ease of learning purpose.
  
  Burglar Alarm
  Connection: Connect the Light, Proximity, and Gesture and OLED boards to the controller. Use its buzzer on GPIO 12.
  Working: Light above 10 lux sounds the buzzer and displays an alert. Darkness clears the alarm.

  Synopsis of MYOSA platform
  MYOSA uses an ESP32 controller with Wi-Fi and Bluetooth connectivity.
  The kit includes motion, pressure, light/proximity/gesture, VL53L0X distance
  and MAX30100 heart-rate/SpO2 boards, with an OLED for measurements.
  The controller provides an LED on GPIO 2 and an active-high buzzer on GPIO 12.
  The libraries support individual sensor examples and combined BLE applications.

  NOTE
  All information, including URL references, is subject to change without prior notice.
  Please always use the latest versions of software-release for best performance.
  Unless required by applicable law or agreed to in writing, this software is distributed on an 
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied

  Modifications
  11 September, 2026 by Pegasus Automation
  (as a part of MYOSA Initiative)
  
  Contact Team MYOSA for any kind of feedback/issues pertaining to performance or any update request.
  Email: myosa.event@gmail.com
*/

/* Library Inclusion */
#include <LightProximityAndGesture.h>
#include <oled.h>

/* Creating Objects of LightProximityAndGesture and oLed Classes */
LightProximityAndGesture Lpg;
oLed display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1, 100000UL, 100000UL);
bool displayReady = false;

/* Global Constants */
const uint8_t BUZZER_PIN = 12u; // Active-high controller buzzer.
const float LIGHT_THRESHOLD_LUX = 10.0f; // Adjust for the locker lighting; this is a lux threshold.
const uint32_t READING_INTERVAL_MS = 150u;

/* Function Declaration */
void showAlarmStatus(const char *title, const char *message);

/* Setup Function */
void setup(void)
{
  /* Keep the buzzer off during initialization. */
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  /* Setting up communication */
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(100000);

  displayReady = display.begin();
  if(!displayReady)
    Serial.printf("OLED initialization failed; buzzer and Serial remain available.\n");
  showAlarmStatus("Starting", "Waiting for sensor");

  /* Initialize ambient-light sensing without interrupts. */
  for(;;)
  {
    if(Lpg.begin() && Lpg.enableAmbientLightSensor(DISABLE))
    {
      Serial.printf("Ambient light sensor is connected and running.\n");
      break;
    }
    Serial.printf("Ambient light sensor initialization failed; retrying.\n");
    delay(500u);
  }
  delay(500u);
  showAlarmStatus("Monitoring", "Waiting for reading");
}

/* Loop Function */
void loop(void)
{
  const float ambientLightLux = Lpg.getAmbientLightLux(false);
  if(!isfinite(ambientLightLux))
  {
    /* Keep the last buzzer state; an unavailable reading does not mean darkness. */
    Serial.printf("Ambient light unavailable; buzzer state unchanged.\n");
    showAlarmStatus("Sensor", "Light unavailable");
    delay(READING_INTERVAL_MS);
    return;
  }

  Serial.printf("Ambient Light: %.2f lux\n", ambientLightLux);
  if(ambientLightLux > LIGHT_THRESHOLD_LUX)
  {
    digitalWrite(BUZZER_PIN, HIGH);
    Serial.printf("Alert! Alert! Alert! Burglar Detected...\n");
    showAlarmStatus("ALERT!", "ALERT! Light detected");
  }
  else
  {
    digitalWrite(BUZZER_PIN, LOW);
    showAlarmStatus("Monitoring", "Locker is dark");
  }
  delay(READING_INTERVAL_MS);
}

/* Draw the current alarm or sensor status on the OLED. */
void showAlarmStatus(const char *title, const char *message)
{
  if(!displayReady)
    return;

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.printf("%s", title);
  display.setTextSize(1);
  display.setCursor(0, 32);
  display.printf("%s", message);
  display.display();
}
