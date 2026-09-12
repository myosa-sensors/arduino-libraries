/*
  This code is developed under the MYOSA (LearnTheEasyWay) initiative of MakeSense EduTech and Pegasus Automation.
  Code has been derived from internet sources and component datasheets.
  Existing readily-available libraries would have been used "AS IS" and modified for ease of learning purpose.
  
  Synopsis of Accelerometer and Gyroscope
  The MYOSA motion board uses the MPU6050 six-axis sensor at I2C address 0x69.
  Acceleration is reported in cm/s^2, angular velocity in degrees/s, and tilt in degrees.
  Temperature is available in Celsius and Fahrenheit; failed measurements return NAN.
  Optional calibration requires a stationary, level board with +Z pointing upward.

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

#ifndef __ACCELANDGYRO_H__
#define __ACCELANDGYRO_H__

#include <stdint.h>
#include <math.h>
#include <Arduino.h>
#include <Wire.h>

#define MPU6050_ADDRESS_AD0_LOW             0x68u
#define MPU6050_ADDRESS_AD0_HIGH            0x69u

#define MPU6050_XA_OFFS_USRH_REG            0x06u
#define MPU6050_XA_OFFS_USRL_REG            0x07u
#define MPU6050_YA_OFFS_USRH_REG            0x08u
#define MPU6050_YA_OFFS_USRL_REG            0x09u
#define MPU6050_ZA_OFFS_USRH_REG            0x0Au
#define MPU6050_ZA_OFFS_USRL_REG            0x0Bu
#define MPU6050_SELF_TEST_X_REG             0x0Du
#define MPU6050_SELF_TEST_Y_REG             0x0Eu
#define MPU6050_SELF_TEST_Z_REG             0x0Fu
#define MPU6050_SELF_TEST_A_REG             0x10u
#define MPU6050_XG_OFFS_USRH_REG            0x13u
#define MPU6050_XG_OFFS_USRL_REG            0x14u
#define MPU6050_YG_OFFS_USRH_REG            0x15u
#define MPU6050_YG_OFFS_USRL_REG            0x16u
#define MPU6050_ZG_OFFS_USRH_REG            0x17u
#define MPU6050_ZG_OFFS_USRL_REG            0x18u
#define MPU6050_SMPLRT_DIV_REG              0x19u
#define MPU6050_CONFIG_REG                  0x1Au
#define MPU6050_GYRO_CONFIG_REG             0x1Bu
#define MPU6050_ACCEL_CONFIG_REG            0x1Cu
#define MPU6050_MOTION_THR                  0x1Fu
#define MPU6050_MOTION_DUR                  0x20u
#define MPU6050_ZERO_MOTION_THR             0x21u
#define MPU6050_ZERO_MOTION_DUR             0x22u
#define MPU6050_INT_ENABLE                  0x38u
#define MPU6050_INT_STATUS                  0x3Au
#define MPU6050_ACCEL_XOUT_H_REG            0x3Bu
#define MPU6050_ACCEL_XOUT_L_REG            0x3Cu
#define MPU6050_ACCEL_YOUT_H_REG            0x3Du
#define MPU6050_ACCEL_YOUT_L_REG            0x3Eu
#define MPU6050_ACCEL_ZOUT_H_REG            0x3Fu
#define MPU6050_ACCEL_ZOUT_L_REG            0x40u
#define MPU6050_TEMP_OUT_H_REG              0x41u
#define MPU6050_TEMP_OUT_L_REG              0x42u
#define MPU6050_GYRO_XOUT_H_REG             0x43u
#define MPU6050_GYRO_XOUT_L_REG             0x44u
#define MPU6050_GYRO_YOUT_H_REG             0x45u
#define MPU6050_GYRO_YOUT_L_REG             0x46u
#define MPU6050_GYRO_ZOUT_H_REG             0x47u
#define MPU6050_GYRO_ZOUT_L_REG             0x48u
#define MPU6050_SIGNAL_PATH_RESET_REG       0x68u
#define MPU6050_USER_CTRL_REG               0x6Au
#define MPU6050_PWR_MGMT_1_REG              0x6Bu
#define MPU6050_PWR_MGMT_2_REG              0x6Cu
#define MPU6050_FIFO_COUNTH_REG             0x72u
#define MPU6050_FIFO_COUNTL_REG             0x73u
#define MPU6050_FIFO_R_W_REG                0x74u
#define MPU6050_WHO_AM_I_REG                0x75u

#define MPU_CONFIG_DLPF_CFG_MSK             0x07u
#define MPU_CONFIG_DLPF_CFG_POS             0x00u
#define MPU_CONFIG_EXT_SYNC_SET_MSK         0x38u
#define MPU_CONFIG_EXT_SYNC_SET_POS         0x03u

#define MPU_GYRO_CONFIG_XG_ST_MASK          0x80u
#define MPU_GYRO_CONFIG_XG_ST_POS           0x07u
#define MPU_GYRO_CONFIG_YG_ST_MASK          0x40u
#define MPU_GYRO_CONFIG_YG_ST_POS           0x06u
#define MPU_GYRO_CONFIG_ZG_ST_MASK          0x20u
#define MPU_GYRO_CONFIG_ZG_ST_POS           0x05u
#define MPU_GYRO_CONFIG_FS_SEL_MASK         0x18u
#define MPU_GYRO_CONFIG_FS_SEL_POS          0x03u

#define MPU_GYRO_CONFIG_FS_SEL_250          0x00u
#define MPU_GYRO_CONFIG_FS_SEL_500          0x01u
#define MPU_GYRO_CONFIG_FS_SEL_1000         0x02u
#define MPU_GYRO_CONFIG_FS_SEL_2000         0x03u

#define MPU_ACCEL_CONFIG_XG_ST_MASK         0x80u
#define MPU_ACCEL_CONFIG_XG_ST_POS          0x07u
#define MPU_ACCEL_CONFIG_YG_ST_MASK         0x40u
#define MPU_ACCEL_CONFIG_YG_ST_POS          0x06u
#define MPU_ACCEL_CONFIG_ZG_ST_MASK         0x20u
#define MPU_ACCEL_CONFIG_ZG_ST_POS          0x05u
#define MPU_ACCEL_CONFIG_FS_SEL_MASK        0x18u
#define MPU_ACCEL_CONFIG_FS_SEL_POS         0x03u

#define MPU_ACCEL_CONFIG_FS_SEL_2g          0x00u
#define MPU_ACCEL_CONFIG_FS_SEL_4g          0x01u
#define MPU_ACCEL_CONFIG_FS_SEL_8g          0x02u
#define MPU_ACCEL_CONFIG_FS_SEL_16g         0x03u

#define MPU_SIGNAL_PATH_GYRO_RESET_MSK      0x04u
#define MPU_SIGNAL_PATH_GYRO_RESET_POS      0x02u
#define MPU_SIGNAL_PATH_ACCEL_RESET_MSK     0x02u
#define MPU_SIGNAL_PATH_ACCEL_RESET_POS     0x01u
#define MPU_SIGNAL_PATH_TEMP_RESET_MSK      0x01u
#define MPU_SIGNAL_PATH_TEMP_RESET_POS      0x00u

#define MPU_PWR_MGMT_1_DEVICE_RESET_MSK     0x80u
#define MPU_PWR_MGMT_1_DEVICE_RESET_POS     0x07u
#define MPU_PWR_MGMT_1_SLEEP_MSK            0x40u
#define MPU_PWR_MGMT_1_SLEEP_POS            0x06u
#define MPU_PWR_MGMT_1_CYCLE_MSK            0x20u
#define MPU_PWR_MGMT_1_CYCLE_POS            0x05u
#define MPU_PWR_MGMT_1_TEMP_DIS_MSK         0x08u
#define MPU_PWR_MGMT_1_TEMP_DIS_POS         0x03u
#define MPU_PWR_MGMT_1_CLKSEL_MSK           0x07u
#define MPU_PWR_MGMT_1_CLKSEL_POS           0x00u

#define MPU6050_CLOCK_INTERNAL              0x00u
#define MPU6050_CLOCK_PLL_XGYRO             0x01u
#define MPU6050_CLOCK_PLL_YGYRO             0x02u
#define MPU6050_CLOCK_PLL_ZGYRO             0x03u
#define MPU6050_CLOCK_PLL_EXT32K            0x04u
#define MPU6050_CLOCK_PLL_EXT19M            0x05u
#define MPU6050_CLOCK_KEEP_RESET            0x07u

#define MPU_PWR_MGMT_2_LP_WAKE_CTRL_MSK     0xC0u
#define MPU_PWR_MGMT_2_LP_WAKE_CTRL_POS     0x06u
#define MPU_PWR_MGMT_2_LP_STBY_XA_MSK       0x20u
#define MPU_PWR_MGMT_2_LP_STBY_XA_POS       0x05u
#define MPU_PWR_MGMT_2_LP_STBY_YA_MSK       0x10u
#define MPU_PWR_MGMT_2_LP_STBY_YA_POS       0x04u
#define MPU_PWR_MGMT_2_LP_STBY_ZA_MSK       0x08u
#define MPU_PWR_MGMT_2_LP_STBY_ZA_POS       0x03u
#define MPU_PWR_MGMT_2_LP_STBY_XG_MSK       0x04u
#define MPU_PWR_MGMT_2_LP_STBY_XG_POS       0x02u
#define MPU_PWR_MGMT_2_LP_STBY_YG_MSK       0x02u
#define MPU_PWR_MGMT_2_LP_STBY_YG_POS       0x01u
#define MPU_PWR_MGMT_2_LP_STBY_ZG_MSK       0x01u
#define MPU_PWR_MGMT_2_LP_STBY_ZG_POS       0x00u

#define MPU_PWR_MGMT_2_LP_WAKE_1P25Hz       0x00u
#define MPU_PWR_MGMT_2_LP_WAKE_5Hz          0x01u
#define MPU_PWR_MGMT_2_LP_WAKE_20Hz         0x02u
#define MPU_PWR_MGMT_2_LP_WAKE_40Hz         0x03u

#define MPU_INT_MOTION_DETECT_MSK           0x40u
#define MPU_INT_MOTION_DETECT_POS           0x06u
#define MPU_INT_ZMOTION_DETECT_MSK          0x20u
#define MPU_INT_ZMOTION_DETECT_POS          0x05u

#define MPU_WHO_AM_I_MSK                    0x7Eu
#define CALIBRATION_READINGS                50u

/*
 * Initialize Wire before begin(); optional calibration requires a stationary, level sensor.
 * Acceleration is cm/s^2, angular velocity is deg/s, and tilt is degrees.
 * Floating measurements return NAN on failure; raw/offset output pointers must be valid.
 */
