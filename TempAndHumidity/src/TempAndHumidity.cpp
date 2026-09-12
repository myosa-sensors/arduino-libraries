/*
  This code is developed under the MYOSA (LearnTheEasyWay) initiative of MakeSense EduTech and Pegasus Automation.
  Code has been derived from internet sources and component datasheets.
  Existing readily-available libraries would have been used "AS IS" and modified for ease of learning purpose.

  Synopsis of Temperature And Humidity Board
  The MYOSA temperature and humidity board uses the Si7021 at I2C address 0x40.
  Temperature is available in Celsius and Fahrenheit; relative humidity is a percentage.
  Measurements and serial-number bytes are checked using the sensor CRC.
  Failed floating measurements return NAN.

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

#include "TempAndHumidity.h"

// Si7021 CRC-8: polynomial x^8 + x^5 + x^4 + 1, initial value zero.
static uint8_t si7021Crc(const uint8_t *data, uint8_t size) {
  uint8_t crc = 0;
  while(size--) {
    crc ^= *data++;
    for(uint8_t bit = 0; bit < 8; ++bit) crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : crc << 1;
  }
  return crc;
}

/*
 * Initialize the device address and local state before any measurement is requested.
 */
TempAndHumidity::TempAndHumidity()
{
  _i2cSlaveAddress = Si7021_I2C_ADDRESS;
  _isConnected = false;
}

/*
 * Reset the Si7021 and wait for it to become ready; return false if reset is not acknowledged.
 */
bool TempAndHumidity::begin(void)
{
  _isConnected = false;
  if(reset())
  {
    delay_ms(Si7021_SOFT_RESET_DELAY);
    _isConnected = true;
    return true;
  }
  else
  {
    return false;
  }
}

/*
 * Send the Si7021 software-reset command; the caller supplies the recovery delay.
 */
bool TempAndHumidity::reset(void)
{
  return writeByte(Si7021_RESET);
}

/*
 * Probe the device and initialize it again after a lost connection.
 */
bool TempAndHumidity::ping(void)
{
  if(!writeAddress()) { _isConnected = false; return false; }
  if(!_isConnected) _isConnected = begin();
  return _isConnected;
}

/*
 * Read a fresh CRC-checked humidity sample and clamp it to 0-100%; failures return NAN.
 */
float TempAndHumidity::getRelativeHumdity(bool print)
{
  uint8_t data[3];
  if(!writeByte(Si7021_MEAS_RH_NOHOLD_MODE)) return NAN;
  delay_ms(25);
  if(!readMultiBytes(sizeof(data), data) || si7021Crc(data, 2) != data[2]) return NAN;
  const uint16_t raw = (((uint16_t)data[0] << 8) | data[1]) & 0xFFFCu;
  float value = 125.f * raw / 65536.f - 6.f;
  value = fminf(100.f, fmaxf(0.f, value));
  if(print) { Serial.print("Relative Humidity (%): "); Serial.println(value, 2); }
  return value;
}

/*
 * Read a fresh CRC-checked temperature in Celsius; conversion or CRC failure returns NAN.
 */
float TempAndHumidity::getTempC(bool print)
{
  uint8_t data[3];
  if(!writeByte(Si7021_MEAS_TEMP_NOHOLD_MODE)) return NAN;
  delay_ms(25);
  if(!readMultiBytes(sizeof(data), data) || si7021Crc(data, 2) != data[2]) return NAN;
  const uint16_t raw = (((uint16_t)data[0] << 8) | data[1]) & 0xFFFCu;
  float value = 175.72f * raw / 65536.f - 46.85f;
  if(print) { Serial.print("Temperature (C): "); Serial.println(value, 2); }
  return value;
}

/*
 * Convert a fresh temperature reading to Fahrenheit; preserve NAN on failure.
 */
float TempAndHumidity::getTempF(bool print)
{
  float temperature = (getTempC(false) * (9.f / 5.f)) + 32.f;
  if(print)
  {
    Serial.print("Temperature (°F): ");
    Serial.print(temperature,2);
    Serial.println("°F");
  }
  return temperature;
}

/*
 * Apply the Celsius heat-index polynomial to fresh temperature and humidity readings.
 */
float TempAndHumidity::getHeatIndexC(bool print)
 {
   float T = getTempC(false);
   float RH = getRelativeHumdity(false);
   float HI = 0.f;

    float c1 = -8.78469475556;
    float c2 = 1.61139411;
    float c3 = 2.33854883889;
    float c4 = -0.14611605;
    float c5 = -0.012308094;
    float c6 = -0.0164248277778;
    float c7 = 0.002211732;
    float c8 = 0.00072546;
    float c9 = -0.000003582;
   HI = c1 + (c2*T) + (c3*RH) + (c4*T*RH) + (c5*T*T) + (c6*RH*RH) + (c7*T*T*RH) + (c8*T*RH*RH) + (c9*T*T*RH*RH);
    if(print)
    {
      Serial.print("Heat Index (°C): ");
      Serial.print(HI,2);
      Serial.println("°C");
    }
   return HI;
 }


/*
 * Apply the heat-index polynomial to fresh Fahrenheit temperature and relative humidity.
 */
