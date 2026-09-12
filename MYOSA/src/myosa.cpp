/*
  This code is developed under the MYOSA (LearnTheEasyWay) initiative of MakeSense EduTech and Pegasus Automation.
  Code has been derived from internet sources and component datasheets.
  Existing readily-available libraries would have been used "AS IS" and modified for ease of learning purpose.
  
  Synopsis of MYOSA platform
  MYOSA uses an ESP32 controller with Wi-Fi and Bluetooth connectivity.
  The kit includes motion, pressure, light/proximity/gesture, VL53L0X distance
  and MAX30100 heart-rate/SpO2 boards, with an OLED for measurements.
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
#include "myosa.h"

#define NUM_SERVICES 8
#define MAX_CHARACTERISTICS 5

static BLECharacteristic *pCharacteristics[NUM_SERVICES][MAX_CHARACTERISTICS] = {};
// Logical group indices retain the existing sensor characteristic UUID suffixes.
static const uint8_t characteristicCounts[NUM_SERVICES] = {4, 0, 3, 3, 0, 0, 1, 2};
static const uint8_t parameterCounts[NUM_SERVICES][4] = {
    {3, 3, 3, 2}, {}, {2, 3, 1, 0}, {1, 1, 3, 0}, {}, {}, {1, 0, 0, 0}, {1, 1, 0, 0}};

class MyServerCallbacks : public BLEServerCallbacks
{
    void onDisconnect(BLEServer *server) override
    {
      server->getAdvertising()->start();
    }
};

MYOSA::MYOSA()
    : Ag(MPU6050_ADDRESS_AD0_HIGH),     /* Accelerometer and Gyroscope sensor init */
      Pr(ULTRA_HIGH_RESOLUTION),        /* Barometric Pressure sensor init */
      Lpg(),                            /* Light, Proximity and Gesture sensor init */
      Hr(),                             /* MAX30100 optical sensor init */
      Pd(),                             /* VL53L0X distance sensor init */
      display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) /* OLED display */
{
}

/*
 * Create BLE services and initialize the supplied boards; false means at least one setup step failed.
 */
bool MYOSA::begin(void)
{
  BLEDevice::init("MYOSA_1");
  BLEServer *pServer = BLEDevice::createServer();
  if(!pServer)
    return false;
  pServer->setCallbacks(new MyServerCallbacks());

  // Keep existing sensor UUIDs; the optical values share the service ending in b5.
  BLEService *pService = nullptr;
  for(int i = 0; i < NUM_SERVICES; ++i)
  {
    const uint8_t count = characteristicCounts[i];
    if(count == 0)
      continue;
    if(i != 7)
    {
      char serviceUUID[37];
      const int serviceId = i == 6 ? 5 : i;
      sprintf(serviceUUID, "4fafc201-1fb5-459e-8fcc-c5c9c33191b%d", serviceId);
      Serial.println(serviceUUID);
      const uint32_t numHandles = 1 + 3 * (i == 6 ? count + characteristicCounts[7] : count);
      pService = pServer->createService(BLEUUID(serviceUUID), numHandles);
      if(!pService)
        return false;
    }
    for(int j = 0; j < count; ++j)
    {
      char characteristicUUID[37];
      sprintf(characteristicUUID, "beb5483e-36e1-4688-b7f5-ea07361b2b%d%d", i, j);
      Serial.println(characteristicUUID);
      pCharacteristics[i][j] =
          pService->createCharacteristic(characteristicUUID, BLECharacteristic::PROPERTY_NOTIFY);
      if(!pCharacteristics[i][j])
        return false;
      pCharacteristics[i][j]->addDescriptor(new BLE2902());
    }
    // Start the shared optical service only after all of its values are registered.
    if(i != 6 && !pService->start())
      return false;
  }

  pServer->getAdvertising()->start();
  Serial.println("BLE server started!");

  bool ready = true;
  _displayReady = display.begin();
  ready = _displayReady && ready;
  ready = Ag.begin() && ready;
  ready = Pr.begin() && ready;
  const bool lightReady = Lpg.begin();
  ready = lightReady && ready;
  if(lightReady)
  {
    ready = Lpg.enableAmbientLightSensor(DISABLE) && ready;
    ready = Lpg.enableProximitySensor(DISABLE) && ready;
    ready = Lpg.setProximityGain(PGAIN_2X) && ready;
  }
  // Start optical sampling last, after the other boards' blocking startup sequences.
  ready = initializeProximityAndDistance() && ready;
  ready = initializeHeartRateAndSpO2() && ready;
  if(!ready)
    Serial.println("One or more MYOSA boards could not be initialized.");
  return ready;
}

