/*
  This code is developed under the MYOSA (LearnTheEasyWay) initiative of MakeSense EduTech and Pegasus Automation.
  Code has been derived from internet sources and component datasheets.
  Existing readily-available libraries would have been used "AS IS" and modified for ease of learning purpose.
 
  Synopsis of Barometric Pressure Board
  The MYOSA pressure board uses the BMP180 sensor at I2C address 0x77.
  Factory coefficients compensate the temperature and pressure measurements.
  Pressure is available in kPa, mmHg and mbar, with altitude estimates in metres.
  Floating measurements return NAN if a transaction or compensation calculation fails.

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

#include "BarometricPressure.h"

/**
 *   @brief constructor to initialise the BMP180 sampling accuracy mode
 */
BarometricPressure::BarometricPressure(bmp180AccuracyMode_t accr){
  _accuracy = (unsigned)accr <= 3 ? accr : ULTRA_LOW_POWER;
  _calibCoeff = {};
  _i2cSlaveAddress = BMP180_I2C_ADDRESS;
  _isConnected = false;
}

/*
 * Select the pressure oversampling code; higher modes require a longer conversion wait.
 */
void BarometricPressure::setAccuracyMode(bmp180AccuracyMode_t mode){
  if((unsigned)mode <= 3) _accuracy = mode;
}

/*
 * Verify the BMP180 identity and load its factory compensation coefficients.
 */
bool BarometricPressure::begin(void){
  _isConnected = false;
  if(getDeviceId() != BMP180_CHIP_ID) return false;
  _isConnected = readCalibrationCoefficients();
  return _isConnected;
}

/*
 * Probe the device and reload initialization data after a lost connection.
 */
bool BarometricPressure::ping(void)
{
  if(!writeAddress()) { _isConnected = false; return false; }
  if(!_isConnected) _isConnected = begin();
  return _isConnected;
}

/*
 * Apply factory temperature compensation to a fresh raw reading; return NAN on failure.
 */
float BarometricPressure::getTemperature(void){
  const int32_t raw = readRawTemperature();
  int32_t b5;
  if(raw < 0 || !computeB5(raw, &b5)) return NAN;
  return ((b5 + 8) >> 4) / 10.f;
}

/*
 * Return a fresh compensated temperature in Celsius, optionally printing it.
 */
float BarometricPressure::getTempC(bool print)
{
  float temperature = getTemperature();
  if(temperature == BMP180_ERROR)
  {
    temperature = NAN;
  }
  if(print)
  {
    Serial.print("Temperature (°C): ");
    Serial.print(temperature,2);
    Serial.println("°C");
  }
  return temperature;
}

/*
 * Convert a fresh compensated temperature to Fahrenheit; preserve NAN on failure.
 */
float BarometricPressure::getTempF(bool print)
{
  float temperature = (getTemperature() * (9.f/5.f)) + 32.f;
  if(temperature == BMP180_ERROR)
  {
    temperature = NAN;
  }
  if(print)
  {
    Serial.print("Temperature (°F): ");
    Serial.print(temperature,2);
    Serial.println("°F");
  }
  return temperature;
}

/*
 * Return compensated pressure in Pa, or BMP180_ERROR if conversion or compensation fails.
 */
int32_t BarometricPressure::getPressure(void)
 {
   int32_t  UT       = 0;
   int32_t  UP       = 0;
   int32_t  B3       = 0;
   int32_t  B5       = 0;
   int32_t  B6       = 0;
   int32_t  X1       = 0;
   int32_t  X2       = 0;
   int32_t  X3       = 0;
   int32_t  pressure = 0;
   uint32_t B4       = 0;
   uint64_t B7       = 0;

   UT = readRawTemperature();                           //read uncompensated temperature, 16-bit
   if (UT < 0) return BMP180_ERROR;         //error handler, collision on i2c bus

   UP = readRawPressure();                              //read uncompensated pressure, 19-bit
   if (UP < 0) return BMP180_ERROR;         //error handler, collision on i2c bus

   if(!computeB5(UT, &B5)) return BMP180_ERROR;

   /* pressure calculation */
   B6 = B5 - 4000;
   X1 = ((int32_t)_calibCoeff._B2 * (((int64_t)B6 * B6) >> 12)) >> 11;
   X2 = ((int32_t)_calibCoeff._AC2 * B6) >> 11;
   X3 = X1 + X2;
   B3 = ((((int64_t)_calibCoeff._AC1 * 4 + X3) * (1u << _accuracy)) + 2) / 4;

   X1 = ((int32_t)_calibCoeff._AC3 * B6) >> 13;
   X2 = ((int32_t)_calibCoeff._B1 * (((int64_t)B6 * B6) >> 12)) >> 16;
   X3 = ((X1 + X2) + 2) >> 2;
   B4 = ((uint64_t)_calibCoeff._AC4 * (X3 + 32768L)) >> 15;
   if(UP < B3 || X3 < -32768) return BMP180_ERROR;
   B7 = (uint64_t)(UP - B3) * (50000UL >> _accuracy);

   if (B4 == 0) return BMP180_ERROR;                                     //safety check, avoiding division by zero

   if   (B7 < 0x80000000) pressure = (B7 * 2) / B4;
   else                   pressure = (B7 / B4) * 2;

   X1 = (pressure >> 8) * (pressure >> 8);
   X1 = ((int64_t)X1 * 3038L) >> 16;
   X2 = (-7357LL * pressure) >> 16;

   return pressure = pressure + ((X1 + X2 + 3791L) >> 4);
 }

