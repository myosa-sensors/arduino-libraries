/*
  This code is developed under the MYOSA (LearnTheEasyWay) initiative of MakeSense EduTech and Pegasus Automation.
  Code has been derived from internet sources and component datasheets.
  Existing readily-available libraries would have been used "AS IS" and modified for ease of learning purpose.

  Synopsis of Heart Rate and SpO2 sensor
  MAX30100 optical heart-rate and estimated SpO2 sensing at I2C address 0x57.
  Reads infrared/red samples and die temperature using a caller-initialized Wire bus.
  Call update frequently; unavailable measurements return NAN.
  SpO2 uses an approximate calibration and requires validation for the actual module.

  NOTE
  All information, including URL references, is subject to change without prior notice.
  Please always use the latest versions of software-release for best performance.
  Unless required by applicable law or agreed to in writing, this software is distributed on an
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied

  Modifications
  11 September, 2026 by Pegasus Automation
  (as a part of MYOSA Initiative)

  Contact Team MYOSA for feedback, issues, or update requests.
  Email: myosa.event@gmail.com
*/

/* Library Inclusion */
#include "HeartRateAndSpO2.h"

namespace
{
const uint16_t sampleRates[] = {50, 100, 167, 200, 400, 600, 800, 1000};
const uint16_t pulseWidths[] = {200, 400, 800, 1600};
const uint16_t ledTenths[] = {0,   44,  76,  110, 142, 174, 208, 240,
                              271, 306, 338, 370, 402, 436, 468, 500};

uint8_t nearestLed(uint8_t ma)
{
  uint8_t best = 0;
  uint16_t error = 65535;
  for(uint8_t i = 0; i < 16; ++i)
  {
    int delta = int(ma) * 10 - ledTenths[i];
    uint16_t distance = delta < 0 ? -delta : delta;
    if(distance < error)
    {
      error = distance;
      best = i;
    }
  }
  return best;
}

bool compatible(uint8_t rate, uint8_t width, HEART_RATE_AND_SPO2_MODE_t mode)
{
  if(rate > 7 || width > 3)
    return false;
  if(mode != HEART_RATE_AND_SPO2_MODE_HRONLY && mode != HEART_RATE_AND_SPO2_MODE_SPO2_HR)
    return false;
  if(width == 3)
    return sampleRates[rate] <= 100;
  if(width == 2)
    return sampleRates[rate] <= 200;
  if(width == 1 && mode == HEART_RATE_AND_SPO2_MODE_SPO2_HR)
    return sampleRates[rate] <= 400;
  return true;
}
} // namespace

HeartRateAndSpO2::HeartRateAndSpO2(TwoWire *wire, uint8_t address)
    : _wire(wire), _address(address), _heartRate(NAN), _spO2(NAN), _temperature(NAN)
{
}

/*
 * Record the error and invalidate caches when a transfer or timeout makes state uncertain.
 */
bool HeartRateAndSpO2::fail(HEART_RATE_AND_SPO2_STATUS_t status)
{
  _lastStatus = status;
  if(status == HEART_RATE_AND_SPO2_I2C_ERROR || status == HEART_RATE_AND_SPO2_TIMEOUT)
  {
    resetDetectionState();
    clearRawBuffer();
    _temperatureReady = _temperaturePending = false;
    _temperature = NAN;
    // Configuration may have been partially written; require a fresh begin.
    _initialized = false;
  }
  return false;
}

bool HeartRateAndSpO2::ready(void)
{
  if(!_initialized)
    return fail(HEART_RATE_AND_SPO2_NOT_INITIALIZED);
  return true;
}

/*
 * Require an acknowledged register selection and the exact requested receive length.
 */