class AccelAndGyro
{
  public:
      AccelAndGyro(uint8_t i2c_add=MPU6050_ADDRESS_AD0_HIGH);
      bool begin(bool calibrate=false);
      bool accelGyroCalibrate(void);
      bool ping(void);
      uint8_t getDeviceId(void);
      bool reset(void);
      bool resetGyroPath(void);
      bool resetAccelPath(void);
      bool resetTempPath(void);
      bool setFullScaleGyroRange(uint8_t range);
      uint8_t getFullScaleGyroRange(void);
      bool setFullScaleAccelRange(uint8_t range);
      uint8_t getFullScaleAccelRange(void);
      bool setSleep(bool enable=false);
      bool getSleepSts(void);
      bool setCycleMode(bool enable=false);
      bool getCycleMode(void);
      bool setTempSensorDisable(bool enable=true);
      bool getTempSensorDisableSts(void);
      bool setClkSource(uint8_t src);
      uint8_t getClkSource(void);
      bool setWakeFrequency(uint8_t frequency);
      uint8_t getWakeFrequency(void);
      bool setStandbyXAccel(bool enable);
      bool getStandbyXAccelSts(void);
      bool setStandbyYAccel(bool enable);
      bool getStandbyYAccelSts(void);
      bool setStandbyZAccel(bool enable);
      bool getStandbyZAccelSts(void);
      bool setStandbyXGyro(bool enable);
      bool getStandbyXGyroSts(void);
      bool setStandbyYGyro(bool enable);
      bool getStandbyYGyroSts(void);
      bool setStandbyZGyro(bool enable);
      bool getStandbyZGyroSts(void);
      uint8_t getMotionDetectionThreshold(void);
      bool setMotionDetectionThreshold(uint8_t threshold);
      uint8_t getMotionDetectionDuration(void);
      bool setMotionDetectionDuration(uint8_t duration);
      uint8_t getZeroMotionDetectionThreshold(void);
      bool setZeroMotionDetectionThreshold(uint8_t threshold);
      uint8_t getZeroMotionDetectionDuration(void);
      bool setZeroMotionDetectionDuration(uint8_t duration);
      bool getIntMotionEnabled(void);
      bool setIntMotionEnabled(bool enable);
      bool getIntMotionStatus(void);
      bool getIntZeroMotionEnabled(void);
      bool setIntZeroMotionEnabled(bool enable);
      bool getIntZeroMotionStatus(void);

