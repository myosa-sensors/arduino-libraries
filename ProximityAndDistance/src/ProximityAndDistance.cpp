/*
  This code is developed under the MYOSA (LearnTheEasyWay) initiative of MakeSense EduTech and Pegasus Automation.
  Code has been derived from internet sources and component datasheets.
  Existing readily-available libraries would have been used "AS IS" and modified for ease of learning purpose.

  Synopsis of Proximity and Distance sensor
  STMicroelectronics VL53L0X time-of-flight ranging; default I2C address 0x29.
  Provides single and continuous distance measurements in millimeters.
  Uses a caller-initialized Wire bus, bounded waits, and checked transfers.
  The bundled core is adapted from Pololu and ST; see LICENSE.txt.

  NOTE
  All information, including URL references, is subject to change without prior notice.
  Please always use the latest versions of software-release for best performance.
  Unless required by applicable law or agreed to in writing, this software is distributed on an
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied

  Modifications
  10 September, 2026 by Pegasus Automation
  (as a part of MYOSA Initiative)

  Contact Team MYOSA for feedback, issues, or update requests.
  Email: myosa.event@gmail.com
*/

/* Library Inclusion */
#include "ProximityAndDistance.h"

ProximityAndDistance::ProximityAndDistance(TwoWire *wire, uint8_t address)
{
  _core.bus = wire;
  _core.address = address;
}

/*
 * Clear the last measurement together with its status, rates, and event-count cache.
 */
void ProximityAndDistance::invalidate(void)
{
  _valid = false;
  _lastDistance = PROXIMITY_AND_DISTANCE_ERROR;
  _rangeStatus = VL53L0X_NO_UPDATE;
  _rawRange = _ambientRate = _signalRate = 0;
  _events = 0;
}

bool ProximityAndDistance::fail(PROXIMITY_AND_DISTANCE_STATUS_t status)
{
  _lastStatus = status;
  if(status == PROXIMITY_AND_DISTANCE_I2C_ERROR || status == PROXIMITY_AND_DISTANCE_TIMED_OUT)
  {
    invalidate();
    _initialized = false;
    _continuous = false;
    if(status == PROXIMITY_AND_DISTANCE_TIMED_OUT)
      _didTimeout = true;
  }
  return false;
}

/*
 * Check lifecycle and idle requirements before clearing the operation's sticky transport error.
 */
bool ProximityAndDistance::prepare(bool idle)
{
  if(!_initialized)
    return fail(PROXIMITY_AND_DISTANCE_NOT_INITIALIZED);
  if(idle && _continuous)
    return fail(PROXIMITY_AND_DISTANCE_BUSY);
  _core.io_error = false;
  _core.last_status = 0;
  _core.did_timeout = false;
  return true;
}

/*
 * Preserve I2C-error precedence over timeout and configuration errors from the private core.
 */
bool ProximityAndDistance::finish(bool success)
{
  if(_core.io_error)
    return fail(PROXIMITY_AND_DISTANCE_I2C_ERROR);
  if(_core.did_timeout)
    return fail(PROXIMITY_AND_DISTANCE_TIMED_OUT);
  if(!success)
    return fail(PROXIMITY_AND_DISTANCE_INVALID_ARGUMENT);
  _lastStatus = PROXIMITY_AND_DISTANCE_OK;
  return true;
}

/*
 * Recover the register page, verify model ID, then run initialization and reference calibration.
 */
bool ProximityAndDistance::begin(bool io2v8)
{
  _initialized = _continuous = false;
  _didTimeout = false;
  invalidate();
  if(!_core.bus || _core.address < 8 || _core.address > 0x77)
    return fail(PROXIMITY_AND_DISTANCE_INVALID_ARGUMENT);
  _core.io_error = false;
  _core.last_status = 0;
  _core.did_timeout = false;
  // Recover the public register page after an interrupted banked transaction.
  _core.writeReg(0xFF, 0);
  uint8_t id = _core.readReg(VL53L0X_REG_IDENTIFICATION_MODEL_ID);
  if(!finish())
    return false;
  if(id != VL53L0X_EXPECTED_MODEL_ID)
    return fail(PROXIMITY_AND_DISTANCE_NOT_FOUND);
  bool ok = _core.init(io2v8);
  if(!finish(ok))
    return false;
  _initialized = true;
  _thresholdLow = _thresholdHigh = 0;
  _period = 0;
  _interruptConfig = PROXIMITY_AND_DISTANCE_INTERRUPT_NEW_SAMPLE_READY;
  return setInterruptThresholdLow(0) && setInterruptThresholdHigh(0);
}