/*
 * Return pressure in kPa despite the legacy method name; failures return NAN.
 */
float BarometricPressure::getPressurePascal(bool print)
{
  float pressurePascal = getPressure();
  if(pressurePascal == BMP180_ERROR)
  {
    pressurePascal = NAN;
  }
  if(print)
  {
    Serial.print("Pressure (kilo-pascal): ");
    Serial.print((pressurePascal/1000.f),2);
    Serial.println("kilo-pascal");
  }
  return pressurePascal/1000.f;
}

/*
 * Convert compensated pressure to mmHg; failures return NAN.
 */
float BarometricPressure::getPressureHg(bool print)
{
  float pressurePascal = getPressure();
  if(pressurePascal == BMP180_ERROR)
  {
    pressurePascal = NAN;
  }
  if(print)
  {
    Serial.print("Pressure (mmHg): ");
    Serial.print((pressurePascal/133.322368f),2);
    Serial.println("mmHg");
  }
  return pressurePascal/133.322368f;
}

/*
 * Return compensated pressure in millibar (hPa); failures return NAN.
 */
float BarometricPressure::getPressureBar(bool print)
{
  float pressurePascal = getPressure();
  if(pressurePascal == BMP180_ERROR)
  {
    pressurePascal = NAN;
  }
  if(print)
  {
    Serial.print("Pressure (mbar): ");
    Serial.print((pressurePascal/100.f),2);
    Serial.println("mbar");
  }
  return pressurePascal/100.f;
}

/*
 * Estimate sea-level pressure in millibar using the supplied altitude in meters.
 */
float BarometricPressure::getSeaLevelPressure(float altitude, bool print)
{
    if(!isfinite(altitude) || altitude >= 44330.f) return NAN;
    float slp;
    float pressure = getPressureBar(false);
    slp = pressure / pow(1.0 - altitude/44330.0 , 5.255);
    if(print)
    {
        Serial.print("Sea Level Pressure: ");
        Serial.print(slp,2);
        Serial.println("mbar");
    }
    return slp;
}

/*
 * Estimate altitude in meters; p0 is the positive sea-level reference pressure in millibar.
 */
float BarometricPressure::getAltitude(float p0, bool print)
{
    if(!isfinite(p0) || p0 <= 0) return NAN;
    float altitude;
    float pressure = getPressureBar(false);
    altitude = 44330.0 * (1.0 - pow((pressure/p0),(1.0/5.255)));
    if(print)
    {
        Serial.print("Altitude: ");
        Serial.print(altitude,2);
        Serial.println("meters");
    }
    return altitude;
}

/*
 * Compute the shared temperature-compensation term, rejecting invalid divisors and overflow.
 */
bool BarometricPressure::computeB5(int32_t UT, int32_t *result)
{
  const int32_t x1 = (((int64_t)UT - _calibCoeff._AC6) * _calibCoeff._AC5) >> 15;
  const int32_t divisor = x1 + _calibCoeff._MD;
  if(divisor == 0) return false;
  *result = x1 + ((int32_t)_calibCoeff._MC * 2048) / divisor;
  return true;
}

/*
 * Request a software reset and clear connection state before the recovery delay.
 */
void BarometricPressure::reset(void){
  _isConnected = false;
  write8bit(SOFT_RESET_REG, BMP180_SOFT_REST_VALUE);
  delay_ms(5);
}

/*
 * Return the expected chip ID when verified, otherwise BMP180_ERROR.
 */
uint8_t BarometricPressure::getDeviceId(void){
   if (read8bit(CHIP_ID_REG) == BMP180_CHIP_ID)
   {
      return BMP180_CHIP_ID;
   }
   else
   {
     return BMP180_ERROR;
   }
 }

/*
 * Decode all factory coefficients from one transfer; commit only a valid complete set.
 */
