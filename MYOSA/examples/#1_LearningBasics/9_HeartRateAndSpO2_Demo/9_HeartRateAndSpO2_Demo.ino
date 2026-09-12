/*
  This code is developed under the MYOSA (LearnTheEasyWay) initiative of MakeSense EduTech and Pegasus Automation.
  Code has been derived from internet sources and component datasheets.
  Existing readily-available libraries would have been used "AS IS" and modified for ease of learning purpose.

  HeartRateAndSpO2 Demo
  Connection: Connect the "Heart Rate and SpO2" board to the MYOSA controller using its I2C connector.
  Working: Open Serial Monitor at 115200 baud and rest a fingertip gently over both sensor windows.

  Synopsis of Heart Rate and SpO2 Board
  The MYOSA heart-rate board uses the MAX30100 sensor at I2C address 0x57.
  Samples are processed continuously at 100 Hz; results are printed once per second.
  Raw infrared/red counts help check finger placement and signal saturation.
  Keep the finger still for several seconds while pulse measurements settle.
  Heart rate is shown in BPM; SpO2 is an approximate percentage requiring module validation.
  Missing readings are shown as unavailable, and connection errors trigger a retry.

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
#include <HeartRateAndSpO2.h>

/* Creating Object of HeartRateAndSpO2 Class */
HeartRateAndSpO2 Hr;

const uint32_t REPORT_INTERVAL_MS = 1000;
const uint32_t RECONNECT_INTERVAL_MS = 2000;
const uint32_t SAMPLE_TIMEOUT_MS = 2000;

/* Exact LED-current selectors; reduce these if the raw readings saturate. */
const uint8_t IR_LED_CURRENT = HEART_RATE_AND_SPO2_LED_CURRENT_27_1_MA;
const uint8_t RED_LED_CURRENT = HEART_RATE_AND_SPO2_LED_CURRENT_27_1_MA;

bool sensorReady = false;
bool haveRawSample = false;
bool overflowSinceReport = false;
uint16_t latestInfrared = 0;
uint16_t latestRed = 0;
uint32_t lastReportTime = 0;
uint32_t lastSampleTime = 0;
uint32_t lastConnectionAttempt = 0;

/* Read the cached error before another operation can replace it. */
const char *sensorStatusText(HEART_RATE_AND_SPO2_STATUS_t status)
{
  switch(status)
  {
    case HEART_RATE_AND_SPO2_OK:
      return "OK";
    case HEART_RATE_AND_SPO2_NOT_FOUND:
      return "MAX30100 not found or incorrect part ID";
    case HEART_RATE_AND_SPO2_I2C_ERROR:
      return "I2C transfer failed";
    case HEART_RATE_AND_SPO2_TIMEOUT:
      return "Sensor operation timed out";
    case HEART_RATE_AND_SPO2_INVALID_ARGUMENT:
      return "Invalid configuration";
    case HEART_RATE_AND_SPO2_NO_DATA:
      return "No new samples";
    case HEART_RATE_AND_SPO2_BUSY:
      return "Sensor busy";
    case HEART_RATE_AND_SPO2_NOT_INITIALIZED:
      return "Sensor not initialized";
    default:
      return "Unknown status";
  }
}

void scheduleReconnect(const char *reason)
{
  Serial.print("MAX30100: ");
  Serial.println(reason);
  Serial.println("Check the sensor connection. Retrying initialization in 2 seconds.");
  sensorReady = false;
  haveRawSample = false;
  overflowSinceReport = false;
  lastConnectionAttempt = millis();
}