/*
 * Center text horizontally using its bounds in the currently selected font.
 */
void MYOSA::drawCentreString(const String &buf)
{
  if(!_displayReady || buf.length() == 0)
    return;
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(buf, display.getCursorX(), display.getCursorY(), &x1, &y1, &w,
                        &h); //calc width of new string
  display.setCursor((SCREEN_WIDTH - w) / 2, y1 + h);
  display.println(buf);
}

/*
 * Draw a smaller symbol below the current text baseline.
 */
void MYOSA::drawSubscriptSymbol(const String &buf)
{
  if(!_displayReady || buf.length() == 0)
    return;
  display.setFont(nullptr);
  display.setCursor(display.getCursorX(), display.getCursorY() + 3);
  display.print(buf);
  display.setCursor(display.getCursorX(), display.getCursorY() - 3);
  display.setFont(&VeraMono7pt7b);
}

/*
 * Draw a smaller symbol above the current text baseline.
 */
void MYOSA::drawSuperscriptSymbol(const String &buf)
{
  if(!_displayReady || buf.length() == 0)
    return;
  display.setFont(nullptr);
  display.setCursor(display.getCursorX(), display.getCursorY() - 6);
  display.print(buf);
  display.setCursor(display.getCursorX(), display.getCursorY() + 6);
  display.setFont(&VeraMono7pt7b);
}

/*
 * Draw the degree marker used by the sensor temperature pages.
 */
void MYOSA::drawDegreeSymbol(void)
{
  if(!_displayReady)
    return;
  display.drawCircle(display.getCursorX() + 4, display.getCursorY() - 8, 2, 1);
  display.setCursor(display.getCursorX() + 6, display.getCursorY());
}

// void MYOSA::findIndex(void)
// {

// }

/*
 * Collect one sensor snapshot and send notifications only for valid readings.
 */