bool BarometricPressure::readCalibrationCoefficients(void)
{
  uint8_t data[22];
  if(!readBytes(AC1_REG, data, sizeof(data))) return false;
  bmp180CalibCoeff_t next = {};
  next._AC1 = (int16_t)(((uint16_t)data[0] << 8) | data[1]);
  next._AC2 = (int16_t)(((uint16_t)data[2] << 8) | data[3]);
  next._AC3 = (int16_t)(((uint16_t)data[4] << 8) | data[5]);
  next._AC4 = (uint16_t)(((uint16_t)data[6] << 8) | data[7]);
  next._AC5 = (uint16_t)(((uint16_t)data[8] << 8) | data[9]);
  next._AC6 = (uint16_t)(((uint16_t)data[10] << 8) | data[11]);
  next._B1 = (int16_t)(((uint16_t)data[12] << 8) | data[13]);
  next._B2 = (int16_t)(((uint16_t)data[14] << 8) | data[15]);
  next._MB = (int16_t)(((uint16_t)data[16] << 8) | data[17]);
  next._MC = (int16_t)(((uint16_t)data[18] << 8) | data[19]);
  next._MD = (int16_t)(((uint16_t)data[20] << 8) | data[21]);
  if(next._AC4 == 0 || next._AC4 == 0xFFFF || next._AC5 == 0 || next._AC5 == 0xFFFF) return false;
  _calibCoeff = next;
  return true;
}

/*
 * Start a temperature conversion and wait for its result; return -1 on transfer failure.
 */
int32_t BarometricPressure::readRawTemperature(void)
{
  uint8_t data[2];
  if(!write8bit(CONTROL_REG, BMP180_GET_TEMPERATURE)) return -1;
  delay_ms(5);
  if(!readBytes(ADC_OUT_MSB_REG, data, sizeof(data))) return -1;
  return ((uint16_t)data[0] << 8) | data[1];
}

/*
 * Wait for the selected oversampling conversion and assemble the raw ADC value; -1 is failure.
 */
int32_t BarometricPressure::readRawPressure(void)
{
  static const uint8_t delays[] = {5, 8, 14, 26};
  uint8_t data[3];
  if(_accuracy > 3 || !write8bit(CONTROL_REG, BMP180_GET_PRESSURE_OSS0 | (_accuracy << 6))) return -1;
  delay_ms(delays[_accuracy]);
  if(!readBytes(ADC_OUT_MSB_REG, data, sizeof(data))) return -1;
  return (((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2]) >> (8 - _accuracy);
}

/***********************************************************************************************
 * Platform dependent routines. Change these functions implementation based on microcontroller *
 ***********************************************************************************************/
/*
 * Initialize the shared Wire bus at 100 kHz; normal sketches configure Wire before using drivers.
 */
void BarometricPressure::i2c_init(void)
{
  Wire.begin();
  Wire.setClock(100000);
}

/*
 * Read an 8-bit register; a failed transfer returns the BMP180 error sentinel.
 */
uint8_t BarometricPressure::read8bit(bmp180Reg_t reg)
{
  Wire.beginTransmission(_i2cSlaveAddress);
  Wire.write(reg);
  if(Wire.endTransmission(true) != 0)
  {
    return BMP180_ERROR;
  }
  Wire.requestFrom(_i2cSlaveAddress, (uint8_t)1u, (uint8_t)1u);
  if(Wire.available() != 1u)
  {
    return BMP180_ERROR;
  }
  return Wire.read();
}

/*
 * Read a big-endian register word; callers must account for the legacy error sentinel.
 */
uint16_t BarometricPressure::read16bit(bmp180Reg_t reg)
{
  uint16_t value=0u;
  Wire.beginTransmission(_i2cSlaveAddress);
  Wire.write(reg);
  if(Wire.endTransmission(true) != 0)
  {
    return BMP180_ERROR;
  }
  Wire.requestFrom(_i2cSlaveAddress, (uint8_t)2u, (uint8_t)1u);
  if(Wire.available() != 2u)
  {
    return BMP180_ERROR;
  }
  value  = Wire.read() << 8;
  value |= Wire.read();
  return value;
}

/*
 * Write one register byte and report whether the device acknowledged the transfer.
 */
bool BarometricPressure::write8bit(bmp180Reg_t reg, uint8_t val)
{
  Wire.beginTransmission(_i2cSlaveAddress);
  Wire.write(reg);
  Wire.write(val);
  if (Wire.endTransmission(true) == 0)
  {
    return true;
  }
  return false;
}

/*
 * Probe the I2C address without sending register data.
 */
bool BarometricPressure::writeAddress(void)
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
void BarometricPressure::delay_ms(uint16_t ms)
{
  delay(ms);
}

// Keep transaction status separate from a valid register value of 0x00FF.
bool BarometricPressure::readBytes(bmp180Reg_t reg, uint8_t *data, uint8_t length)
{
  Wire.beginTransmission(_i2cSlaveAddress);
  Wire.write(reg);
  if(Wire.endTransmission(true) != 0) return false;
  if(Wire.requestFrom(_i2cSlaveAddress, length, (uint8_t)1) != length) return false;
  for(uint8_t i = 0; i < length; ++i) data[i] = Wire.read();
  return true;
}
