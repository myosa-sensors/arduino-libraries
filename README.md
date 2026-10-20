# MYOSA Arduino Libraries

**Version 2.0.0**

[MYOSA (Make Your Own Sensors Applications)](https://myosa-sensors.org/) is a modular platform for learning and building sensor applications. This repository provides the latest MYOSA Arduino libraries and example sketches.

## Documentation and setup

For installation, hardware connections, Arduino IDE setup, uploading sketches and getting started, follow the [official MYOSA documentation](https://wiki.myosa-sensors.org/docs/intro/).

## Included libraries

| Folder | Hardware | Purpose |
| --- | --- | --- |
| [AccelAndGyro](AccelAndGyro/) | MPU6050 | Acceleration, angular velocity, tilt and temperature |
| [BarometricPressure](BarometricPressure/) | BMP180 | Pressure, temperature and calculated altitude |
| [HeartRateAndSpO2](HeartRateAndSpO2/) | MAX30100 | Heart rate, estimated SpO2, raw optical samples and die temperature |
| [LightProximityAndGesture](LightProximityAndGesture/) | APDS9960 | Ambient light, RGB proportions, proximity and gestures |
| [MYOSA](MYOSA/) | Combined library | Sensor initialization, OLED pages, Wi-Fi and BLE integration |
| [OLED](OLED/) | SSD1306 | 128 x 64 display, graphics and fonts |
| [ProximityAndDistance](ProximityAndDistance/) | VL53L0X | Distance in millimeters and range-status reporting |

Each library contains `src/`, `library.properties` and `keywords.txt`. All bundled sketches are inside `MYOSA/examples/`.

## Master example

[1_MasterCode](MYOSA/examples/%232_Experiments/1_MasterCode/1_MasterCode.ino) prints readings to Serial at **115200 baud**, cycles through OLED pages and sends readings over Wi-Fi or BLE. It initializes shared `Wire` at **100 kHz** and continues with available boards when a module cannot be initialized.

The master cycles through **10 sensor OLED pages** at 1.5 seconds per page, plus a connection-address page in Wi-Fi mode. Keep calling `myosa.update()` frequently in custom loops so MAX30100 sampling and VL53L0X servicing continue between display and network updates; avoid long blocking delays.

## Master connection settings

Set these five fields in `1_MasterCode.ino` before uploading:

| Field | Purpose | Default |
| --- | --- | --- |
| `mode` | `WIFI` or `BLE` | `BLE` |
| `ap_ssid` | MYOSA hotspot name | `MYOSA` |
| `ap_password` | Hotspot password, 8 to 63 characters | `myosa1234` |
| `sta_ssid` | Existing Wi-Fi network name | Empty |
| `sta_password` | Existing Wi-Fi network password | Empty |

**Wi-Fi:** AP and STA run together. Join the MYOSA hotspot and open **http://192.168.4.1/**, or join the same LAN as the controller and open its assigned IP shown on Serial and OLED. Empty STA credentials or an unavailable router leave the hotspot accessible. The station retries after disconnection. `http://myosa.local/` is an optional address where mDNS works; use the numeric IP otherwise. Use a LAN subnet other than `192.168.4.0/24` to avoid an overlap with the hotspot. No Internet or separate webpage upload is needed. The dashboard shows measurements; it does not control outputs.

**BLE:** Connect to `MYOSA_1` with a BLE-capable application or a custom client and subscribe to the sensor notification characteristics. Students can choose how to display, record or process the readings. BLE mode operates independently of the Wi-Fi dashboard.

The Wi-Fi dashboard clears stale readings instead of leaving them displayed as current. Pulse values may remain unavailable while the finger signal settles. All scripts and styling are embedded in the controller for offline use.

The editable dashboard source is `MYOSA/extras/web/dashboard.html`. After changing it, run `python MYOSA/extras/web/build_dashboard.py` to regenerate the compressed Wi-Fi asset in `MYOSA/src/myosaDashboard.h`. Re-upload the master sketch to update the ESP32 webpage, then open it at the controller's AP or LAN address.

## Controller outputs and measurements

- **Burglar alarm:** ambient light above the configurable `LIGHT_THRESHOLD_LUX` value of **10 lux** sounds the active-high buzzer on **GPIO 12** and shows an OLED alert. At or below the threshold it clears the alarm. Unavailable readings show an error and preserve the previous buzzer state.
- **Wi-Fi control panel:** switches the LED on **GPIO 2** and buzzer on **GPIO 12**. Enter your Wi-Fi credentials in the sketch, then open the printed IP address from a device on the same network. Local control does not require Internet access.
- **Ambient light:** the master and light example use the APDS9960 RGB-derived lux approximation with gain/integration compensation. It is nominal illuminance, not a meter-calibrated measurement. Proximity and MAX30100 raw optical samples keep their raw units.
- **APDS9960 IDs:** `0xAB`, `0x9C`, `0xA8` and `0x9E` are accepted by initialization.
- **MAX30100:** pulse and estimated SpO2 require a stable finger signal; unavailable values are reported while samples settle or signal quality is insufficient. This driver targets MAX30100, not MAX30102. See [HeartRateAndSpO2](HeartRateAndSpO2/README.md).
- **VL53L0X:** inspect range status as well as distance; invalid or out-of-range results are not valid distances. See [ProximityAndDistance](ProximityAndDistance/README.md).

## Bluetooth Low Energy

The master advertises as `MYOSA_1` and publishes sensor notifications. Motion, pressure and light retain their existing service UUIDs. Distance and pulse characteristics share the service ending in `b5`; their characteristic suffixes are `60`, `70` and `71`.

The master queues a fresh snapshot every second. Call `update()` frequently: it spaces notifications and services the optical sensors without a blocking delay. A stalled transmission closes the BLE link so the client can reconnect.

Notifications contain text numbers; multi-value readings use comma-separated values in the existing field order. Clients must enable notifications for each required characteristic. Request an ATT MTU of **128**, where the client API supports it, to accommodate multi-value readings; payloads larger than the negotiated MTU minus three bytes are skipped. Unavailable measurements are not notified, so clients should track freshness and clear stale values.

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
| [1_MasterCode](MYOSA/examples/%232_Experiments/1_MasterCode/1_MasterCode.ino) | Cycle through sensor readings on Serial and OLED; send Wi-Fi or BLE data |
| [2_BurglarAlarm](MYOSA/examples/%232_Experiments/2_BurglarAlarm/2_BurglarAlarm.ino) | Detect light entering a normally dark locker |

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

20 October, 2026 by Pegasus Automation  
(as a part of MYOSA Initiative)

Bundled third-party code retains its original attribution and license notices in the source files and component license files.