/*
 * Probe the model ID; a successful probe alone does not initialize ranging configuration.
 */
bool ProximityAndDistance::ping(void)
{
  if(!_core.bus || _core.address < 8 || _core.address > 0x77)
    return fail(PROXIMITY_AND_DISTANCE_INVALID_ARGUMENT);
  _core.io_error = false;
  _core.last_status = 0;
  _core.did_timeout = false;
  _core.writeReg(0xFF, 0);
  uint8_t id = _core.readReg(VL53L0X_REG_IDENTIFICATION_MODEL_ID);
  if(!finish())
    return false;
  if(id != VL53L0X_EXPECTED_MODEL_ID)
  {
    invalidate();
    _initialized = false;
    return fail(PROXIMITY_AND_DISTANCE_NOT_FOUND);
  }
  return true;
}

/*
 * Change the bus only while idle; a bus change requires begin() before further operations.
 */
void ProximityAndDistance::setBus(TwoWire *bus)
{
  if(_continuous)
  {
    fail(PROXIMITY_AND_DISTANCE_BUSY);
    return;
  }
  _core.bus = bus;
  _initialized = false;
  invalidate();
  _lastStatus =
      bus ? PROXIMITY_AND_DISTANCE_NOT_INITIALIZED : PROXIMITY_AND_DISTANCE_INVALID_ARGUMENT;
}

TwoWire *ProximityAndDistance::getBus(void) const
{
  return _core.bus;
}

bool ProximityAndDistance::isConnected(void) const
{
  return _initialized;
}

bool ProximityAndDistance::isRangeValid(void) const
{
  return _valid;
}

bool ProximityAndDistance::writeByte(uint8_t reg, uint8_t value)
{
  if(!prepare())
    return false;
  _core.writeReg(reg, value);
  return finish();
}

bool ProximityAndDistance::writeWord(uint8_t reg, uint16_t value)
{
  if(!prepare())
    return false;
  _core.writeReg16Bit(reg, value);
  return finish();
}

bool ProximityAndDistance::setInterruptPolarityHigh(bool high)
{
  if(!prepare())
    return false;
  uint8_t old = _core.readReg(0x84);
  _core.writeReg(0x84, high ? (old | 0x10) : (old & ~0x10));
  return finish();
}

/*
 * Encode millimeters into 2 mm register steps while respecting the configured upper bound.
 */
bool ProximityAndDistance::setInterruptThresholdLow(uint16_t value)
{
  if(value > 8190 || (_thresholdHigh && value > _thresholdHigh))
    return fail(PROXIMITY_AND_DISTANCE_INVALID_ARGUMENT);
  if(!writeWord(0x0E, value >> 1))
    return false;
  _thresholdLow = value;
  return true;
}

/*
 * Encode millimeters into 2 mm register steps; values below the lower threshold are rejected.
 */
bool ProximityAndDistance::setInterruptThresholdHigh(uint16_t value)
{
  if(value > 8190 || value < _thresholdLow)
    return fail(PROXIMITY_AND_DISTANCE_INVALID_ARGUMENT);
  if(!writeWord(0x0C, value >> 1))
    return false;
  _thresholdHigh = value;
  return true;
}

bool ProximityAndDistance::clearInterrupt(void)
{
  return writeByte(0x0B, 1);
}

uint8_t ProximityAndDistance::getInterruptStatus(void)
{
  if(!prepare())
    return 0;
  uint8_t value = _core.readReg(0x13) & 7;
  return finish() ? value : 0;
}

bool ProximityAndDistance::clearAndReturnInterrupt(uint8_t *status)
{
  uint8_t value = getInterruptStatus();
  if(_lastStatus != PROXIMITY_AND_DISTANCE_OK)
    return false;
  if(!clearInterrupt())
    return false;
  if(status)
    *status = value;
  return true;
}

/*
 * Interrupt modes are selector values, not bit flags; clear any previous latched event.
 */
bool ProximityAndDistance::setInterruptConfig(uint8_t mode)
{
  if(mode > 4)
    return fail(PROXIMITY_AND_DISTANCE_INVALID_ARGUMENT);
  if(!prepare(true))
    return false;
  _core.writeReg(0x0A, mode);
  if(!finish())
    return false;
  _interruptConfig = mode;
  return clearInterrupt();
}