float TempAndHumidity::getHeatIndexF(bool print)
{
  float T = getTempF(false);
  float RH = getRelativeHumdity(false);
  float HI = 0.f;
  HI = -42.379f + (2.04901523f*T) + (10.14333127f*RH) - (0.22475541f*T*RH) -
       (0.00683783f*T*T) - (0.05481717f*RH*RH) + (0.00122874f*T*T*RH) +
       (0.00085282f*T*RH*RH) - (0.00000199f*T*T*RH*RH);
   if(print)
   {
     Serial.print("Heat Index (°F): ");
     Serial.print(HI,2);
     Serial.println("°F");
   }
  return HI;
}

/*
 * Validate both serial-number blocks with CRC; return zero on failure and print the valid ID.
 */
uint64_t TempAndHumidity::getSerialNumber(void)
{
  uint8_t first[8], second[6];
  if(!writeByte(Si7021_ID1_CMD0, Si7021_ID1_CMD1) || !readMultiBytes(sizeof(first), first)) return 0;
  for(uint8_t i = 0; i < 8; i += 2) if(si7021Crc(first + i, 1) != first[i + 1]) return 0;
  if(!writeByte(Si7021_ID2_CMD0, Si7021_ID2_CMD1) || !readMultiBytes(sizeof(second), second)) return 0;
  if(si7021Crc(second, 2) != second[2] || si7021Crc(second + 3, 2) != second[5]) return 0;
  uint64_t serialNumber = 0;
  for(uint8_t i = 0; i < 8; i += 2) serialNumber = (serialNumber << 8) | first[i];
  const uint8_t positions[] = {0, 1, 3, 4};
  for(uint8_t i : positions) serialNumber = (serialNumber << 8) | second[i];
  Serial.print("Temperature and Humidity Sensor Serial Number: 0x");
  Serial.print((uint32_t)(serialNumber >> 32), HEX);
  Serial.println((uint32_t)serialNumber, HEX);
  return serialNumber;
}

/*
 * Return and print a static version string; unknown IDs or failed reads produce Unknown.
 */
char *TempAndHumidity::getFirmwareVersion(void)
{
  uint8_t data;
  static char version[8];
  strcpy(version, "Unknown");
  if(writeByte(Si7021_FIMWARE_REV_CMD0,Si7021_FIMWARE_REV_CMD1))
  {
    if(readMultiBytes(sizeof(data),&data))
    {
      if(data == 0xFFu)
      {
        strcpy(version, "1.0");
      }
      if(data == 0x20u)
      {
        strcpy(version, "2.0");
      }
    }
  }
  Serial.print("Si7021 Chip Firmware Revision: ");
  Serial.println(version);
  return version;
}

/***********************************************************************************************
 * Platform dependent routines. Change these functions implementation based on microcontroller *
 ***********************************************************************************************/
/*
 * Initialize the shared Wire bus at 100 kHz; normal sketches configure Wire before using drivers.
 */
void TempAndHumidity::i2c_init(void)
{
  Wire.begin();
  Wire.setClock(100000);
}

/*
 * Select one register and require a complete one-byte response.
 */
bool TempAndHumidity::readByte(uint8_t reg, uint8_t *in)
{
  Wire.beginTransmission((uint8_t)_i2cSlaveAddress);
  Wire.write(reg);
  if(Wire.endTransmission(true) != 0)
  {
    return false;
  }
  Wire.requestFrom((uint8_t)_i2cSlaveAddress, (uint8_t)1u, (uint8_t)1u);
  if(Wire.available() != 1u)
  {
    return false;
  }
  *in = Wire.read();
  return true;
}

/*
 * Select the starting register and read the requested number of bytes.
 */
bool TempAndHumidity::readMultiBytes(uint8_t reg, uint8_t length, uint8_t *in)
{
  Wire.beginTransmission((uint8_t)_i2cSlaveAddress);
  Wire.write(reg);
  if(Wire.endTransmission(true) != 0)
  {
    return false;
  }
  Wire.requestFrom((uint8_t)_i2cSlaveAddress, (uint8_t)length, (uint8_t)1u);
  if(Wire.available() != length)
  {
    return false;
  }
  uint8_t nData;
  for(nData = 0u; nData < length; nData++)
  {
    in[nData] = Wire.read();
  }
  return true;
}

/*
 * Read from the current device pointer and require the requested number of bytes.
 */
bool TempAndHumidity::readMultiBytes(uint8_t length, uint8_t *in)
{
  Wire.requestFrom((uint8_t)_i2cSlaveAddress, (uint8_t)length, (uint8_t)1u);
  if(Wire.available() != length)
  {
    return false;
  }
  uint8_t nData;
  for(nData = 0u; nData < length; nData++)
  {
    in[nData] = Wire.read();
  }
  return true;
}

/*
 * Send a register/command byte and report the transfer result.
 */
bool TempAndHumidity::writeByte(uint8_t reg)
{
  Wire.beginTransmission((uint8_t)_i2cSlaveAddress);
  Wire.write(reg);
  if (Wire.endTransmission(true) == 0)
  {
    return true;
  }
  return false;
}

/*
 * Send a register/command byte and its value and report the transfer result.
 */
bool TempAndHumidity::writeByte(uint8_t reg, uint8_t val)
{
  Wire.beginTransmission((uint8_t)_i2cSlaveAddress);
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
bool TempAndHumidity::writeAddress(void)
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
void TempAndHumidity::delay_ms(uint16_t ms)
{
  delay(ms);
}
