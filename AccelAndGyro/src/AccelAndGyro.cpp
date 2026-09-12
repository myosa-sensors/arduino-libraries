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

#include "AccelAndGyro.h"

/*
 * Initialize the device address and local state before any measurement is requested.
 */
AccelAndGyro::AccelAndGyro(uint8_t i2c_add)
{
	_i2cSlaveAddress	= i2c_add;
	_isConnected		= false;
	/* 1g = 9.80665 m/s^2 */
	/* Update the Accelerometer and Gyrometer scale factors */
	for(uint32_t fsr_sel=0u; fsr_sel < 4u; fsr_sel++)
	{
		_gyroScale[fsr_sel] = (250.f*(float)(1u << fsr_sel))/32768.f;
		_accelScale[fsr_sel] = (2.f*9.80665f*(float)(1u << fsr_sel))/32768.f;
	}
}

/*
 * Configure the default ranges and motion detection; optionally calibrate while stationary.
 */
bool AccelAndGyro::begin(bool calibrate)
{
  _isConnected = false;
	if(setClkSource(MPU6050_CLOCK_PLL_XGYRO) == false)
	{
		return false;
	}
	if(setFullScaleGyroRange(MPU_GYRO_CONFIG_FS_SEL_250) == false)
	{
		return false;
	}
	if(setFullScaleAccelRange(MPU_ACCEL_CONFIG_FS_SEL_2g) == false)
	{
		return false;
	}
	if(setSleep(false) == false)
	{
		return false;
	}
	if(setIntZeroMotionEnabled(false) == false)
	{
		return false;
	}
	if(setIntMotionEnabled(true) == false)
	{
		return false;
	}
	if(setMotionDetectionThreshold(2) == false)
	{
		return false;
	}
	if(setZeroMotionDetectionThreshold(2) == false)
	{
		return false;
	}
	if(setMotionDetectionDuration(40) == false)
	{
		return false;
	}
	if(setZeroMotionDetectionDuration(1) == false)
	{
		return false;
	}
	/* Calibrate the sensor if required */
	if(calibrate)
	{
		if(accelGyroCalibrate() == false)
		{
			return false;
		}
	}
	_isConnected = true;
	return true;
}

bool AccelAndGyro::accelGyroCalibrate(void)
{
  // Keep the sensor stationary and level, with +Z pointing upward.
  const uint8_t ar = getFullScaleAccelRange(), gr = getFullScaleGyroRange();
  if(ar > 3 || gr > 3) return false;
  float acceleration[3] = {}, rotation[3] = {};
  for(uint8_t n = 0; n < CALIBRATION_READINGS; ++n) {
    int16_t a[3], g[3];
    if(!getAccel(&a[0], &a[1], &a[2]) || !getGyro(&g[0], &g[1], &g[2])) return false;
    for(uint8_t i = 0; i < 3; ++i) {
      acceleration[i] += a[i] * _accelScale[ar] * 100.f;
      rotation[i] += g[i] * _gyroScale[gr];
    }
    delay_ms(20);
  }
  // Commit only after all samples succeed; retain the one-g gravity component.
  for(uint8_t i = 0; i < 3; ++i) {
    _accelBias[i] = acceleration[i] / CALIBRATION_READINGS - (i == 2 ? 980.665f : 0.f);
    _gyroBias[i] = rotation[i] / CALIBRATION_READINGS;
  }
  return true;
}

/*
 * Probe the device and restore its default configuration after a disconnect.
 */
bool AccelAndGyro::ping(void)
{
  if(!writeAddress()) { _isConnected = false; return false; }
  if(!_isConnected) _isConnected = begin();
  return _isConnected;
}

/*
 * Return the masked WHO_AM_I value, or zero if the register cannot be read.
 */
uint8_t AccelAndGyro::getDeviceId(void)
{
	uint8_t deviceId;
	if(readByte(MPU6050_WHO_AM_I_REG, &deviceId) == false)
	{
		deviceId = 0U;
	}
	return (deviceId & MPU_WHO_AM_I_MSK);
}

/*
 * Request a device reset and invalidate the connection state.
 */
bool AccelAndGyro::reset(void)
{
  _isConnected = false;
	uint8_t pwrMgmt1Val;
	if(readByte(MPU6050_PWR_MGMT_1_REG,&pwrMgmt1Val) == false)
	{
		return false;
	}
	pwrMgmt1Val |= MPU_PWR_MGMT_1_DEVICE_RESET_MSK;
	return writeByte(MPU6050_PWR_MGMT_1_REG,pwrMgmt1Val);
}

/*
 * Reset the gyroscope signal path without changing the power configuration.
 */
bool AccelAndGyro::resetGyroPath(void)
{
	uint8_t signalPath;
	if(readByte(MPU6050_SIGNAL_PATH_RESET_REG,&signalPath) == false)
	{
		return false;
	}
	signalPath |= MPU_SIGNAL_PATH_GYRO_RESET_MSK;
	return writeByte(MPU6050_SIGNAL_PATH_RESET_REG,signalPath);
}

/*
 * Reset the accelerometer signal path without changing the power configuration.
 */