bool HeartRateAndSpO2::readMultiBytes(uint8_t reg, uint8_t length, uint8_t *values)
{
  if(!_wire || !values || length == 0 || length > 32 || _address != MAX30100_I2C_ADDRESS)
    return fail(HEART_RATE_AND_SPO2_INVALID_ARGUMENT);
  _wire->beginTransmission(_address);
  size_t written = _wire->write(reg);
  uint8_t result = _wire->endTransmission(false);
  if(written != 1 || result)
    return fail(HEART_RATE_AND_SPO2_I2C_ERROR);
  if(_wire->requestFrom(_address, length) != length)
    return fail(HEART_RATE_AND_SPO2_I2C_ERROR);
  for(uint8_t i = 0; i < length; ++i)
  {
    if(!_wire->available())
      return fail(HEART_RATE_AND_SPO2_I2C_ERROR);
    int value = _wire->read();
    if(value < 0)
      return fail(HEART_RATE_AND_SPO2_I2C_ERROR);
    values[i] = uint8_t(value);
  }
  _lastStatus = HEART_RATE_AND_SPO2_OK;
  return true;
}

bool HeartRateAndSpO2::readByte(uint8_t reg, uint8_t *value)
{
  return readMultiBytes(reg, 1, value);
}

bool HeartRateAndSpO2::writeByte(uint8_t reg, uint8_t value)
{
  if(!_wire || _address != MAX30100_I2C_ADDRESS)
    return fail(HEART_RATE_AND_SPO2_INVALID_ARGUMENT);
  _wire->beginTransmission(_address);
  size_t written = _wire->write(reg);
  written += _wire->write(value);
  uint8_t result = _wire->endTransmission();
  if(written != 2 || result)
    return fail(HEART_RATE_AND_SPO2_I2C_ERROR);
  _lastStatus = HEART_RATE_AND_SPO2_OK;
  return true;
}

/*
 * Preserve unrelated bits; a failed register read must never be followed by a write.
 */
bool HeartRateAndSpO2::modifyByte(uint8_t reg, uint8_t mask, uint8_t value)
{
  uint8_t old;
  return readByte(reg, &old) && writeByte(reg, (old & ~mask) | (value & mask));
}

/*
 * Check the fixed MAX30100 part ID without implicitly reconfiguring a connected device.
 */
bool HeartRateAndSpO2::ping(void)
{
  uint8_t id;
  if(!readByte(MAX30100_REG_PART_ID, &id))
    return false;
  if(id != MAX30100_EXPECTED_PART_ID)
  {
    _initialized = false;
    resetDetectionState();
    return fail(HEART_RATE_AND_SPO2_NOT_FOUND);
  }
  return true;
}

/*
 * Request reset with a bounded wait; call begin() before taking more measurements.
 */
bool HeartRateAndSpO2::reset(void)
{
  _initialized = false;
  _sleeping = false;
  clearRawBuffer();
  resetDetectionState();
  _temperaturePending = _temperatureReady = false;
  _temperature = NAN;
  _interruptMask = _interruptStatus = 0;
  _overflow = false;
  if(!writeByte(MAX30100_REG_MODE_CONFIGURATION, MAX30100_REG_MODE_RESET))
    return false;
  uint32_t start = millis();
  do
  {
    uint8_t mode;
    if(!readByte(MAX30100_REG_MODE_CONFIGURATION, &mode))
      return false;
    if(!(mode & MAX30100_REG_MODE_RESET))
      return true;
    delay(1);
  } while(uint32_t(millis() - start) < 500);
  return fail(HEART_RATE_AND_SPO2_TIMEOUT);
}

/*
 * Verify identity and configure defaults; Wire initialization remains the caller's responsibility.
 */
bool HeartRateAndSpO2::begin(HEART_RATE_AND_SPO2_MODE_t mode)
{
  if(!compatible(1, 3, mode))
    return fail(HEART_RATE_AND_SPO2_INVALID_ARGUMENT);
  _initialized = false;
  if(!ping() || !reset())
    return false;
  _mode = mode;
  _rate = 1;
  _width = 3;
  _samplesPerSecond = 100;
  if(!writeByte(MAX30100_REG_SPO2_CONFIGURATION, 0x47) ||
     !writeByte(MAX30100_REG_LED_CONFIGURATION, 0xFF) ||
     !writeByte(MAX30100_REG_INTERRUPT_ENABLE, 0) ||
     !writeByte(MAX30100_REG_MODE_CONFIGURATION, uint8_t(mode)))
    return false;
  _initialized = true;
  return resetFifo() && clearInterrupts();
}

