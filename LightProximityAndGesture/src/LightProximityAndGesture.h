/*
  This code is developed under the MYOSA (LearnTheEasyWay) initiative of MakeSense EduTech and Pegasus Automation.
  Code has been derived from internet sources and component datasheets.
  Existing readily-available libraries would have been used "AS IS" and modified for ease of learning purpose.

  Synopsis of Light Proximity and Gesture Board
  The MYOSA light and gesture board uses the APDS9960 sensor at I2C address 0x39.
  Ambient light is reported in lux; RGB is in percentages and proximity in raw counts.
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

#ifndef MYOSA_LIGHT_PROXIMITY_GESTURE_H
#define MYOSA_LIGHT_PROXIMITY_GESTURE_H

#include <stdint.h>
#include <math.h>
#include <Arduino.h>
#include <Wire.h>

/* APDS-9960 I2C address */
#define APDS9960_I2C_ADDRESS    0x39

/* Gesture parameters */
#define GESTURE_THRESHOLD_OUT   10
#define GESTURE_SENSITIVITY_1   50
#define GESTURE_SENSITIVITY_2   20

/* Error code for returned values */
#define ERROR                   0xFF

/* Acceptable device IDs */
#define APDS9960_ID_1           0xAB
#define APDS9960_ID_2           0x9C
#define APDS9960_ID_3           0xA8
#define APDS9960_ID_4           0x9E

/* Misc parameters */
#define FIFO_PAUSE_TIME         30      // Wait period (ms) between FIFO reads

/* APDS-9960 register addresses */
#define APDS9960_ENABLE         0x80
#define APDS9960_ATIME          0x81
#define APDS9960_WTIME          0x83
#define APDS9960_AILTL          0x84
#define APDS9960_AILTH          0x85
#define APDS9960_AIHTL          0x86
#define APDS9960_AIHTH          0x87
#define APDS9960_PILT           0x89
#define APDS9960_PIHT           0x8B
#define APDS9960_PERS           0x8C
#define APDS9960_CONFIG1        0x8D
#define APDS9960_PPULSE         0x8E
#define APDS9960_CONTROL        0x8F
#define APDS9960_CONFIG2        0x90
#define APDS9960_ID             0x92
#define APDS9960_STATUS         0x93
#define APDS9960_CDATAL         0x94
#define APDS9960_CDATAH         0x95
#define APDS9960_RDATAL         0x96
#define APDS9960_RDATAH         0x97
#define APDS9960_GDATAL         0x98
#define APDS9960_GDATAH         0x99
#define APDS9960_BDATAL         0x9A
#define APDS9960_BDATAH         0x9B
#define APDS9960_PDATA          0x9C
#define APDS9960_POFFSET_UR     0x9D
#define APDS9960_POFFSET_DL     0x9E
#define APDS9960_CONFIG3        0x9F
#define APDS9960_GPENTH         0xA0
#define APDS9960_GEXTH          0xA1
#define APDS9960_GCONF1         0xA2
#define APDS9960_GCONF2         0xA3
#define APDS9960_GOFFSET_U      0xA4
#define APDS9960_GOFFSET_D      0xA5
#define APDS9960_GOFFSET_L      0xA7
#define APDS9960_GOFFSET_R      0xA9
#define APDS9960_GPULSE         0xA6
#define APDS9960_GCONF3         0xAA
#define APDS9960_GCONF4         0xAB
#define APDS9960_GFLVL          0xAE
#define APDS9960_GSTATUS        0xAF
#define APDS9960_IFORCE         0xE4
#define APDS9960_PICLEAR        0xE5
#define APDS9960_CICLEAR        0xE6
#define APDS9960_AICLEAR        0xE7
#define APDS9960_GFIFO_U        0xFC
#define APDS9960_GFIFO_D        0xFD
#define APDS9960_GFIFO_L        0xFE
#define APDS9960_GFIFO_R        0xFF

