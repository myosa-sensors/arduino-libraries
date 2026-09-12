/*
  This code is developed under the MYOSA (LearnTheEasyWay) initiative of MakeSense EduTech and Pegasus Automation.
  Code has been derived from internet sources and component datasheets.
  Existing readily-available libraries would have been used "AS IS" and modified for ease of learning purpose.

  Synopsis of Light Proximity and Gesture Board
  The MYOSA light and gesture board uses the APDS9960 sensor at I2C address 0x39.
  Ambient light is available in lux or raw counts; RGB is in percentages and proximity in counts.
  Gesture sensing reports direction, near/far motion and timeout status.
  Successful configuration settings are restored when the board reconnects.

  NOTE
  All information, including URL references, is subject to change without prior notice.
  Please always use the latest versions of software-release for best performance.
  Unless required by applicable law or agreed to in writing, this software is distributed on an
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied

  Modifications
  11 September, 2026 by Pegasus Automation
  (as a part of MYOSA Initiative)
 
  Contact Team MYOSA for any kind of feedback/issues pertaining to performance or any update request.
  Email: myosa.event@gmail.com
*/

#include "LightProximityAndGesture.h"

/*
 * Initialize the device address and local state before any measurement is requested.
 */
LightProximityAndGesture::LightProximityAndGesture()
{
  _i2cSlaveAddress = APDS9960_I2C_ADDRESS;
  _isConnected = false;
  resetGestureParameters();
}

/*
 * Reconnect with begin() when needed, then restore cached configuration before enabling engines.
 */
bool LightProximityAndGesture::ping(void)
{
  if(!writeAddress()) { _isConnected = false; return false; }
  if(!_isConnected) {
    uint8_t values[128], valid[16];
    memcpy(values, _configValues, sizeof(values));
    memcpy(valid, _configValid, sizeof(valid));
    bool ready = begin();
    if(ready) {
      // Restore configuration first, then enable the previously selected engines.
      for(uint16_t reg = 0x81; reg < 0xB0 && ready; ++reg)
        if(valid[(reg - 0x80) / 8] & (1u << ((reg - 0x80) % 8))) ready = writeByte(reg, values[reg - 0x80]);
      if(ready && (valid[0] & 1)) ready = writeByte(APDS9960_ENABLE, values[0]);
    }
    memcpy(_configValues, values, sizeof(values));
    memcpy(_configValid, valid, sizeof(valid));
    _isConnected = ready;
  }
  return _isConnected;
}

/*
 * Validate the APDS9960 identity and load defaults; sensing engines remain disabled.
 */
bool LightProximityAndGesture::begin(void)
{
  _isConnected = false;
  resetGestureParameters();
  uint8_t deviceId;
  /* Read ID register and check against known values for APDS9960 */
  if( !readByte(APDS9960_ID,&deviceId) )
  {
    return false;
  }
  if( (deviceId != APDS9960_ID_1) && (deviceId != APDS9960_ID_2) && (deviceId != APDS9960_ID_3) )
  {
    return false;
  }
  /* Set ENABLE register to 0 (disable all features) */
  if( !writeByte(APDS9960_ENABLE,0u) )
  {
    return false;
  }
  /* Set default values for ambient light and proximity registers */
  if( !writeByte(APDS9960_ATIME,DEFAULT_ATIME) )
  {
    return false;
  }
  if( !writeByte(APDS9960_WTIME,DEFAULT_WTIME) )
  {
    return false;
  }
  if( !writeByte(APDS9960_PPULSE,DEFAULT_PROX_PPULSE) )
  {
    return false;
  }
  if( !writeByte(APDS9960_POFFSET_UR,DEFAULT_POFFSET_UR) )
  {
    return false;
  }
  if( !writeByte(APDS9960_POFFSET_DL,DEFAULT_POFFSET_DL) )
  {
    return false;
  }
  if( !writeByte(APDS9960_CONFIG1,DEFAULT_CONFIG1) )
  {
    return false;
  }
  if( !setLedDrive(DEFAULT_LDRIVE) )
  {
    return false;
  }
  if( !setProximityGain(DEFAULT_PGAIN) )
  {
    return false;
  }
  if( !setAmbientLightGain(DEFAULT_AGAIN) )
  {
    return false;
  }
  if( !writeByte(APDS9960_PILT,DEFAULT_PILT))
  {
    return false;
  }
  if( !writeByte(APDS9960_PIHT,DEFAULT_PIHT))
  {
    return false;
  }
  if( !setLightIntLowThreshold(DEFAULT_AILT) )
  {
    return false;
  }
  if( !setLightIntHighThreshold(DEFAULT_AIHT) )
  {
    return false;
  }
  if( !writeByte(APDS9960_PERS, DEFAULT_PERS) )
  {
    return false;
  }
  if( !writeByte(APDS9960_CONFIG2, DEFAULT_CONFIG2) )
  {
    return false;
  }
  if( !writeByte(APDS9960_CONFIG3, DEFAULT_CONFIG3) )
  {
    return false;
  }
  /* Set default values for gesture sense registers */
  if( !writeByte(APDS9960_GPENTH,DEFAULT_GPENTH) )
  {
    return false;
  }
  if( !writeByte(APDS9960_GEXTH,DEFAULT_GEXTH) )
  {
    return false;
  }
  if( !writeByte(APDS9960_GCONF1, DEFAULT_GCONF1) )
  {
    return false;
  }
  if( !setGestureGain(DEFAULT_GGAIN) )
  {
    return false;
  }
  if( !setGestureLedDrive(DEFAULT_GLDRIVE) )
  {
    return false;
  }
  if( !setGestureWaitTime(DEFAULT_GWTIME) )
  {
    return false;
  }
  if( !writeByte(APDS9960_GOFFSET_U, DEFAULT_GOFFSET) )
  {
    return false;
  }
  if( !writeByte(APDS9960_GOFFSET_D, DEFAULT_GOFFSET) )
  {
    return false;
  }
  if( !writeByte(APDS9960_GOFFSET_L, DEFAULT_GOFFSET) )
  {
    return false;
  }
  if( !writeByte(APDS9960_GOFFSET_R, DEFAULT_GOFFSET) )
  {
    return false;
  }
  if( !writeByte(APDS9960_GPULSE, DEFAULT_GPULSE) )
  {
    return false;
  }
  if( !writeByte(APDS9960_GCONF3, DEFAULT_GCONF3) )
  {
    return false;
  }
  if( !setGestureInt(DISABLE) )
  {
    return false;
  }
  _isConnected = true;
  return true;
}