bool HeartRateAndSpO2::shutdown(void)
{
  if(!ready() || !modifyByte(MAX30100_REG_MODE_CONFIGURATION, 0x80, 0x80))
    return false;
  _sleeping = true;
  resetDetectionState();
  clearRawBuffer();
  _temperatureReady = _temperaturePending = false;
  _temperature = NAN;
  return true;
}

bool HeartRateAndSpO2::resume(void)
{
  if(!ready() || !resetFifo() || !modifyByte(MAX30100_REG_MODE_CONFIGURATION, 0x80, 0))
    return false;
  _sleeping = false;
  return true;
}

/*
 * Reject unsupported rate/width/mode combinations before writing, then discard old samples.
 */
bool HeartRateAndSpO2::applyConfiguration(uint8_t rate, uint8_t width,
                                          HEART_RATE_AND_SPO2_MODE_t mode)
{
  if(!compatible(rate, width, mode))
    return fail(HEART_RATE_AND_SPO2_INVALID_ARGUMENT);
  if(!ready())
    return false;
  if(!modifyByte(MAX30100_REG_SPO2_CONFIGURATION, 0x1F, (rate << 2) | width) ||
     !modifyByte(MAX30100_REG_MODE_CONFIGURATION, 0x07, uint8_t(mode)))
    return false;
  _rate = rate;
  _width = width;
  _mode = mode;
  _samplesPerSecond = sampleRates[rate];
  return resetFifo();
}

bool HeartRateAndSpO2::setSampleRate(HEART_RATE_AND_SPO2_SAMPLING_RATE_t rate)
{
  if(unsigned(rate) > 7)
    return fail(HEART_RATE_AND_SPO2_INVALID_ARGUMENT);
  return applyConfiguration(uint8_t(rate), _width, _mode);
}

bool HeartRateAndSpO2::setSampleRate(uint16_t rate)
{
  for(uint8_t i = 0; i < 8; ++i)
    if(sampleRates[i] == rate)
      return applyConfiguration(i, _width, _mode);
  return fail(HEART_RATE_AND_SPO2_INVALID_ARGUMENT);
}

bool HeartRateAndSpO2::setPulseWidth(HEART_RATE_AND_SPO2_PULSE_WIDTH_t width)
{
  if(unsigned(width) > 3)
    return fail(HEART_RATE_AND_SPO2_INVALID_ARGUMENT);
  return applyConfiguration(_rate, uint8_t(width), _mode);
}

bool HeartRateAndSpO2::setPulseWidth(uint16_t width)
{
  for(uint8_t i = 0; i < 4; ++i)
    if(pulseWidths[i] == width)
      return applyConfiguration(_rate, i, _mode);
  return fail(HEART_RATE_AND_SPO2_INVALID_ARGUMENT);
}

bool HeartRateAndSpO2::setMode(HEART_RATE_AND_SPO2_MODE_t mode)
{
  return applyConfiguration(_rate, _width, mode);
}

HEART_RATE_AND_SPO2_MODE_t HeartRateAndSpO2::getMode(void) const
{
  return _mode;
}

bool HeartRateAndSpO2::setHighResolutionMode(bool enable)
{
  if(!ready() || !modifyByte(MAX30100_REG_SPO2_CONFIGURATION, 0x40, enable ? 0x40 : 0))
    return false;
  return resetFifo();
}

bool HeartRateAndSpO2::setLedCurrent(uint8_t ma)
{
  return setLedCurrents(ma, ma);
}

/*
 * Round each requested current to the nearest hardware step; values above 50 mA are invalid.
 */