/*
 * Accept milliseconds at the public API and pass microseconds to the private timing routine.
 */
bool ProximityAndDistance::setTimingBudget(uint16_t ms)
{
  if(ms < 20 || ms > 1000)
    return fail(PROXIMITY_AND_DISTANCE_INVALID_ARGUMENT);
  if(!prepare(true))
    return false;
  bool ok = _core.setMeasurementTimingBudget(uint32_t(ms) * 1000);
  return finish(ok);
}

uint16_t ProximityAndDistance::getTimingBudget(void)
{
  if(!prepare())
    return 0;
  uint32_t value = _core.getMeasurementTimingBudget();
  return finish() ? uint16_t((value + 999) / 1000) : 0;
}

bool ProximityAndDistance::setSignalRateLimit(float value)
{
  if(!isfinite(value) || value < 0 || value > 511.99f)
    return fail(PROXIMITY_AND_DISTANCE_INVALID_ARGUMENT);
  if(!prepare(true))
    return false;
  return finish(_core.setSignalRateLimit(value));
}

float ProximityAndDistance::getSignalRateLimit(void)
{
  if(!prepare())
    return NAN;
  float value = _core.getSignalRateLimit();
  return finish() ? value : NAN;
}

/*
 * Validate the period before calibration; partial failure requires a new begin().
 */
bool ProximityAndDistance::setVcselPulsePeriod(PROXIMITY_AND_DISTANCE_VCSEL_PERIOD_t type,
                                               uint8_t period)
{
  bool valid = type == VL53L0X_VCSEL_PERIOD_PRE_RANGE
                   ? (period == 12 || period == 14 || period == 16 || period == 18)
                   : type == VL53L0X_VCSEL_PERIOD_FINAL_RANGE &&
                         (period == 8 || period == 10 || period == 12 || period == 14);
  if(!valid)
    return fail(PROXIMITY_AND_DISTANCE_INVALID_ARGUMENT);
  if(!prepare(true))
    return false;
  bool ok = _core.setVcselPulsePeriod(static_cast<myosa_detail::Vl53l0xCore::vcselPeriodType>(type),
                                      period);
  if(!finish(ok))
  {
    _initialized = false;
    invalidate();
    return false;
  }
  invalidate();
  return true;
}

uint8_t ProximityAndDistance::getVcselPulsePeriod(PROXIMITY_AND_DISTANCE_VCSEL_PERIOD_t type)
{
  if(type != VL53L0X_VCSEL_PERIOD_PRE_RANGE && type != VL53L0X_VCSEL_PERIOD_FINAL_RANGE)
  {
    fail(PROXIMITY_AND_DISTANCE_INVALID_ARGUMENT);
    return 0;
  }
  if(!prepare())
    return 0;
  uint8_t value =
      _core.getVcselPulsePeriod(static_cast<myosa_detail::Vl53l0xCore::vcselPeriodType>(type));
  return finish() ? value : 0;
}

/*
 * Change the 7-bit address only while idle; retain the old software address if the write fails.
 */
bool ProximityAndDistance::setAddress(uint8_t value)
{
  if(value < 8 || value > 0x77)
    return fail(PROXIMITY_AND_DISTANCE_INVALID_ARGUMENT);
  if(!prepare(true))
    return false;
  _core.setAddress(value);
  return finish();
}

uint8_t ProximityAndDistance::getAddress(void) const
{
  return _core.address;
}

/*
 * Apply signal-limit and VCSEL settings; these presets do not guarantee a particular range.
 */
bool ProximityAndDistance::setPreset(float signal, uint8_t pre, uint8_t final)
{
  return setSignalRateLimit(signal) && setVcselPulsePeriod(VL53L0X_VCSEL_PERIOD_PRE_RANGE, pre) &&
         setVcselPulsePeriod(VL53L0X_VCSEL_PERIOD_FINAL_RANGE, final);
}

bool ProximityAndDistance::setDistanceModeLong(void)
{
  return setPreset(0.1f, 18, 14);
}

bool ProximityAndDistance::setDistanceModeShort(void)
{
  return setPreset(0.25f, 12, 8);
}

bool ProximityAndDistance::setDistanceModeDefault(void)
{
  return setPreset(0.25f, 14, 10);
}

/*
 * Require a finite wait in milliseconds; zero cannot disable timeout protection.
 */
