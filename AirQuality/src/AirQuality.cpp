/*
  This code is developed under the MYOSA (LearnTheEasyWay) initiative of MakeSense EduTech and Pegasus Automation.
  Code has been derived from internet sources and component datasheets.
  Existing readily-available libraries would have been used "AS IS" and modified for ease of learning purpose.

  Synopsis of Air Quality
  The MYOSA air-quality board uses the CCS811 to report eCO2 (ppm) and TVOC (ppb).
  The default I2C address is 0x5B; select 0x5A when the hardware address pin requires it.
  Read algorithm results when data is ready; hasReading() reports valid cached data.

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

#include "AirQuality.h"

/*
 * Initialize the device address and local state before any measurement is requested.
 */
AirQuality::AirQuality(uint8_t i2c_add, float refRes)
{
  _i2cSlaveAddress = i2c_add;
  _refResistance   = (isfinite(refRes) && refRes > 0) ? refRes : 10000.f;
  _isConnected     = false;
}

/*
 * Reset and verify the CCS811, start its application, and select drive mode 1.
 */
CCS811_STATUS_t AirQuality::begin(void)
{
  _isConnected = false;
  _hasReading = false;
  CCS811_STATUS_t result;

  /* make soft-reset the chip */
  result = reset();
  if(result != SENSOR_SUCCESS)
  {
    return result;
  }

  /* wait until soft-reset completion */
  delay_ms(100u);
  if(getHwId() != CCS811_HW_ID)
  {
    return SENSOR_ID_ERROR;
  }

  /* get the status */
  if(getStatusReg(&statusReg) != SENSOR_SUCCESS)
  {
    return SENSOR_I2C_ERROR;
  }

  /* Check for Application validity */
  if((statusReg.APP_VALID == 0u)||(statusReg.ERROR))
  {
    return SENSOR_INTERNAL_ERROR;
  }

  /* Change from bootmode to application running */
  if(writeByte(CCS811_APP_START_REG) != SENSOR_SUCCESS)
  {
    return SENSOR_I2C_ERROR;
  }

  delay_ms(100u);
  /* get the status */
  if(getStatusReg(&statusReg) != SENSOR_SUCCESS)
  {
    return SENSOR_I2C_ERROR;
  }

  /* Check the firmware mode */
  if(statusReg.FW_MODE == 0u)
  {
    return SENSOR_INTERNAL_ERROR;
  }

  if(setDriveMode(CCS811_DRIVE_MODE1) != SENSOR_SUCCESS)
  {
    return SENSOR_I2C_ERROR;
  }
  _isConnected = true;
  return SENSOR_SUCCESS;
}

/*
 * Send the software-reset sequence and invalidate previously cached algorithm readings.
 */
CCS811_STATUS_t AirQuality::reset(void)
{
  _isConnected = false;
  _hasReading = false;
  _CO2 = _tVOC = 0;
  const uint8_t seq[] = {0x11u, 0xE5u, 0x72u, 0x8Au};
  return writeMultiBytes(CCS811_SOFT_RESET_REG,sizeof(seq),seq);
}

/*
 * Probe the device and reinitialize it when the previous connection was lost.
 */
bool AirQuality::ping(void)
{
  if(!writeAddress()) { _isConnected = false; _hasReading = false; return false; }
  if(!_isConnected) _isConnected = (begin() == SENSOR_SUCCESS);
  return _isConnected;
}

/*
 * Update cached eCO2/TVOC only after a complete, error-free algorithm-result transfer.
 */
CCS811_STATUS_t AirQuality::readAlgorithmResults(void)
{
  uint8_t data[8];
  const CCS811_STATUS_t result = readMultiBytes(CCS811_ALG_RESULT_DATA_REG, sizeof(data), data);
  if(result != SENSOR_SUCCESS) { _hasReading = false; return result; }
  if(data[4] & 0x01u) { _hasReading = false; return SENSOR_INTERNAL_ERROR; }
  _CO2 = ((uint16_t)data[0] << 8) | data[1];
  _tVOC = ((uint16_t)data[2] << 8) | data[3];
  _hasReading = true;
  return SENSOR_SUCCESS;
}

/*
 * Set the reference resistor value in ohms used by the NTC conversion.
 */
void AirQuality::setRefResistance(float refRes)
{
  if(isfinite(refRes) && refRes > 0) _refResistance = refRes;
}

/*
 * Restore a previously saved raw algorithm baseline; return the transfer status.
 */
