# MYOSA Arduino Libraries

**Version 2.0.0 | Branch: `main`**

[MYOSA (Make Your Own Sensors Applications)](https://myosa-sensors.org/) is a modular platform for learning and building sensor applications. This branch contains the Arduino libraries and examples for the current MYOSA hardware bundle.

## Choose your branch

| Branch | Hardware bundle |
| --- | --- |
| [main](https://github.com/myosa-sensors/arduino-libraries/tree/main) | MYOSA with MAX30100 and VL53L0X; controller LED and buzzer |
| [legacy](https://github.com/myosa-sensors/arduino-libraries/tree/legacy) | MYOSA Legacy with temperature/humidity, air quality and an actuator board |

Install one complete branch at a time. The branches share library names, so mixing their files can cause incompatible dependencies or duplicate-library selection.

## Included libraries

| Folder | Hardware | Purpose |
| --- | --- | --- |
| [AccelAndGyro](AccelAndGyro/) | MPU6050 | Acceleration, angular velocity, tilt and temperature |
| [BarometricPressure](BarometricPressure/) | BMP180 | Pressure, temperature and calculated altitude |
| [HeartRateAndSpO2](HeartRateAndSpO2/) | MAX30100 | Heart rate, estimated SpO2, raw optical samples and die temperature |
| [LightProximityAndGesture](LightProximityAndGesture/) | APDS9960 | Ambient light, RGB proportions, proximity and gestures |
| [MYOSA](MYOSA/) | Combined library | Sensor initialization, OLED pages and BLE integration |
| [OLED](OLED/) | SSD1306 | 128 x 64 display, graphics and fonts |
| [ProximityAndDistance](ProximityAndDistance/) | VL53L0X | Distance in millimeters and range-status reporting |

Each library contains `src/`, `library.properties` and `keywords.txt`. All bundled sketches are inside `MYOSA/examples/`.

## Installation

1. Install Arduino IDE and the **esp32 by Espressif Systems** board package using the [official ESP32 installation guide](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html).
2. Select the appropriate branch on GitHub, choose **Code > Download ZIP**, and extract it.
3. Find your sketchbook location in Arduino IDE preferences. Copy **each library folder listed above** directly into its `libraries` folder. For example, the combined header must be at `<sketchbook>/libraries/MYOSA/src/myosa.h`, with `AccelAndGyro`, `OLED` and the other libraries alongside `MYOSA`.
4. Keep only one installed copy of each library. Separate sketchbooks can be used for the two bundles. Restart Arduino IDE after copying the folders.
5. Connect the controller by USB, select its ESP32 board and port, and open an example through **File > Examples > MYOSA**, or open its `.ino` file directly.

This download contains multiple Arduino libraries. Extract and install the individual library folders rather than treating the entire repository ZIP as one library. Arduino also documents [manual library installation](https://docs.arduino.cc/software/ide-v2/tutorials/ide-v2-installing-a-library/).

The combined library targets ESP32. The build reference used for this bundle is **ESP32 Dev Module** (`esp32:esp32:esp32`) with **ESP32 core 3.3.11**. Select board settings appropriate to your controller.

## Getting started

Connect the boards required by the selected example, upload the sketch, and open Serial Monitor at **115200 baud**. The master initializes shared `Wire` at **100 kHz** and prints initialization results before starting its display cycle.

Start with a basic example for each connected board, then open [1_MasterCode](MYOSA/examples/%232_Experiments/1_MasterCode/1_MasterCode.ino). The master reports unavailable boards and continues with the boards it can initialize. Individual examples may wait for their required board during setup.

The master cycles through **10 OLED pages** at 1.5 seconds per page. Keep calling `myosa.update()` frequently in custom loops so MAX30100 sampling and VL53L0X servicing continue between display and BLE updates; avoid long blocking delays.

## Controller outputs and measurements

- **Burglar alarm:** ambient light above the configurable `LIGHT_THRESHOLD_LUX` value of **10 lux** sounds the active-high buzzer on **GPIO 12** and shows an OLED alert. At or below the threshold it clears the alarm. Unavailable readings show an error and preserve the previous buzzer state.
- **Wi-Fi control panel:** switches the LED on **GPIO 2** and buzzer on **GPIO 12**. Enter your Wi-Fi credentials in the sketch, then open the printed IP address from a device on the same network. Local control does not require Internet access.
- **Ambient light:** the master and light example use the APDS9960 RGB-derived lux approximation with gain/integration compensation. It is nominal illuminance, not a meter-calibrated measurement. Proximity and MAX30100 raw optical samples keep their raw units.
- **APDS9960 IDs:** `0xAB`, `0x9C`, `0xA8` and `0x9E` are accepted by initialization.
- **MAX30100:** pulse and estimated SpO2 require a stable finger signal; unavailable values are reported while samples settle or signal quality is insufficient. This driver targets MAX30100, not MAX30102. See [HeartRateAndSpO2](HeartRateAndSpO2/README.md).
- **VL53L0X:** inspect range status as well as distance; invalid or out-of-range results are not valid distances. See [ProximityAndDistance](ProximityAndDistance/README.md).

## Bluetooth Low Energy

The master advertises as `MYOSA_1` and publishes sensor notifications. Motion, pressure and light retain their existing service UUIDs. Distance and pulse characteristics share the service ending in `b5`; their characteristic suffixes are `60`, `70` and `71`. This branch has no actuator event-command endpoint. BLE client layouts should match the selected branch.

## Examples

### Learning basics

| Example | Purpose |
| --- | --- |
| [1_Blink](MYOSA/examples/%231_LearningBasics/1_Blink/1_Blink.ino) | Blink the controller LED |
| [2_OLED_demo](MYOSA/examples/%231_LearningBasics/2_OLED_demo/2_OLED_demo.ino) | Display graphics and text |
| [4_BarometricPressure_Demo](MYOSA/examples/%231_LearningBasics/4_BarometricPressure_Demo/4_BarometricPressure_Demo.ino) | Read pressure and temperature |
| [5_AccelAndGyro_Demo](MYOSA/examples/%231_LearningBasics/5_AccelAndGyro_Demo/5_AccelAndGyro_Demo.ino) | Read motion, tilt and temperature |
| [6_LightProximity_Demo](MYOSA/examples/%231_LearningBasics/6_LightProximity_Demo/6_LightProximity_Demo.ino) | Read ambient lux, RGB proportions and raw proximity |
| [7_Gesture_Demo](MYOSA/examples/%231_LearningBasics/7_Gesture_Demo/7_Gesture_Demo.ino) | Report detected hand gestures |
| [8_ProximityAndDistance_Demo](MYOSA/examples/%231_LearningBasics/8_ProximityAndDistance_Demo/8_ProximityAndDistance_Demo.ino) | Read distance and range status |
| [9_HeartRateAndSpO2_Demo](MYOSA/examples/%231_LearningBasics/9_HeartRateAndSpO2_Demo/9_HeartRateAndSpO2_Demo.ino) | Read heart rate, estimated SpO2 and optical samples |

### Experiments

| Example | Purpose |
| --- | --- |
| [1_MasterCode](MYOSA/examples/%232_Experiments/1_MasterCode/1_MasterCode.ino) | Cycle through sensor readings on Serial and OLED; publish BLE data |
| [2_BurglarAlarm](MYOSA/examples/%232_Experiments/2_BurglarAlarm/2_BurglarAlarm.ino) | Detect light entering a normally dark locker |

### Wi-Fi

| Example | Purpose |
| --- | --- |
| [1_ScanNearbyWiFi](MYOSA/examples/%233_Wi-Fi/1_ScanNearbyWiFi/1_ScanNearbyWiFi.ino) | List nearby Wi-Fi networks |
| [2_GetTimeFromInternet](MYOSA/examples/%233_Wi-Fi/2_GetTimeFromInternet/2_GetTimeFromInternet.ino) | Retrieve time over Wi-Fi |
| [3_WiFiControlPanel](MYOSA/examples/%233_Wi-Fi/3_WiFiControlPanel/3_WiFiControlPanel.ino) | Control the LED and buzzer from a local webpage |

## Troubleshooting

- **Header not found or wrong library selected:** check that the library folders are direct children of your sketchbook's `libraries` directory and that both branch versions are not installed together.
- **Board not detected:** check its power and I2C connections, then run its basic example and read the initialization messages.
- **Upload reports access denied:** close Serial Monitor or other software using the selected serial port, then retry.
- **Unreadable Serial output:** select 115200 baud and confirm the correct port.

## Support and attribution

- [MYOSA website](https://myosa-sensors.org/)
- [Report an issue](https://github.com/myosa-sensors/arduino-libraries/issues) with the branch, example, controller, ESP32 core version and relevant Serial output.
- Contact Team MYOSA: [myosa.event@gmail.com](mailto:myosa.event@gmail.com)

12 September, 2026 by Pegasus Automation  
(as a part of MYOSA Initiative)

Bundled third-party code retains its original attribution and license notices in the source files and component license files.