/* Enable register masks */
#define ENABLE_MSK              0x01u
#define PON_EN_MSK              0x01u
#define AEN_EN_MSK              0x02u
#define PEN_EN_MSK              0x04u
#define WEN_EN_MSK              0x08u
#define AIEN_EN_MSK             0x10u
#define PIEN_EN_MSK             0x20u
#define GEN_EN_MSK              0x40u
#define PON_EN_POS              0x00u
#define AEN_EN_POS              0x01u
#define PEN_EN_POS              0x02u
#define WEN_EN_POS              0x03u
#define AIEN_EN_POS             0x04u
#define PIEN_EN_POS             0x05u
#define GEN_EN_POS              0x06u

/* Gain control masks */
#define APSD9960_GAIN_MSK       0x03u
#define ALS_GAIN_MSK            0x03u
#define PRX_GAIN_MSK            0x0Cu
#define LED_DRIVE_MSK           0xC0u
#define ALS_GAIN_POS            0x00u
#define PRX_GAIN_POS            0x02u
#define LED_DRIVE_POS           0x06u

/* Gesture config2 masks */
#define GES_WTIME_MSK           0x07u
#define GES_LDRIVE_MSK          0x18u
#define GES_GAIN_MSK            0x60u
#define GES_WTIME_POS           0x00u
#define GES_LDRIVE_POS          0x03u
#define GES_GAIN_POS            0x05u

/* Gesture config4 masks */
#define GES_GCONFIG4_MSK        0x01u
#define GES_GMODE_MSK           0x01u
#define GES_GIEN_MSK            0x02u
#define GES_GMODE_POS           0x00u
#define GES_GIEN_POS            0x01u

/* Config2 masks */
#define CFG2_LED_BOOST_MSK      0x30u
#define CFG2_LED_BOOST_POS      0x04u

/* Gesture status masks */
#define GES_STATUS_MSK          0x01u
#define GSTS_GVALID_MSK         0x01u
#define GSTS_GFOV_MSK           0x02u
#define GSTS_GVALID_POS         0x00u
#define GSTS_GFOV_POS           0x01u

/* LED Drive values */
#define LED_DRIVE_100MA         0
#define LED_DRIVE_50MA          1
#define LED_DRIVE_25MA          2
#define LED_DRIVE_12_5MA        3

/* Proximity Gain (PGAIN) values */
#define PGAIN_1X                0
#define PGAIN_2X                1
#define PGAIN_4X                2
#define PGAIN_8X                3

/* ALS Gain (AGAIN) values */
#define AGAIN_1X                0
#define AGAIN_4X                1
#define AGAIN_16X               2
#define AGAIN_64X               3

/* Gesture Gain (GGAIN) values */
#define GGAIN_1X                0
#define GGAIN_2X                1
#define GGAIN_4X                2
#define GGAIN_8X                3

/* LED Boost values */
#define LED_BOOST_100           0
#define LED_BOOST_150           1
#define LED_BOOST_200           2
#define LED_BOOST_300           3

/* Gesture wait time values */
#define GWTIME_0MS              0
#define GWTIME_2_8MS            1
#define GWTIME_5_6MS            2
#define GWTIME_8_4MS            3
#define GWTIME_14_0MS           4
#define GWTIME_22_4MS           5
#define GWTIME_30_8MS           6
#define GWTIME_39_2MS           7

