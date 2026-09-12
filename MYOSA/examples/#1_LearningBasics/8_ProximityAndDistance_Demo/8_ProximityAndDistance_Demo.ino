/*
  This code is developed under the MYOSA (LearnTheEasyWay) initiative of MakeSense EduTech and Pegasus Automation.
  Code has been derived from internet sources and component datasheets.
  Existing readily-available libraries would have been used "AS IS" and modified for ease of learning purpose.

  ProximityAndDistance Demo
  Connection: Connect the "Proximity and Distance" board to the MYOSA controller using its I2C connector.
  Working: Open Serial Monitor at 115200 baud to view single-shot distance readings and sensor status.

  Synopsis of Proximity and Distance Board
  The MYOSA distance board uses the VL53L0X sensor at the default I2C address 0x29.
  This example requests one distance measurement every 500 ms while the sensor is ready.
  Valid measurements are printed in millimeters, with signal and ambient rates in MCPS.
  Readings outside the demo limits are labeled too close or too far.
  Invalid ranges are reported without guessing which side of the limits the target is on.
  Initialization is retried every two seconds after a connection or timeout error.

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

/* Library Inclusion */
#include <ProximityAndDistance.h>

/* Creating Object of ProximityAndDistance Class */
ProximityAndDistance Pd;

/* Timing values are in milliseconds. The timeout must exceed the measurement budget. */
const uint16_t MEASUREMENT_BUDGET_MS = 50;
const uint16_t MEASUREMENT_TIMEOUT_MS = 500;
const uint32_t READING_INTERVAL_MS = 500;
const uint32_t RECONNECT_INTERVAL_MS = 2000;

/* Application limits for this demo, not guaranteed sensor limits; adjust for your target.
   Only sensor-valid readings can identify which side of these limits the object is on. */
const uint16_t MIN_DISTANCE_MM = 30;
const uint16_t MAX_DISTANCE_MM = 2000;

bool sensorReady = false;
uint32_t lastReadingTime = 0;
uint32_t lastConnectionAttempt = 0;

/* Describe driver failures without performing another I2C transaction. */
const char *sensorStatusText(PROXIMITY_AND_DISTANCE_STATUS_t status)
{
  switch(status)
  {
    case PROXIMITY_AND_DISTANCE_OK:
      return "OK";
    case PROXIMITY_AND_DISTANCE_NOT_FOUND:
      return "Sensor not found";
    case PROXIMITY_AND_DISTANCE_I2C_ERROR:
      return "I2C transfer failed";
    case PROXIMITY_AND_DISTANCE_TIMED_OUT:
      return "Measurement/calibration timed out";
    case PROXIMITY_AND_DISTANCE_INVALID_ARGUMENT:
      return "Invalid configuration";
    case PROXIMITY_AND_DISTANCE_NOT_INITIALIZED:
      return "Sensor not initialized";
    case PROXIMITY_AND_DISTANCE_NO_DATA:
      return "No valid range";
    case PROXIMITY_AND_DISTANCE_BUSY:
      return "Sensor busy";
    default:
      return "Unknown status";
  }
}

/* Invalid measurements cannot reliably identify whether the target is too close or too far. */
const char *rangeStatusText(VL53L0X_RANGE_STATUS_t status)
{
  switch(status)
  {
    case VL53L0X_RANGE_VALID:
      return "Valid";
    case VL53L0X_SIGMA_FAIL:
      return "Range unavailable: noisy measurement";
    case VL53L0X_SIGNAL_FAIL:
      return "Range unavailable: target too far or reflection too weak";
    case VL53L0X_MIN_RANGE_FAIL:
      return "Range unavailable: minimum-range check failed";
    case VL53L0X_PHASE_FAIL:
      return "Out of range or unreliable signal: too close/too far cannot be determined";
    case VL53L0X_HARDWARE_FAIL:
      return "Sensor fault: hardware check failed";
    case VL53L0X_NO_UPDATE:
      return "Range unavailable: no measurement update";
    default:
      return "Range unavailable: unknown sensor status";
  }
}

