# MYOSA Legacy Arduino Libraries

**Version 2.0.0**

[MYOSA (Make Your Own Sensors Applications)](https://myosa-sensors.org/) is a modular platform for learning and building sensor applications. This repository contains the Arduino libraries and examples for the MYOSA Legacy hardware bundle.

## Documentation and setup

For installation, hardware connections, Arduino IDE setup, uploading sketches and getting started, follow the [official MYOSA documentation](https://wiki.myosa-sensors.org/docs/intro/).

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

## Master example

[1_MasterCode](MYOSA/examples/%232_Experiments/1_MasterCode/1_MasterCode.ino) prints readings to Serial at **115200 baud**, cycles through OLED pages and publishes BLE data. It initializes shared `Wire` at **100 kHz** and continues with available boards when a module cannot be initialized.

The master cycles through **11 OLED pages** at 1.5 seconds per page.

## Actuator examples and measurements

- **Output mapping:** the actuator board uses **IO0 for the relay** and **IO1 for the buzzer**. These are expander outputs, not ESP32 GPIO numbers. Check the connected load before running an actuator example.
- **Burglar alarm:** ambient light above **10 raw counts** turns the actuator buzzer on; a reading at or below that threshold turns it off. This legacy experiment retains the original raw-light threshold.
- **Gesture-controlled light:** UP or LEFT turns the relay on; DOWN or RIGHT turns it off. Other gesture results leave its state unchanged.
- **Wi-Fi control panel:** switches the controller LED on **GPIO 2** and the actuator buzzer. Enter your Wi-Fi credentials in the sketch, then open the printed IP address from a device on the same network. Local control does not require Internet access.
- **Ambient light:** the master and light/proximity demo report the APDS9960 RGB-derived lux approximation. The raw accessor remains available for legacy thresholds. Proximity remains a raw reading; nominal lux is not meter-calibrated.
- **APDS9960 IDs:** initialization accepts `0xAB`, `0x9C` and `0xA8`.

## Bluetooth Low Energy

The master advertises as `MYOSA_1`. Services 0 through 4 publish sensor data; service 5 retains the event-command endpoint. Existing UUIDs and command-field order are preserved for compatible clients. BLE events can control the relay and buzzer.

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

## Support and attribution

- [MYOSA website](https://myosa-sensors.org/)
- [Setup and usage documentation](https://wiki.myosa-sensors.org/docs/intro/)
- [Report an issue](https://github.com/myosa-sensors/arduino-libraries/issues) with the library version, example, controller, ESP32 core version and relevant Serial output.
- Contact Team MYOSA: [myosa.event@gmail.com](mailto:myosa.event@gmail.com)

12 September, 2026 by Pegasus Automation  
(as a part of MYOSA Initiative)

Bundled third-party code retains its original attribution and license notices in the source files and component license files.