bool HeartRateAndSpO2::setLedCurrents(uint8_t ir, uint8_t red)
{
  if(ir > 50 || red > 50)
    return fail(HEART_RATE_AND_SPO2_INVALID_ARGUMENT);
  return setLedCurrentByCode(nearestLed(ir), nearestLed(red));
}

bool HeartRateAndSpO2::setLedCurrentByCode(uint8_t ir, uint8_t red)
{
  if(ir > 15 || red > 15)
    return fail(HEART_RATE_AND_SPO2_INVALID_ARGUMENT);
  if(!ready() || !writeByte(MAX30100_REG_LED_CONFIGURATION, (red << 4) | ir))
    return false;
  return resetFifo();
}

/*
 * Use separate on/off thresholds to avoid rapid finger-detection toggling near the boundary.
 */
bool HeartRateAndSpO2::setFingerModeThresholds(uint16_t on, uint16_t off)
{
  if(!on || off >= on)
    return fail(HEART_RATE_AND_SPO2_INVALID_ARGUMENT);
  _fingerOn = on;
  _fingerOff = off;
  resetDetectionState();
  _lastStatus = HEART_RATE_AND_SPO2_OK;
  return true;
}

bool HeartRateAndSpO2::setFingerDetectionThreshold(uint16_t threshold)
{
  return setFingerModeThresholds(threshold, uint32_t(threshold) * 4 / 5);
}

uint16_t HeartRateAndSpO2::getFingerDetectionThreshold(void) const
{
  return _fingerOn;
}

/*
 * Bound the average to the fixed 16-entry beat-interval array and reset its history.
 */
bool HeartRateAndSpO2::configureHeartRateWindow(uint16_t size)
{
  if(!size || size > 16)
    return fail(HEART_RATE_AND_SPO2_INVALID_ARGUMENT);
  _heartWindow = size;
  resetProcessing();
  _lastStatus = HEART_RATE_AND_SPO2_OK;
  return true;
}

/*
 * Set the optical averaging length in samples; accumulation uses a bounded counter.
 */
bool HeartRateAndSpO2::configureSpO2Window(uint16_t size)
{
  if(size < 50 || size > 4096)
    return fail(HEART_RATE_AND_SPO2_INVALID_ARGUMENT);
  _spo2Window = size;
  resetProcessing();
  _lastStatus = HEART_RATE_AND_SPO2_OK;
  return true;
}

uint16_t HeartRateAndSpO2::getDefaultSpO2Window(void) const
{
  return 200;
}

uint16_t HeartRateAndSpO2::getDefaultHeartRateWindow(void) const
{
  return 4;
}

bool HeartRateAndSpO2::setLedPulseWidthWithBuffer(HEART_RATE_AND_SPO2_PULSE_WIDTH_t width,
                                                  uint16_t window)
{
  if(!window || window > 16)
    return fail(HEART_RATE_AND_SPO2_INVALID_ARGUMENT);
  return setPulseWidth(width) && configureHeartRateWindow(window);
}

/*
 * Set the linear ratio-to-percentage curve; this does not validate real-module accuracy.
 */
bool HeartRateAndSpO2::setSpO2Calibration(float intercept, float slope)
{
  if(!isfinite(intercept) || !isfinite(slope) || intercept <= 0 || intercept > 200 || slope <= 0 ||
     slope > 200)
    return fail(HEART_RATE_AND_SPO2_INVALID_ARGUMENT);
  _spo2Intercept = intercept;
  _spo2Slope = slope;
  resetProcessing();
  _lastStatus = HEART_RATE_AND_SPO2_OK;
  return true;
}

bool HeartRateAndSpO2::setInterruptsEnabled(bool hr, bool spo2, bool temp, bool full)
{
  uint8_t mask = (hr ? 0x20 : 0) | (spo2 ? 0x10 : 0) | (temp ? 0x40 : 0) | (full ? 0x80 : 0);
  if(!ready() || !writeByte(MAX30100_REG_INTERRUPT_ENABLE, mask))
    return false;
  _interruptMask = mask;
  return true;
}

