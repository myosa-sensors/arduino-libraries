/*
  This code is developed under the MYOSA (LearnTheEasyWay) initiative of MakeSense EduTech and Pegasus Automation.
  Code has been derived from internet sources and component datasheets.
  Existing readily-available libraries would have been used "AS IS" and modified for ease of learning purpose.

  Synopsis of Heart Rate and SpO2 sensor
  MAX30100 optical heart-rate and estimated SpO2 sensing at I2C address 0x57.
  Reads infrared/red samples and die temperature using a caller-initialized Wire bus.
  Call update frequently; unavailable measurements return NAN.
  SpO2 uses an approximate calibration and requires validation for the actual module.

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

#ifndef MYOSA_HEART_RATE_AND_SPO2_H
#define MYOSA_HEART_RATE_AND_SPO2_H

#include <stdint.h>
#include <Arduino.h>
#include <math.h>
#include <Wire.h>

#define MAX30100_I2C_ADDRESS 0x57u
#define MAX30100_EXPECTED_PART_ID 0x11u

// Register map
#define MAX30100_REG_INTERRUPT_STATUS 0x00u
#define MAX30100_REG_INTERRUPT_ENABLE 0x01u
#define MAX30100_REG_FIFO_WRITE_POINTER 0x02u
#define MAX30100_REG_FIFO_OVERFLOW_COUNTER 0x03u
#define MAX30100_REG_FIFO_READ_POINTER 0x04u
#define MAX30100_REG_FIFO_DATA 0x05u
#define MAX30100_REG_MODE_CONFIGURATION 0x06u
#define MAX30100_REG_SPO2_CONFIGURATION 0x07u
#define MAX30100_REG_LED_CONFIGURATION 0x09u
#define MAX30100_REG_TEMPERATURE_DATA_INT 0x16u
#define MAX30100_REG_TEMPERATURE_DATA_FRAC 0x17u
#define MAX30100_REG_REVISION_ID 0xFEu
#define MAX30100_REG_PART_ID 0xFFu

// Register bit masks
#define MAX30100_REG_INTERRUPT_STATUS_PWR_RDY (1u << 0)
#define MAX30100_REG_INTERRUPT_STATUS_SPO2_RDY (1u << 4)
#define MAX30100_REG_INTERRUPT_STATUS_HR_RDY (1u << 5)
#define MAX30100_REG_INTERRUPT_STATUS_TEMP_RDY (1u << 6)
#define MAX30100_REG_INTERRUPT_STATUS_A_FULL (1u << 7)

#define MAX30100_REG_INTERRUPT_ENABLE_A_FULL (1u << 7)
#define MAX30100_REG_INTERRUPT_ENABLE_TEMP_RDY (1u << 6)
#define MAX30100_REG_INTERRUPT_ENABLE_HR_RDY (1u << 5)
#define MAX30100_REG_INTERRUPT_ENABLE_SPO2_RDY (1u << 4)

#define MAX30100_REG_MODE_TEMP_EN (1u << 3)
#define MAX30100_REG_MODE_RESET (1u << 6)
#define MAX30100_REG_MODE_SHUTDOWN (1u << 7)

#define MAX30100_REG_SPO2_HI_RES_EN (1u << 6)

// Device feature constants
#define MAX30100_FIFO_DEPTH 16u
#define MAX30100_DEFAULT_FIFO_BUFFER_SIZE 16u
#define MAX30100_DEFAULT_IR_LED_CURRENT_mA 50u
#define MAX30100_DEFAULT_RED_LED_CURRENT_mA 50u

// Datasheet-compatible configuration enums
typedef enum
{
  HEART_RATE_AND_SPO2_OK = 0,
  HEART_RATE_AND_SPO2_NOT_FOUND,
  HEART_RATE_AND_SPO2_I2C_ERROR,
  HEART_RATE_AND_SPO2_TIMEOUT,
  HEART_RATE_AND_SPO2_INVALID_ARGUMENT,
  HEART_RATE_AND_SPO2_NO_DATA,
  HEART_RATE_AND_SPO2_BUSY,
  HEART_RATE_AND_SPO2_NOT_INITIALIZED
} HEART_RATE_AND_SPO2_STATUS_t;

typedef enum
{
  HEART_RATE_AND_SPO2_MODE_HRONLY = 0x02u,
  HEART_RATE_AND_SPO2_MODE_SPO2_HR = 0x03u
} HEART_RATE_AND_SPO2_MODE_t;

typedef enum
{
  HEART_RATE_AND_SPO2_SAMPLING_RATE_50HZ = 0x00u,
  HEART_RATE_AND_SPO2_SAMPLING_RATE_100HZ = 0x01u,
  HEART_RATE_AND_SPO2_SAMPLING_RATE_167HZ = 0x02u,
  HEART_RATE_AND_SPO2_SAMPLING_RATE_200HZ = 0x03u,
  HEART_RATE_AND_SPO2_SAMPLING_RATE_400HZ = 0x04u,
  HEART_RATE_AND_SPO2_SAMPLING_RATE_600HZ = 0x05u,
  HEART_RATE_AND_SPO2_SAMPLING_RATE_800HZ = 0x06u,
  HEART_RATE_AND_SPO2_SAMPLING_RATE_1000HZ = 0x07u
} HEART_RATE_AND_SPO2_SAMPLING_RATE_t;

typedef enum
{
  HEART_RATE_AND_SPO2_PULSE_WIDTH_200US_13BITS = 0x00u,
  HEART_RATE_AND_SPO2_PULSE_WIDTH_400US_14BITS = 0x01u,
  HEART_RATE_AND_SPO2_PULSE_WIDTH_800US_15BITS = 0x02u,
  HEART_RATE_AND_SPO2_PULSE_WIDTH_1600US_16BITS = 0x03u
} HEART_RATE_AND_SPO2_PULSE_WIDTH_t;

typedef enum
{
  HEART_RATE_AND_SPO2_LED_CURRENT_0_0_MA = 0x00u,
  HEART_RATE_AND_SPO2_LED_CURRENT_4_4_MA = 0x01u,
  HEART_RATE_AND_SPO2_LED_CURRENT_7_6_MA = 0x02u,
  HEART_RATE_AND_SPO2_LED_CURRENT_11_0_MA = 0x03u,
  HEART_RATE_AND_SPO2_LED_CURRENT_14_2_MA = 0x04u,
  HEART_RATE_AND_SPO2_LED_CURRENT_17_4_MA = 0x05u,
  HEART_RATE_AND_SPO2_LED_CURRENT_20_8_MA = 0x06u,
  HEART_RATE_AND_SPO2_LED_CURRENT_24_0_MA = 0x07u,
  HEART_RATE_AND_SPO2_LED_CURRENT_27_1_MA = 0x08u,
  HEART_RATE_AND_SPO2_LED_CURRENT_30_6_MA = 0x09u,
  HEART_RATE_AND_SPO2_LED_CURRENT_33_8_MA = 0x0Au,
  HEART_RATE_AND_SPO2_LED_CURRENT_37_0_MA = 0x0Bu,
  HEART_RATE_AND_SPO2_LED_CURRENT_40_2_MA = 0x0Cu,
  HEART_RATE_AND_SPO2_LED_CURRENT_43_6_MA = 0x0Du,
  HEART_RATE_AND_SPO2_LED_CURRENT_46_8_MA = 0x0Eu,
  HEART_RATE_AND_SPO2_LED_CURRENT_50_0_MA = 0x0Fu
} HEART_RATE_AND_SPO2_LED_CURRENT_t;

typedef struct
{
    uint16_t infrared;
    uint16_t red;
    uint32_t micros;
    bool fingerDetected;
} MAX30100_RAW_DATA_t;

typedef struct
{
    uint16_t heartRateWindow;
    uint16_t spO2Window;
    float filterAlpha;
    float beatTimeoutMinMs;
    float beatTimeoutMaxMs;
} HEART_RATE_AND_SPO2_ALGORITHM_CONFIG_t;

/*
 * Initialize the supplied Wire bus before begin(); update() must service the FIFO frequently.
 * Measurements are cached: heart rate is BPM, estimated SpO2 is percent, and errors return NAN.
 * Configuration and transfer failures are reported through getLastStatus().
 * Printing getters use labeled values and units; pass false to suppress serial output.
 */