bool ProximityAndDistance::setTimeout(uint16_t ms)
{
  if(!ms)
    return fail(PROXIMITY_AND_DISTANCE_INVALID_ARGUMENT);
  _core.io_timeout = ms;
  _lastStatus = PROXIMITY_AND_DISTANCE_OK;
  return true;
}

uint16_t ProximityAndDistance::getTimeout(void) const
{
  return _core.io_timeout;
}

bool ProximityAndDistance::timeoutOccurred(void)
{
  bool result = _didTimeout;
  _didTimeout = false;
  return result;
}

/*
 * Check period versus timing budget and oscillator scaling before starting repeated measurements.
 */
bool ProximityAndDistance::startContinuous(uint32_t period)
{
  if(!prepare(true))
    return false;
  uint32_t budget = _core.getMeasurementTimingBudget();
  uint16_t oscillator = _core.readReg16Bit(0xF8);
  if(!finish())
    return false;
  if(period &&
     (uint64_t(period) * 1000 < budget || (oscillator && period > 0xFFFFFFFFUL / oscillator)))
    return fail(PROXIMITY_AND_DISTANCE_INVALID_ARGUMENT);
  if(_interruptConfig == 0)
    return fail(PROXIMITY_AND_DISTANCE_INVALID_ARGUMENT);
  _core.writeReg(0x0B, 1);
  _core.startContinuous(period);
  if(!finish())
    return false;
  _continuous = true;
  _period = period;
  invalidate();
  return true;
}

bool ProximityAndDistance::stopContinuous(void)
{
  if(!prepare())
    return false;
  if(_continuous)
    _core.stopContinuous();
  _core.writeReg(0x0B, 1);
  if(!finish())
    return false;
  _continuous = false;
  invalidate();
  return true;
}

/*
 * Validate the new period, then start or restart continuous ranging with that period.
 */
bool ProximityAndDistance::setContinuousModeInterMeasurementPeriod(uint16_t ms)
{
  if(!prepare())
    return false;
  uint32_t budget = _core.getMeasurementTimingBudget();
  if(!finish())
    return false;
  if(ms && uint32_t(ms) * 1000 < budget)
    return fail(PROXIMITY_AND_DISTANCE_INVALID_ARGUMENT);
  if(_continuous && !stopContinuous())
    return false;
  return startContinuous(ms);
}

/*
 * Poll the selected status bits with rollover-safe timing and yield between reads.
 */
bool ProximityAndDistance::waitFor(uint8_t reg, uint8_t mask, bool set)
{
  uint32_t start = millis();
  for(;;)
  {
    uint8_t value = _core.readReg(reg);
    if(!finish())
      return false;
    if(bool(value & mask) == set)
      return true;
    if(uint32_t(millis() - start) >= _core.io_timeout)
      return fail(PROXIMITY_AND_DISTANCE_TIMED_OUT);
    delay(1);
  }
}

/*
 * Translate device status only; ST host-side sigma estimation is not included.
 */
VL53L0X_RANGE_STATUS_t ProximityAndDistance::decodeRangeStatus(uint8_t raw)
{
  // Device status mapping follows ST; host sigma estimation is not implemented.
  switch((raw >> 3) & 15)
  {
    case 11:
      return VL53L0X_RANGE_VALID;
    case 1:
    case 2:
    case 3:
      return VL53L0X_HARDWARE_FAIL;
    case 6:
    case 9:
      return VL53L0X_PHASE_FAIL;
    case 8:
    case 10:
      return VL53L0X_MIN_RANGE_FAIL;
    case 4:
      return VL53L0X_SIGNAL_FAIL;
    default:
      return VL53L0X_NO_UPDATE;
  }
}

/*
 * Read one complete result and its banked event count before clearing the interrupt.
 */
int ProximityAndDistance::collectRange(void)
{
  invalidate();
  if(!waitFor(0x13, 7, true))
    return _lastStatus == PROXIMITY_AND_DISTANCE_TIMED_OUT ? -2 : -1;
  uint8_t data[12];
  _core.readMulti(0x14, data, 12);
  _core.writeReg(0xFF, 1);
  uint32_t events = _core.readReg32Bit(0xC0);
  _core.writeReg(0xFF, 0);
  _core.writeReg(0x0B, 1);
  if(!finish())
    return -1;
  _rangeStatus = decodeRangeStatus(data[0]);
  _signalRate = uint16_t(data[6]) << 8 | data[7];
  _ambientRate = uint16_t(data[8]) << 8 | data[9];
  _rawRange = uint16_t(data[10]) << 8 | data[11];
  _events = events;
  _valid = _rangeStatus == VL53L0X_RANGE_VALID && _rawRange <= 32767;
  if(!_valid)
  {
    fail(PROXIMITY_AND_DISTANCE_NO_DATA);
    return -1;
  }
  _lastDistance = _rawRange;
  return _lastDistance;
}