bool AccelAndGyro::resetAccelPath(void)
{
	uint8_t signalPath;
	if(readByte(MPU6050_SIGNAL_PATH_RESET_REG,&signalPath) == false)
	{
		return false;
	}
	signalPath |= MPU_SIGNAL_PATH_ACCEL_RESET_MSK;
	return writeByte(MPU6050_SIGNAL_PATH_RESET_REG,signalPath);
}

/*
 * Reset the temperature signal path without changing the power configuration.
 */
bool AccelAndGyro::resetTempPath(void)
{
	uint8_t signalPath;
	if(readByte(MPU6050_SIGNAL_PATH_RESET_REG,&signalPath) == false)
	{
		return false;
	}
	signalPath |= MPU_SIGNAL_PATH_TEMP_RESET_MSK;
	return writeByte(MPU6050_SIGNAL_PATH_RESET_REG,signalPath);
}

/*
 * Select a gyroscope range code (0-3), preserving unrelated configuration bits.
 */
bool AccelAndGyro::setFullScaleGyroRange(uint8_t range)
{
  if(range > 3) return false;
	uint8_t gyroConfig;
	if(readByte(MPU6050_GYRO_CONFIG_REG,&gyroConfig) == false)
	{
		return false;
	}
	gyroConfig &= ~MPU_GYRO_CONFIG_FS_SEL_MASK;
	gyroConfig |= (range << MPU_GYRO_CONFIG_FS_SEL_POS);
	return writeByte(MPU6050_GYRO_CONFIG_REG,gyroConfig);
}

/*
 * Return the gyroscope range code; 0xFF indicates a failed register read.
 */
uint8_t AccelAndGyro::getFullScaleGyroRange(void)
{
	uint8_t gyroConfig;
	uint8_t range;
	if(readByte(MPU6050_GYRO_CONFIG_REG,&gyroConfig) == false)
	{
		return 0x0Fu;
	}
	range = (gyroConfig & MPU_GYRO_CONFIG_FS_SEL_MASK)>>MPU_GYRO_CONFIG_FS_SEL_POS;
	return range&0x0Fu;
}

/*
 * Select an accelerometer range code (0-3), preserving unrelated configuration bits.
 */
bool AccelAndGyro::setFullScaleAccelRange(uint8_t range)
{
  if(range > 3) return false;
	uint8_t accelConfig;
	if(readByte(MPU6050_ACCEL_CONFIG_REG,&accelConfig) == false)
	{
		return false;
	}
	accelConfig &= ~MPU_ACCEL_CONFIG_FS_SEL_MASK;
	accelConfig |= (range << MPU_ACCEL_CONFIG_FS_SEL_POS);
	return writeByte(MPU6050_ACCEL_CONFIG_REG,accelConfig);
}

/*
 * Return the accelerometer range code; 0xFF indicates a failed register read.
 */
uint8_t AccelAndGyro::getFullScaleAccelRange(void)
{
	uint8_t accelConfig;
	uint8_t range;
	if(readByte(MPU6050_ACCEL_CONFIG_REG,&accelConfig) == false)
	{
		return 0x0Fu;
	}
	range = (accelConfig & MPU_ACCEL_CONFIG_FS_SEL_MASK)>>MPU_ACCEL_CONFIG_FS_SEL_POS;
	return range&0x0Fu;
}

/*
 * Update the sleep enable while preserving unrelated register bits.
 */
bool AccelAndGyro::setSleep(bool enable)
{
	uint8_t pwrMgmt1Val;
	if(readByte(MPU6050_PWR_MGMT_1_REG,&pwrMgmt1Val) == false)
	{
		return false;
	}
	pwrMgmt1Val &= ~MPU_PWR_MGMT_1_SLEEP_MSK;
	if(enable)
	{
		pwrMgmt1Val |= MPU_PWR_MGMT_1_SLEEP_MSK;
	}
	return writeByte(MPU6050_PWR_MGMT_1_REG,pwrMgmt1Val);
}

/*
 * Read the sleep enable from the sensor configuration.
 */
bool AccelAndGyro::getSleepSts(void)
{
	uint8_t pwrMgmt1Val;
	uint8_t sleepState;
	if(readByte(MPU6050_PWR_MGMT_1_REG,&pwrMgmt1Val) == false)
	{
		return false;
	}
	sleepState = (pwrMgmt1Val & MPU_PWR_MGMT_1_SLEEP_MSK)>>MPU_PWR_MGMT_1_SLEEP_POS;
	return (bool)sleepState;
}

/*
 * Update the low-power cycle mode while preserving unrelated register bits.
 */
bool AccelAndGyro::setCycleMode(bool enable)
{
	uint8_t pwrMgmt1Val;
	if(readByte(MPU6050_PWR_MGMT_1_REG,&pwrMgmt1Val) == false)
	{
		return false;
	}
	pwrMgmt1Val &= ~MPU_PWR_MGMT_1_CYCLE_MSK;
	if(enable)
	{
		pwrMgmt1Val |= MPU_PWR_MGMT_1_CYCLE_MSK;
	}
	return writeByte(MPU6050_PWR_MGMT_1_REG,pwrMgmt1Val);
}

/*
 * Read the low-power cycle mode from the sensor configuration.
 */