uint8_t HeartRateAndSpO2::getInterruptEnableMask(void) const
{
  return _interruptMask;
}

uint8_t HeartRateAndSpO2::getInterruptStatus(uint8_t mask) const
{
  return _interruptStatus & mask;
}

bool HeartRateAndSpO2::clearInterrupts(void)
{
  uint8_t ignored;
  if(!ready() || !readByte(MAX30100_REG_INTERRUPT_STATUS, &ignored))
    return false;
  _interruptStatus = 0;
  return true;
}

/*
 * Start a die-temperature conversion without blocking; update/getters poll its completion.
 */
bool HeartRateAndSpO2::startTemperatureSampling(void)
{
  if(!ready())
    return false;
  if(_sleeping || _temperaturePending)
    return fail(HEART_RATE_AND_SPO2_BUSY);
  _temperatureReady = false;
  _temperature = NAN;
  if(!modifyByte(MAX30100_REG_MODE_CONFIGURATION, 0x08, 0x08))
    return false;
  _temperaturePending = true;
  _temperatureStarted = millis();
  return true;
}

/*
 * Complete or time out the pending conversion; decode the integer byte as signed.
 */
bool HeartRateAndSpO2::pollTemperature(void)
{
  if(!_temperaturePending)
    return true;
  uint8_t mode;
  if(!readByte(MAX30100_REG_MODE_CONFIGURATION, &mode))
    return false;
  if(!(mode & 0x08))
  {
    uint8_t bytes[2];
    if(!readMultiBytes(MAX30100_REG_TEMPERATURE_DATA_INT, 2, bytes))
      return false;
    _temperature = int8_t(bytes[0]) + (bytes[1] & 15) * 0.0625f;
    _temperaturePending = false;
    _temperatureReady = true;
    return true;
  }
  if(uint32_t(millis() - _temperatureStarted) >= 500)
    return fail(HEART_RATE_AND_SPO2_TIMEOUT);
  return true;
}

bool HeartRateAndSpO2::isTemperatureReady(void) const
{
  return _temperatureReady;
}

/*
 * Expose the cached signed integer byte and fractional sixteenths after a completed conversion.
 */
bool HeartRateAndSpO2::readTemperatureRaw(uint8_t *integer, uint8_t *fraction)
{
  if(!integer || !fraction)
    return fail(HEART_RATE_AND_SPO2_INVALID_ARGUMENT);
  if(!ready() || !pollTemperature())
    return false;
  if(!_temperatureReady)
    return fail(HEART_RATE_AND_SPO2_NO_DATA);
  int value = int(floor(_temperature));
  *integer = uint8_t(int8_t(value));
  *fraction = uint8_t((_temperature - value) * 16);
  _lastStatus = HEART_RATE_AND_SPO2_OK;
  return true;
}

float HeartRateAndSpO2::getTemperatureC(bool print)
{
  if(!ready() || !pollTemperature())
    return NAN;
  if(!_temperatureReady)
  {
    fail(HEART_RATE_AND_SPO2_NO_DATA);
    return NAN;
  }
  _lastStatus = HEART_RATE_AND_SPO2_OK;
  if(print)
  {
    Serial.print("Die Temperature (C): ");
    Serial.println(_temperature, 2);
  }
  return _temperature;
}

float HeartRateAndSpO2::getTemperatureF(bool print)
{
  float value = getTemperatureC(false) * 1.8f + 32;
  if(print)
  {
    Serial.print("Die Temperature (F): ");
    if(isfinite(value))
      Serial.println(value, 2);
    else
      Serial.println("Unavailable");
  }
  return value;
}

uint8_t HeartRateAndSpO2::getDeviceId(void)
{
  uint8_t id = 0;
  readByte(MAX30100_REG_PART_ID, &id);
  return id;
}

uint8_t HeartRateAndSpO2::getRevisionId(void)
{
  uint8_t id = 0;
  readByte(MAX30100_REG_REVISION_ID, &id);
  return id;
}