void printSensorError(const char *operation)
{
  Serial.print(operation);
  Serial.print(": ");
  Serial.println(sensorStatusText(Pd.getLastStatus()));
  Serial.print("I2C Status: 0x");
  Serial.print(Pd.getLastI2cStatus(), HEX);
  if(Pd.getLastStatus() == PROXIMITY_AND_DISTANCE_I2C_ERROR && Pd.getLastI2cStatus() == 0xFD)
  {
    Serial.print(" (incomplete I2C read)");
  }
  Serial.println();
}

/* Reapply the complete configuration after a failed transfer or timeout. */
bool initializeSensor(void)
{
  if(!Pd.setTimeout(MEASUREMENT_TIMEOUT_MS) || !Pd.begin() ||
     !Pd.setTimingBudget(MEASUREMENT_BUDGET_MS))
  {
    printSensorError("VL53L0X initialization failed");
    Serial.println("Check the sensor connection. Retrying in 2 seconds.");
    lastConnectionAttempt = millis();
    return false;
  }

  Serial.print("VL53L0X connected at I2C address 0x");
  Serial.println(Pd.getAddress(), HEX);
  Serial.println("Ranging Mode: Single-shot");
  Serial.print("Measurement Budget (ms): ");
  Serial.println(MEASUREMENT_BUDGET_MS);
  Serial.print("Reading Interval (ms): ");
  Serial.println(READING_INTERVAL_MS);
  Serial.print("Demo distance limits: ");
  Serial.print(MIN_DISTANCE_MM);
  Serial.print(" to ");
  Serial.print(MAX_DISTANCE_MM);
  Serial.println(" mm (target and lighting affect usable range)");
  lastReadingTime = millis();
  return true;
}

/* Setup Function */
void setup(void)
{
  /* Setting up communication */
  Serial.begin(115200);

  /* Use the controller's default I2C pins, as in the other MYOSA basic examples. */
  Wire.begin();
  Wire.setClock(100000);

  Serial.println();
  Serial.println("MYOSA - VL53L0X Proximity and Distance Demo");
  /* Setting up the Proximity and Distance Board. */
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
    delay(10);
    return;
  }

  /* Unsigned elapsed time keeps the schedule valid when millis() wraps. */
  if(uint32_t(now - lastReadingTime) < READING_INTERVAL_MS)
  {
    delay(1);
    return;
  }
  lastReadingTime = now;

  uint8_t rangeStatus = VL53L0X_NO_UPDATE;
  uint16_t signalRateQ97 = 0;
  const int distance = Pd.readRangeSingleMillimeters(&rangeStatus, &signalRateQ97);
  if(distance >= 0 && Pd.isRangeValid())
  {
    if(distance < MIN_DISTANCE_MM)
    {
      Serial.print("Out of range: too close (below ");
      Serial.print(MIN_DISTANCE_MM);
      Serial.println(" mm demo limit)");
      return;
    }
    if(distance > MAX_DISTANCE_MM)
    {
      Serial.print("Out of range: too far (above ");
      Serial.print(MAX_DISTANCE_MM);
      Serial.println(" mm demo limit)");
      return;
    }

    Serial.print("Distance (mm): ");
    Serial.println(distance);
    Serial.print("Signal Rate (MCPS): ");
    Serial.println(signalRateQ97 / 128.0f, 2);
    Serial.print("Ambient Rate (MCPS): ");
    Serial.println(Pd.getAmbientRate() / 128.0f, 2);
    Serial.println();
    return;
  }

  if(Pd.getLastStatus() == PROXIMITY_AND_DISTANCE_NO_DATA)
  {
    Serial.print(rangeStatusText(static_cast<VL53L0X_RANGE_STATUS_t>(rangeStatus)));
    Serial.print(" (status ");
    Serial.print(rangeStatus);
    Serial.println(")");
    return;
  }

  /* Communication and timeout errors invalidate the driver; begin() is required again. */
  printSensorError("VL53L0X read failed");
  sensorReady = false;
  lastConnectionAttempt = millis();
  Serial.println("Retrying initialization in 2 seconds.");
}