      float getAccelX(bool print=true);
      float getAccelY(bool print=true);
      float getAccelZ(bool print=true);
      float getGyroX(bool print=true);
      float getGyroY(bool print=true);
      float getGyroZ(bool print=true);
      float getTempC(bool print=true);
      float getTempF(bool print=true);
      float getTiltX(bool print=true);
      float getTiltY(bool print=true);
      float getTiltZ(bool print=true);
      bool getMotionStatus(bool print=true);
  private:
      float _accelBias[3] = {};
      float _gyroBias[3] = {};
      float _accelScale[4u];
      float _gyroScale[4u];
      uint8_t _i2cSlaveAddress;
      bool _isConnected;
      bool getAccel(int16_t *aX, int16_t *aY, int16_t *aZ);
      bool getGyro(int16_t *gX, int16_t *gY, int16_t *gZ);
      bool getAccelOffset(int16_t *aX, int16_t *aY, int16_t *aZ);
      bool setAccelOffset(int16_t *aX, int16_t *aY, int16_t *aZ);
      bool getGyroOffset(int16_t *gX, int16_t *gY, int16_t *gZ);
      bool setGyroOffset(int16_t *gX, int16_t *gY, int16_t *gZ);
      void i2c_init(void);
      bool readByte(uint8_t reg, uint8_t *in);
      bool readMultiBytes(uint8_t reg, uint8_t length, uint8_t *in);
      bool writeByte(uint8_t reg);
      bool writeByte(uint8_t reg, uint8_t val);
      bool writeAddress(void);
      bool writeMultiBytes(uint8_t reg, uint8_t length, const uint8_t *in);
      void delay_ms(uint16_t ms);
};

#endif