bool AccelAndGyro::getCycleMode(void)
{
	uint8_t pwrMgmt1Val;
	uint8_t cycleMode;
	if(readByte(MPU6050_PWR_MGMT_1_REG,&pwrMgmt1Val) == false)
	{
		return false;
	}
	cycleMode = (pwrMgmt1Val & MPU_PWR_MGMT_1_CYCLE_MSK)>>MPU_PWR_MGMT_1_CYCLE_POS;
	return (bool)cycleMode;
}

/*
 * Update the temperature-disable bit while preserving unrelated register bits.
 */
bool AccelAndGyro::setTempSensorDisable(bool enable)
{
	uint8_t pwrMgmt1Val;
	if(readByte(MPU6050_PWR_MGMT_1_REG,&pwrMgmt1Val) == false)
	{
		return false;
	}
	pwrMgmt1Val &= ~MPU_PWR_MGMT_1_TEMP_DIS_MSK;
	if(enable)
	{
		pwrMgmt1Val |= MPU_PWR_MGMT_1_TEMP_DIS_MSK;
	}
	return writeByte(MPU6050_PWR_MGMT_1_REG,pwrMgmt1Val);
}

/*
 * Read the temperature-disable bit from the sensor configuration.
 */
bool AccelAndGyro::getTempSensorDisableSts(void)
{
	uint8_t pwrMgmt1Val;
	uint8_t cycleMode;
	if(readByte(MPU6050_PWR_MGMT_1_REG,&pwrMgmt1Val) == false)
	{
		return false;
	}
	cycleMode = (pwrMgmt1Val & MPU_PWR_MGMT_1_TEMP_DIS_MSK)>>MPU_PWR_MGMT_1_TEMP_DIS_POS;
	return (bool)cycleMode;
}

/*
 * Update the clock-source code while preserving unrelated register bits.
 */
bool AccelAndGyro::setClkSource(uint8_t src)
{
  if(src > 7 || src == 6) return false;
	uint8_t pwrMgmt1Val;
	if(readByte(MPU6050_PWR_MGMT_1_REG,&pwrMgmt1Val) == false)
	{
		return false;
	}
	pwrMgmt1Val &= ~MPU_PWR_MGMT_1_CLKSEL_MSK;
	pwrMgmt1Val |= (src << MPU_PWR_MGMT_1_CLKSEL_POS);
	return writeByte(MPU6050_PWR_MGMT_1_REG,pwrMgmt1Val);
}

/*
 * Read the clock-source code from the sensor configuration.
 */
uint8_t AccelAndGyro::getClkSource(void)
{
	uint8_t pwrMgmt1Val;
	uint8_t clkSrc;
	if(readByte(MPU6050_PWR_MGMT_1_REG,&pwrMgmt1Val) == false)
	{
		return 0u;
	}
	clkSrc = (pwrMgmt1Val & MPU_PWR_MGMT_1_CLKSEL_MSK)>>MPU_PWR_MGMT_1_CLKSEL_POS;
	return clkSrc;
}

/*
 * Update the wake-frequency code while preserving unrelated register bits.
 */
bool AccelAndGyro::setWakeFrequency(uint8_t frequency)
{
  if(frequency > 3) return false;
	uint8_t pwrMgmt2Val;
	if(readByte(MPU6050_PWR_MGMT_2_REG,&pwrMgmt2Val) == false)
	{
		return false;
	}
	pwrMgmt2Val &= ~MPU_PWR_MGMT_2_LP_WAKE_CTRL_MSK;
	pwrMgmt2Val |= (frequency << MPU_PWR_MGMT_2_LP_WAKE_CTRL_POS);
	return writeByte(MPU6050_PWR_MGMT_2_REG,pwrMgmt2Val);
}

/*
 * Read the wake-frequency code from the sensor configuration.
 */
uint8_t AccelAndGyro::getWakeFrequency(void)
{
	uint8_t pwrMgmt2Val;
	uint8_t frequency;
	if(readByte(MPU6050_PWR_MGMT_2_REG,&pwrMgmt2Val) == false)
	{
		return 0u;
	}
	frequency = (pwrMgmt2Val & MPU_PWR_MGMT_2_LP_WAKE_CTRL_MSK)>>MPU_PWR_MGMT_2_LP_WAKE_CTRL_POS;
	return frequency;
}

/*
 * Update the X-axis accelerometer standby bit while preserving unrelated register bits.
 */
bool AccelAndGyro::setStandbyXAccel(bool enable)
{
	uint8_t pwrMgmt2Val;
	if(readByte(MPU6050_PWR_MGMT_2_REG,&pwrMgmt2Val) == false)
	{
		return false;
	}
	pwrMgmt2Val &= ~MPU_PWR_MGMT_2_LP_STBY_XA_MSK;
	if(enable)
	{
		pwrMgmt2Val |= MPU_PWR_MGMT_2_LP_STBY_XA_MSK;
	}
	return writeByte(MPU6050_PWR_MGMT_2_REG,pwrMgmt2Val);
}

/*
 * Read the X-axis accelerometer standby bit from the sensor configuration.
 */
bool AccelAndGyro::getStandbyXAccelSts(void)
{
	uint8_t pwrMgmt2Val;
	uint8_t standySts;
	if(readByte(MPU6050_PWR_MGMT_2_REG,&pwrMgmt2Val) == false)
	{
		return false;
	}
	standySts = (pwrMgmt2Val & MPU_PWR_MGMT_2_LP_STBY_XA_MSK)>>MPU_PWR_MGMT_2_LP_STBY_XA_POS;
	return (bool)standySts;
}

