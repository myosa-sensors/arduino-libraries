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

#ifndef __AIRQUALITY_H__
#define __AIRQUALITY_H__

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <Arduino.h>
#include <Wire.h>

#define CCS811_HW_ID                0x81u
#define CCS811_I2C_ADDRESS0         0x5Au
#define CCS811_I2C_ADDRESS1         0x5Bu

/* Defining constant used in the calculation. Depends on hardware. Don't change. */
const float refResitance = 10000.f;

#define CCS811_STATUS_REG           0x00u   /**< Status register */
#define CCS811_MEAS_MODE_REG        0x01u   /**< Measurement mode and conditions register */
#define CCS811_ALG_RESULT_DATA_REG  0x02u   /**< Algorithm result */
#define CCS811_RAW_DATA_REG         0x03u   /**< Raw ADC data values for resistance and current source used */
#define CCS811_ENV_DATA_REG         0x05u   /**< Temperature and Humidity data can be written to enable compensation */
#define CCS811_NTC_REG              0x06u   /**< Provides the voltage across the reference resistor and the voltage across the NTC resistor – from which the ambient temperature can be determined*/
#define CCS811_THRESHOLDS_REG       0x10u   /**< Thresholds for operation when interrupts are only generated when eCO2 ppm crosses a threshold */
#define CCS811_BASELINE_REG         0x11u   /**< The encoded current baseline value can be read. A previously saved encoded baseline can be written */
#define CCS811_HW_ID_REG            0x20u   /**< Hardware ID register. The value is 0x81 */
#define CCS811_HW_VER_REG           0x21u   /**< Hardware Version register */
#define CCS811_FW_BOOT_VER_REG      0x23u   /**< Firmware Boot Version. The first 2 bytes contain the firmware version number for the boot code */
#define CCS811_FW_APP_VER_REG       0x24u   /**< Firmware Application Version. The first 2 bytes contain the firmware version number for the application code */
#define CCS811_ERROR_ID_REG         0xE0u   /**< Error ID. When the status register reports an error its source is located in this register */
#define CCS811_SOFT_RESET_REG       0xFFu   /**< If f the correct 4 bytes (0x11 0xE5 0x72 0x8A) are written to this register in a single sequence the device will reset and return to BOOT mode.*/
#define CCS811_APP_START_REG        0xF4u   /**< */

#define CCS811_DRIVE_MODE_MSK       0x70u
#define CCS811_INT_DATARDY_MSK      0x08u
#define CCS811_INT_THRESH_MSK       0x04u
#define CCS811_DRIVE_MODE_POS       0x04u
#define CCS811_INT_DATARDY_POS      0x03u
#define CCS811_INT_THRESH_POS       0x02u

#define CCS811_DRIVE_MODE0          0x00u
#define CCS811_DRIVE_MODE1          0x01u
#define CCS811_DRIVE_MODE2          0x02u
#define CCS811_DRIVE_MODE3          0x03u
#define CCS811_DRIVE_MODE4          0x04u

/*!
 *  list of status codes to indicate sensor operation
 */
typedef enum {
  SENSOR_SUCCESS,
  SENSOR_ID_ERROR,
  SENSOR_I2C_ERROR,
  SENSOR_INTERNAL_ERROR,
  SENSOR_GENERIC_ERROR
}CCS811_STATUS_t;

/*!
 * Status register bit fields
 */
typedef struct
{
  uint8_t ERROR:1;
  uint8_t Reserved:2;
  uint8_t DATA_READY:1;
  uint8_t APP_VALID:1;
  uint8_t Reserved1:2;
  uint8_t FW_MODE:1;
}CCS811_STATUS_REG_t;

/*!
 * Measurement Mode register bit fields
 */
typedef struct
{
  uint8_t Reserved:2;
  uint8_t INT_THRESH:1;
  uint8_t INT_DATARDY:1;
  uint8_t DRIVE_MODE:3;
  uint8_t Reserved1:1;
}CCS811_MEAS_MODE_REG_t;

/*!
 * Error ID register bit fields
 */
typedef struct
{
  uint8_t WRITE_REG_INVALID:1;
  uint8_t READ_REG_INVALID:1;
  uint8_t MEASMODE_INVALID:1;
  uint8_t MAX_RESISTANCE:1;
  uint8_t HEATER_FAULT:1;
  uint8_t HEATER_SUPPLY:1;
  uint8_t Reserved:2;
}CCS811_ERROR_ID_REG_t;

/*
 * Call begin() after Wire setup; poll isDataAvailable() before readAlgorithmResults().
 * CO2/TVOC getters return cached ppm/ppb values; check hasReading() before using the cache.
 * NTC getters use the separate readNTC() cache; reference resistance is specified in ohms.
 */
class AirQuality
{
  public:
    AirQuality(uint8_t=CCS811_I2C_ADDRESS1, float=10000);
    CCS811_STATUS_t begin(void);
    CCS811_STATUS_t reset(void);
    bool ping(void);
    CCS811_STATUS_t readAlgorithmResults(void);
    uint16_t getTVOC(bool print=true);
    uint16_t getCO2(bool print=true);
    float getResistance(void);
    float getTemperature(void);
    uint8_t getHwId(void);
    uint16_t getBaseLine(void);
    void setRefResistance(float refRes);
    bool isDataAvailable(void);
    bool hasReading(void) const { return _hasReading; }
    char *getHwVersion(void);
    char *getFwAppVersion(void);
    char *getFwBootVersion(void);
    CCS811_STATUS_t readNTC(void);
    CCS811_STATUS_t setEnvironmentalData(float relativeHumidity , float ambientTemperature);
    CCS811_STATUS_t setDriveMode(uint8_t mode);
    CCS811_STATUS_t enableDataInterrupt(void);
    CCS811_STATUS_t disableDataInterrupt(void);
    CCS811_STATUS_t enableThreshInterrupt(void);
    CCS811_STATUS_t disableThreshInterrupt(void);
    CCS811_STATUS_t getStatusReg(CCS811_STATUS_REG_t *status);
    CCS811_STATUS_t getMeasModeReg(CCS811_MEAS_MODE_REG_t *measMode);
    CCS811_STATUS_t getErrorIdReg(CCS811_ERROR_ID_REG_t *errorId);
    CCS811_STATUS_t setBaseLine(uint16_t baseLine);
  private:
    uint16_t _tVOC = {};
    uint16_t _CO2 = {};
    uint16_t _vREF = {};
    uint16_t _vNTC = {};
    float _refResistance;
    float _resistance = {};
    float _temperature = {};
    CCS811_ERROR_ID_REG_t errorIdReg = {};
    CCS811_MEAS_MODE_REG_t measModeReg = {};
    CCS811_STATUS_REG_t statusReg = {};
    uint8_t _i2cSlaveAddress;
    bool _isConnected;
    bool _hasReading = false;
    char _versionInfo[10u];
    void i2c_init(void);
    CCS811_STATUS_t readByte(uint8_t reg, uint8_t *in);
    CCS811_STATUS_t readMultiBytes(uint8_t reg, uint8_t length, uint8_t *in);
    CCS811_STATUS_t writeByte(uint8_t reg);
    CCS811_STATUS_t writeByte(uint8_t reg, uint8_t val);
    CCS811_STATUS_t writeMultiBytes(uint8_t reg, uint8_t length, const uint8_t *out);
    bool writeAddress(void);
    void delay_ms(uint16_t ms);
};

#endif