HEART_RATE_AND_SPO2_STATUS_t HeartRateAndSpO2::getLastStatus(void) const
{
  return _lastStatus;
}

uint8_t HeartRateAndSpO2::getWritePointer(void)
{
  uint8_t v = 0;
  if(ready())
    readByte(0x02, &v);
  return v & 15;
}

uint8_t HeartRateAndSpO2::getReadPointer(void)
{
  uint8_t v = 0;
  if(ready())
    readByte(0x04, &v);
  return v & 15;
}

uint8_t HeartRateAndSpO2::getOverflowCounter(void)
{
  uint8_t v = 0;
  if(ready())
    readByte(0x03, &v);
  return v & 15;
}

bool HeartRateAndSpO2::isFifoOverflowed(void) const
{
  return _overflow;
}

/*
 * Clear hardware pointers and software history together so samples use the current configuration.
 */
bool HeartRateAndSpO2::resetFifo(void)
{
  if(!ready())
    return false;
  clearRawBuffer();
  resetDetectionState();
  _overflow = false;
  return writeByte(0x02, 0) && writeByte(0x03, 0) && writeByte(0x04, 0);
}

bool HeartRateAndSpO2::update(void)
{
  return readFifo();
}

/*
 * Drain available samples in order; overflow or missing data cannot produce a fresh valid reading.
 */
bool HeartRateAndSpO2::readFifo(void)
{
  if(!ready())
    return false;
  if(_sleeping)
    return fail(HEART_RATE_AND_SPO2_BUSY);
  if(!pollTemperature())
    return false;
  uint8_t status, pointers[3];
  if(!readByte(0x00, &status) || !readMultiBytes(0x02, 3, pointers))
    return false;
  _interruptStatus |= status;
  uint8_t count = (pointers[0] - pointers[2]) & 15;
  // Equal pointers plus a full indication is ambiguous; discard and resynchronize.
  if((pointers[1] & 15) || (!count && (status & 0xB0)))
  {
    if(!resetFifo())
      return false;
    _overflow = true;
    return fail(HEART_RATE_AND_SPO2_NO_DATA);
  }
  _overflow = false;
  if(!count)
  {
    if(_hasSample && uint32_t(micros() - _lastSample) > 2000000UL)
      resetDetectionState();
    return fail(HEART_RATE_AND_SPO2_NO_DATA);
  }
  uint32_t now = micros(), interval = 1000000UL / _samplesPerSecond;
  uint32_t first = now - uint32_t(count - 1) * interval;
  if(_hasSample)
  {
    uint32_t elapsed = now - _lastSample;
    if(elapsed > uint32_t(count + 3) * interval)
      resetDetectionState();
    // Anchor the batch to its observation time; never date a sample in the future.
  }
  // A hardware FIFO sample is four bytes, infrared first, then red.
  for(uint8_t i = 0; i < count; ++i)
  {
    uint8_t bytes[4];
    if(!readMultiBytes(0x05, 4, bytes))
      return false;
    uint16_t ir = uint16_t(bytes[0]) << 8 | bytes[1];
    uint16_t red = uint16_t(bytes[2]) << 8 | bytes[3];
    uint32_t timestamp = first + uint32_t(i) * interval;
    processSample(ir, red, timestamp);
    pushRawData(ir, red, timestamp);
    _lastSample = timestamp;
    _hasSample = true;
  }
  _lastStatus = HEART_RATE_AND_SPO2_OK;
  return true;
}

void HeartRateAndSpO2::clearRawBuffer(void)
{
  _head = _count = 0;
}

bool HeartRateAndSpO2::isRawDataAvailable(void) const
{
  return _count != 0;
}

int HeartRateAndSpO2::getBufferedSampleCount(void) const
{
  return _count;
}

/*
 * Keep the newest 16 samples; overwrite the oldest entry when the software queue is full.
 */