/*
 * Enable the power bit while preserving the selected sensing engines.
 */
bool LightProximityAndGesture::enablePower(void)
{
  return setMode(POWER, ENABLE);
}

/*
 * Clear the power bit without overwriting the other enable bits.
 */
bool LightProximityAndGesture::disablePower(void)
{
  return setMode(POWER, DISABLE);
}

/*
 * Read the ENABLE register bitmask; ERROR indicates a failed transfer.
 */
uint8_t LightProximityAndGesture::getMode(void)
{
  uint8_t enableVal;
  if(readByte(APDS9960_ENABLE,&enableVal) == false)
  {
    return ERROR;
  }
  return enableVal;
}

/*
 * Change the requested engine bit, or all mode bits, after validating the selector.
 */
bool LightProximityAndGesture::setMode(APDS9960_MODE_t mode, STATE_t state)
{
  if((unsigned)mode > 6 || (unsigned)state > 1) return false;
  uint8_t enableVal = getMode();
  if(enableVal == ERROR)
  {
    return false;
  }
  enableVal &= ~(1u << mode);
  enableVal |= (state << mode);
  if(writeByte(APDS9960_ENABLE,enableVal) == false)
  {
    return false;
  }
  return true;
}

/*
 * Configure and enable ambient-light sensing with optional interrupts.
 */
bool LightProximityAndGesture::enableAmbientLightSensor(STATE_t interrupt)
{
   /* Set default gain, interrupts, enable power, and enable sensor */
  if( !setAmbientLightGain(DEFAULT_AGAIN) )
  {
    return false;
  }
  if( !setAmbientLightInt(interrupt) )
  {
    return false;
  }
  if( !enablePower() )
  {
    return false;
  }
  if( !setMode(AMBIENT_LIGHT,ENABLE) )
  {
    return false;
  }
  return true;
}

/*
 * Disable ambient-light sensing and its interrupt.
 */
bool LightProximityAndGesture::disableAmbientLightSensor(void)
{
  if( !setAmbientLightInt(DISABLE) )
  {
    return false;
  }
  if( !setMode(AMBIENT_LIGHT,DISABLE) )
  {
    return false;
  }
  return true;
}

/*
 * Configure and enable proximity sensing with optional interrupts.
 */
bool LightProximityAndGesture::enableProximitySensor(STATE_t interrupt)
{
  /* Set default gain, LED, interrupts, enable power, and enable sensor */
  if( !setProximityGain(DEFAULT_PGAIN) )
  {
    return false;
  }
  if( !setLedDrive(DEFAULT_LDRIVE) )
  {
    return false;
  }
  if( !setProximityInt(interrupt) )
  {
    return false;
  }
  if( !enablePower() )
  {
    return false;
  }
  if( !setMode(PROXIMITY,ENABLE) )
  {
    return false;
  }
  return true;
}

/*
 * Disable proximity sensing and its interrupt.
 */
bool LightProximityAndGesture::disableProximitySensor(void)
{
  if( !setProximityInt(DISABLE) )
  {
    return false;
  }
  if( !setMode(PROXIMITY,DISABLE) )
  {
    return false;
  }
  return true;
}

/*
 * Enable the gesture pipeline and its required power, wait, and proximity engines.
 */
bool LightProximityAndGesture::enableGestureSensor(STATE_t interrupt)
{
  /*
    Enable gesture mode
    Set ENABLE to 0 (power off)
    Set WTIME to 0xFF
    Set AUX to LED_BOOST_300
    Enable PON, WEN, PEN, GEN in ENABLE
  */
  resetGestureParameters();
  if( !writeByte(APDS9960_WTIME,0xFFu) )
  {
    return false;
  }
  if( !writeByte(APDS9960_PPULSE,DEFAULT_GESTURE_PPULSE) )
  {
    return false;
  }
  if( !setLedBoost(LED_BOOST_300) )
  {
    return false;
  }
  if( !setGestureInt(interrupt) )
  {
    return false;
  }
  if( !setGestureMode(1u) )
  {
    return false;
  }
  if( !enablePower() )
  {
    return false;
  }
  if( !setMode(WAIT,ENABLE) )
  {
    return false;
  }
  if( !setMode(PROXIMITY,ENABLE) )
  {
    return false;
  }
  if( !setMode(GESTURE,ENABLE) )
  {
    return false;
  }
  return true;
}

/*
 * Reset gesture processing and disable gesture mode and interrupts.
 */
bool LightProximityAndGesture::disableGestureSensor(void)
{
  resetGestureParameters();
  if( !setGestureInt(DISABLE) )
  {
    return false;
  }
  if( !setGestureMode(0u) )
  {
    return false;
  }
  if( !setMode(GESTURE,DISABLE) )
  {
    return false;
  }
  return true;
}

/*
 * Read the ambient-light interrupt enable from the sensor configuration.
 */
bool LightProximityAndGesture::getAmbientLightIntState(void)
{
  uint8_t enableVal;
  if(readByte(APDS9960_ENABLE,&enableVal) == false)
  {
    return false;
  }
  return ((enableVal & AIEN_EN_MSK) >> AIEN_EN_POS);
}

/*
 * Update the ambient-light interrupt enable while preserving unrelated register bits.
 */
