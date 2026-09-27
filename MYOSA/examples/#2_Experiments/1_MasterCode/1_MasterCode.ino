/*
  This code is developed under the MYOSA (LearnTheEasyWay) initiative of MakeSense EduTech and Pegasus Automation.
  Code has been derived from internet sources and component datasheets.
  Existing readily-available libraries would have been used "AS IS" and modified for ease of learning purpose.
  
  Master Code
  Connection: Connect all the boards from the MYOSA kit with the "Controller" board and power them up.
  Working: Controller board will display data (on the OLED board) from all the modules in cyclic fashion demonstrating complete RAW capabilities of the kit.

  Synopsis of MYOSA platform
  MYOSA uses an ESP32 controller with Wi-Fi and Bluetooth connectivity.
  The kit includes motion, pressure, light/proximity/gesture, VL53L0X distance
  and MAX30100 heart-rate/SpO2 boards, with an OLED for measurements.
  The libraries support individual sensor examples and combined Wi-Fi and BLE applications.

  NOTE
  All information, including URL references, is subject to change without prior notice.
  Please always use the latest versions of software-release for best performance.
  Unless required by applicable law or agreed to in writing, this software is distributed on an 
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied

  Modifications
  14 September, 2026 by Pegasus Automation
  (as a part of MYOSA Initiative)
  
  Contact Team MYOSA for any kind of feedback/issues pertaining to performance or any update request.
  Email: myosa.event@gmail.com
*/

/* Library Inclusion */
#include <myosa.h>
#include <myosaWeb.h>

/* Connection settings: Wi-Fi always runs the hotspot and station together. */
enum ConnectionMode { WIFI, BLE };
constexpr ConnectionMode mode = BLE;
const char *ap_ssid = "MYOSA";
const char *ap_password = "myosa1234"; // At least 8 characters.
const char *sta_ssid = "";             // Empty keeps the hotspot available on its own.
const char *sta_password = "";

/* Create Object of MYOSA class */
MYOSA myosa;
MYOSAWeb *web = nullptr;

/* Set the timer to zero */
uint32_t previousMillis = 0; // Last screen update time.

/* Global Constants */
const uint32_t perModuleInterval = 1500; // Time per OLED page in milliseconds.
uint8_t nScreen = 0u;
uint32_t previousBleMillis = 0;

/* Setup Function */
void setup(void)
{

  /* Setting up communication */
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(100000);

  /* Start the selected radio before optical sampling begins. */
  if(mode == WIFI)
  {
    web = new MYOSAWeb(myosa);
    if(!web->begin(ap_ssid, ap_password, sta_ssid, sta_password))
      Serial.printf("Wi-Fi startup failed. Check the hotspot name and password.\n");
  }
  else if(!myosa.beginBle())
    Serial.printf("BLE startup failed. Check Serial output and restart the controller.\n");

  /* This function initializes all the modules attached. */
  if(!myosa.beginSensors())
    Serial.println("Master code running with the available boards.");

  if(mode == WIFI && web)
    web->printConnectionInfo();
  else
    Serial.printf("Open MYOSA/extras/web/MYOSA_BLE.html in a supported browser and select Connect BLE.\n");
}

/* Loop Function */
void loop(void)
{
  /* Keep sampling and network work active between OLED pages. */
  myosa.update();

  if(mode == WIFI && web)
    web->update();
  if(mode == BLE && uint32_t(millis() - previousBleMillis) >= 1000)
  {
    previousBleMillis = millis();
    myosa.sendBleData(); // Queue a snapshot every second; update() spaces its notifications.
  }

  /* Cycle through the connected modules and print each page to Serial and the OLED. */
  uint32_t currentMillis = millis();
  if(currentMillis - previousMillis >= perModuleInterval)
  {
    previousMillis = currentMillis;
    switch(nScreen)
    {
      case 0u:
        myosa.printAceelAndGyro();
        nScreen = 1u;
        break;
      case 1u:
        myosa.printAceelAndGyro();
        nScreen = 2u;
        break;
      case 2u:
        myosa.printAceelAndGyro();
        nScreen = 3u;
        break;
      case 3u:
        myosa.printAceelAndGyro();
        nScreen = 4u;
        break;
      case 4u:
        myosa.printBarometricPressure();
        nScreen = 5u;
        break;
      case 5u:
        myosa.printBarometricPressure();
        nScreen = 6u;
        break;
      case 6u:
        myosa.printLightProximityAndGesture();
        nScreen = 7u;
        break;
      case 7u:
        myosa.printLightProximityAndGesture();
        nScreen = 8u;
        break;
      case 8u:
        myosa.printProximityAndDistance();
        nScreen = 9u;
        break;
      case 9u:
        myosa.printHeartRateAndSpO2();
        nScreen = mode == WIFI ? 10u : 0u;
        break;
      case 10u:
        if(mode == WIFI && web)
          web->printConnectionInfo();
        nScreen = 0u;
        break;
      default:
        nScreen = 0u;
        break;
    }
  }
  myosa.update();
  delay(1);
}