class HeartRateAndSpO2
{
  public:
    explicit HeartRateAndSpO2(TwoWire *wire = &Wire, uint8_t i2cAddress = MAX30100_I2C_ADDRESS);

    // Initialize Wire in the sketch before calling begin.
    bool begin(HEART_RATE_AND_SPO2_MODE_t mode = HEART_RATE_AND_SPO2_MODE_SPO2_HR);
    bool ping(void);
    bool reset(void);
    bool shutdown(void);
    bool resume(void);
    // Returns true when samples were consumed; false with NO_DATA is normal between samples.
    bool update(void);

    bool isFingerDetected(void) const;
    bool isReadingValid(void) const;
    bool isRawDataAvailable(void) const;

    // Latest buffered sample; popRawValue removes the oldest, index 0 is oldest.
    bool getRawValues(uint16_t *infrared, uint16_t *red, bool print = true);
    bool popRawValue(uint16_t *infrared, uint16_t *red, uint32_t *sampleTimeMicros = NULL);
    bool getRawSample(uint16_t index, uint16_t *infrared, uint16_t *red,
                      uint32_t *sampleTimeMicros = NULL);
    int getBufferedSampleCount(void) const;
    void clearRawBuffer(void);

    // Cached BPM and estimated SpO2 percent; unavailable or stale readings return NAN.
    float getHeartRate(bool print = true) const;
    float getSpO2(bool print = true) const;