/*
 * Update the Y-axis accelerometer standby bit while preserving unrelated register bits.
 */
bool AccelAndGyro::setStandbyYAccel(bool enable)
{
	uint8_t pwrMgmt2Val;
	if(readByte(MPU6050_PWR_MGMT_2_REG,&pwrMgmt2Val) == false)
	{
		return false;
	}
	pwrMgmt2Val &= ~MPU_PWR_MGMT_2_LP_STBY_YA_MSK;
	if(enable)
	{
		pwrMgmt2Val |= MPU_PWR_MGMT_2_LP_STBY_YA_MSK;
	}
	return writeByte(MPU6050_PWR_MGMT_2_REG,pwrMgmt2Val);
}

/*
 * Read the Y-axis accelerometer standby bit from the sensor configuration.
 */
bool AccelAndGyro::getStandbyYAccelSts(void)
{
	uint8_t pwrMgmt2Val;
	uint8_t standySts;
	if(readByte(MPU6050_PWR_MGMT_2_REG,&pwrMgmt2Val) == false)
	{
		return false;
	}
	standySts = (pwrMgmt2Val & MPU_PWR_MGMT_2_LP_STBY_YA_MSK)>>MPU_PWR_MGMT_2_LP_STBY_YA_POS;
	return (bool)standySts;
}

/*
 * Update the Z-axis accelerometer standby bit while preserving unrelated register bits.
 */
bool AccelAndGyro::setStandbyZAccel(bool enable)
{
	uint8_t pwrMgmt2Val;
	if(readByte(MPU6050_PWR_MGMT_2_REG,&pwrMgmt2Val) == false)
	{
		return false;
	}
	pwrMgmt2Val &= ~MPU_PWR_MGMT_2_LP_STBY_ZA_MSK;
	if(enable)
	{
		pwrMgmt2Val |= MPU_PWR_MGMT_2_LP_STBY_ZA_MSK;
	}
	return writeByte(MPU6050_PWR_MGMT_2_REG,pwrMgmt2Val);
}

/*
 * Read the Z-axis accelerometer standby bit from the sensor configuration.
 */
bool AccelAndGyro::getStandbyZAccelSts(void)
{
	uint8_t pwrMgmt2Val;
	uint8_t standySts;
	if(readByte(MPU6050_PWR_MGMT_2_REG,&pwrMgmt2Val) == false)
	{
		return false;
	}
	standySts = (pwrMgmt2Val & MPU_PWR_MGMT_2_LP_STBY_ZA_MSK)>>MPU_PWR_MGMT_2_LP_STBY_ZA_POS;
	return (bool)standySts;
}

/*
 * Update the X-axis gyroscope standby bit while preserving unrelated register bits.
 */
bool AccelAndGyro::setStandbyXGyro(bool enable)
{
	uint8_t pwrMgmt2Val;
	if(readByte(MPU6050_PWR_MGMT_2_REG,&pwrMgmt2Val) == false)
	{
		return false;
	}
	pwrMgmt2Val &= ~MPU_PWR_MGMT_2_LP_STBY_XG_MSK;
	if(enable)
	{
		pwrMgmt2Val |= MPU_PWR_MGMT_2_LP_STBY_XG_MSK;
	}
	return writeByte(MPU6050_PWR_MGMT_2_REG,pwrMgmt2Val);
}

/*
 * Read the X-axis gyroscope standby bit from the sensor configuration.
 */
bool AccelAndGyro::getStandbyXGyroSts(void)
{
	uint8_t pwrMgmt2Val;
	uint8_t standySts;
	if(readByte(MPU6050_PWR_MGMT_2_REG,&pwrMgmt2Val) == false)
	{
		return false;
	}
	standySts = (pwrMgmt2Val & MPU_PWR_MGMT_2_LP_STBY_XG_MSK)>>MPU_PWR_MGMT_2_LP_STBY_XG_POS;
	return (bool)standySts;
}

/*
 * Update the Y-axis gyroscope standby bit while preserving unrelated register bits.
 */
bool AccelAndGyro::setStandbyYGyro(bool enable)
{
	uint8_t pwrMgmt2Val;
	if(readByte(MPU6050_PWR_MGMT_2_REG,&pwrMgmt2Val) == false)
	{
		return false;
	}
	pwrMgmt2Val &= ~MPU_PWR_MGMT_2_LP_STBY_YG_MSK;
	if(enable)
	{
		pwrMgmt2Val |= MPU_PWR_MGMT_2_LP_STBY_YG_MSK;
	}
	return writeByte(MPU6050_PWR_MGMT_2_REG,pwrMgmt2Val);
}

/*
 * Read the Y-axis gyroscope standby bit from the sensor configuration.
 */
bool AccelAndGyro::getStandbyYGyroSts(void)
{
	uint8_t pwrMgmt2Val;
	uint8_t standySts;
	if(readByte(MPU6050_PWR_MGMT_2_REG,&pwrMgmt2Val) == false)
	{
		return false;
	}
	standySts = (pwrMgmt2Val & MPU_PWR_MGMT_2_LP_STBY_YG_MSK)>>MPU_PWR_MGMT_2_LP_STBY_YG_POS;
	return (bool)standySts;
}