bool LightProximityAndGesture::setAmbientLightInt(STATE_t intState)
{
  uint8_t enableVal;
  /* get the existing gain value from sensor */
  if(readByte(APDS9960_ENABLE,&enableVal) == false)
  {
    return false;
  }
  /* update the ALS sensor gain value */
  enableVal &= ~AIEN_EN_MSK;
  enableVal |= (((uint8_t)intState & ENABLE_MSK) << AIEN_EN_POS);
  /* write new gain value to register */
  if(writeByte(APDS9960_ENABLE,enableVal) == false)
  {
    return false;
  }
  return true;
}

/*
 * Read the proximity interrupt enable from the sensor configuration.
 */
bool LightProximityAndGesture::getProximityIntState(void)
{
  uint8_t enableVal;
  if(readByte(APDS9960_ENABLE,&enableVal) == false)
  {
    return false;
  }
  return ((enableVal & PIEN_EN_MSK) >> PIEN_EN_POS);
}

/*
 * Update the proximity interrupt enable while preserving unrelated register bits.
 */
bool LightProximityAndGesture::setProximityInt(STATE_t intState)
{
  uint8_t enableVal;
  /* get the existing gain value from sensor */
  if(readByte(APDS9960_ENABLE,&enableVal) == false)
  {
    return false;
  }
  /* update the ALS sensor gain value */
  enableVal &= ~PIEN_EN_MSK;
  enableVal |= (((uint8_t)intState & ENABLE_MSK) << PIEN_EN_POS);
  /* write new gain value to register */
  if(writeByte(APDS9960_ENABLE,enableVal) == false)
  {
    return false;
  }
  return true;
}

/*
 * Read the gesture interrupt enable from the sensor configuration.
 */
bool LightProximityAndGesture::getGestureIntState(void)
{
  uint8_t gconfig4;
  if(readByte(APDS9960_GCONF4,&gconfig4) == false)
  {
    return false;
  }
  return ((gconfig4 & GES_GIEN_MSK) >> GES_GIEN_POS);
}

/*
 * Update the gesture interrupt enable while preserving unrelated register bits.
 */
bool LightProximityAndGesture::setGestureInt(STATE_t intState)
{
  uint8_t gconfig4;
  /* get the existing gain value from sensor */
  if(readByte(APDS9960_GCONF4,&gconfig4) == false)
  {
    return false;
  }
  /* update the ALS sensor gain value */
  gconfig4 &= ~GES_GIEN_MSK;
  gconfig4 |= (((uint8_t)intState & GES_GCONFIG4_MSK) << GES_GIEN_POS);
  /* write new gain value to register */
  if(writeByte(APDS9960_GCONF4,gconfig4) == false)
  {
    return false;
  }
  return true;
}

/*
 * Read the ambient-light gain code from the sensor configuration.
 */
uint8_t LightProximityAndGesture::getAmbientLightGain(void)
{
  uint8_t gain;
  if(readByte(APDS9960_CONTROL,&gain) == false)
  {
    return ERROR;
  }
  return ((gain & ALS_GAIN_MSK) >> ALS_GAIN_POS);
}

/*
 * Update the ambient-light gain code while preserving unrelated register bits.
 */
bool LightProximityAndGesture::setAmbientLightGain(uint8_t alsGain)
{
  uint8_t gain;
  /* get the existing gain value from sensor */
  if(readByte(APDS9960_CONTROL,&gain) == false)
  {
    return false;
  }
  /* update the ALS sensor gain value */
  gain &= ~ALS_GAIN_MSK;
  gain |= ((alsGain & APSD9960_GAIN_MSK) << ALS_GAIN_POS);
  /* write new gain value to register */
  if(writeByte(APDS9960_CONTROL,gain) == false)
  {
    return false;
  }
  return true;
}

/*
 * Read the proximity gain code from the sensor configuration.
 */
uint8_t LightProximityAndGesture::getProximityGain(void)
{
  uint8_t gain;
  if( !readByte(APDS9960_CONTROL,&gain) )
  {
    return ERROR;
  }
  return ((gain & PRX_GAIN_MSK) >> PRX_GAIN_POS);
}

/*
 * Update the proximity gain code while preserving unrelated register bits.
 */
bool LightProximityAndGesture::setProximityGain(uint8_t prxGain)
{
  uint8_t gain;
  /* get the existing gain value from sensor */
  if( !readByte(APDS9960_CONTROL,&gain) )
  {
    return false;
  }
  /* update the ALS sensor gain value */
  gain &= ~PRX_GAIN_MSK;
  gain |= ((prxGain & APSD9960_GAIN_MSK) << PRX_GAIN_POS);
  /* write new gain value to register */
  if( !writeByte(APDS9960_CONTROL,gain) )
  {
    return false;
  }
  return true;
}

/*
 * Read the proximity LED-drive code from the sensor configuration.
 */
uint8_t LightProximityAndGesture::getLedDrive(void)
{
  uint8_t gain;
  if(readByte(APDS9960_CONTROL,&gain) == false)
  {
    return ERROR;
  }
  return ((gain & LED_DRIVE_MSK) >> LED_DRIVE_POS);
}

/*
 * Update the proximity LED-drive code while preserving unrelated register bits.
 */
bool LightProximityAndGesture::setLedDrive(uint8_t drive)
{
  uint8_t gain;
  /* get the existing gain value from sensor */
  if(readByte(APDS9960_CONTROL,&gain) == false)
  {
    return false;
  }
  /* update the ALS sensor gain value */
  gain &= ~LED_DRIVE_MSK;
  gain |= ((drive & APSD9960_GAIN_MSK) << LED_DRIVE_POS);
  /* write new gain value to register */
  if(writeByte(APDS9960_CONTROL,gain) == false)
  {
    return false;
  }
  return true;
}

/*
 * Read the gesture gain code from the sensor configuration.
 */
uint8_t LightProximityAndGesture::getGestureGain(void)
{
  uint8_t gain;
  if(readByte(APDS9960_GCONF2,&gain) == false)
  {
    return ERROR;
  }
  return ((gain & GES_GAIN_MSK) >> GES_GAIN_POS);
}

/*
 * Update the gesture gain code while preserving unrelated register bits.
 */
