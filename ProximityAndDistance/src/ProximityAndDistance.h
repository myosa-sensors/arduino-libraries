/*
  This code is developed under the MYOSA (LearnTheEasyWay) initiative of MakeSense EduTech and Pegasus Automation.
  Code has been derived from internet sources and component datasheets.
  Existing readily-available libraries would have been used "AS IS" and modified for ease of learning purpose.

  Synopsis of Proximity and Distance sensor
  STMicroelectronics VL53L0X time-of-flight ranging; default I2C address 0x29.
  Provides single and continuous distance measurements in millimeters.
  Uses a caller-initialized Wire bus, bounded waits, and checked transfers.
  The bundled core is adapted from Pololu and ST; see LICENSE.txt.

  NOTE
  All information, including URL references, is subject to change without prior notice.
  Please always use the latest versions of software-release for best performance.
  Unless required by applicable law or agreed to in writing, this software is distributed on an
  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied

  Modifications
  10 September, 2026 by Pegasus Automation
  (as a part of MYOSA Initiative)

  Contact Team MYOSA for feedback, issues, or update requests.
  Email: myosa.event@gmail.com
*/

#ifndef MYOSA_PROXIMITY_AND_DISTANCE_H
#define MYOSA_PROXIMITY_AND_DISTANCE_H

#include <stdint.h>
#include <Arduino.h>
#include <math.h>
#include <Wire.h>
#include "detail/Vl53l0xCore.h"

#define VL53L0X_I2C_ADDRESS 0x29u
#define VL53L0X_EXPECTED_MODEL_ID 0xEEu

// Register map
#define VL53L0X_REG_SYSRANGE_START 0x00u
#define VL53L0X_REG_SYSTEM_THRESH_HIGH 0x0Cu
#define VL53L0X_REG_SYSTEM_THRESH_LOW 0x0Eu
#define VL53L0X_REG_SYSTEM_SEQUENCE_CONFIG 0x01u
#define VL53L0X_REG_SYSTEM_RANGE_CONFIG 0x09u
#define VL53L0X_REG_SYSTEM_INTERMEASUREMENT_PERIOD 0x04u
#define VL53L0X_REG_SYSTEM_INTERRUPT_CONFIG_GPIO 0x0Au
#define VL53L0X_REG_GPIO_HV_MUX_ACTIVE_HIGH 0x84u
#define VL53L0X_REG_SYSTEM_INTERRUPT_CLEAR 0x0Bu
#define VL53L0X_REG_RESULT_INTERRUPT_STATUS 0x13u
#define VL53L0X_REG_RESULT_RANGE_STATUS 0x14u
#define VL53L0X_REG_RESULT_CORE_AMBIENT_WINDOW_EVENTS_RTN 0xBCu
#define VL53L0X_REG_RESULT_CORE_RANGING_TOTAL_EVENTS_RTN 0xC0u
#define VL53L0X_REG_RESULT_CORE_AMBIENT_WINDOW_EVENTS_REF 0xD0u
#define VL53L0X_REG_RESULT_CORE_RANGING_TOTAL_EVENTS_REF 0xD4u
#define VL53L0X_REG_RESULT_PEAK_SIGNAL_RATE_REF 0xB6u
#define VL53L0X_REG_ALGO_PART_TO_PART_RANGE_OFFSET_MM 0x28u
#define VL53L0X_REG_I2C_SLAVE_DEVICE_ADDRESS 0x8Au
#define VL53L0X_REG_MSRC_CONFIG_CONTROL 0x60u
#define VL53L0X_REG_PRE_RANGE_CONFIG_MIN_SNR 0x27u
#define VL53L0X_REG_PRE_RANGE_CONFIG_VALID_PHASE_LOW 0x56u
#define VL53L0X_REG_PRE_RANGE_CONFIG_VALID_PHASE_HIGH 0x57u
#define VL53L0X_REG_PRE_RANGE_MIN_COUNT_RATE_RTN_LIMIT 0x64u
#define VL53L0X_REG_FINAL_RANGE_CONFIG_MIN_SNR 0x67u
#define VL53L0X_REG_FINAL_RANGE_CONFIG_VALID_PHASE_LOW 0x47u
#define VL53L0X_REG_FINAL_RANGE_CONFIG_VALID_PHASE_HIGH 0x48u
#define VL53L0X_REG_PRE_RANGE_CONFIG_SIGMA_THRESH_HI 0x61u
#define VL53L0X_REG_PRE_RANGE_CONFIG_SIGMA_THRESH_LO 0x62u
#define VL53L0X_REG_PRE_RANGE_CONFIG_VCSEL_PERIOD 0x50u
#define VL53L0X_REG_PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI 0x51u
#define VL53L0X_REG_PRE_RANGE_CONFIG_TIMEOUT_MACROP_LO 0x52u
#define VL53L0X_REG_HISTOGRAM_CONFIG_INITIAL_PHASE_SELECT 0x33u
#define VL53L0X_REG_HISTOGRAM_CONFIG_READOUT_CTRL 0x55u
#define VL53L0X_REG_FINAL_RANGE_CONFIG_VCSEL_PERIOD 0x70u
#define VL53L0X_REG_FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI 0x71u
#define VL53L0X_REG_FINAL_RANGE_CONFIG_TIMEOUT_MACROP_LO 0x72u
#define VL53L0X_REG_CROSSTALK_COMPENSATION_PEAK_RATE_MCPS 0x20u
#define VL53L0X_REG_MSRC_CONFIG_TIMEOUT_MACROP 0x46u
#define VL53L0X_REG_SOFT_RESET_GO2_SOFT_RESET_N 0xBFu
#define VL53L0X_REG_IDENTIFICATION_MODEL_ID 0xC0u
#define VL53L0X_REG_IDENTIFICATION_REVISION_ID 0xC2u
#define VL53L0X_REG_OSC_CALIBRATE_VAL 0xF8u
#define VL53L0X_REG_GLOBAL_CONFIG_VCSEL_WIDTH 0x32u
#define VL53L0X_REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_0 0xB0u
#define VL53L0X_REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_1 0xB1u
#define VL53L0X_REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_2 0xB2u
#define VL53L0X_REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_3 0xB3u
#define VL53L0X_REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_4 0xB4u
#define VL53L0X_REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_5 0xB5u
#define VL53L0X_REG_GLOBAL_CONFIG_REF_EN_START_SELECT 0xB6u
#define VL53L0X_REG_DYN_SPAD_NUM_REQUESTED_REF_SPAD 0x4Eu
#define VL53L0X_REG_DYN_SPAD_REF_EN_START_OFFSET 0x4Fu
#define VL53L0X_REG_POWER_MANAGEMENT_GO1_POWER_FORCE 0x80u
#define VL53L0X_REG_VHV_CONFIG_PAD_SCL_SDA_EXTSUP_HV 0x89u
#define VL53L0X_REG_ALGO_PHASECAL_LIM 0x30u
#define VL53L0X_REG_ALGO_PHASECAL_CONFIG_TIMEOUT 0x30u