/*
 * Update the Z-axis gyroscope standby bit while preserving unrelated register bits.
 */
bool AccelAndGyro::setStandbyZGyro(bool enable)
{
	uint8_t pwrMgmt2Val;
	if(readByte(MPU6050_PWR_MGMT_2_REG,&pwrMgmt2Val) == false)
	{
		return false;
	}
	pwrMgmt2Val &= ~MPU_PWR_MGMT_2_LP_STBY_ZG_MSK;
	if(enable)
	{
		pwrMgmt2Val |= MPU_PWR_MGMT_2_LP_STBY_ZG_MSK;
	}
	return writeByte(MPU6050_PWR_MGMT_2_REG,pwrMgmt2Val);
}

/*
 * Read the Z-axis gyroscope standby bit from the sensor configuration.
 */
bool AccelAndGyro::getStandbyZGyroSts(void)
{
	uint8_t pwrMgmt2Val;
	uint8_t standySts;
	if(readByte(MPU6050_PWR_MGMT_2_REG,&pwrMgmt2Val) == false)
	{
		return false;
	}
	standySts = (pwrMgmt2Val & MPU_PWR_MGMT_2_LP_STBY_ZG_MSK)>>MPU_PWR_MGMT_2_LP_STBY_ZG_POS;
	return (bool)standySts;
}

/*
 * Read the motion threshold in register counts from the sensor configuration.
 */
uint8_t AccelAndGyro::getMotionDetectionThreshold(void)
{
	uint8_t threshold;
	if(readByte(MPU6050_MOTION_THR,&threshold))
	{
		return threshold;
	}
	return 0u;
}

/*
 * Update the motion threshold in register counts while preserving unrelated register bits.
 */
bool AccelAndGyro::setMotionDetectionThreshold(uint8_t threshold)
{
	return writeByte(MPU6050_MOTION_THR,threshold);
}

/*
 * Read the motion duration in register counts from the sensor configuration.
 */
uint8_t AccelAndGyro::getMotionDetectionDuration(void)
{
	uint8_t duration;
	if(readByte(MPU6050_MOTION_DUR,&duration))
	{
		return duration;
	}
	return 0u;
}

/*
 * Update the motion duration in register counts while preserving unrelated register bits.
 */
bool AccelAndGyro::setMotionDetectionDuration(uint8_t threshold)
{
	return writeByte(MPU6050_MOTION_DUR,threshold);
}

/*
 * Read the zero-motion threshold in register counts from the sensor configuration.
 */
uint8_t AccelAndGyro::getZeroMotionDetectionThreshold(void)
{
	uint8_t threshold;
	if(readByte(MPU6050_ZERO_MOTION_THR,&threshold))
	{
		return threshold;
	}
	return 0u;
}

/*
 * Update the zero-motion threshold in register counts while preserving unrelated register bits.
 */
bool AccelAndGyro::setZeroMotionDetectionThreshold(uint8_t threshold)
{
	return writeByte(MPU6050_ZERO_MOTION_THR,threshold);
}

/*
 * Read the zero-motion duration in register counts from the sensor configuration.
 */
uint8_t AccelAndGyro::getZeroMotionDetectionDuration(void)
{
	uint8_t duration;
	if(readByte(MPU6050_ZERO_MOTION_DUR,&duration))
	{
		return duration;
	}
	return 0u;
}

/*
 * Update the zero-motion duration in register counts while preserving unrelated register bits.
 */
bool  AccelAndGyro::setZeroMotionDetectionDuration(uint8_t threshold)
{
	return writeByte(MPU6050_ZERO_MOTION_DUR,threshold);
}

/*
 * Read the motion interrupt enable from the sensor configuration.
 */
bool AccelAndGyro::getIntMotionEnabled(void)
{
	uint8_t intEnable;
	uint8_t motionEn;
	if(readByte(MPU6050_INT_ENABLE,&intEnable) == false)
	{
		return false;
	}
	motionEn = (intEnable & MPU_INT_MOTION_DETECT_MSK)>>MPU_INT_MOTION_DETECT_POS;
	return (bool)motionEn;
}

/*
 * Update the motion interrupt enable while preserving unrelated register bits.
 */
bool AccelAndGyro::setIntMotionEnabled(bool enable)
{
	uint8_t intEnable;
	if(readByte(MPU6050_INT_ENABLE,&intEnable) == false)
	{
		return false;
	}
	intEnable &= ~MPU_INT_MOTION_DETECT_MSK;
	if(enable)
	{
		intEnable |= MPU_INT_MOTION_DETECT_MSK;
	}
	return writeByte(MPU6050_INT_ENABLE,intEnable);
}


/*
 * Read the motion bit from interrupt status; false also represents a failed transfer.
 */
bool AccelAndGyro::getIntMotionStatus(void)
{
	uint8_t intSts;
	uint8_t motionSts;
	if(readByte(MPU6050_INT_STATUS,&intSts) == false)
	{
		return false;
	}
	motionSts = (intSts & MPU_INT_MOTION_DETECT_MSK)>>MPU_INT_MOTION_DETECT_POS;
	return (bool)motionSts;
}

