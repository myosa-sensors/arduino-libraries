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

/* Library Inclusion */
#include "myosa.h"

#define NUM_SERVICES 6
#define MAX_CHARACTERISTICS 5
#define MAX_EVENTS 2

static BLECharacteristic *pCharacteristics[NUM_SERVICES][MAX_CHARACTERISTICS] = {};
static myosa_detail::Event eventArr[MAX_EVENTS] = {};
static SemaphoreHandle_t eventMutex = nullptr;

class MyServerCallbacks : public BLEServerCallbacks
{
    void onDisconnect(BLEServer *server) override
    {
      server->getAdvertising()->start();
    }
};

class MyCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *characteristic) override
    {
      const auto value = characteristic->getValue();
      if(!eventMutex || value.length() < 3 || value.length() > 160)
        return;
      // Parse before taking the lock; invalid commands leave existing events intact.
      const char *command = value.c_str();
      if(strlen(command) != value.length() || command[1] != ',')
        return;
      myosa_detail::Event next = {};
      int slot = -1;
      if(command[0] == 'c' || command[0] == 'u')
      {
        if(!myosa_detail::parseEvent(command + 2, next))
          return;
        slot = next.action == 'b' ? 0 : 1;
      }
      else if(command[0] == 'd' && value.length() == 3)
      {
        if(command[2] != 'b' && command[2] != 'r')
          return;
        slot = command[2] == 'b' ? 0 : 1;
      }
      if(slot < 0)
        return;
      xSemaphoreTake(eventMutex, portMAX_DELAY);
      eventArr[slot] = next;
      xSemaphoreGive(eventMutex);
    }
};

MYOSA::MYOSA()
    : Ag(MPU6050_ADDRESS_AD0_HIGH),     /* Accelerometer and Gyroscope sensor init */
      Aq(CCS811_I2C_ADDRESS1, 10000.f), /* Air Quality sensor init */
      Pr(ULTRA_HIGH_RESOLUTION),        /* Barometric Pressure sensor init */
      Lpg(),                            /* Light, Proximity and Gesture sensor init */
      gpioExpander(),                   /* GPIO expander PCA9536 init */
      Th(),                             /* temperature and Humidity sensor init */
      display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) /* OLED display */
{
}

/*
 * Create BLE services and initialize the supplied boards; false means at least one setup step failed.
 */