void MYOSA::sendBleData(void)
{
  serviceHeartRateAndSpO2();
  float readings[NUM_SERVICES][4][3];
  for(auto &service : readings)
    for(auto &characteristic : service)
      for(float &v : characteristic)
        v = NAN;
  if(Ag.ping())
  {
    readings[0][0][0] = Ag.getAccelX(false);
    readings[0][0][1] = Ag.getAccelY(false);
    readings[0][0][2] = Ag.getAccelZ(false);
    readings[0][1][0] = Ag.getGyroX(false);
    readings[0][1][1] = Ag.getGyroY(false);
    readings[0][1][2] = Ag.getGyroZ(false);
    readings[0][2][0] = Ag.getTiltX(false);
    readings[0][2][1] = Ag.getTiltY(false);
    readings[0][2][2] = Ag.getTiltZ(false);
    readings[0][3][0] = Ag.getTempC(false);
    readings[0][3][1] = readings[0][3][0] * 1.8f + 32.f;
  }
  serviceHeartRateAndSpO2();
  if(Pr.ping())
  {
    readings[2][0][0] = Pr.getTempC(false);
    readings[2][0][1] = readings[2][0][0] * 1.8f + 32.f;
    const int32_t pressure = Pr.getPressure();
    if(pressure != BMP180_ERROR && pressure > 0)
    {
      readings[2][1][0] = pressure / 1000.f;
      readings[2][1][1] = pressure / 133.322368f;
      readings[2][1][2] = pressure / 100.f;
      readings[2][2][0] =
          44330.f * (1.f - powf((pressure / 100.f) / SEA_LEVEL_AVG_PRESSURE, 1.f / 5.255f));
    }
  }
  serviceHeartRateAndSpO2();
  if(Lpg.ping())
  {
    const float ambient = Lpg.getAmbientLightLux(false);
    if(Lpg.lightReadingValid())
      readings[3][0][0] = ambient;
    readings[3][1][0] = Lpg.getProximity(false);
    const uint16_t *rgb = Lpg.getRGBProportion(false);
    if(Lpg.lightReadingValid())
      for(uint8_t i = 0; i < 3; ++i)
        readings[3][2][i] = rgb[i];
  }
  serviceHeartRateAndSpO2();
  // New sensor groups use cached values; unavailable readings are never sent as zero.
  if(_distanceReady && _distanceHasSample && uint32_t(millis() - _distanceSampleTime) <= 1000 &&
     _distanceMillimeters >= 30 && _distanceMillimeters <= 2000)
    readings[6][0][0] = _distanceMillimeters;
  if(_heartReady)
  {
    readings[7][0][0] = Hr.getHeartRate(false);
    readings[7][1][0] = Hr.getSpO2(false);
  }
  for(uint8_t service = 0; service < NUM_SERVICES; ++service)
  {
    for(uint8_t characteristic = 0; characteristic < characteristicCounts[service];
        ++characteristic)
    {
      const uint8_t count = parameterCounts[service][characteristic];
      const float *v = readings[service][characteristic];
      bool valid = true;
      for(uint8_t i = 0; i < count; ++i)
        valid = isfinite(v[i]) && valid;
      if(!valid || !pCharacteristics[service][characteristic])
        continue;
      char payload[96];
      int size;
      const bool integers = service == 6 || (service == 3 && characteristic != 1);
      if(integers && count == 1)
        size = snprintf(payload, sizeof(payload), "%.0f", v[0]);
      else if(integers)
        size = snprintf(payload, sizeof(payload), "%.0f, %.0f, %.0f", v[0], v[1], v[2]);
      else if(count == 1)
        size = snprintf(payload, sizeof(payload), "%.2f", v[0]);
      else if(count == 2)
        size = snprintf(payload, sizeof(payload), "%.2f, %.2f", v[0], v[1]);
      else
        size = snprintf(payload, sizeof(payload), "%.2f, %.2f, %.2f", v[0], v[1], v[2]);
      if(size < 0 || size >= (int)sizeof(payload))
        continue;
      pCharacteristics[service][characteristic]->setValue(payload);
      pCharacteristics[service][characteristic]->notify();
      serviceHeartRateAndSpO2();
    }
  }

}

/*
 * Print the acceleration/rotation page to Serial and the initialized OLED.
 */
void MYOSA::printAceelAndGyro(void)
{
  if(!_displayReady)
    return;
  static uint8_t nCnt = 0;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setFont(&VeraMonoBold7pt7b);
  display.setCursor(0, 9);
  drawCentreString("AccelGyro: Raw");
  if(Ag.ping())
  {
    display.setFont(&VeraMono7pt7b);
    if(nCnt == 0u)
    {
      display.print("aX:");
      display.print(Ag.getAccelX(true), 1);
      display.print("cm/s");
      drawSuperscriptSymbol("2");
      display.println();

      display.print("aY:");
      display.print(Ag.getAccelY(true), 1);
      display.print("cm/s");
      drawSuperscriptSymbol("2");
      display.println();

      display.print("aZ:");
      display.print(Ag.getAccelZ(true), 1);
      display.print("cm/s");
      drawSuperscriptSymbol("2");
    }
    else if(nCnt == 1u)
    {
      display.print("gX:");
      display.print(Ag.getGyroX(true), 1);
      drawDegreeSymbol();
      display.println("/s");
      display.print("gY:");
      display.print(Ag.getGyroY(true), 1);
      drawDegreeSymbol();
      display.println("/s");
      display.print("gZ:");
      display.print(Ag.getGyroZ(true), 1);
      drawDegreeSymbol();
      display.println("/s");
    }
    else if(nCnt == 2u)
    {
      display.print("tiltX:");
      display.print(Ag.getTiltX(true), 1);
      drawDegreeSymbol();
      display.println();
      display.print("tiltY:");
      display.print(Ag.getTiltY(true), 1);
      drawDegreeSymbol();
      display.println();
      display.print("tiltZ:");
      display.print(Ag.getTiltZ(true), 1);
      drawDegreeSymbol();
      display.println();
    }
    else
    {
      display.print("Temp:");
      display.print(Ag.getTempC(true), 1);
      drawDegreeSymbol();
      display.println("C");
      display.print("Temp:");
      display.print(Ag.getTempF(true), 1);
      drawDegreeSymbol();
      display.println("F");
    }
  }
  else
  {
    Serial.println("Accelerometer and Gyroscope: Not connected");
    display.setFont(&VeraMono7pt7b);
    display.println("Not connected");
  }
  Serial.println();
  serviceHeartRateAndSpO2();
  display.display();
  serviceHeartRateAndSpO2();
  nCnt = (nCnt + 1) & 3;
}