// Core API return codes
#define PROXIMITY_AND_DISTANCE_ERROR -1
#define PROXIMITY_AND_DISTANCE_TIMEOUT -2

typedef enum
{
  VL53L0X_RANGE_VALID = 0u,
  VL53L0X_SIGMA_FAIL = 1u,
  VL53L0X_SIGNAL_FAIL = 2u,
  VL53L0X_MIN_RANGE_FAIL = 3u,
  VL53L0X_PHASE_FAIL = 4u,
  VL53L0X_HARDWARE_FAIL = 5u,
  VL53L0X_NO_UPDATE = 255u
} VL53L0X_RANGE_STATUS_t;

typedef enum
{
  PROXIMITY_AND_DISTANCE_OK = 0,
  PROXIMITY_AND_DISTANCE_NOT_FOUND,
  PROXIMITY_AND_DISTANCE_I2C_ERROR,
  PROXIMITY_AND_DISTANCE_TIMED_OUT,
  PROXIMITY_AND_DISTANCE_INVALID_ARGUMENT,
  PROXIMITY_AND_DISTANCE_NOT_INITIALIZED,
  PROXIMITY_AND_DISTANCE_NO_DATA,
  PROXIMITY_AND_DISTANCE_BUSY
} PROXIMITY_AND_DISTANCE_STATUS_t;

typedef enum
{
  VL53L0X_VCSEL_PERIOD_PRE_RANGE,
  VL53L0X_VCSEL_PERIOD_FINAL_RANGE
} PROXIMITY_AND_DISTANCE_VCSEL_PERIOD_t;

typedef enum
{
  PROXIMITY_AND_DISTANCE_INTERRUPT_DISABLED = 0x00u,
  PROXIMITY_AND_DISTANCE_INTERRUPT_NEW_SAMPLE_READY = 0x04u,
  PROXIMITY_AND_DISTANCE_INTERRUPT_THRESHOLD_LOW = 0x01u,
  PROXIMITY_AND_DISTANCE_INTERRUPT_THRESHOLD_HIGH = 0x02u,
  PROXIMITY_AND_DISTANCE_INTERRUPT_OUT_OF_WINDOW = 0x03u
} PROXIMITY_AND_DISTANCE_INTERRUPT_MASK_t;

/*
 * Initialize the supplied Wire bus before begin(); ranging methods wait for completion.
 * Distance is millimeters; -1 is an error/invalid result and -2 is a timeout.
 * Stop continuous mode before changing timing or address; cached getters do not start a read.
 * Printing getters use labeled values and units; pass false to suppress serial output.
 */