CCS811_STATUS_t AirQuality::setBaseLine(uint16_t baseLine)
{
  uint8_t data[2u];
  data[0u] = (baseLine >> 8u);
  data[1u] = (baseLine & 0xFFu);
  return writeMultiBytes(CCS811_BASELINE_REG,2u,data);
}

/*
 * Read the raw algorithm baseline for later restoration.
 */
uint16_t AirQuality::getBaseLine(void)
{
  uint8_t data[2u];
  CCS811_STATUS_t result = readMultiBytes(CCS811_BASELINE_REG,2u,data);
  if(result != SENSOR_SUCCESS)
  {
    return 0u;
  }
  return (((uint16_t)data[0u] << 8u)|data[1u]);
}

/*
 * Encode humidity (%) and ambient temperature (C) for compensation; reject invalid ranges.
 */
CCS811_STATUS_t AirQuality::setEnvironmentalData(float relativeHumidity , float ambientTemperature)
{
  uint8_t data[4];
  if(!isfinite(relativeHumidity) || relativeHumidity < 0 || relativeHumidity > 100 ||
     !isfinite(ambientTemperature) || ambientTemperature < -25 || ambientTemperature > 102.99f) return SENSOR_GENERIC_ERROR;
  uint16_t RH = (uint16_t)(relativeHumidity*512.f);
  uint16_t TEMP = (uint16_t)((ambientTemperature + 25.f)*512.f);
  /* Calculate humidity higher & lower bytes */
  data[0u] = RH >> 8u;
  data[1u] = RH & 0xFFu;
  /* Calculate temperature higher & lower bytes */
  data[2u] = TEMP >> 8u;
  data[3u] = TEMP & 0xFFu;
  /* Write enviromental data */
  return writeMultiBytes(CCS811_ENV_DATA_REG,4u,data);
}

/*
 * Calculate cached resistance (ohms) and temperature (C); invalid ADC ratios leave NAN values.
 */
CCS811_STATUS_t AirQuality::readNTC(void)
{
  _resistance = _temperature = NAN;
  uint8_t data[4];
  float res, temp, lnR;
  CCS811_STATUS_t result = readMultiBytes(CCS811_NTC_REG, sizeof(data), data);
  if(result != SENSOR_SUCCESS)
  {
    return result;
  }
  _vREF = ((uint16_t)data[0] << 8) | data[1];
  _vNTC = ((uint16_t)data[2] << 8) | data[3];
  if(_vREF == 0 || _vNTC == 0) { _resistance = _temperature = NAN; return SENSOR_GENERIC_ERROR; }
  /* Calculate the thermistor resitance */
  res   = (float)_vNTC * _refResistance / (float)_vREF;
  /* Calculate the temperature */
  lnR   = log(res);
  /* T = 1 / {A + B[ln(R)] + C[ln(R)]^3} A = 0.001129148 B = 0.000234125 C = 8.76741E-08 */
  temp  = 1.f/(0.001129148f + (0.000234125f * lnR) + (0.0000000876741f * lnR * lnR * lnR));
  temp -= 273.15;
  /* update the variables */
  _resistance = res;
  _temperature = temp;
  return result;
}

/*
 * Enable the data-ready interrupt bit while preserving the drive mode.
 */
CCS811_STATUS_t AirQuality::enableDataInterrupt(void)
{
  uint8_t data;
  CCS811_STATUS_t result = readByte(CCS811_MEAS_MODE_REG, &data);
  if(result != SENSOR_SUCCESS)
  {
    return result;
  }
  /* set the data interrupt flag */
  data |= CCS811_INT_DATARDY_MSK;
  return writeByte(CCS811_MEAS_MODE_REG, data);
}

/*
 * Disable the data-ready interrupt bit while preserving the drive mode.
 */
CCS811_STATUS_t AirQuality::disableDataInterrupt(void)
{
  uint8_t data;
  CCS811_STATUS_t result = readByte(CCS811_MEAS_MODE_REG, &data);
  if(result != SENSOR_SUCCESS)
  {
    return result;
  }
  /* set the data interrupt flag */
  data &= ~CCS811_INT_DATARDY_MSK;
  return writeByte(CCS811_MEAS_MODE_REG, data);
}

/*
 * Enable threshold-based interrupt selection without changing other mode bits.
 */
CCS811_STATUS_t AirQuality::enableThreshInterrupt(void)
{
  uint8_t data;
  CCS811_STATUS_t result = readByte(CCS811_MEAS_MODE_REG, &data);
  if(result != SENSOR_SUCCESS)
  {
    return result;
  }
  /* set the data interrupt flag */
  data |= CCS811_INT_THRESH_MSK;
  return writeByte(CCS811_MEAS_MODE_REG, data);
}

