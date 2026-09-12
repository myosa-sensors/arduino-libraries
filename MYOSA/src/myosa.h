/*
  This code is developed under the MYOSA (LearnTheEasyWay) initiative of MakeSense EduTech and Pegasus Automation.
  Code has been derived from internet sources and component datasheets.
  Existing readily-available libraries would have been used "AS IS" and modified for ease of learning purpose.

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

#ifndef __MYOSA_H__
#define __MYOSA_H__

/* Library Inclusion */
#include <AccelAndGyro.h>
#include <AirQuality.h>
#include <BarometricPressure.h>
#include <LightProximityAndGesture.h>
#include <oled.h>
#include <Actuator.h>
#include <TempAndHumidity.h>
#include <BLEDevice.h>
#include <BLE2902.h>
#include "MyosaEvents.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "VeraMono7pt7b.h"
#include "VeraMonoBold7pt7b.h"
#include "VeraMonoItalic7pt7b.h"

#define RELAY_IO IO0
#define BUZZER_IO IO1
/* define OLED display instance */
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET 4 // Reset pin # (or -1 if sharing Arduino reset pin)

/*
 * Initialize the shared bus before begin(); the combined class owns board and BLE setup.
 * sendBleData() retains the six legacy services and their event command fields.
 * Display helpers use the initialized OLED and preserve the existing BLE field order.
 */
class MYOSA
{
  public:
    /* Create sensor objects */
    AccelAndGyro Ag;
    AirQuality Aq;
    BarometricPressure Pr;
    LightProximityAndGesture Lpg;
    Actuator gpioExpander;
    TempAndHumidity Th;
    oLed display;

    /* Variables */
    float altitude = 0.f;
    uint16_t *rgbProportion = nullptr;

    /* functions */
    MYOSA();
    bool begin(void);
    void printAceelAndGyro(void);
    void printAirQuality(void);
    void printBarometricPressure(void);
    void printLightProximityAndGesture(void);
    void printTempAndHumidity(void);
    void turnOnRelay(void);
    void turnOffRelay(void);
    void turnOnBuzzer(int print);
    void turnOffBuzzer(int print);
    void drawCentreString(const String &buf);
    void drawDegreeSymbol(void);
    void drawSuperscriptSymbol(const String &buf);
    void drawSubscriptSymbol(const String &buf);
    void sendBleData(void);

  private:
    bool _displayReady = false;
    bool _eventControlled[2] = {};
};

#endif