/*
 * Read the zero-motion interrupt enable from the sensor configuration.
 */
bool AccelAndGyro::getIntZeroMotionEnabled(void)
{
	uint8_t intEnable;
	uint8_t zeroMotionEn;
	if(readByte(MPU6050_INT_ENABLE,&intEnable) == false)
	{
		return false;
	}
	zeroMotionEn = (intEnable & MPU_INT_ZMOTION_DETECT_MSK)>>MPU_INT_ZMOTION_DETECT_POS;
	return (bool)zeroMotionEn;
}

/*
 * Update the zero-motion interrupt enable while preserving unrelated register bits.
 */
bool AccelAndGyro::setIntZeroMotionEnabled(bool enable)
{
	uint8_t intEnable;
	if(readByte(MPU6050_INT_ENABLE,&intEnable) == false)
	{
		return false;
	}
	intEnable &= ~MPU_INT_ZMOTION_DETECT_MSK;
	if(enable)
	{
		intEnable |= MPU_INT_ZMOTION_DETECT_MSK;
	}
	return writeByte(MPU6050_INT_ENABLE,intEnable);
}

/*
 * Read the zero-motion bit from interrupt status; false also represents a failed transfer.
 */
bool AccelAndGyro::getIntZeroMotionStatus(void)
{
	uint8_t intSts;
	uint8_t zeroMotionSts;
	if(readByte(MPU6050_INT_STATUS,&intSts) == false)
	{
		return false;
	}
	zeroMotionSts = (intSts & MPU_INT_ZMOTION_DETECT_MSK)>>MPU_INT_ZMOTION_DETECT_POS;
	return (bool)zeroMotionSts;
}

/*
 * Return calibrated X-axis acceleration in cm/s^2; unavailable readings return NAN.
 */
float AccelAndGyro::getAccelX(bool print)
{
  uint8_t data[2];
  if(!readMultiBytes(MPU6050_ACCEL_XOUT_H_REG, sizeof(data), data)) return NAN;
  const uint8_t range = getFullScaleAccelRange();
  if(range > 3) return NAN;
  const int16_t raw = (int16_t)(((uint16_t)data[0] << 8) | data[1]);
  const float value = raw * _accelScale[range] * 100.f - _accelBias[0];
  if(print) { Serial.print("Acceleration(X): "); Serial.print(value, 2); Serial.println(" cm/s^2"); }
  return value;
}

/*
 * Return calibrated Y-axis acceleration in cm/s^2; unavailable readings return NAN.
 */
float AccelAndGyro::getAccelY(bool print)
{
  uint8_t data[2];
  if(!readMultiBytes(MPU6050_ACCEL_YOUT_H_REG, sizeof(data), data)) return NAN;
  const uint8_t range = getFullScaleAccelRange();
  if(range > 3) return NAN;
  const int16_t raw = (int16_t)(((uint16_t)data[0] << 8) | data[1]);
  const float value = raw * _accelScale[range] * 100.f - _accelBias[1];
  if(print) { Serial.print("Acceleration(Y): "); Serial.print(value, 2); Serial.println(" cm/s^2"); }
  return value;
}

/*
 * Return calibrated Z-axis acceleration in cm/s^2; unavailable readings return NAN.
 */
float AccelAndGyro::getAccelZ(bool print)
{
  uint8_t data[2];
  if(!readMultiBytes(MPU6050_ACCEL_ZOUT_H_REG, sizeof(data), data)) return NAN;
  const uint8_t range = getFullScaleAccelRange();
  if(range > 3) return NAN;
  const int16_t raw = (int16_t)(((uint16_t)data[0] << 8) | data[1]);
  const float value = raw * _accelScale[range] * 100.f - _accelBias[2];
  if(print) { Serial.print("Acceleration(Z): "); Serial.print(value, 2); Serial.println(" cm/s^2"); }
  return value;
}

/*
 * Return calibrated X-axis angular velocity in deg/s; unavailable readings return NAN.
 */
float AccelAndGyro::getGyroX(bool print)
{
  uint8_t data[2];
  if(!readMultiBytes(MPU6050_GYRO_XOUT_H_REG, sizeof(data), data)) return NAN;
  const uint8_t range = getFullScaleGyroRange();
  if(range > 3) return NAN;
  const int16_t raw = (int16_t)(((uint16_t)data[0] << 8) | data[1]);
  const float value = raw * _gyroScale[range] - _gyroBias[0];
  if(print) { Serial.print("Angular Velocity(X): "); Serial.print(value, 2); Serial.println(" deg/s"); }
  return value;
}

/*
 * Return calibrated Y-axis angular velocity in deg/s; unavailable readings return NAN.
 */
float AccelAndGyro::getGyroY(bool print)
{
  uint8_t data[2];
  if(!readMultiBytes(MPU6050_GYRO_YOUT_H_REG, sizeof(data), data)) return NAN;
  const uint8_t range = getFullScaleGyroRange();
  if(range > 3) return NAN;
  const int16_t raw = (int16_t)(((uint16_t)data[0] << 8) | data[1]);
  const float value = raw * _gyroScale[range] - _gyroBias[1];
  if(print) { Serial.print("Angular Velocity(Y): "); Serial.print(value, 2); Serial.println(" deg/s"); }
  return value;
}