/* Default values */
#define DEFAULT_ATIME           219     // 103ms
#define DEFAULT_WTIME           246     // 27ms
#define DEFAULT_PROX_PPULSE     0x87    // 16us, 8 pulses
#define DEFAULT_GESTURE_PPULSE  0x89    // 16us, 10 pulses
#define DEFAULT_POFFSET_UR      0       // 0 offset
#define DEFAULT_POFFSET_DL      0       // 0 offset
#define DEFAULT_CONFIG1         0x60    // No 12x wait (WTIME) factor
#define DEFAULT_LDRIVE          LED_DRIVE_100MA
#define DEFAULT_PGAIN           PGAIN_4X
#define DEFAULT_AGAIN           AGAIN_4X
#define DEFAULT_PILT            0       // Low proximity threshold
#define DEFAULT_PIHT            50      // High proximity threshold
#define DEFAULT_AILT            0xFFFF  // Force interrupt for calibration
#define DEFAULT_AIHT            0
#define DEFAULT_PERS            0x11    // 2 consecutive prox or ALS for int.
#define DEFAULT_CONFIG2         0x01    // No saturation interrupts or LED boost
#define DEFAULT_CONFIG3         0       // Enable all photodiodes, no SAI
#define DEFAULT_GPENTH          40      // Threshold for entering gesture mode
#define DEFAULT_GEXTH           30      // Threshold for exiting gesture mode
#define DEFAULT_GCONF1          0x40    // 4 gesture events for int., 1 for exit
#define DEFAULT_GGAIN           GGAIN_2X
#define DEFAULT_GLDRIVE         LED_DRIVE_100MA
#define DEFAULT_GWTIME          GWTIME_2_8MS
#define DEFAULT_GOFFSET         0       // No offset scaling for gesture mode
#define DEFAULT_GPULSE          0xC9    // 32us, 10 pulses
#define DEFAULT_GCONF3          0       // All photodiodes active during gesture
#define DEFAULT_GIEN            0       // Disable gesture interrupts

/*!
 *
 */
typedef enum
{
  DISABLE=0u,             /**< Enable a feature */
  ENABLE=1u               /**< Disable a feature */
}STATE_t;

/*!
 *
 */
typedef enum
{
  POWER = 0u,             /**< Power ON/OFF */
  AMBIENT_LIGHT = 1u,     /**< ALS Enable/Disable */
  PROXIMITY = 2u,         /**< Proximity Detect Enable/Disable */
  WAIT = 3u,              /**< Wait Enable/Disable */
  AMBIENT_LIGHT_INT = 4u, /**< ALS Interrupt Enable/Disable */
  PROXIMITY_INT = 5u,     /**< Proximity Interrupt Enable/Disable */
  GESTURE = 6u            /**< Gesture Enable/Disable */
}APDS9960_MODE_t;

/* Direction definitions */
enum {
  DIR_NONE,
  DIR_LEFT,
  DIR_RIGHT,
  DIR_UP,
  DIR_DOWN,
  DIR_NEAR,
  DIR_FAR,
  DIR_ALL,
  TIMEOUT
};

/* State definitions */
enum {
  NA_STATE,
  NEAR_STATE,
  FAR_STATE,
  ALL_STATE
};

/* Container for gesture data */
typedef struct gesture_data_type {
    uint8_t u_data[32];
    uint8_t d_data[32];
    uint8_t l_data[32];
    uint8_t r_data[32];
    uint8_t index;
    uint8_t total_gestures;
    uint8_t in_threshold;
    uint8_t out_threshold;
} gesture_data_type_t;

/*
 * Call begin() after Wire setup, then enable the required sensing engines.
 * Use getAmbientLightLux() for illuminance. Proximity remains an uncalibrated raw reading.
 * RGB/gesture pointers refer to internal buffers overwritten by later calls.
 */