/* Reapply the complete configuration after a reset or communication failure. */
bool initializeSensor(void)
{
  if(!Hr.begin(HEART_RATE_AND_SPO2_MODE_SPO2_HR) ||
     !Hr.setSampleRate(HEART_RATE_AND_SPO2_SAMPLING_RATE_100HZ) ||
     !Hr.setPulseWidth(HEART_RATE_AND_SPO2_PULSE_WIDTH_1600US_16BITS) ||
     !Hr.setHighResolutionMode(true) || !Hr.setLedCurrentByCode(IR_LED_CURRENT, RED_LED_CURRENT))
  {
    scheduleReconnect(sensorStatusText(Hr.getLastStatus()));
    return false;
  }

  haveRawSample = false;
  overflowSinceReport = false;
  lastSampleTime = lastReportTime = millis();
  Serial.println("MAX30100 connected at I2C address 0x57");
  Serial.println("Sampling Rate (Hz): 100");
  Serial.println("Pulse Width (us): 1600");
  Serial.println("Reporting Interval (ms): 1000");
  Serial.println("Rest a fingertip over both windows and keep it still for several seconds.");
  return true;
}

/* Reporting uses cached samples; it must not stop continuous FIFO processing. */
void printReading(void)
{
  if(overflowSinceReport)
  {
    Serial.println("Sample buffer overflow: pulse history restarted; keep the finger still.");
    overflowSinceReport = false;
  }
  if(!haveRawSample)
  {
    Serial.println("Waiting for optical samples.");
    return;
  }

  Serial.print("Infrared (counts): ");
  Serial.println(latestInfrared);
  Serial.print("Red (counts): ");
  Serial.println(latestRed);

  /* These clipping limits match the driver's 16-bit signal checks. */
  if(latestInfrared >= 65520 || latestRed >= 65520)
  {
    Serial.println("Signal saturated: adjust placement or reduce LED current.");
    Serial.println();
    return;
  }
  if(!Hr.isFingerDetected())
  {
    Serial.println("No finger detected or signal too weak: adjust fingertip placement.");
    Serial.println();
    return;
  }

  /* Use the library's labeled printing getters, as in the other basic examples. */
  Hr.getHeartRate();
  Hr.getSpO2();
  if(!Hr.isReadingValid())
  {
    Serial.println("Keep the finger still while a usable pulse is collected.");
  }
  Serial.println();
}

/* Setup Function */
void setup(void)
{
  /* Setting up communication */
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(100000);

  Serial.println();
  Serial.println("MYOSA - MAX30100 Heart Rate and SpO2 Demo");
  /* Setting up the Heart Rate and SpO2 Board. */
  sensorReady = initializeSensor();
}

/* Loop Function */
void loop(void)
{
  const uint32_t now = millis();
  if(!sensorReady)
  {
    if(uint32_t(now - lastConnectionAttempt) >= RECONNECT_INTERVAL_MS)
    {
      sensorReady = initializeSensor();
    }
    delay(1);
    return;
  }

  /* Service the 16-sample hardware FIFO on every pass, including between reports. */
  if(Hr.update())
  {
    if(!Hr.getRawValues(&latestInfrared, &latestRed, false))
    {
      scheduleReconnect(sensorStatusText(Hr.getLastStatus()));
      return;
    }
    Hr.clearRawBuffer();
    haveRawSample = true;
    lastSampleTime = millis();
  }
  else if(Hr.getLastStatus() != HEART_RATE_AND_SPO2_NO_DATA)
  {
    scheduleReconnect(sensorStatusText(Hr.getLastStatus()));
    return;
  }

  if(Hr.isFifoOverflowed())
  {
    overflowSinceReport = true;
    haveRawSample = false;
  }

  /* An empty FIFO between samples is normal; a prolonged stall needs recovery. */
  const uint32_t currentTime = millis();
  if(uint32_t(currentTime - lastSampleTime) >= SAMPLE_TIMEOUT_MS)
  {
    scheduleReconnect("No optical samples received for 2 seconds");
    return;
  }
  if(uint32_t(currentTime - lastReportTime) >= REPORT_INTERVAL_MS)
  {
    lastReportTime = currentTime;
    printReading();
  }
  delay(1);
}