bool LightProximityAndGesture::setGestureGain(uint8_t gesGain)
{
  uint8_t gain;
  /* get the existing gain value from sensor */
  if(readByte(APDS9960_GCONF2,&gain) == false)
  {
    return false;
  }
  /* update the ALS sensor gain value */
  gain &= ~GES_GAIN_MSK;
  gain |= ((gesGain & APSD9960_GAIN_MSK) << GES_GAIN_POS);
  /* write new gain value to register */
  if(writeByte(APDS9960_GCONF2,gain) == false)
  {
    return false;
  }
  return true;
}

/*
 * Read the gesture LED-drive code from the sensor configuration.
 */
uint8_t LightProximityAndGesture::getGestureLedDrive(void)
{
  uint8_t config;
  if(readByte(APDS9960_GCONF2,&config) == false)
  {
    return ERROR;
  }
  return ((config & GES_LDRIVE_MSK) >> GES_LDRIVE_POS);
}

/*
 * Update the gesture LED-drive code while preserving unrelated register bits.
 */
bool LightProximityAndGesture::setGestureLedDrive(uint8_t drive)
{
  uint8_t config;
  /* get the existing gain value from sensor */
  if(readByte(APDS9960_GCONF2,&config) == false)
  {
    return false;
  }
  /* update the ALS sensor gain value */
  config &= ~GES_LDRIVE_MSK;
  config |= ((drive & APSD9960_GAIN_MSK) << GES_LDRIVE_POS);
  /* write new gain value to register */
  if(writeByte(APDS9960_GCONF2,config) == false)
  {
    return false;
  }
  return true;
}

/*
 * Read the gesture wait-time code from the sensor configuration.
 */
uint8_t LightProximityAndGesture::getGestureWaitTime(void)
{
  uint8_t config;
  if(readByte(APDS9960_GCONF2,&config) == false)
  {
    return ERROR;
  }
  return ((config & GES_WTIME_MSK) >> GES_WTIME_POS);
}

/*
 * Update the gesture wait-time code while preserving unrelated register bits.
 */
bool LightProximityAndGesture::setGestureWaitTime(uint8_t drive)
{
  uint8_t config;
  /* get the existing gain value from sensor */
  if(readByte(APDS9960_GCONF2,&config) == false)
  {
    return false;
  }
  /* update the ALS sensor gain value */
  config &= ~GES_WTIME_MSK;
  config |= ((drive & GES_WTIME_MSK) << GES_WTIME_POS);
  /* write new gain value to register */
  if(writeByte(APDS9960_GCONF2,config) == false)
  {
    return false;
  }
  return true;
}

/*
 * Read the gesture-mode bit from the sensor configuration.
 */
uint8_t LightProximityAndGesture::getGestureMode(void)
{
  uint8_t gconfig4;
  if(readByte(APDS9960_GCONF4,&gconfig4) == false)
  {
    return ERROR;
  }
  return ((gconfig4 & GES_GMODE_MSK) >> GES_GMODE_POS);
}

/*
 * Update the gesture-mode bit while preserving unrelated register bits.
 */
bool LightProximityAndGesture::setGestureMode(uint8_t mode)
{
  uint8_t gconfig4;
  /* get the existing gain value from sensor */
  if(readByte(APDS9960_GCONF4,&gconfig4) == false)
  {
    return false;
  }
  /* update the ALS sensor gain value */
  gconfig4 &= ~GES_GMODE_MSK;
  gconfig4 |= (((uint8_t)mode & GES_GMODE_MSK) << GES_GMODE_POS);
  /* write new gain value to register */
  if(writeByte(APDS9960_GCONF4,gconfig4) == false)
  {
    return false;
  }
  return true;
}

/*
 * Read the low ambient-light threshold as raw 16-bit ADC counts.
 */
bool LightProximityAndGesture::getLightIntLowThreshold(uint16_t *threshold)
{
  uint8_t data[2];
  if(readByte(APDS9960_AILTL,&data[0u]) == false)
  {
    return false;
  }
  if(readByte(APDS9960_AILTH,&data[1u]) == false)
  {
    return false;
  }
  /* update the threshold */
  *threshold = (((uint16_t)data[1u] << 8u) | data[0u]);
  return true;
}

/*
 * Write the low ambient-light threshold as raw 16-bit ADC counts.
 */
bool LightProximityAndGesture::setLightIntLowThreshold(uint16_t threshold)
{
  uint8_t data = (uint8_t)(threshold & 0xFFu);
  if(writeByte(APDS9960_AILTL,data) == false)
  {
    return false;
  }
  data = (uint8_t)((threshold >> 8u) & 0xFFu);
  if(writeByte(APDS9960_AILTH,data) == false)
  {
    return false;
  }
  return true;
}

/*
 * Read the high ambient-light threshold as raw 16-bit ADC counts.
 */
bool LightProximityAndGesture::getLightIntHighThreshold(uint16_t *threshold)
{
  uint8_t data[2];
  if(readByte(APDS9960_AIHTL,&data[0u]) == false)
  {
    return false;
  }
  if(readByte(APDS9960_AIHTH,&data[1u]) == false)
  {
    return false;
  }
  /* update the threshold */
  *threshold = (((uint16_t)data[1u] << 8u) | data[0u]);
  return true;
}

/*
 * Write the high ambient-light threshold as raw 16-bit ADC counts.
 */
bool LightProximityAndGesture::setLightIntHighThreshold(uint16_t threshold)
{
  uint8_t data = (uint8_t)(threshold & 0xFFu);
  if(writeByte(APDS9960_AIHTL,data) == false)
  {
    return false;
  }
  data = (uint8_t)((threshold >> 8u) & 0xFFu);
  if(writeByte(APDS9960_AIHTH,data) == false)
  {
    return false;
  }
  return true;
}

/*
 * Read the low proximity interrupt threshold in raw counts.
 */
bool LightProximityAndGesture::getProximityIntLowThreshold(uint8_t *threshold)
{
  *threshold = 0u;
  return readByte(APDS9960_PILT,threshold);
}

/*
 * Write the low proximity interrupt threshold in raw counts.
 */