void HeartRateAndSpO2::pushRawData(uint16_t ir, uint16_t red, uint32_t time)
{
  uint8_t index = (_head + _count) % 16;
  _raw[index] = {ir, red, time, _finger};
  if(_count == 16)
    _head = (_head + 1) % 16;
  else
    ++_count;
}

/*
 * Peek by age without consuming data; index zero is oldest and failed calls leave outputs alone.
 */
bool HeartRateAndSpO2::getRawSample(uint16_t index, uint16_t *ir, uint16_t *red, uint32_t *time)
{
  if(!ir || !red)
    return fail(HEART_RATE_AND_SPO2_INVALID_ARGUMENT);
  if(index >= _count)
    return fail(HEART_RATE_AND_SPO2_NO_DATA);
  const MAX30100_RAW_DATA_t &sample = _raw[(_head + index) % 16];
  *ir = sample.infrared;
  *red = sample.red;
  if(time)
    *time = sample.micros;
  _lastStatus = HEART_RATE_AND_SPO2_OK;
  return true;
}

/*
 * Remove the oldest sample only after successfully copying it to valid output pointers.
 */
bool HeartRateAndSpO2::popRawValue(uint16_t *ir, uint16_t *red, uint32_t *time)
{
  if(!getRawSample(0, ir, red, time))
    return false;
  _head = (_head + 1) % 16;
  --_count;
  return true;
}

/*
 * Peek at the latest buffered pair; this method does not fetch new hardware samples.
 */
bool HeartRateAndSpO2::getRawValues(uint16_t *ir, uint16_t *red, bool print)
{
  if(!getRawSample(_count ? _count - 1 : 0, ir, red))
    return false;
  if(print)
  {
    Serial.print("Infrared (counts): ");
    Serial.println(*ir);
    Serial.print("Red (counts): ");
    Serial.println(*red);
  }
  return true;
}

/*
 * Discard filter, pulse, and optical averaging history after configuration or signal changes.
 */
void HeartRateAndSpO2::resetProcessing(void)
{
  _heartRate = _spO2 = NAN;
  _filterInitialized = _abovePeak = _hasBeat = false;
  _irDc = _redDc = _filtered = _envelope = 0;
  _irSquare = _redSquare = _irSum = _redSum = 0;
  _opticalCount = _intervalCount = _intervalIndex = 0;
  _lastBeat = 0;
  _fingerSince = _lastSample;
}

void HeartRateAndSpO2::resetDetectionState(void)
{
  _finger = false;
  _hasSample = false;
  resetProcessing();
}

bool HeartRateAndSpO2::resetReadingAlgorithm(void)
{
  resetDetectionState();
  _lastStatus = HEART_RATE_AND_SPO2_OK;
  return true;
}

/*
 * Require both a detected finger and a sample no more than two seconds old.
 */
bool HeartRateAndSpO2::isFingerDetected(void) const
{
  return _finger && _hasSample && uint32_t(micros() - _lastSample) <= 2000000UL;
}

bool HeartRateAndSpO2::isReadingValid(void) const
{
  return isfinite(getHeartRate(false)) &&
         (_mode == HEART_RATE_AND_SPO2_MODE_HRONLY || isfinite(getSpO2(false)));
}

/*
 * Return cached BPM only while the finger signal and last detected beat remain recent.
 */
float HeartRateAndSpO2::getHeartRate(bool print) const
{
  float value = isFingerDetected() && _hasBeat && uint32_t(micros() - _lastBeat) <= 2000000UL
                    ? _heartRate
                    : NAN;
  if(print)
  {
    Serial.print("Heart Rate (BPM): ");
    if(isfinite(value))
      Serial.println(value, 2);
    else
      Serial.println("Unavailable");
  }
  return value;
}

/*
 * Require SpO2 mode and a usable pulse before returning the cached percentage estimate.
 */
float HeartRateAndSpO2::getSpO2(bool print) const
{
  float value =
      _mode == HEART_RATE_AND_SPO2_MODE_SPO2_HR && isfinite(getHeartRate(false)) ? _spO2 : NAN;
  if(print)
  {
    Serial.print("SpO2 (%): ");
    if(isfinite(value))
      Serial.println(value, 2);
    else
      Serial.println("Unavailable");
  }
  return value;
}