/*
 * Print the pressure page to Serial and the initialized OLED.
 */
void MYOSA::printBarometricPressure(void)
{
  if(!_displayReady)
    return;
  static uint8_t nCnt = 0;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setFont(&VeraMonoBold7pt7b);
  display.setCursor(0, 9);
  drawCentreString("Pressure");
  if(Pr.ping())
  {
    display.setFont(&VeraMono7pt7b);
    if(nCnt == 0u)
    {
      display.print("Temp:");
      display.print(Pr.getTempC(true), 1);
      serviceHeartRateAndSpO2();
      drawDegreeSymbol();
      display.println("C");
      display.print("Temp:");
      display.print(Pr.getTempF(true), 1);
      serviceHeartRateAndSpO2();
      drawDegreeSymbol();
      display.println("F");
      display.print("Alti:");
      display.print(Pr.getAltitude(SEA_LEVEL_AVG_PRESSURE), 1);
      serviceHeartRateAndSpO2();
      display.println("m");
    }
    else
    {
      display.print("Pres:");
      display.print(Pr.getPressurePascal(true), 1);
      serviceHeartRateAndSpO2();
      display.println("kPa");
      display.print("Pres:");
      display.print(Pr.getPressureHg(true), 1);
      serviceHeartRateAndSpO2();
      display.println("mmHg");
      display.print("Pres:");
      display.print(Pr.getPressureBar(true), 1);
      serviceHeartRateAndSpO2();
      display.println("mbar");
    }
  }
  else
  {
    Serial.println("Barometric Pressure: Not connected");
    display.setFont(&VeraMono7pt7b);
    display.println("Not connected");
  }
  Serial.println();
  serviceHeartRateAndSpO2();
  display.display();
  serviceHeartRateAndSpO2();
  nCnt = (nCnt + 1) & 1;
}

/*
 * Print the light/proximity page to Serial and the initialized OLED.
 */
void MYOSA::printLightProximityAndGesture(void)
{
  if(!_displayReady)
    return;
  static uint8_t nCnt = 0;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setFont(&VeraMonoBold7pt7b);
  display.setCursor(0, 9);
  drawCentreString("Light Prox RGB");
  if(Lpg.ping())
  {
    display.setFont(&VeraMono7pt7b);
    if(nCnt == 0u)
    {
      display.print("Ambient:");
      const float ambient = Lpg.getAmbientLightLux(true);
      if(isfinite(ambient))
      {
        display.print(ambient, 1);
        display.println(" lux");
      }
      else
        display.println("--");
      display.print("Proximity:");
      display.print(Lpg.getProximity(true), 1);
      display.println();
    }
    else
    {
      const uint16_t *rgb = Lpg.getRGBProportion(true);
      display.print("Red  :");
      display.print(rgb[0]);
      display.println("%");
      display.print("Green:");
      display.print(rgb[1]);
      display.println("%");
      display.print("Blue :");
      display.print(rgb[2]);
      display.println("%");
    }
  }
  else
  {
    Serial.println("Light, Proximity and Gesture: Not connected");
    display.setFont(&VeraMono7pt7b);
    display.println("Not connected");
  }
  Serial.println();
  serviceHeartRateAndSpO2();
  display.display();
  serviceHeartRateAndSpO2();
  nCnt = (nCnt + 1) & 1;
}