bool LightProximityAndGesture::setProximityIntLowThreshold(uint8_t threshold)
{
  return writeByte(APDS9960_PILT,threshold);
}

/*
 * Read the high proximity interrupt threshold in raw counts.
 */
bool LightProximityAndGesture::getProximityIntHighThreshold(uint8_t *threshold)
{
  *threshold = 0u;
  return readByte(APDS9960_PIHT,threshold);
}

/*
 * Write the high proximity interrupt threshold in raw counts.
 */
bool LightProximityAndGesture::setProximityIntHighThreshold(uint8_t threshold)
{
  return writeByte(APDS9960_PIHT,threshold);
}

/*
 * Discard the current gesture batch, accumulated deltas, and decoded direction.
 */
void LightProximityAndGesture::resetGestureParameters(void)
{
  _gesture_data.index = 0;
  _gesture_data.total_gestures = 0;

  _gesture_ud_delta = 0;
  _gesture_lr_delta = 0;

  _gesture_ud_count = 0;
  _gesture_lr_count = 0;

  _gesture_near_count = 0;
  _gesture_far_count = 0;

  _gesture_state = 0;
  _gesture_motion = DIR_NONE;
}

/*
 * Compare valid first/last directional samples and accumulate near/far or swipe evidence.
 */
bool LightProximityAndGesture::processGestureData(void)
{
  uint8_t u_first = 0;
  uint8_t d_first = 0;
  uint8_t l_first = 0;
  uint8_t r_first = 0;
  uint8_t u_last = 0;
  uint8_t d_last = 0;
  uint8_t l_last = 0;
  uint8_t r_last = 0;
  int ud_ratio_first;
  int lr_ratio_first;
  int ud_ratio_last;
  int lr_ratio_last;
  int ud_delta;
  int lr_delta;
  int i;

  /* If we have less than 4 total gestures, that's not enough */
  if( _gesture_data.total_gestures <= 4 || _gesture_data.total_gestures > 32 )
  {
    return false;
  }

  /* Check to make sure our data isn't out of bounds */
  if( (_gesture_data.total_gestures <= 32) && \
      (_gesture_data.total_gestures > 0) )
  {
    /* Find the first value in U/D/L/R above the threshold */
    for( i = 0; i < _gesture_data.total_gestures; i++ )
    {
      if( (_gesture_data.u_data[i] > GESTURE_THRESHOLD_OUT) &&
          (_gesture_data.d_data[i] > GESTURE_THRESHOLD_OUT) &&
          (_gesture_data.l_data[i] > GESTURE_THRESHOLD_OUT) &&
          (_gesture_data.r_data[i] > GESTURE_THRESHOLD_OUT) )
      {
          u_first = _gesture_data.u_data[i];
          d_first = _gesture_data.d_data[i];
          l_first = _gesture_data.l_data[i];
          r_first = _gesture_data.r_data[i];
          break;
      }
    }
    /* If one of the _first values is 0, then there is no good data */
    if( (u_first == 0) || (d_first == 0) || \
        (l_first == 0) || (r_first == 0) ) {
              return false;
    }
    /* Find the last value in U/D/L/R above the threshold */
    for( i = _gesture_data.total_gestures - 1; i >= 0; i-- )
    {
      if( (_gesture_data.u_data[i] > GESTURE_THRESHOLD_OUT) &&
          (_gesture_data.d_data[i] > GESTURE_THRESHOLD_OUT) &&
          (_gesture_data.l_data[i] > GESTURE_THRESHOLD_OUT) &&
          (_gesture_data.r_data[i] > GESTURE_THRESHOLD_OUT) )
      {
          u_last = _gesture_data.u_data[i];
          d_last = _gesture_data.d_data[i];
          l_last = _gesture_data.l_data[i];
          r_last = _gesture_data.r_data[i];
          break;
      }
    }
  }
  /* Calculate the first vs. last ratio of up/down and left/right */
  ud_ratio_first = ((u_first - d_first) * 100) / (u_first + d_first);
  lr_ratio_first = ((l_first - r_first) * 100) / (l_first + r_first);
  ud_ratio_last = ((u_last - d_last) * 100) / (u_last + d_last);
  lr_ratio_last = ((l_last - r_last) * 100) / (l_last + r_last);

  /* Determine the difference between the first and last ratios */
  ud_delta = ud_ratio_last - ud_ratio_first;
  lr_delta = lr_ratio_last - lr_ratio_first;

  /* Accumulate the UD and LR delta values */
  _gesture_ud_delta += ud_delta;
  _gesture_lr_delta += lr_delta;

  /* Determine U/D gesture */
  if( _gesture_ud_delta >= GESTURE_SENSITIVITY_1 ) {
      _gesture_ud_count = 1;
  } else if( _gesture_ud_delta <= -GESTURE_SENSITIVITY_1 ) {
      _gesture_ud_count = -1;
  } else {
      _gesture_ud_count = 0;
  }

  /* Determine L/R gesture */
  if( _gesture_lr_delta >= GESTURE_SENSITIVITY_1 ) {
      _gesture_lr_count = 1;
  } else if( _gesture_lr_delta <= -GESTURE_SENSITIVITY_1 ) {
      _gesture_lr_count = -1;
  } else {
      _gesture_lr_count = 0;
  }

  /* Determine Near/Far gesture */
  if( (_gesture_ud_count == 0) && (_gesture_lr_count == 0) ) {
    if( (abs(ud_delta) < GESTURE_SENSITIVITY_2) && \
        (abs(lr_delta) < GESTURE_SENSITIVITY_2) )
    {
      if( (ud_delta == 0) && (lr_delta == 0) ) {
          _gesture_near_count++;
      } else if( (ud_delta != 0) || (lr_delta != 0) ) {
          _gesture_far_count++;
      }

      if( (_gesture_near_count >= 10) && (_gesture_far_count >= 2) )
      {
        if( (ud_delta == 0) && (lr_delta == 0) ) {
            _gesture_state = NEAR_STATE;
        } else if( (ud_delta != 0) && (lr_delta != 0) ) {
            _gesture_state = FAR_STATE;
        }
        return true;
      }
    }
  } else {
    if( (abs(ud_delta) < GESTURE_SENSITIVITY_2) && \
        (abs(lr_delta) < GESTURE_SENSITIVITY_2) ) {
        if( (ud_delta == 0) && (lr_delta == 0) ) {
            _gesture_near_count++;
        }

        if( _gesture_near_count >= 10 ) {
            _gesture_ud_count = 0;
            _gesture_lr_count = 0;
            _gesture_ud_delta = 0;
            _gesture_lr_delta = 0;
        }
    }
  }
  return false;
}