/*
 * Start an idle single-shot measurement and return millimeters or a documented negative error.
 */
int ProximityAndDistance::readRangeSingleMillimeters(void)
{
  if(!prepare(true))
    return -1;
  if(_interruptConfig == 0)
  {
    fail(PROXIMITY_AND_DISTANCE_INVALID_ARGUMENT);
    return -1;
  }
  invalidate();
  _core.writeReg(0x0B, 1);
  _core.writeReg(0x80, 1);
  _core.writeReg(0xFF, 1);
  _core.writeReg(0x00, 0);
  _core.writeReg(0x91, _core.stop_variable);
  _core.writeReg(0x00, 1);
  _core.writeReg(0xFF, 0);
  _core.writeReg(0x80, 0);
  _core.writeReg(0x00, 1);
  if(!finish())
    return -1;
  if(!waitFor(0x00, 1, false))
    return _lastStatus == PROXIMITY_AND_DISTANCE_TIMED_OUT ? -2 : -1;
  return collectRange();
}

/*
 * Return hardware range status and Q9.7 signal rate only for the measurement just attempted.
 */
int ProximityAndDistance::readRangeSingleMillimeters(uint8_t *status, uint16_t *signal)
{
  if(!status || !signal)
  {
    fail(PROXIMITY_AND_DISTANCE_INVALID_ARGUMENT);
    return -1;
  }
  *status = VL53L0X_NO_UPDATE;
  *signal = 0;
  int value = readRangeSingleMillimeters();
  if(_lastStatus == PROXIMITY_AND_DISTANCE_OK || _lastStatus == PROXIMITY_AND_DISTANCE_NO_DATA)
  {
    *status = _rangeStatus;
    *signal = _signalRate;
  }
  return value;
}

/*
 * Wait for a result only when continuous ranging has already been started.
 */
int ProximityAndDistance::readRangeContinuousMillimeters(void)
{
  if(!prepare())
    return -1;
  if(!_continuous)
  {
    fail(PROXIMITY_AND_DISTANCE_BUSY);
    return -1;
  }
  return collectRange();
}

int ProximityAndDistance::readDistance(bool print)
{
  int value = readRangeSingleMillimeters();
  if(print)
  {
    Serial.print("Distance (mm): ");
    if(value >= 0)
      Serial.println(value);
    else
      Serial.println("Unavailable");
  }
  return value;
}

int ProximityAndDistance::readDistanceContinuous(bool print)
{
  int value = readRangeContinuousMillimeters();
  if(print)
  {
    Serial.print("Distance (mm): ");
    if(value >= 0)
      Serial.println(value);
    else
      Serial.println("Unavailable");
  }
  return value;
}

PROXIMITY_AND_DISTANCE_STATUS_t ProximityAndDistance::getLastStatus(void) const
{
  return _lastStatus;
}

uint8_t ProximityAndDistance::getLastI2cStatus(void) const
{
  return _core.last_status;
}

int16_t ProximityAndDistance::getLastRange(void) const
{
  return _lastDistance;
}

uint16_t ProximityAndDistance::getRawRange(void) const
{
  return _rawRange;
}

uint16_t ProximityAndDistance::getAmbientRate(void) const
{
  return _ambientRate;
}

uint32_t ProximityAndDistance::getTotalEvents(void) const
{
  return _events;
}

VL53L0X_RANGE_STATUS_t ProximityAndDistance::getRangeStatus(void) const
{
  return _rangeStatus;
}

uint8_t ProximityAndDistance::getModelId(void)
{
  if(!prepare())
    return 0;
  uint8_t value = _core.readReg(0xC0);
  return finish() ? value : 0;
}

uint8_t ProximityAndDistance::getRevisionId(void)
{
  if(!prepare())
    return 0;
  uint8_t value = _core.readReg(0xC2);
  return finish() ? value : 0;
}

uint8_t ProximityAndDistance::getDeviceId(void)
{
  return getModelId();
}