/*
 * Match the standalone optical demo and keep its retry state independent of other boards.
 */
bool MYOSA::initializeHeartRateAndSpO2(void)
{
  _heartHasRaw = false;
  _heartReady = Hr.begin(HEART_RATE_AND_SPO2_MODE_SPO2_HR) &&
                Hr.setSampleRate(HEART_RATE_AND_SPO2_SAMPLING_RATE_100HZ) &&
                Hr.setPulseWidth(HEART_RATE_AND_SPO2_PULSE_WIDTH_1600US_16BITS) &&
                Hr.setHighResolutionMode(true) &&
                Hr.setLedCurrentByCode(HEART_RATE_AND_SPO2_LED_CURRENT_27_1_MA,
                                       HEART_RATE_AND_SPO2_LED_CURRENT_27_1_MA);
  _heartAttemptTime = _heartSampleTime = _heartPollTime = millis();
  if(_heartReady)
    Serial.println("Heart Rate and SpO2 Sensor is Connected");
  else
    Serial.println("Heart Rate and SpO2 Sensor is Disconnected; retrying in 2 seconds.");
  return _heartReady;
}

/*
 * Continuous ranging lets the master fetch ready samples without waiting for each exposure.
 */
bool MYOSA::initializeProximityAndDistance(void)
{
  _distanceHasSample = false;
  _distanceMillimeters = -1;
  _distanceReady = Pd.setTimeout(500) && Pd.begin() && Pd.setTimingBudget(50) &&
                   Pd.setTimeout(100) && Pd.startContinuous(100);
  _distanceAttemptTime = _distanceSampleTime = _distancePollTime = millis();
  if(_distanceReady)
    Serial.println("Proximity and Distance Sensor is Connected");
  else
    Serial.println("Proximity and Distance Sensor is Disconnected; retrying in 2 seconds.");
  return _distanceReady;
}

/*
 * Drain optical samples between slower board operations; no background task shares Wire.
 */
void MYOSA::serviceHeartRateAndSpO2(void)
{
  if(!_heartReady || uint32_t(millis() - _heartPollTime) < 5)
    return;
  _heartPollTime = millis();
  if(Hr.update())
  {
    if(Hr.getRawValues(&_infrared, &_red, false))
    {
      Hr.clearRawBuffer();
      _heartHasRaw = true;
      _heartSampleTime = millis();
      return;
    }
  }
  else if(Hr.getLastStatus() == HEART_RATE_AND_SPO2_NO_DATA)
  {
    if(Hr.isFifoOverflowed())
    {
      _heartHasRaw = false;
      Serial.println("MAX30100 sample buffer overflow; collecting a new pulse.");
    }
    if(uint32_t(millis() - _heartSampleTime) < 2000)
      return;
    Serial.println("MAX30100: No optical samples received for 2 seconds.");
  }
  else
  {
    Serial.print("MAX30100 read failed; sensor status: ");
    Serial.println(Hr.getLastStatus());
  }
  _heartReady = _heartHasRaw = false;
  _heartAttemptTime = millis();
}

/*
 * Service both sensors frequently while the OLED remains on its current page.
 */
