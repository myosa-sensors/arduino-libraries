# MYOSA Legacy Arduino Libraries

**Version 2.0.0 | Branch: `legacy`**

[MYOSA (Make Your Own Sensors Applications)](https://myosa-sensors.org/) is a modular platform for learning and building sensor applications. This branch contains the Arduino libraries and examples for the MYOSA Legacy hardware bundle.

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
| [Actuator](Actuator/) | PCA9536 | Relay and buzzer control through an I2C GPIO expander |
| [AirQuality](AirQuality/) | CCS811 | Equivalent CO2 and total volatile organic compounds |
| [BarometricPressure](BarometricPressure/) | BMP180 | Pressure, temperature and calculated altitude |
| [LightProximityAndGesture](LightProximityAndGesture/) | APDS9960 | Ambient light, RGB proportions, proximity and gestures |
| [MYOSA](MYOSA/) | Combined library | Sensor initialization, OLED pages and BLE integration |
| [OLED](OLED/) | SSD1306 | 128 x 64 display, graphics and fonts |
| [TempAndHumidity](TempAndHumidity/) | Si7021 | Temperature, relative humidity and heat index |

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

The master cycles through **11 OLED pages** at 1.5 seconds per page and publishes the sensor readings over BLE.

## Actuator examples and measurements

- **Output mapping:** the actuator board uses **IO0 for the relay** and **IO1 for the buzzer**. These are expander outputs, not ESP32 GPIO numbers. Check the connected load before running an actuator example.
- **Burglar alarm:** ambient light above **10 raw counts** turns the actuator buzzer on; a reading at or below that threshold turns it off. This legacy experiment retains the original raw-light threshold.
- **Gesture-controlled light:** UP or LEFT turns the relay on; DOWN or RIGHT turns it off. Other gesture results leave its state unchanged.
- **Wi-Fi control panel:** switches the controller LED on **GPIO 2** and the actuator buzzer. Enter your Wi-Fi credentials in the sketch, then open the printed IP address from a device on the same network. Local control does not require Internet access.
- **Ambient light:** the master and light/proximity demo report the APDS9960 RGB-derived lux approximation. The raw accessor remains available for legacy thresholds. Proximity remains a raw reading; nominal lux is not meter-calibrated.
- **APDS9960 IDs:** this branch accepts `0xAB`, `0x9C` and `0xA8`.

## Bluetooth Low Energy

The master advertises as `MYOSA_1`. Services 0 through 4 publish sensor data; service 5 retains the event-command endpoint. Existing UUIDs and command-field order are preserved for compatible clients. BLE events can control the relay and buzzer; use the legacy branch's command format.

## Examples

### Learning basics

| Example | Purpose |
| --- | --- |
| [1_Blink](MYOSA/examples/%231_LearningBasics/1_Blink/1_Blink.ino) | Blink the controller LED |
| [2_OLED_demo](MYOSA/examples/%231_LearningBasics/2_OLED_demo/2_OLED_demo.ino) | Display graphics and text |
| [3_TempAndHumidity_Demo](MYOSA/examples/%231_LearningBasics/3_TempAndHumidity_Demo/3_TempAndHumidity_Demo.ino) | Read temperature and humidity |
| [4_BarometricPressure_Demo](MYOSA/examples/%231_LearningBasics/4_BarometricPressure_Demo/4_BarometricPressure_Demo.ino) | Read pressure and temperature |
| [5_AccelAndGyro_Demo](MYOSA/examples/%231_LearningBasics/5_AccelAndGyro_Demo/5_AccelAndGyro_Demo.ino) | Read motion, tilt and temperature |
| [6_LightProximity_Demo](MYOSA/examples/%231_LearningBasics/6_LightProximity_Demo/6_LightProximity_Demo.ino) | Read ambient lux, RGB proportions and raw proximity |
| [7_Gesture_Demo](MYOSA/examples/%231_LearningBasics/7_Gesture_Demo/7_Gesture_Demo.ino) | Report detected hand gestures |
| [8_AirQuality_Demo](MYOSA/examples/%231_LearningBasics/8_AirQuality_Demo/8_AirQuality_Demo.ino) | Read equivalent CO2 and TVOC |
| [9_Actuator_Demo](MYOSA/examples/%231_LearningBasics/9_Actuator_Demo/9_Actuator_Demo.ino) | Operate the relay and buzzer |

### Experiments

| Example | Purpose |
| --- | --- |
| [1_MasterCode](MYOSA/examples/%232_Experiments/1_MasterCode/1_MasterCode.ino) | Cycle through sensor readings on Serial and OLED; publish BLE data |
| [2_BurglarAlarm](MYOSA/examples/%232_Experiments/2_BurglarAlarm/2_BurglarAlarm.ino) | Detect light entering a normally dark locker |
| [3_GestureControlledLight](MYOSA/examples/%232_Experiments/3_GestureControlledLight/3_GestureControlledLight.ino) | Switch the relay with hand gestures |

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