/*
 * Disable threshold-based interrupt selection without changing other mode bits.
 */
CCS811_STATUS_t AirQuality::disableThreshInterrupt(void)
{
  uint8_t data;
  CCS811_STATUS_t result = readByte(CCS811_MEAS_MODE_REG, &data);
  if(result != SENSOR_SUCCESS)
  {
    return result;
  }
  /* set the data interrupt flag */
  data &= ~CCS811_INT_THRESH_MSK;
  return writeByte(CCS811_MEAS_MODE_REG, data);
}

/*
 * Write a drive-mode code (0-4), preserving the interrupt-enable bits.
 */
CCS811_STATUS_t AirQuality::setDriveMode(uint8_t mode)
{
  if(mode > 4) return SENSOR_GENERIC_ERROR;
  uint8_t data;
  CCS811_STATUS_t result = readByte(CCS811_MEAS_MODE_REG, &data);
  if(result != SENSOR_SUCCESS)
  {
    return result;
  }
  /* Replace only the drive-mode field. */
  data &= ~CCS811_DRIVE_MODE_MSK;
  data |= (uint8_t)mode << CCS811_DRIVE_MODE_POS;
  return writeByte(CCS811_MEAS_MODE_REG, data);
}

/*
 * Read the status byte into the caller-provided bit-field structure.
 */
CCS811_STATUS_t AirQuality::getStatusReg(CCS811_STATUS_REG_t *status)
{
  return readByte(CCS811_STATUS_REG, (uint8_t *)status);
}

/*
 * Read the measurement-mode byte into the caller-provided bit-field structure.
 */
CCS811_STATUS_t AirQuality::getMeasModeReg(CCS811_MEAS_MODE_REG_t *measMode)
{
  return readByte(CCS811_MEAS_MODE_REG, (uint8_t *)measMode);
}

/*
 * Read the sensor error flags into the caller-provided bit-field structure.
 */
CCS811_STATUS_t AirQuality::getErrorIdReg(CCS811_ERROR_ID_REG_t *errorId)
{
  return readByte(CCS811_ERROR_ID_REG, (uint8_t *)errorId);
}

/*
 * Return cached TVOC in ppb; call readAlgorithmResults() and check hasReading() first.
 */
uint16_t AirQuality::getTVOC(bool print)
{
  if(print)
  {
    Serial.print("TVOC: ");
    Serial.print(_tVOC);
    Serial.println("ppb");
  }
  return _tVOC;
}

/*
 * Return cached equivalent CO2 in ppm; this getter does not request a new measurement.
 */
uint16_t AirQuality::getCO2(bool print)
{
  if(print)
  {
    Serial.print("eCO2: ");
    Serial.print(_CO2);
    Serial.println("ppm");
  }
  return _CO2;
}

/*
 * Return the resistance in ohms cached by readNTC().
 */
float AirQuality::getResistance(void)
{
  return _resistance;
}

/*
 * Return the NTC temperature in Celsius cached by readNTC().
 */
float AirQuality::getTemperature(void)
{
  return _temperature;
}

/*
 * Read the hardware identification byte used during initialization.
 */
uint8_t AirQuality::getHwId(void)
{
  uint8_t HwId;
  CCS811_STATUS_t result = readByte(CCS811_HW_ID_REG,&HwId);
  if(result != SENSOR_SUCCESS)
  {
    return 0u;
  }
  return HwId;
}

/*
 * Poll data-ready status; no new data preserves the cache, but a transfer failure invalidates it.
 */
bool AirQuality::isDataAvailable(void)
{
 if(getStatusReg(&statusReg) != SENSOR_SUCCESS || statusReg.ERROR || !statusReg.FW_MODE)
 {
   _hasReading = false;
   return false;
 }
 return (bool)statusReg.DATA_READY;
}

/*
 * Return hardware version text in the shared object buffer, or NULL on transfer failure.
 */
char * AirQuality::getHwVersion(void)
{
  uint8_t data;
  memset(_versionInfo,0u,sizeof(_versionInfo));
  if(readByte(CCS811_HW_VER_REG,&data) != SENSOR_SUCCESS)
  {
    return NULL;
  }
  sprintf(_versionInfo, "%u.%u", (data>>4u), data&0x0Fu);
  return _versionInfo;
}

/*
 * Return boot-firmware version text; the next version query overwrites the shared buffer.
 */
