/*
  This code is developed under the MYOSA (LearnTheEasyWay) initiative of MakeSense EduTech and Pegasus Automation.
  Code has been derived from internet sources and component datasheets.
  Existing readily-available libraries would have been used "AS IS" and modified for ease of learning purpose.
  
  Scanning Near By WiFi
  Connection: Connect the "Controller" board and power it up.
  Working: This example scans the Nearby WiFi networks along with relevant details.

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
#include "WiFi.h"

/* Setup Function */
void setup()
{
    /* Setting up the communication */
    Serial.begin(115200);

    /* Set WiFi to station mode */
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    Serial.println("Setup done");
}

/* Loop Function */
void loop()
{

    /* Loop function continously scans the available WiFi networks with detailed RSSI values every 5 seconds. */
    Serial.println("Scanning...");

    int n = WiFi.scanNetworks();
    
    if(n < 0) {
        Serial.println("WiFi scan failed; retrying shortly.");
    }
    else if (n == 0) {
        Serial.println("No nearby networks found");
    }
    else {
        Serial.print(n);
        Serial.println(" networks found\n");
        for (int i = 0; i < n; ++i) {
            // Print SSID and RSSI for each network found
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            int rssi = WiFi.RSSI(i);
            Serial.print(rssi);
            Serial.print(")");
            if(rssi > -85)
            {
              Serial.print(" Good Signal Strength ");
            }
            else if(rssi > -100)
            {
              Serial.print(" Fair Signal Strength ");
            }
            else if(rssi > -110)
            {
              Serial.print(" Poor Signal Strength ");
            }
            else if(rssi > -120)
            {
              Serial.print(" No Signal ");
            }
            Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
            
            delay(200);
        }
    }
    Serial.println("\nScanning done");
    Serial.println("");

    WiFi.scanDelete();
    // Wait a bit before scanning again
    delay(5000);
}