/*
 * Convert accumulated directional deltas and near/far state into a gesture code.
 */
bool LightProximityAndGesture::decodeGesture(void)
{
  /* Return if near or far event is detected */
   if( _gesture_state == NEAR_STATE ) {
       _gesture_motion = DIR_NEAR;
       return true;
   } else if ( _gesture_state == FAR_STATE ) {
       _gesture_motion = DIR_FAR;
       return true;
   }

   /* Determine swipe direction */
   if( (_gesture_ud_count == -1) && (_gesture_lr_count == 0) ) {
       _gesture_motion = DIR_UP;
   } else if( (_gesture_ud_count == 1) && (_gesture_lr_count == 0) ) {
       _gesture_motion = DIR_DOWN;
   } else if( (_gesture_ud_count == 0) && (_gesture_lr_count == 1) ) {
       _gesture_motion = DIR_RIGHT;
   } else if( (_gesture_ud_count == 0) && (_gesture_lr_count == -1) ) {
       _gesture_motion = DIR_LEFT;
   } else if( (_gesture_ud_count == -1) && (_gesture_lr_count == 1) ) {
       if( abs(_gesture_ud_delta) > abs(_gesture_lr_delta) ) {
           _gesture_motion = DIR_UP;
       } else {
           _gesture_motion = DIR_RIGHT;
       }
   } else if( (_gesture_ud_count == 1) && (_gesture_lr_count == -1) ) {
       if( abs(_gesture_ud_delta) > abs(_gesture_lr_delta) ) {
           _gesture_motion = DIR_DOWN;
       } else {
           _gesture_motion = DIR_LEFT;
       }
   } else if( (_gesture_ud_count == -1) && (_gesture_lr_count == -1) ) {
       if( abs(_gesture_ud_delta) > abs(_gesture_lr_delta) ) {
           _gesture_motion = DIR_UP;
       } else {
           _gesture_motion = DIR_LEFT;
       }
   } else if( (_gesture_ud_count == 1) && (_gesture_lr_count == 1) ) {
       if( abs(_gesture_ud_delta) > abs(_gesture_lr_delta) ) {
           _gesture_motion = DIR_DOWN;
       } else {
           _gesture_motion = DIR_RIGHT;
       }
   } else {
       return false;
   }
   return true;
}

/*
 * Read the LED-current boost code from the sensor configuration.
 */
uint8_t LightProximityAndGesture::getLedBoost(void)
{
  uint8_t config2;
  if( !readByte(APDS9960_CONFIG2, &config2) )
  {
    return ERROR;
  }
  return ((config2 & CFG2_LED_BOOST_MSK) >> CFG2_LED_BOOST_POS);
}

/*
 * Update the LED-current boost code while preserving unrelated register bits.
 */
bool LightProximityAndGesture::setLedBoost(uint8_t boost)
{
  uint8_t config2;
  if( !readByte(APDS9960_CONFIG2, &config2) )
  {
    return false;
  }
  config2 &= ~CFG2_LED_BOOST_MSK;
  config2 |= ((boost & 0x03u) << CFG2_LED_BOOST_POS);
  if( !writeByte(APDS9960_CONFIG2, config2) )
  {
    return false;
  }
  return true;
}

/*
 * Send the dedicated command that clears the ambient-light interrupt latch.
 */
bool LightProximityAndGesture::clearAmbientLightInt(void)
{
  if( !writeByte(APDS9960_AICLEAR) )
  {
    return false;
  }
  return true;
}

/*
 * Send the dedicated command that clears the proximity interrupt latch.
 */
bool LightProximityAndGesture::clearProximityInt(void)
{
  if( !writeByte(APDS9960_PICLEAR))
  {
    return false;
  }
  return true;
}

/*
 * Read the clear channel as raw 16-bit counts into caller-provided storage.
 */
bool LightProximityAndGesture::readAmbientLight(uint16_t *val)
{
  uint8_t data[2u];
  if (!readByte(APDS9960_CDATAL,&data[0u]))
  {
    return false;
  }
  if (!readByte(APDS9960_CDATAH,&data[1u]))
  {
    return false;
  }
  *val = (((uint16_t)data[1u] << 8u)|data[0]);
  return true;
}

/*
 * Read the red channel as raw 16-bit counts into caller-provided storage.
 */
bool LightProximityAndGesture::readRedLight(uint16_t *val)
{
  uint8_t data[2u];
  if (!readByte(APDS9960_RDATAL,&data[0u]))
  {
    return false;
  }
  if (!readByte(APDS9960_RDATAH,&data[1u]))
  {
    return false;
  }
  *val = (((uint16_t)data[1u] << 8u)|data[0]);
  return true;
}

/*
 * Read the green channel as raw 16-bit counts into caller-provided storage.
 */
bool LightProximityAndGesture::readGreenLight(uint16_t *val)
{
  uint8_t data[2u];
  if (!readByte(APDS9960_GDATAL,&data[0u]))
  {
    return false;
  }
  if (!readByte(APDS9960_GDATAH,&data[1u]))
  {
    return false;
  }
  *val = (((uint16_t)data[1u] << 8u)|data[0]);
  return true;
}

/*
 * Read the blue channel as raw 16-bit counts into caller-provided storage.
 */
bool LightProximityAndGesture::readBlueLight(uint16_t *val)
{
  uint8_t data[2u];
  if (!readByte(APDS9960_BDATAL,&data[0u]))
  {
    return false;
  }
  if (!readByte(APDS9960_BDATAH,&data[1u]))
  {
    return false;
  }
  *val = (((uint16_t)data[1u] << 8u)|data[0]);
  return true;
}