class ProximityAndDistance
{
  public:
    explicit ProximityAndDistance(TwoWire *wire = &Wire, uint8_t i2cAddress = VL53L0X_I2C_ADDRESS);

    bool begin(bool io2v8 = true);
    bool ping(void);
    void setBus(TwoWire *bus);
    TwoWire *getBus(void) const;

    bool isConnected(void) const;
    bool isRangeValid(void) const;

    // Single-shot measurement pipeline
    int readDistance(bool print = true);
    int readRangeSingleMillimeters(void);
    // Both pointers are required; divide the returned signal rate by 128 to obtain MCPS.
    int readRangeSingleMillimeters(uint8_t *rangeStatus, uint16_t *signalRateQ97);

    // Continuous mode
    bool startContinuous(uint32_t periodMilliseconds = 0u);
    bool stopContinuous(void);
    // Starts continuous mode if idle; otherwise restarts it with the requested period.
    bool setContinuousModeInterMeasurementPeriod(uint16_t periodMilliseconds);
    int readDistanceContinuous(bool print = true);
    int readRangeContinuousMillimeters(void);

    // Interrupts / status
    bool setInterruptPolarityHigh(bool high);
    // Thresholds are 0-8190 mm, rounded down to 2 mm steps; set the high threshold first.
    bool setInterruptThresholdLow(uint16_t lowMillimeters);
    bool setInterruptThresholdHigh(uint16_t highMillimeters);
    bool clearInterrupt(void);
    uint8_t getInterruptStatus(void);
    bool clearAndReturnInterrupt(uint8_t *status = NULL);
    bool setInterruptConfig(uint8_t mask);

    // Configuration controls
    // Budget: 20..1000 ms. Timeout: 1..65535 ms; zero never disables waits.
    bool setTimingBudget(uint16_t milliseconds);
    uint16_t getTimingBudget(void);
    bool setSignalRateLimit(float limitMcps);
    float getSignalRateLimit(void);
    bool setVcselPulsePeriod(PROXIMITY_AND_DISTANCE_VCSEL_PERIOD_t type, uint8_t periodPclks);
    uint8_t getVcselPulsePeriod(PROXIMITY_AND_DISTANCE_VCSEL_PERIOD_t type);
    bool setAddress(uint8_t newAddress);
    uint8_t getAddress(void) const;
    // Software tuning presets; stop continuous operation before selecting one.
    bool setDistanceModeLong(void);
    bool setDistanceModeShort(void);
    bool setDistanceModeDefault(void);

    // Timeouts
    bool setTimeout(uint16_t milliseconds);
    uint16_t getTimeout(void) const;
    // Returns and clears the timeout flag.
    bool timeoutOccurred(void);

    // Error/status introspection
    PROXIMITY_AND_DISTANCE_STATUS_t getLastStatus(void) const;
    uint8_t getLastI2cStatus(void) const;
    int16_t getLastRange(void) const;
    uint16_t getRawRange(void) const;
    // Cached rates are Q9.7 MCPS (divide by 128); events are a 32-bit count.
    uint16_t getAmbientRate(void) const;
    uint32_t getTotalEvents(void) const;

    VL53L0X_RANGE_STATUS_t getRangeStatus(void) const;
    uint8_t getModelId(void);
    uint8_t getRevisionId(void);
    uint8_t getDeviceId(void);

  private:
    bool prepare(bool requireIdle = false);
    bool finish(bool success = true);
    bool fail(PROXIMITY_AND_DISTANCE_STATUS_t status);
    void invalidate(void);
    bool writeByte(uint8_t reg, uint8_t value);
    bool writeWord(uint8_t reg, uint16_t value);
    int collectRange(void);
    bool waitFor(uint8_t reg, uint8_t mask, bool set);
    bool setPreset(float signal, uint8_t pre, uint8_t final);
    static VL53L0X_RANGE_STATUS_t decodeRangeStatus(uint8_t raw);

    myosa_detail::Vl53l0xCore _core;
    bool _initialized = false, _continuous = false, _valid = false, _didTimeout = false;
    int16_t _lastDistance = PROXIMITY_AND_DISTANCE_ERROR;
    uint16_t _rawRange = 0, _ambientRate = 0, _signalRate = 0;
    uint32_t _events = 0, _period = 0;
    uint16_t _thresholdLow = 0, _thresholdHigh = 0;
    uint8_t _interruptConfig = PROXIMITY_AND_DISTANCE_INTERRUPT_NEW_SAMPLE_READY;
    VL53L0X_RANGE_STATUS_t _rangeStatus = VL53L0X_NO_UPDATE;
    PROXIMITY_AND_DISTANCE_STATUS_t _lastStatus = PROXIMITY_AND_DISTANCE_NOT_INITIALIZED;
};

#endif
