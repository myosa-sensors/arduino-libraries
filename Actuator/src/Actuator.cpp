/*
  This code is developed under the MYOSA (LearnTheEasyWay) initiative of MakeSense EduTech and Pegasus Automation.
  Code has been derived from internet sources and component datasheets.
  Existing readily-available libraries would have been used "AS IS" and modified for ease of learning purpose.
  
  Synopsis of Actuator Board
  The MYOSA actuator board uses the PCA9536 four-bit I2C GPIO expander at 0x41.
  IO0 controls the AC switching output; IO1 controls the buzzer.
  IO2 and IO3 are available for user configuration.
  Output latches are set before changing pin direction to avoid startup pulses.

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

#include "Actuator.h"

/*
 * Initialize the device address and local state before any measurement is requested.
 */
Actuator::Actuator()
{
  _i2cSlaveAddress = Actuator_I2C_ADDRESS;
}

/*
 * Check whether the PCA9536 acknowledges its I2C address.
 */
bool Actuator::ping(void)
{
  return writeAddress();
}

/*
 * Read one pin direction; invalid pins and transfer failures return IO_INPUT.
 */
PIN_MODE_t Actuator::getMode(PCA_PIN_t pin)
{
  uint8_t mode;
  if((unsigned)pin > 3 || !readByte(CONFIG_REG, &mode)) return IO_INPUT;
  return (PIN_MODE_t)((mode >> pin) & 1u);
}

/*
 * Read an input pin or output latch according to direction; failures return IO_LOW.
 */
PIN_STATE_t Actuator::getState(PCA_PIN_t pin)
{
  uint8_t mode, state;
  if((unsigned)pin > 3 || !readByte(CONFIG_REG, &mode)) return IO_LOW;
  if(!readByte((mode & (1u << pin)) ? INPUT_REG : OUTPUT_REG, &state)) return IO_LOW;
  return (PIN_STATE_t)((state >> pin) & 1u);
}

/*
 * Read one input-polarity bit; invalid pins and transfer failures return zero.
 */
PIN_POLARITY_t Actuator::getPolarity(PCA_PIN_t pin)
{
  uint8_t polarity;
  if((unsigned)pin > 3 || !readByte(POLARITY_REG, &polarity)) return (PIN_POLARITY_t)0;
  return (PIN_POLARITY_t)((polarity >> pin) & 1u);
}

/*
 * Change pin direction after validating the enum; a failed read prevents a write. Applies to one pin.
 */
void Actuator::setMode(PCA_PIN_t pin, PIN_MODE_t newMode)
{
  if((unsigned)pin > 3 || (unsigned)newMode > 1) return;
  uint8_t mode_all;
  if(!readByte(CONFIG_REG,&mode_all)) return;
  mode_all &= ~(1u << pin);
  mode_all |= (newMode << pin);
  writeByte(CONFIG_REG,mode_all);
}

/*
 * Change pin direction after validating the enum; a failed read prevents a write. Applies to all four pins.
 */
void Actuator::setMode(PIN_MODE_t newMode)
{
  if((unsigned)newMode > 1) return;
  uint8_t mode_all = newMode ? ALL_INPUT : ALL_OUTPUT;
  writeByte(CONFIG_REG,mode_all);
}

/*
 * Update output latches without changing pin direction; invalid enum values are ignored. Applies to one pin.
 */
void Actuator::setState(PCA_PIN_t pin, PIN_STATE_t newState)
{
  if((unsigned)pin > 3 || (unsigned)newState > 1) return;
  uint8_t state_all;
  if(!readByte(OUTPUT_REG,&state_all)) return;
  state_all &= ~(1u << pin);
  state_all |= (newState << pin);
  writeByte(OUTPUT_REG,state_all);
}

/*
 * Update output latches without changing pin direction; invalid enum values are ignored. Applies to all four pins.
 */
void Actuator::setState(PIN_STATE_t newState)
{
  if((unsigned)newState > 1) return;
  uint8_t state_all = newState ? ALL_HIGH : ALL_LOW;
  writeByte(OUTPUT_REG,state_all);
}

/*
 * Invert the selected output latch bits; leave the register untouched if its read fails. Applies to one pin.
 */
void Actuator::toggleState(PCA_PIN_t pin)
{
  if((unsigned)pin > 3) return;
  uint8_t state_all;
  if(!readByte(OUTPUT_REG,&state_all)) return;
  state_all ^= (1u << pin);
  writeByte(OUTPUT_REG,state_all);
}

/*
 * Invert the selected output latch bits; leave the register untouched if its read fails. Applies to all four pins.
 */
void Actuator::toggleState(void)
{
  uint8_t state_all;
  if(!readByte(OUTPUT_REG,&state_all)) return;
  state_all ^= 0xFFu;
  writeByte(OUTPUT_REG,state_all);
}

/*
 * Change polarity only for pins configured as inputs, preserving the remaining bits. Applies to one pin.
 */
void Actuator::setPolarity(PCA_PIN_t pin, PIN_POLARITY_t newPolarity)
{
  uint8_t mode, polarity;
  if((unsigned)pin > 3 || (unsigned)newPolarity > 1) return;
  if(!readByte(CONFIG_REG, &mode) || !(mode & (1u << pin))) return;
  if(!readByte(POLARITY_REG, &polarity)) return;
  polarity = (polarity & ~(1u << pin)) | ((uint8_t)newPolarity << pin);
  writeByte(POLARITY_REG, polarity);
}

/*
 * Change polarity only for pins configured as inputs, preserving the remaining bits. Applies to all four pins.
 */
void Actuator::setPolarity(PIN_POLARITY_t newPolarity)
{
  if((unsigned)newPolarity > 1) return;
  uint8_t polarity_all;
  uint8_t polarity_msk;
  uint8_t polarity_new;
  if(!readByte(POLARITY_REG,&polarity_all)) return;
  if(!readByte(CONFIG_REG,&polarity_msk)) return;
  polarity_new = newPolarity ? ALL_INVERTED : ALL_NON_INVERTED;
  writeByte(POLARITY_REG,(polarity_all & ~polarity_msk) | (polarity_new & polarity_msk));
}

/***********************************************************************************************
 * Platform dependent routines. Change these functions implementation based on microcontroller *
 ***********************************************************************************************/
/*
 * Initialize the shared Wire bus at 100 kHz; normal sketches configure Wire before using drivers.
 */
void Actuator::i2c_init(void)
{
  Wire.begin();
  Wire.setClock(100000);
}

/*
 * Select one register and require a complete one-byte response.
 */
bool Actuator::readByte(uint8_t reg, uint8_t *in)
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
 * Probe the I2C address without sending register data.
 */
bool Actuator::writeAddress(void)
{
  Wire.beginTransmission((uint8_t)_i2cSlaveAddress);
  if (Wire.endTransmission(true) == 0)
  {
    return true;
  }
  return false;
}

/*
 * Send a register/command byte and its value and report the transfer result.
 */
bool Actuator::writeByte(uint8_t reg, uint8_t val)
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
