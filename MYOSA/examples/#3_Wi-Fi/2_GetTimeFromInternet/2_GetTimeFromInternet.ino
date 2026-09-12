/*
  This code is developed under the MYOSA (LearnTheEasyWay) initiative of MakeSense EduTech and Pegasus Automation.
  Code has been derived from internet sources and component datasheets.
  Existing readily-available libraries would have been used "AS IS" and modified for ease of learning purpose.
  
  Get Date and Time from Internet
  Connection: Connect the "Controller" board and power it up.
  Working: This example is intended to further demonstrate the capabilities of WiFi (Controller) board. Here the controller board connects to a existing WiFi network, uses the internet to get the current date and time information from the internet.

  Synopsis of MYOSA platform
  MYOSA uses an ESP32 controller with Wi-Fi and Bluetooth connectivity.
  The kit includes motion, temperature/humidity, pressure, light/proximity/gesture,
  and air-quality sensor boards, plus an OLED display and an actuator board.
  The actuator board provides relay and buzzer outputs.
  The libraries support individual sensor examples and combined BLE applications.

  NOTE
  All information, including URL references, is subject to change without prior notice.
  Please always use the latest versions of software-release for best performance.
  Unless required by applicable law or agreed to in writing, this software is distributed on an 
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied

  Modifications
  10 September, 2026 by Pegasus Automation
  (as a part of MYOSA Initiative)
  
  Contact Team MYOSA for any kind of feedback/issues pertaining to performance or any update request.
  Email: myosa.event@gmail.com
*/

/* Library Inclusion - WiFi.h is generic ESP32 library available */
#include <WiFi.h>

/* Enter your WiFi credentials here so that MYOSA Controller Board can connect to Internet. */
const char* ssid       = "MYOSAbyMakeSense";
const char* password   = "LearnTheEasyWay";

/* NTP Server Host Name Used - pool.ntp.org. It is available worldwide. */
const char* ntpServer = "pool.ntp.org";

/* Adjust UTC Offset for local timezone. For India, it is UTC +5:30 hrs (=19,800 seconds). */
const long  UTCOffset_sec = 19800;

/* If your country uses daylight savings, update that information accordingly below. India don't use it. Hence 0 (ZERO) is set. */
const int   daylightOffset_sec = 0;

bool timeSynced = false;
unsigned long lastReconnect = 0;

bool printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo, 1000)) {
    Serial.println("Waiting for network time...");
    return false;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  return true;
}

void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  configTime(UTCOffset_sec, daylightOffset_sec, ntpServer);
  lastReconnect = millis();
}

void loop()
{
  const bool clockReady = printLocalTime();
  if(!timeSynced && clockReady) {
    timeSynced = true;
    // Turn off the radio only after a successful time synchronization.
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
  } else if(!timeSynced && WiFi.status() != WL_CONNECTED && millis() - lastReconnect >= 30000UL) {
    lastReconnect = millis();
    WiFi.disconnect();
    WiFi.begin(ssid, password);
  }
  delay(2000);
}
