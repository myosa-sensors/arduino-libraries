/*
  This code is developed under the MYOSA (LearnTheEasyWay) initiative of MakeSense EduTech and Pegasus Automation.
  Code has been derived from internet sources and component datasheets.
  Existing readily-available libraries would have been used "AS IS" and modified for ease of learning purpose.
  
  Master Code
  Connection: Connect all the boards from the MYOSA kit with the "Controller" board and power them up.
  Working: Controller board will display data (on the OLED board) from all the modules in cyclic fashion demonstrating complete RAW capabilities of the kit.

  Synopsis of MYOSA platform
  MYOSA uses an ESP32 controller with Wi-Fi and Bluetooth connectivity.
  The kit includes motion, temperature/humidity, pressure, light/proximity/gesture
  and air-quality boards, with an OLED and relay/buzzer outputs.
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
#include <myosa.h>

/* Create Object of MYOSA class */
MYOSA myosa;

/* Set the timer to zero */
uint32_t previousMillis = 0; // Last screen update time.

/* Global Constants */
const uint32_t perModuleInterval = 1500; // Time per OLED page in milliseconds.
uint8_t nScreen = 0u;

/* Setup Function */
void setup(void)
{

  /* Setting up communication */
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(100000);

  /* This function initializes all the modules attached. */
  if(!myosa.begin())
    Serial.println("Master code running with the available boards.");
}

/* Loop Function */
void loop(void)
{

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
        myosa.printAirQuality();
        nScreen = 5u;
        break;
      case 5u:
        myosa.printBarometricPressure();
        nScreen = 6u;
        break;
      case 6u:
        myosa.printBarometricPressure();
        nScreen = 7u;
        break;
      case 7u:
        myosa.printLightProximityAndGesture();
        nScreen = 8u;
        break;
      case 8u:
        myosa.printLightProximityAndGesture();
        nScreen = 9u;
        break;
      case 9u:
        myosa.printTempAndHumidity();
        nScreen = 10u;
        break;
      case 10u:
        myosa.printTempAndHumidity();
        nScreen = 0u;
        break;
      default:
        nScreen = 0u;
        break;
    }
    myosa.sendBleData();
  }
  delay(1);
}