class LightProximityAndGesture
{
  public:
    LightProximityAndGesture();
    bool begin(void);
    uint8_t getDeviceId(void);
    bool ping(void);
    uint8_t getMode(void);
    bool setMode(APDS9960_MODE_t mode, STATE_t state);
     /* Turn the APDS-9960 on and off */
    bool enablePower(void);
    bool disablePower(void);
    /* Enable or disable specific sensors */
    bool enableAmbientLightSensor(STATE_t interrupt = DISABLE);
    bool disableAmbientLightSensor(void);
    bool enableProximitySensor(STATE_t interrupt = DISABLE);
    bool disableProximitySensor(void);
    bool enableGestureSensor(STATE_t interrupt = DISABLE);
    bool disableGestureSensor(void);
    /* Get and set interrupt enables */
    bool setAmbientLightInt(STATE_t intState);
    bool getAmbientLightIntState(void);
    bool setProximityInt(STATE_t intState);
    bool getProximityIntState();
    bool setGestureInt(STATE_t intState);
    bool getGestureIntState(void);
    /* Gain control */
    uint8_t getAmbientLightGain(void);
    bool setAmbientLightGain(uint8_t alsGain);
    uint8_t getProximityGain(void);
    bool setProximityGain(uint8_t prxGain);
    uint8_t getGestureGain(void);
    bool setGestureGain(uint8_t gesGain);
    /* Get and set light interrupt thresholds */
    bool getLightIntLowThreshold(uint16_t *threshold);
    bool setLightIntLowThreshold(uint16_t threshold);
    bool getLightIntHighThreshold(uint16_t *threshold);
    bool setLightIntHighThreshold(uint16_t threshold);
    /* Get and set proximity interrupt thresholds */
    bool getProximityIntLowThreshold(uint8_t *threshold);
    bool setProximityIntLowThreshold(uint8_t threshold);
    bool getProximityIntHighThreshold(uint8_t *threshold);
    bool setProximityIntHighThreshold(uint8_t threshold);
    /* LED drive strength control */
    uint8_t getLedDrive(void);
    bool setLedDrive(uint8_t drive);
    uint8_t getGestureLedDrive(void);
    bool setGestureLedDrive(uint8_t drive);
    /* Clear interrupts */
    bool clearAmbientLightInt(void);
    bool clearProximityInt(void);
    /* Ambient light methods */
    // The raw accessor remains for compatibility; examples use the RGB-derived lux approximation.
    uint16_t getAmbientLight(bool print=true);
    float getAmbientLightLux(bool print=true);
    bool lightReadingValid(void) const { return _lightReadingValid; }
    uint16_t *getRGBProportion(bool print=true);
    uint16_t getRedProportion(void);
    uint16_t getGreenProportion(void);
    uint16_t getBlueProportion(void);
    /* Proximity methods */
    float getProximity(bool print=true);
    /* Gesture methods */
    char *getGesture(bool print=true);
  private:
    gesture_data_type_t _gesture_data;
    int _gesture_ud_delta;
    int _gesture_lr_delta;
    int _gesture_ud_count;
    int _gesture_lr_count;
    int _gesture_near_count;
    int _gesture_far_count;
    int _gesture_state;
    int _gesture_motion;
    bool _isConnected;
    uint8_t _configValues[128] = {};
    uint8_t _configValid[16] = {};
    uint16_t _color[3] = {};
    bool _lightReadingValid = false;
    /* Gesture processing */
    void resetGestureParameters(void);
    bool processGestureData(void);
    bool decodeGesture(void);
    /* LED Boost Control */
    uint8_t getLedBoost(void);
    bool setLedBoost(uint8_t boost);
    /* Gesture wait time control */
    uint8_t getGestureWaitTime();
    bool setGestureWaitTime(uint8_t time);
    /* Gesture mode */
    uint8_t getGestureMode();
    bool setGestureMode(uint8_t mode);
    /* Ambient light methods */
    bool readAmbientLight(uint16_t *val);
    bool readAmbientLightLux(float *lux);
    bool readRedLight(uint16_t *val);
    bool readGreenLight(uint16_t *val);
    bool readBlueLight(uint16_t *val);
    /* Proximity methods */
    bool readProximity(uint8_t *val);
    /* Gesture methods */
    bool isGestureAvailable(void);
    int readGesture(void);
    /* low level I2C functions */
    uint8_t _i2cSlaveAddress;
    void i2c_init(void);
    bool readByte(uint8_t reg, uint8_t *in);
    int16_t readMultiBytes(uint8_t reg, uint8_t length, uint8_t *in);
    bool readMultiBytes(uint8_t length, uint8_t *in);
    bool writeByte(uint8_t reg, uint8_t val);
    bool writeByte(uint8_t reg);
    bool writeAddress(void);
    void delay_ms(uint16_t ms);
};

#endif
