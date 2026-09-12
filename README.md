# MYOSA_Legacy

MYOSA (Make Your Own Sensor Applications), legacy hardware bundle, version 2.0.0.

## Libraries

- `AccelAndGyro/`
- `Actuator/`
- `AirQuality/`
- `BarometricPressure/`
- `LightProximityAndGesture/`
- `MYOSA/`
- `OLED/`
- `TempAndHumidity/`

The inner `MYOSA/` directory is the combined Arduino library; its `examples/` directory contains the sketches. Original example names and numbering are retained.

## Master code

`MYOSA/examples/#2_Experiments/1_MasterCode/1_MasterCode.ino` cycles through 11 OLED pages and prints the same readings to Serial at 115200 baud. Initialize shared Wire at 100 kHz.

Services 0..4 publish sensor values; service 5 retains the event-command endpoint. Relay is IO0 and buzzer is IO1. UUIDs and command fields retain their existing format.

Ambient light uses an RGB-derived lux approximation; absolute accuracy needs comparison with a lux meter.

## Examples

Open the examples under `MYOSA/examples/` using Arduino IDE with the ESP32 board package and this bundle installed.

11 September, 2026 by Pegasus Automation  
(as a part of MYOSA Initiative)

Contact Team MYOSA  
Email: myosa.event@gmail.com