char * AirQuality::getFwBootVersion(void)
{
  uint8_t data[2u];
  memset(_versionInfo,0u,sizeof(_versionInfo));
  if(readMultiBytes(CCS811_FW_BOOT_VER_REG,sizeof(data),data) != SENSOR_SUCCESS)
  {
    return NULL;
  }
  sprintf(_versionInfo, "%u.%u.%u", (data[0u]>>4u), data[0u]&0x0Fu, data[1u]);
  return _versionInfo;
}

/*
 * Return application-firmware version text; the next version query overwrites the shared buffer.
 */
char * AirQuality::getFwAppVersion(void)
{
  uint8_t data[2u];
  memset(_versionInfo,0u,sizeof(_versionInfo));
  if(readMultiBytes(CCS811_FW_APP_VER_REG,sizeof(data),data) != SENSOR_SUCCESS)
  {
    return NULL;
  }
  sprintf(_versionInfo, "%u.%u.%u", (data[0u]>>4u), data[0u]&0x0Fu, data[1u]);
  return _versionInfo;
}

/***********************************************************************************************
 * Platform dependent routines. Change these functions implementation based on microcontroller *
 ***********************************************************************************************/
/*
 * Initialize the shared Wire bus at 100 kHz; normal sketches configure Wire before using drivers.
 */
void AirQuality::i2c_init(void)
{
  Wire.begin();
  Wire.setClock(100000);
}

/*
 * Select one register and require a complete one-byte response.
 */
CCS811_STATUS_t AirQuality::readByte(uint8_t reg, uint8_t *in)
{
  Wire.beginTransmission((uint8_t)_i2cSlaveAddress);
  Wire.write(reg);
  if(Wire.endTransmission(true) != 0)
  {
   return SENSOR_I2C_ERROR;
  }
  Wire.requestFrom((uint8_t)_i2cSlaveAddress, (uint8_t)1u, (uint8_t)1u);
  if(Wire.available() != 1u)
  {
   return SENSOR_I2C_ERROR;
  }
  *in = Wire.read();
  return SENSOR_SUCCESS;
}

/*
 * Select the starting register and read the requested number of bytes.
 */
CCS811_STATUS_t AirQuality::readMultiBytes(uint8_t reg, uint8_t length, uint8_t *in)
{
  Wire.beginTransmission((uint8_t)_i2cSlaveAddress);
  Wire.write(reg);
  if(Wire.endTransmission(true) != 0)
  {
    return SENSOR_I2C_ERROR;
  }
  Wire.requestFrom((uint8_t)_i2cSlaveAddress, (uint8_t)length, (uint8_t)1u);
  if(Wire.available() != length)
  {
    return SENSOR_I2C_ERROR;
  }
  uint8_t nData;
  for(nData = 0u; nData < length; nData++)
  {
    in[nData] = Wire.read();
  }
  return SENSOR_SUCCESS;
}

/*
 * Send a register/command byte and report the transfer result.
 */
CCS811_STATUS_t AirQuality::writeByte(uint8_t reg)
{
  Wire.beginTransmission((uint8_t)_i2cSlaveAddress);
  Wire.write(reg);
  if (Wire.endTransmission(true) == 0)
  {
    return SENSOR_SUCCESS;
  }
  return SENSOR_I2C_ERROR;
}

/*
 * Send a register/command byte and its value and report the transfer result.
 */
CCS811_STATUS_t AirQuality::writeByte(uint8_t reg, uint8_t val)
{
  Wire.beginTransmission((uint8_t)_i2cSlaveAddress);
  Wire.write(reg);
  Wire.write(val);
  if (Wire.endTransmission(true) == 0)
  {
    return SENSOR_SUCCESS;
  }
  return SENSOR_I2C_ERROR;
}

/*
 * Write consecutive bytes starting at the selected register and report the transfer result.
 */
CCS811_STATUS_t AirQuality::writeMultiBytes(uint8_t reg, uint8_t length, const uint8_t *out)
{
  Wire.beginTransmission((uint8_t)_i2cSlaveAddress);
  Wire.write(reg);
  Wire.write(out,length);
  if (Wire.endTransmission(true) == 0)
  {
    return SENSOR_SUCCESS;
  }
  return SENSOR_I2C_ERROR;
}

/*
 * Probe the I2C address without sending register data.
 */
bool AirQuality::writeAddress(void)
{
  Wire.beginTransmission((uint8_t)_i2cSlaveAddress);
  if (Wire.endTransmission(true) == 0)
  {
    return true;
  }
  return false;
}

/*
 * Wait for the requested number of milliseconds using the Arduino platform delay.
 */
void AirQuality::delay_ms(uint16_t ms)
{
  delay(ms);
}