/*
 * Return calibrated Z-axis angular velocity in deg/s; unavailable readings return NAN.
 */
float AccelAndGyro::getGyroZ(bool print)
{
  uint8_t data[2];
  if(!readMultiBytes(MPU6050_GYRO_ZOUT_H_REG, sizeof(data), data)) return NAN;
  const uint8_t range = getFullScaleGyroRange();
  if(range > 3) return NAN;
  const int16_t raw = (int16_t)(((uint16_t)data[0] << 8) | data[1]);
  const float value = raw * _gyroScale[range] - _gyroBias[2];
  if(print) { Serial.print("Angular Velocity(Z): "); Serial.print(value, 2); Serial.println(" deg/s"); }
  return value;
}

/*
 * Read all three signed raw acceleration axes in one transfer; outputs require valid pointers.
 */
bool AccelAndGyro::getAccel(int16_t *aX, int16_t *aY, int16_t *aZ)
{
	uint8_t accel[6u];
	if(readMultiBytes(MPU6050_ACCEL_XOUT_H_REG,6u,(uint8_t *)accel))
	{
		*aX = (int16_t)(((uint16_t)accel[0u] << 8u) | accel[1u]);
		*aY = (int16_t)(((uint16_t)accel[2u] << 8u) | accel[3u]);
		*aZ = (int16_t)(((uint16_t)accel[4u] << 8u) | accel[5u]);
		return true;
	}
	return false;
}

/*
 * Read all three signed raw angular-rate axes in one transfer; outputs require valid pointers.
 */
bool AccelAndGyro::getGyro(int16_t *gX, int16_t *gY, int16_t *gZ)
{
	uint8_t gyro[6u];
	if(readMultiBytes(MPU6050_GYRO_XOUT_H_REG,6u,(uint8_t *)gyro))
	{
		*gX = (int16_t)(((uint16_t)gyro[0u] << 8u) | gyro[1u]);
		*gY = (int16_t)(((uint16_t)gyro[2u] << 8u) | gyro[3u]);
		*gZ = (int16_t)(((uint16_t)gyro[4u] << 8u) | gyro[5u]);
		return true;
	}
	return false;
}


/*
 * Read the three hardware acceleration offset words into caller-provided storage.
 */
bool AccelAndGyro::getAccelOffset(int16_t *aX, int16_t *aY, int16_t *aZ)
{
	uint8_t data[6u];
	if(readMultiBytes(MPU6050_XA_OFFS_USRH_REG,6u,(uint8_t *)data))
	{
		*aX = (int16_t)(((uint16_t)data[0u] << 8u) | data[1u]);
		*aY = (int16_t)(((uint16_t)data[2u] << 8u) | data[3u]);
		*aZ = (int16_t)(((uint16_t)data[4u] << 8u) | data[5u]);
		return true;
	}
	return false;
}

/*
 * Write the three hardware offset words; these are separate from software calibration biases.
 */
bool AccelAndGyro::setAccelOffset(int16_t *aX, int16_t *aY, int16_t *aZ)
{
	uint8_t data[6u];
	data[0u] = (*aX >> 8);
	data[1u] = (*aX & 0xFF);
	data[2u] = (*aY >> 8);
	data[3u] = (*aY & 0xFF);
	data[4u] = (*aZ >> 8);
	data[5u] = (*aZ & 0xFF);

	if(writeMultiBytes(MPU6050_XA_OFFS_USRH_REG,6u,data))
	{
		return true;
	}
	return false;
}

/*
 * Read the three hardware gyroscope offset words into caller-provided storage.
 */
bool AccelAndGyro::getGyroOffset(int16_t *gX, int16_t *gY, int16_t *gZ)
{
	uint8_t data[6u];
	if(readMultiBytes(MPU6050_XG_OFFS_USRH_REG,6u,(uint8_t *)data))
	{
		*gX = (int16_t)(((uint16_t)data[0u] << 8u) | data[1u]);
		*gY = (int16_t)(((uint16_t)data[2u] << 8u) | data[3u]);
		*gZ = (int16_t)(((uint16_t)data[4u] << 8u) | data[5u]);
		return true;
	}
	return false;
}

/*
 * Write the three hardware gyroscope offset words from caller-provided storage.
 */
bool AccelAndGyro::setGyroOffset(int16_t *gX, int16_t *gY, int16_t *gZ)
{
	uint8_t data[6u];
	data[0u] = (*gX >> 8);
	data[1u] = (*gX & 0xFF);
	data[2u] = (*gY >> 8);
	data[3u] = (*gY & 0xFF);
	data[4u] = (*gZ >> 8);
	data[5u] = (*gZ & 0xFF);

	if(writeMultiBytes(MPU6050_XG_OFFS_USRH_REG,6u,data))
	{
		return true;
	}
	return false;
}

/*
 * Read the sensor die temperature in Celsius; return NAN on transfer failure.
 */
float AccelAndGyro::getTempC(bool print)
{
	uint8_t data[2u];
	int16_t temp;
	float tempC;
	if(readMultiBytes(MPU6050_TEMP_OUT_H_REG,2u,(uint8_t *)data))
	{
		temp 	= (int16_t)(((uint16_t)data[0u] << 8u) | data[1u]) ;
		tempC 	= ((float)temp/340.f)+36.53f;
		if(print)
		{
			Serial.print("Temperature (°C): ");
		    Serial.print(tempC,2);
		    Serial.println("°C");
		}
		return tempC;
	}
	return NAN;
}

