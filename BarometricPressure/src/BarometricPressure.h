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

#ifndef _BAROMETRICPRESSURE_H_
#define _BAROMETRICPRESSURE_H_

#include <stdint.h>
#include <math.h>
#include <Arduino.h>
#include <Wire.h>

#define BMP180_SOFT_REST_VALUE    0xB6u   /**< write this value in SOFT_RESET_REG to soft reset the BMP180 */
#define BMP180_GET_TEMPERATURE    0x2Eu   /**< write this value in CONTROL_REG to get the temperature */
#define BMP180_GET_PRESSURE_OSS0  0x34u   /**< write this value in CONTROL_REG to get the pressure with oversampling 0 */
#define BMP180_GET_PRESSURE_OSS1  0x74u   /**< write this value in CONTROL_REG to get the pressure with oversampling 1 */
#define BMP180_GET_PRESSURE_OSS2  0xB4u   /**< write this value in CONTROL_REG to get the pressure with oversampling 2 */
#define BMP180_GET_PRESSURE_OSS3  0xF4u   /**< write this value in CONTROL_REG to get the pressure with oversampling 3 */
#define BMP180_I2C_ADDRESS        0x77u   /**< I2C slave address */
#define BMP180_CHIP_ID            0x55u   /**< BMP180 Chip ID */
#define BMP180_ERROR              255
#define BMP180_MAX_COEFF_REGS     11u     /* number of coefficient registers in BMP180 */

#define SEA_LEVEL_AVG_PRESSURE    1013.25   /* Average sea-level pressure is 1013.25 mbar */

/*!
* List of registers to control and configure the BMP180 sensor
*/
typedef enum
{
  AC1_REG           = 0xAAu,
  AC2_REG           = 0xACu,
  AC3_REG           = 0xAEu,
  AC4_REG           = 0xB0u,
  AC5_REG           = 0xB2u,
  AC6_REG           = 0xB4u,
  B1_REG            = 0xB6u,
  B2_REG            = 0xB8u,
  MB_REG            = 0xBAu,
  MC_REG            = 0xBCu,
  MD_REG            = 0xBEu,
  ADC_OUT_XLSB_REG  = 0xF8u,  /**< raw data XLSB */
  ADC_OUT_LSB_REG   = 0xF7u,  /**< raw data LSB */
  ADC_OUT_MSB_REG   = 0xF6u,  /**< raw data MSB */
  CONTROL_REG       = 0xF4u,  /**< Controls the measurement, conversion & oversampling */
  SOFT_RESET_REG    = 0xE0u,  /**< Resets the BMP180 */
  CHIP_ID_REG       = 0xD0u   /**< BMP180 Chip ID */
}bmp180Reg_t;

/*!
* different sampling accuracy modes in BMP180
*/
typedef enum
{
  ULTRA_LOW_POWER = 0x00u,    /**< OSS0, 4.5ms conversion time */
  STANDARD        = 0x01u,    /**< OSS1, 7.5ms conversion time */
  HIGH_RESOLUTION = 0x02u,    /**< OSS2, 13.5ms conversion time */
  ULTRA_HIGH_RESOLUTION = 0x03u /**< OSS3, 25.5ms conversion time */
}bmp180AccuracyMode_t;

/*!
* to store the calibration coefficients
*/
typedef struct
{
  int16_t _AC1;
  int16_t _AC2;
  int16_t _AC3;
  uint16_t _AC4;
  uint16_t _AC5;
  uint16_t _AC6;
  int16_t _B1;
  int16_t _B2;
  int16_t _MB;
  int16_t _MC;
  int16_t _MD;
}bmp180CalibCoeff_t;

/*
 * Initialize Wire, then begin() to load factory compensation data.
 * Raw getPressure() returns Pa; the legacy Pascal/Bar getters return kPa/millibar.
 * Altitude inputs use meters and reference pressure uses millibar; floating errors return NAN.
 */
class BarometricPressure
{
  public:
    BarometricPressure(bmp180AccuracyMode_t=ULTRA_LOW_POWER);
    bool begin(void);
    int32_t getPressure(void);
    // Legacy method name: returns kPa. Floating measurements return NAN on failure.
    float getPressurePascal(bool print=true);
    float getPressureHg(bool print=true);
    float getPressureBar(bool print=true);
    float getTempC(bool print=true);
    float getTempF(bool print=true);
    float getAltitude(float p0, bool print=true);
    float getSeaLevelPressure(float altitude, bool print=true);
    void reset(void);
    bool ping(void);
    uint8_t getDeviceId(void);
    void setAccuracyMode(bmp180AccuracyMode_t mode);
  private:
    uint8_t _i2cSlaveAddress;
    bool _isConnected;
    uint8_t  _accuracy;
    bmp180CalibCoeff_t _calibCoeff;
    float getTemperature(void);
    bool readCalibrationCoefficients(void);
    bool computeB5(int32_t UT, int32_t *result);
    bool readBytes(bmp180Reg_t reg, uint8_t *data, uint8_t length);
    int32_t readRawTemperature(void);
    int32_t readRawPressure(void);
    void i2c_init(void);
    uint8_t  read8bit(bmp180Reg_t reg);
    uint16_t read16bit(bmp180Reg_t reg);
    bool write8bit(bmp180Reg_t reg, uint8_t val);
    bool writeAddress(void);
    void delay_ms(uint16_t ms);
};

#endif