void MYOSA::update(void)
{
  if(!_heartReady && uint32_t(millis() - _heartAttemptTime) >= 2000)
    initializeHeartRateAndSpO2();
  serviceHeartRateAndSpO2();

  if(!_distanceReady)
  {
    if(uint32_t(millis() - _distanceAttemptTime) >= 2000)
    {
      initializeProximityAndDistance();
      serviceHeartRateAndSpO2();
    }
    return;
  }
  if(uint32_t(millis() - _distancePollTime) < 20)
    return;
  _distancePollTime = millis();
  const uint8_t interrupt = Pd.getInterruptStatus();
  bool failed = Pd.getLastStatus() != PROXIMITY_AND_DISTANCE_OK;
  if(!failed && interrupt == PROXIMITY_AND_DISTANCE_INTERRUPT_NEW_SAMPLE_READY)
  {
    _distanceMillimeters = Pd.readRangeContinuousMillimeters();
    failed = Pd.getLastStatus() != PROXIMITY_AND_DISTANCE_OK &&
             Pd.getLastStatus() != PROXIMITY_AND_DISTANCE_NO_DATA;
    _distanceHasSample = !failed;
    _distanceSampleTime = millis();
  }
  if(failed || uint32_t(millis() - _distanceSampleTime) >= 2000)
  {
    Serial.println("VL53L0X: Communication failed or measurements stopped; retrying in 2 seconds.");
    _distanceReady = _distanceHasSample = false;
    _distanceMillimeters = -1;
    _distanceAttemptTime = millis();
  }
  serviceHeartRateAndSpO2();
}

/*
 * Show cached distance with the standalone demo's 30..2000 mm acceptance limits.
 */
void MYOSA::printProximityAndDistance(void)
{
  serviceHeartRateAndSpO2();
  const bool fresh =
      _distanceReady && _distanceHasSample && uint32_t(millis() - _distanceSampleTime) <= 1000;
  const char *message = nullptr;
  if(!_distanceReady)
    message = "Not connected";
  else if(!fresh)
    message = "Awaiting range";
  else if(_distanceMillimeters < 0)
    message = Pd.getRangeStatus() == VL53L0X_HARDWARE_FAIL ? "Sensor fault" : "Range invalid";
  else if(_distanceMillimeters < 30)
    message = "Too close";
  else if(_distanceMillimeters > 2000)
    message = "Too far";

  Serial.print("Distance (mm): ");
  if(message)
    Serial.println(message);
  else
    Serial.println(_distanceMillimeters);
  Serial.println();
  if(!_displayReady)
    return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setFont(&VeraMonoBold7pt7b);
  display.setCursor(0, 9);
  drawCentreString("Distance");
  display.setFont(&VeraMono7pt7b);
  if(message)
    display.println(message);
  else
  {
    display.print("Range: ");
    display.print(_distanceMillimeters);
    display.println(" mm");
  }
  serviceHeartRateAndSpO2();
  display.display();
  serviceHeartRateAndSpO2();
}

/*
 * Show pulse estimates only while the driver has a recent usable finger signal.
 */
void MYOSA::printHeartRateAndSpO2(void)
{
  serviceHeartRateAndSpO2();
  const char *message = nullptr;
  if(!_heartReady)
    message = "Not connected";
  else if(!_heartHasRaw)
    message = "Awaiting samples";
  else if(_infrared >= 65520 || _red >= 65520)
    message = "Signal saturated";
  else if(!Hr.isFingerDetected())
    message = "Place finger";

  if(message)
  {
    Serial.print("Heart Rate and SpO2: ");
    Serial.println(message);
  }
  else
  {
    Hr.getHeartRate();
    Hr.getSpO2();
  }
  Serial.println();
  if(!_displayReady)
    return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setFont(&VeraMonoBold7pt7b);
  display.setCursor(0, 9);
  drawCentreString("Heart Rate/SpO2");
  display.setFont(&VeraMono7pt7b);
  if(message)
    display.println(message);
  else
  {
    const float heartRate = Hr.getHeartRate(false);
    const float spO2 = Hr.getSpO2(false);
    display.print("HR: ");
    if(isfinite(heartRate))
    {
      display.print(heartRate, 2);
      display.println(" BPM");
    }
    else
      display.println("--");
    display.print("SpO2: ");
    if(isfinite(spO2))
    {
      display.print(spO2, 2);
      display.println("%");
    }
    else
      display.println("--");
  }
  serviceHeartRateAndSpO2();
  display.display();
  serviceHeartRateAndSpO2();
}