/*
 * Convert a fresh die-temperature reading to Fahrenheit; preserve NAN on failure.
 */
float AccelAndGyro::getTempF(bool print)
{
	float tempF = (getTempC(false) * (9.f / 5.f)) + 32.f;
	if(print)
    {
      Serial.print("Temperature (°F): ");
      Serial.print(tempF,2);
      Serial.println("°F");
    }
	return tempF;
}

/*
 * Return calibrated X-axis tilt in degrees; unavailable readings return NAN.
 */
float AccelAndGyro::getTiltX(bool print)
{
  int16_t a[3];
  if(!getAccel(&a[0], &a[1], &a[2])) return NAN;
  const uint8_t range = getFullScaleAccelRange();
  if(range > 3) return NAN;
  const float scale = _accelScale[range] * 100.f;
  const float x = a[0] * scale - _accelBias[0];
  const float y = a[1] * scale - _accelBias[1];
  const float z = a[2] * scale - _accelBias[2];
  if(x == 0 && y == 0 && z == 0) return NAN;
  const float angle = atan2f(x, sqrtf(y*y + z*z)) * 180.f / M_PI;
  if(print) { Serial.print("Tilt Angle(X): "); Serial.print(angle, 2); Serial.println(" deg"); }
  return angle;
}

/*
 * Return calibrated Y-axis tilt in degrees; unavailable readings return NAN.
 */
float AccelAndGyro::getTiltY(bool print)
{
  int16_t a[3];
  if(!getAccel(&a[0], &a[1], &a[2])) return NAN;
  const uint8_t range = getFullScaleAccelRange();
  if(range > 3) return NAN;
  const float scale = _accelScale[range] * 100.f;
  const float x = a[0] * scale - _accelBias[0];
  const float y = a[1] * scale - _accelBias[1];
  const float z = a[2] * scale - _accelBias[2];
  if(x == 0 && y == 0 && z == 0) return NAN;
  const float angle = atan2f(y, sqrtf(x*x + z*z)) * 180.f / M_PI;
  if(print) { Serial.print("Tilt Angle(Y): "); Serial.print(angle, 2); Serial.println(" deg"); }
  return angle;
}

/*
 * Return calibrated Z-axis tilt in degrees; unavailable readings return NAN.
 */
float AccelAndGyro::getTiltZ(bool print)
{
  int16_t a[3];
  if(!getAccel(&a[0], &a[1], &a[2])) return NAN;
  const uint8_t range = getFullScaleAccelRange();
  if(range > 3) return NAN;
  const float scale = _accelScale[range] * 100.f;
  const float x = a[0] * scale - _accelBias[0];
  const float y = a[1] * scale - _accelBias[1];
  const float z = a[2] * scale - _accelBias[2];
  if(x == 0 && y == 0 && z == 0) return NAN;
  const float angle = atan2f(sqrtf(x*x + y*y), z) * 180.f / M_PI;
  if(print) { Serial.print("Tilt Angle(Z): "); Serial.print(angle, 2); Serial.println(" deg"); }
  return angle;
}

/*
 * Read the motion interrupt flag and optionally print it; false also represents a read failure.
 */
bool AccelAndGyro::getMotionStatus(bool print)
{
  const bool motion = getIntMotionStatus();
  if(print) { Serial.print("Motion Detection Status: "); Serial.println(motion ? "True" : "False"); }
  return motion;
}

/***********************************************************************************************
 * Platform dependent routines. Change these functions implementation based on microcontroller *
 ***********************************************************************************************/
/*
 * Initialize the shared Wire bus at 100 kHz; normal sketches configure Wire before using drivers.
 */
void AccelAndGyro::i2c_init(void)
{
	Wire.begin();
	Wire.setClock(100000);
}

/*
 * Select one register and require a complete one-byte response.
 */
bool AccelAndGyro::readByte(uint8_t reg, uint8_t *in)
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
bool AccelAndGyro::readMultiBytes(uint8_t reg, uint8_t length, uint8_t *in)
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
 * Send a register/command byte and report the transfer result.
 */
bool AccelAndGyro::writeByte(uint8_t reg)
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
bool AccelAndGyro::writeByte(uint8_t reg, uint8_t val)
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
bool AccelAndGyro::writeAddress(void)
{
   Wire.beginTransmission((uint8_t)_i2cSlaveAddress);
   if (Wire.endTransmission(true) == 0)
   {
     return true;
   }
   return false;
}

/*
 * Write consecutive bytes starting at the selected register and report the transfer result.
 */
bool AccelAndGyro::writeMultiBytes(uint8_t reg, uint8_t length, const uint8_t *in)
{
  Wire.beginTransmission((uint8_t)_i2cSlaveAddress);
  Wire.write(reg);
  Wire.write(in,length);
  if (Wire.endTransmission(true) == 0)
  {
    return true;
  }
  return false;
}

/*
 * Wait for the requested number of milliseconds using the Arduino platform delay.
 */
void AccelAndGyro::delay_ms(uint16_t ms)
{
  delay(ms);
}