/*
 * Read one raw proximity byte; counts are not a calibrated distance.
 */
bool LightProximityAndGesture::readProximity(uint8_t *val)
{
  return readByte(APDS9960_PDATA,val);
}

/*
 * Poll the gesture-valid flag before draining the FIFO.
 */
bool LightProximityAndGesture::isGestureAvailable(void)
{
  uint8_t gsts;
  if (!readByte(APDS9960_GSTATUS,&gsts))
  {
    return false;
  }
  return (bool)((gsts&GSTS_GVALID_MSK) >> GSTS_GVALID_POS);
}

/*
 * Drain directional FIFO batches and decode a gesture; NONE and TIMEOUT are non-gesture results.
 */
int LightProximityAndGesture::readGesture(void)
{
  uint8_t fifo_level = 0;
  uint8_t fifo_data[128];
  uint8_t gstatus;
  int bytes_read = 0;
  int motion;
  int i;
  int timeCnt = 0;

  resetGestureParameters();
  /* Make sure that power and gesture is on and data is valid */
  if( !isGestureAvailable() || (getMode() & (GEN_EN_MSK|PON_EN_MSK)) != (GEN_EN_MSK|PON_EN_MSK) ) {
      return DIR_NONE;
  }

  /* Keep looping as long as gesture data is valid */
  for(;;)
  {
    /* Wait some time to collect next batch of FIFO data */
    delay_ms(FIFO_PAUSE_TIME);

    /* increment timeout count */
    timeCnt++;

    /* Get the contents of the STATUS register. Is data still valid? */
    if( !readByte(APDS9960_GSTATUS, &gstatus) ) {
        resetGestureParameters();
        return ERROR;
    }

    /* If we have valid data, read in FIFO */
    if( ((gstatus & GSTS_GVALID_MSK) == GSTS_GVALID_MSK ) && (timeCnt <= 10u)) {

        /* Read the current FIFO level */
        if( !readByte(APDS9960_GFLVL, &fifo_level) ) {
            resetGestureParameters();
        return ERROR;
        }

        /* If there's stuff in the FIFO, read it into our data block */
        if(fifo_level > 32) { resetGestureParameters(); return ERROR; }
        if( fifo_level > 0) {
            bytes_read = readMultiBytes(APDS9960_GFIFO_U,
                                        (fifo_level * 4),
                                        (uint8_t*)fifo_data);
            if( bytes_read != fifo_level * 4 ) {
                resetGestureParameters();
        return ERROR;
            }

            /* If at least 1 set of data, sort the data into U/D/L/R */
            if( bytes_read >= 4 ) {
                for( i = 0; i < bytes_read; i += 4 ) {
                    _gesture_data.u_data[_gesture_data.index] = \
                                                        fifo_data[i + 0];
                    _gesture_data.d_data[_gesture_data.index] = \
                                                        fifo_data[i + 1];
                    _gesture_data.l_data[_gesture_data.index] = \
                                                        fifo_data[i + 2];
                    _gesture_data.r_data[_gesture_data.index] = \
                                                        fifo_data[i + 3];
                    _gesture_data.index++;
                    _gesture_data.total_gestures++;
                }

                /* Filter and process gesture data. Decode near/far state */
                if( processGestureData() ) {
                    if( decodeGesture() ) {
                        //***TODO: U-Turn Gestures
                    }
                }

                /* Reset data */
                _gesture_data.index = 0;
                _gesture_data.total_gestures = 0;
            }
        }
    } else {
        if(timeCnt >= 10u)
        {
            motion = TIMEOUT;
        }
        else
        {
            /* Determine best guessed gesture and clean up */
            delay_ms(FIFO_PAUSE_TIME);
            decodeGesture();
            motion = _gesture_motion;
            resetGestureParameters();
        }
        resetGestureParameters();
        return motion;
    }
  }
}

/*
 * Return refreshed RGB percentages in the object buffer; check lightReadingValid() before use.
 */
uint16_t *LightProximityAndGesture::getRGBProportion(bool print)
{
  uint8_t data[6];
  _lightReadingValid = false;
  memset(_color, 0, sizeof(_color));
  if(readMultiBytes(APDS9960_RDATAL, sizeof(data), data) != sizeof(data)) return _color;
  uint16_t raw[3];
  uint32_t sum = 0;
  for(uint8_t i = 0; i < 3; ++i) { raw[i] = data[2*i] | ((uint16_t)data[2*i+1] << 8); sum += raw[i]; }
  for(uint8_t i = 0; i < 3; ++i) _color[i] = sum ? (uint32_t)raw[i] * 100 / sum : 0;
  _lightReadingValid = true;
  if(print) {
    Serial.print("RGB (%): "); Serial.print(_color[0]); Serial.print(", ");
    Serial.print(_color[1]); Serial.print(", "); Serial.println(_color[2]);
  }
  return _color;
}

/*
 * Return raw clear-channel counts; use lightReadingValid() to distinguish zero from failure.
 */
uint16_t LightProximityAndGesture::getAmbientLight(bool print)
{
    _lightReadingValid = false;
    uint16_t ambient_light;
    if(readAmbientLight(&ambient_light))
    {
        if(print)
        {
            Serial.print("Ambient Light: ");
            Serial.print(ambient_light);
            Serial.println(" counts");
        }
        _lightReadingValid = true;
        return ambient_light;
    }
    return 0u;
}

/*
 * Convert one valid RGBC sample using the Adafruit APDS9960 RGB-to-lux approximation.
 * Normalize exposure to MYOSA's nominal 103 ms / 4x setup; absolute accuracy needs calibration.
 */