bool MYOSA::begin(void)
{
  if(!eventMutex)
    eventMutex = xSemaphoreCreateMutex();
  if(!eventMutex)
    return false;
  BLEDevice::init("MYOSA_1");
  BLEServer *pServer = BLEDevice::createServer();
  if(!pServer)
    return false;
  pServer->setCallbacks(new MyServerCallbacks());

  for(int i = 0; i < NUM_SERVICES; ++i)
  {
    char serviceUUID[37];
    sprintf(serviceUUID, "4fafc201-1fb5-459e-8fcc-c5c9c33191b%d", i);
    Serial.println(serviceUUID);
    // The last service accepts event commands; the others publish sensor values.
    const uint8_t count = i == 5 ? 1 : myosa_detail::characteristicCounts[i];
    const uint32_t numHandles = i == 5 ? 3 : 1 + 3 * count;
    BLEService *pService = pServer->createService(BLEUUID(serviceUUID), numHandles);
    if(!pService)
      return false;
    for(int j = 0; j < count; ++j)
    {
      char characteristicUUID[37];
      sprintf(characteristicUUID, "beb5483e-36e1-4688-b7f5-ea07361b2b%d%d", i, j);
      Serial.println(characteristicUUID);
      if(i == 5)
      {
        pCharacteristics[i][j] = pService->createCharacteristic(
            characteristicUUID,
            BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
        if(!pCharacteristics[i][j])
          return false;
        pCharacteristics[i][j]->setCallbacks(new MyCallbacks());
        pCharacteristics[i][j]->setValue("Hello from MYOSA");
      }
      else
      {
        pCharacteristics[i][j] =
            pService->createCharacteristic(characteristicUUID, BLECharacteristic::PROPERTY_NOTIFY);
        if(!pCharacteristics[i][j])
          return false;
        pCharacteristics[i][j]->addDescriptor(new BLE2902());
      }
    }
    if(!pService->start())
      return false;
  }

  pServer->getAdvertising()->start();
  Serial.println("BLE server started!");

  bool ready = true;
  _displayReady = display.begin();
  ready = _displayReady && ready;
  ready = Ag.begin() && ready;
  ready = (Aq.begin() == SENSOR_SUCCESS) && ready;
  ready = Pr.begin() && ready;
  const bool lightReady = Lpg.begin();
  ready = lightReady && ready;
  if(lightReady)
  {
    ready = Lpg.enableAmbientLightSensor(DISABLE) && ready;
    ready = Lpg.enableProximitySensor(DISABLE) && ready;
    ready = Lpg.setProximityGain(PGAIN_2X) && ready;
  }
  const bool actuatorReady = gpioExpander.ping();
  ready = actuatorReady && ready;
  if(actuatorReady)
  {
    // Set output latches before changing direction to avoid startup pulses.
    gpioExpander.setState(RELAY_IO, IO_LOW);
    gpioExpander.setMode(RELAY_IO, IO_OUTPUT);
    gpioExpander.setState(BUZZER_IO, IO_LOW);
    gpioExpander.setMode(BUZZER_IO, IO_OUTPUT);
  }
  ready = Th.begin() && ready;
  if(!ready)
    Serial.println("One or more MYOSA boards could not be initialized.");
  return ready;
}

/*
 * Drive the configured relay output high without changing its pin direction.
 */
void MYOSA::turnOnRelay(void)
{
  gpioExpander.setState(RELAY_IO, IO_HIGH);
  gpioExpander.setMode(RELAY_IO, IO_OUTPUT);
}

/*
 * Drive the configured relay output low without changing its pin direction.
 */
void MYOSA::turnOffRelay(void)
{
  gpioExpander.setState(RELAY_IO, IO_LOW);
  gpioExpander.setMode(RELAY_IO, IO_OUTPUT);
}

/*
 * Drive the buzzer output high; print controls the optional Serial message.
 */
void MYOSA::turnOnBuzzer(int print)
{
  gpioExpander.setState(BUZZER_IO, IO_HIGH);
  gpioExpander.setMode(BUZZER_IO, IO_OUTPUT);

  if(print && _displayReady)
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setFont(&VeraMonoBold7pt7b);
    display.setCursor(0, 9);
    drawCentreString("Actuator");
    if(gpioExpander.ping())
    {
      display.setFont(&VeraMono7pt7b);
      display.print("Buzzer: Turn On");
      display.println();

      display.display();

    }
  }
}

/*
 * Drive the buzzer output low; print controls the optional Serial message.
 */
void MYOSA::turnOffBuzzer(int print)
{
  gpioExpander.setState(BUZZER_IO, IO_LOW);
  gpioExpander.setMode(BUZZER_IO, IO_OUTPUT);
  if(print && _displayReady)
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setFont(&VeraMonoBold7pt7b);
    display.setCursor(0, 9);
    drawCentreString("Actuator");
    display.setFont(&VeraMono7pt7b);
    display.println("Buzzer: Turn Off");

    display.display();

  }
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
 * Collect readings once, notify only valid values, and evaluate events using that same snapshot.
 */