    // Current requests use the nearest hardware step; code setters accept exact 0-15 selectors.
    bool setLedCurrent(uint8_t milliamps);
    bool setLedCurrents(uint8_t infraredMilliamps, uint8_t redMilliamps);
    bool setLedCurrentByCode(uint8_t infraredCode, uint8_t redCode);
    bool setSampleRate(HEART_RATE_AND_SPO2_SAMPLING_RATE_t samplesPerSecond);
    bool setSampleRate(uint16_t samplesPerSecond);
    bool setPulseWidth(HEART_RATE_AND_SPO2_PULSE_WIDTH_t microseconds);
    bool setPulseWidth(uint16_t microseconds);
    bool setHighResolutionMode(bool enable = true);
    bool setMode(HEART_RATE_AND_SPO2_MODE_t mode);
    HEART_RATE_AND_SPO2_MODE_t getMode(void) const;
    bool setFingerDetectionThreshold(uint16_t threshold);
    uint16_t getFingerDetectionThreshold(void) const;

    bool setInterruptsEnabled(bool hrReady = false, bool spo2Ready = false, bool tempReady = false,
                              bool fifoAlmostFull = false);
    uint8_t getInterruptEnableMask(void) const;
    // Cached interrupt bits, refreshed by update/readFifo.
    uint8_t getInterruptStatus(uint8_t mask = 0xFFu) const;
    bool clearInterrupts(void);

    // Start each new die-temperature conversion explicitly; update/getters poll completion.
    bool startTemperatureSampling(void);
    bool isTemperatureReady(void) const;
    bool readTemperatureRaw(uint8_t *integerPart, uint8_t *fractionalPart);
    float getTemperatureC(bool print = true);
    float getTemperatureF(bool print = true);

    uint8_t getDeviceId(void);
    uint8_t getRevisionId(void);
    HEART_RATE_AND_SPO2_STATUS_t getLastStatus(void) const;

    bool setLedPulseWidthWithBuffer(HEART_RATE_AND_SPO2_PULSE_WIDTH_t width,
                                    uint16_t heartRateWindowSize);
    bool setFingerModeThresholds(uint16_t onThreshold, uint16_t offThreshold);
    bool resetReadingAlgorithm(void);
    void resetDetectionState(void);

    bool configureHeartRateWindow(uint16_t movingAverageSize);
    bool configureSpO2Window(uint16_t averagingWindow);
    uint16_t getDefaultSpO2Window(void) const;
    uint16_t getDefaultHeartRateWindow(void) const;

    bool readFifo(void);
    bool isFifoOverflowed(void) const;
    uint8_t getWritePointer(void);
    uint8_t getReadPointer(void);
    uint8_t getOverflowCounter(void);
    // Discard both hardware samples and software processing history.
    bool resetFifo(void);

    // Window limits: 1..16 beat intervals, 50..4096 optical samples.
    bool setSpO2Calibration(float intercept, float slope);

  private:
    bool fail(HEART_RATE_AND_SPO2_STATUS_t status);
    bool ready(void);
    bool readByte(uint8_t reg, uint8_t *value);
    bool readMultiBytes(uint8_t reg, uint8_t length, uint8_t *values);
    bool writeByte(uint8_t reg, uint8_t value);
    bool modifyByte(uint8_t reg, uint8_t mask, uint8_t value);
    bool applyConfiguration(uint8_t rate, uint8_t width, HEART_RATE_AND_SPO2_MODE_t mode);
    bool pollTemperature(void);
    void processSample(uint16_t infrared, uint16_t red, uint32_t sampleTime);
    void pushRawData(uint16_t infrared, uint16_t red, uint32_t sampleTime);
    void resetProcessing(void);
    void recordBeat(uint32_t sampleTime);

    TwoWire *_wire;
    uint8_t _address;
    HEART_RATE_AND_SPO2_STATUS_t _lastStatus = HEART_RATE_AND_SPO2_NOT_INITIALIZED;
    HEART_RATE_AND_SPO2_MODE_t _mode = HEART_RATE_AND_SPO2_MODE_SPO2_HR;
    bool _initialized = false, _sleeping = false, _finger = false;
    bool _filterInitialized = false, _abovePeak = false, _hasBeat = false;
    bool _temperaturePending = false, _temperatureReady = false, _overflow = false;
    uint8_t _rate = 1, _width = 3, _interruptMask = 0, _interruptStatus = 0;
    uint16_t _samplesPerSecond = 100, _fingerOn = 10000, _fingerOff = 8000;
    uint16_t _heartWindow = 4, _spo2Window = 200;
    float _heartRate, _spO2, _temperature;
    float _irDc = 0, _redDc = 0, _filtered = 0, _envelope = 0;
    float _irSquare = 0, _redSquare = 0, _irSum = 0, _redSum = 0;
    float _spo2Intercept = 110.0f, _spo2Slope = 25.0f;
    uint16_t _opticalCount = 0, _intervals[16] = {};
    uint8_t _intervalCount = 0, _intervalIndex = 0;
    uint32_t _fingerSince = 0, _lastBeat = 0, _lastSample = 0, _temperatureStarted = 0;
    bool _hasSample = false;
    MAX30100_RAW_DATA_t _raw[16] = {};
    uint8_t _head = 0, _count = 0;
};

#endif