bool LightProximityAndGesture::readAmbientLightLux(float *lux)
{
  uint8_t enabled, status, integration, control;
  if(!readByte(APDS9960_ENABLE, &enabled) || (enabled & 0x03) != 0x03 ||
     !readByte(APDS9960_STATUS, &status) || !(status & 0x01))
    return false;
  if(status & 0x80)
  {
    // Clear only the ALS latch; preserve any pending proximity interrupt.
    writeByte(APDS9960_CICLEAR);
    return false;
  }
  if(!readByte(APDS9960_ATIME, &integration) || !readByte(APDS9960_CONTROL, &control))
    return false;
  uint8_t data[8];
  if(readMultiBytes(APDS9960_CDATAL, sizeof(data), data) != sizeof(data))
    return false;
  uint16_t channels[4];
  const uint16_t cycles = 256u - integration;
  const uint32_t limit = (uint32_t)cycles * 1025u;
  const uint16_t saturation = limit < 65535u ? limit : 65535u;
  for(uint8_t i = 0; i < 4; ++i)
  {
    channels[i] = data[2 * i] | ((uint16_t)data[2 * i + 1] << 8);
    if(channels[i] >= saturation)
      return false;
  }
  // Coefficients use raw red, green and blue channels, never RGB percentages.
  const float weighted = -0.32466f * channels[1] + 1.57837f * channels[2] - 0.73191f * channels[3];
  const uint8_t gains[4] = {1, 4, 16, 64};
  const float exposure = (float)cycles * gains[control & ALS_GAIN_MSK];
  *lux = fmaxf(0.0f, weighted * (37.0f * 4.0f) / exposure);
  return true;
}

/*
 * Return illuminance in lux, or NAN for unavailable, saturated or failed measurements.
 */
float LightProximityAndGesture::getAmbientLightLux(bool print)
{
  float lux = NAN;
  _lightReadingValid = readAmbientLightLux(&lux);
  if(print)
  {
    Serial.print("Ambient Light (lux): ");
    if(_lightReadingValid)
      Serial.println(lux, 2);
    else
      Serial.println("Unavailable");
  }
  return lux;
}

/*
 * Refresh normalized RGB proportions and return the red percentage.
 */
uint16_t LightProximityAndGesture::getRedProportion(void)
{
  return getRGBProportion(false)[0];
}

/*
 * Refresh normalized RGB proportions and return the green percentage.
 */
uint16_t LightProximityAndGesture::getGreenProportion(void)
{
  return getRGBProportion(false)[1];
}

/*
 * Refresh normalized RGB proportions and return the blue percentage.
 */
uint16_t LightProximityAndGesture::getBlueProportion(void)
{
  return getRGBProportion(false)[2];
}

/*
 * Return raw proximity counts as a float, or NAN on transfer failure.
 */
float LightProximityAndGesture::getProximity(bool print)
{
    uint8_t proximity_data = 0;
    if(readProximity(&proximity_data))
    {
        if(print)
        {
            Serial.print("Proximity: ");
            Serial.print((float)proximity_data,2);
            Serial.println();
        }
        return (float)proximity_data;
    }
    return NAN;
}

/*
 * Read and decode a gesture into a static text buffer, overwritten by the next call.
 */
char *LightProximityAndGesture::getGesture(bool print)
{
  // Writable storage preserves the existing char* API without exposing a string literal.
  static char gesture[8];
  const int direction = readGesture();
  const char *name = "NONE";
  switch(direction) {
    case DIR_UP: name = "UP"; break;
    case DIR_DOWN: name = "DOWN"; break;
    case DIR_LEFT: name = "LEFT"; break;
    case DIR_RIGHT: name = "RIGHT"; break;
    case DIR_NEAR: name = "NEAR"; break;
    case DIR_FAR: name = "FAR"; break;
    case TIMEOUT: name = "TIMEOUT"; break;
    case ERROR: name = "ERROR"; break;
  }
  strcpy(gesture, name);
  if(print) { Serial.print("Gesture: "); Serial.println(gesture); }
  return gesture;
}

/***********************************************************************************************
* Platform dependent routines. Change these functions implementation based on microcontroller *
***********************************************************************************************/
/*
 * Initialize the shared Wire bus at 100 kHz; normal sketches configure Wire before using drivers.
 */
void LightProximityAndGesture::i2c_init(void)
{
  Wire.begin();
  Wire.setClock(100000);
}

/*
 * Select one register and require a complete one-byte response.
 */
bool LightProximityAndGesture::readByte(uint8_t reg, uint8_t *in)
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
 * Read a complete FIFO/register block; return its byte count, or -1 on an invalid/short transfer.
 */
int16_t LightProximityAndGesture::readMultiBytes(uint8_t reg, uint8_t length, uint8_t *in)
{
  if(!in || length == 0 || length > 128) return -1;
  Wire.beginTransmission(_i2cSlaveAddress);
  Wire.write(reg);
  if(Wire.endTransmission(true) != 0) return -1;
  if(Wire.requestFrom(_i2cSlaveAddress, length, (uint8_t)1) != length) return -1;
  for(uint16_t i = 0; i < length; ++i) in[i] = Wire.read();
  return length;
}

/*
 * Read from the current device pointer and require the requested number of bytes.
 */
bool LightProximityAndGesture::readMultiBytes(uint8_t length, uint8_t *in)
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
bool LightProximityAndGesture::writeByte(uint8_t reg)
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
bool LightProximityAndGesture::writeByte(uint8_t reg, uint8_t val)
{
  Wire.beginTransmission(_i2cSlaveAddress);
  Wire.write(reg);
  Wire.write(val);
  if(Wire.endTransmission(true) != 0) return false;
  if(reg >= 0x80 && reg < 0xB0) {
    _configValues[reg - 0x80] = val;
    _configValid[(reg - 0x80) / 8] |= 1u << ((reg - 0x80) % 8);
  }
  return true;
}

/*
 * Probe the I2C address without sending register data.
 */
bool LightProximityAndGesture::writeAddress(void)
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
void LightProximityAndGesture::delay_ms(uint16_t ms)
{
  delay(ms);
}

uint8_t LightProximityAndGesture::getDeviceId(void)
{
  uint8_t id;
  return readByte(APDS9960_ID, &id) ? id : ERROR;
}