void MYOSA::sendBleData(void)
{

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

  if(Aq.ping())
  {
    if(Aq.isDataAvailable())
      Aq.readAlgorithmResults();
    if(Aq.hasReading())
    {
      readings[1][0][0] = Aq.getCO2(false);
      readings[1][1][0] = Aq.getTVOC(false);
    }
  }

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

  if(Th.ping())
  {
    readings[4][0][0] = Th.getTempC(false);
    readings[4][0][1] = readings[4][0][0] * 1.8f + 32.f;

    readings[4][1][0] = Th.getRelativeHumdity(false);

    readings[4][2][0] = Th.getHeatIndexC(false);
    readings[4][2][1] = readings[4][2][0] * 1.8f + 32.f;
  }

  for(uint8_t service = 0; service < NUM_SERVICES; ++service)
  {
    for(uint8_t characteristic = 0; characteristic < myosa_detail::characteristicCounts[service];
        ++characteristic)
    {
      const uint8_t count = myosa_detail::parameterCounts[service][characteristic];
      const float *v = readings[service][characteristic];
      bool valid = true;
      for(uint8_t i = 0; i < count; ++i)
        valid = isfinite(v[i]) && valid;
      if(!valid || !pCharacteristics[service][characteristic])
        continue;
      char payload[96];
      int size;
      const bool integers = service == 1 || (service == 3 && characteristic != 1);
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

    }
  }
  if(!eventMutex)
    return;
  // The callback shares only event settings. All sensor and actuator I/O stays in this task.
  xSemaphoreTake(eventMutex, portMAX_DELAY);
  for(uint8_t i = 0; i < MAX_EVENTS; ++i)
  {
    const myosa_detail::Event &event = eventArr[i];
    const bool active =
        event.enabled && myosa_detail::matches(
                             event, readings[event.service][event.characteristic][event.parameter]);
    // Once an event is removed, release its output once without overriding manual control forever.
    if(event.enabled || _eventControlled[i])
    {
      if(i == 0)
      {
        if(active)
          turnOnBuzzer(0);
        else
          turnOffBuzzer(0);
      }
      else
      {
        if(active)
          turnOnRelay();
        else
          turnOffRelay();
      }
    }
    _eventControlled[i] = event.enabled;
  }
  xSemaphoreGive(eventMutex);
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

  display.display();

  nCnt = (nCnt + 1) & 3;
}

/*
 * Print the air-quality page to Serial and the initialized OLED.
 */
void MYOSA::printAirQuality(void)
{
  if(!_displayReady)
    return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setFont(&VeraMonoBold7pt7b);
  display.setCursor(0, 9);
  drawCentreString("Air Quality");
  if(Aq.ping())
  {
    /* Check if data is ready or not */
    if(Aq.isDataAvailable())
    {
      if(Aq.readAlgorithmResults() == SENSOR_SUCCESS)
      {
        display.setFont(&VeraMono7pt7b);
        display.print("eCO2 :");
        display.print(Aq.getCO2(true));
        display.println("ppm");
        display.print("TVOC :");
        display.print(Aq.getTVOC(true));
        display.println("ppb");
      }
    }
  }
  else
  {
    Serial.println("Air Quality: Not connected");
    display.setFont(&VeraMono7pt7b);
    display.println("Not connected");
  }
  Serial.println();

  display.display();

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

      drawDegreeSymbol();
      display.println("C");
      display.print("Temp:");
      display.print(Pr.getTempF(true), 1);

      drawDegreeSymbol();
      display.println("F");
      display.print("Alti:");
      display.print(Pr.getAltitude(SEA_LEVEL_AVG_PRESSURE), 1);

      display.println("m");
    }
    else
    {
      display.print("Pres:");
      display.print(Pr.getPressurePascal(true), 1);

      display.println("kPa");
      display.print("Pres:");
      display.print(Pr.getPressureHg(true), 1);

      display.println("mmHg");
      display.print("Pres:");
      display.print(Pr.getPressureBar(true), 1);

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

  display.display();

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

  display.display();

  nCnt = (nCnt + 1) & 1;
}

/*
 * Print the temperature/humidity page to Serial and the initialized OLED.
 */
void MYOSA::printTempAndHumidity(void)
{
  if(!_displayReady)
    return;
  static uint8_t nCnt = 0;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setFont(&VeraMonoBold7pt7b);
  display.setCursor(0, 9);
  drawCentreString("Temp Humidity");
  if(Th.ping())
  {
    display.setFont(&VeraMono7pt7b);
    if(nCnt == 0u)
    {
      display.print("RH  :");
      display.print(Th.getRelativeHumdity(true), 1);

      display.println("%");
      display.print("Temp:");
      display.print(Th.getTempC(true), 1);

      drawDegreeSymbol();
      display.println("C");
      display.print("Temp:");
      display.print(Th.getTempF(true), 1);

      drawDegreeSymbol();
      display.println("F");
    }
    else
    {
      display.print("HI :");
      display.print(Th.getHeatIndexC(true), 1);

      drawDegreeSymbol();
      display.println("C");
      display.print("HI :");
      display.print(Th.getHeatIndexF(true), 1);

      drawDegreeSymbol();
      display.println("F");
    }
  }
  else
  {
    Serial.println("Temperature and Humidity: Not connected");
    display.setFont(&VeraMono7pt7b);
    display.println("Not connected");
  }
  Serial.println();

  display.display();

  nCnt = (nCnt + 1) & 1;
}