/*
 * Reject implausibly short intervals and average accepted beat intervals in milliseconds.
 */
void HeartRateAndSpO2::recordBeat(uint32_t time)
{
  if(_hasBeat)
  {
    uint32_t elapsed = time - _lastBeat;
    if(elapsed < 250000UL)
      return;
    if(elapsed <= 2000000UL)
    {
      _intervals[_intervalIndex] = (elapsed + 500) / 1000;
      _intervalIndex = (_intervalIndex + 1) % _heartWindow;
      if(_intervalCount < _heartWindow)
        ++_intervalCount;
      uint32_t total = 0;
      for(uint8_t i = 0; i < _intervalCount; ++i)
        total += _intervals[i];
      _heartRate = 60000.0f * _intervalCount / total;
    }
    else
    {
      _intervalCount = _intervalIndex = 0;
      _heartRate = _spO2 = NAN;
    }
  }
  _lastBeat = time;
  _hasBeat = true;
}

/*
 * Reject missing/saturated signals, settle filters, then accumulate pulse and optical estimates.
 */
void HeartRateAndSpO2::processSample(uint16_t ir, uint16_t red, uint32_t time)
{
  bool present = ir >= (_finger ? _fingerOff : _fingerOn);
  if(!present || ir >= 65520 ||
     (_mode == HEART_RATE_AND_SPO2_MODE_SPO2_HR && (red == 0 || red >= 65520)))
  {
    resetDetectionState();
    return;
  }
  if(!_finger)
  {
    resetProcessing();
    _finger = true;
    _fingerSince = time;
  }
  if(!_filterInitialized)
  {
    _irDc = ir;
    _redDc = red;
    _filterInitialized = true;
  }
  // Sample-rate-scaled DC removal and 5 Hz low-pass smoothing.
  float dcAlpha = 1.0f / (_samplesPerSecond + 1.0f);
  _irDc += dcAlpha * (ir - _irDc);
  _redDc += dcAlpha * (red - _redDc);
  float irAc = ir - _irDc, redAc = red - _redDc;
  float alpha = 31.4159f / (_samplesPerSecond + 31.4159f);
  _filtered += alpha * (irAc - _filtered);
  float magnitude = _filtered < 0 ? -_filtered : _filtered;
  _envelope += dcAlpha * (magnitude - _envelope);
  if(uint32_t(time - _fingerSince) < 2000000UL)
    return;
  float threshold = _envelope * 0.6f;
  if(threshold < 20)
    threshold = 20;
  if(!_abovePeak && _filtered > threshold)
  {
    recordBeat(time);
    _abovePeak = true;
  }
  if(_abovePeak && _filtered < -threshold * 0.5f)
    _abovePeak = false;
  if(_hasBeat && uint32_t(time - _lastBeat) > 2000000UL)
  {
    _heartRate = _spO2 = NAN;
    _hasBeat = false;
    _intervalCount = _intervalIndex = 0;
  }
  if(_mode != HEART_RATE_AND_SPO2_MODE_SPO2_HR)
    return;
  _irSquare += irAc * irAc;
  _redSquare += redAc * redAc;
  _irSum += _irDc;
  _redSum += _redDc;
  if(++_opticalCount >= _spo2Window)
  {
    _spO2 = NAN;
    if(_irSquare > _opticalCount * 25.0f && _redSquare > _opticalCount * 25.0f && _irSum > 0 &&
       _redSum > 0)
    {
      float ratio = sqrtf(_redSquare / _irSquare) * _irSum / _redSum;
      float value = _spo2Intercept - _spo2Slope * ratio;
      if(isfinite(value) && value >= 70 && value <= 100)
        _spO2 = value;
    }
    _irSquare = _redSquare = _irSum = _redSum = 0;
    _opticalCount = 0;
  }
}
