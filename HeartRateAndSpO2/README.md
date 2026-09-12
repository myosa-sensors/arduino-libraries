# HeartRateAndSpO2 2.0.0

MAX30100 support for MYOSA. This driver does not support MAX30102.

## Layout and initialization

- `src/HeartRateAndSpO2.h` contains the public interface; the matching `.cpp` contains its implementation.
- Initialize the selected `TwoWire` bus in the application, then call `begin()`. The driver does not call `Wire.begin()` or change the shared bus clock.
- Address is fixed at `0x57`. `begin()` checks part ID `0x11`, resets the device, and selects 100 Hz, 1600 us, high-resolution SpO2/HR mode, and 50 mA LEDs. Adjust LED current for the module and finger signal; reject saturated samples.
- `reset()` leaves the driver uninitialized; call `begin()` afterwards. I2C errors and conversion/reset timeouts also require `begin()` to recover safely.

## Measurements and buffering

Call `update()` every 5-10 ms at the default 100 Hz. At higher sample rates, service it faster than the 16-sample hardware FIFO fills. `update()` returns true when it consumes samples; false with `NO_DATA` is normal when no sample is ready. Check `getLastStatus()` for other failures.

`getRawValues()` peeks at the latest buffered sample. `getRawSample(0, ...)` reads the oldest, and `popRawValue()` removes the oldest. The software queue retains the newest 16 samples and overwrites the oldest when full. Timestamps are estimated sample times in microseconds and wrap as `micros()` does. Hardware overflow discards the FIFO and resets processing; `isFifoOverflowed()` reports that event until the next FIFO operation.

`getHeartRate()` returns beats/minute. `getSpO2()` returns an estimated percentage. Both return `NAN` until sufficient usable samples exist; lost fingers, saturated signals, stale data, and transfer failures invalidate measurements. Keep calling `update()` even when displaying readings less often. Processing uses a two-second settling period, adaptive pulse detection, and bounded averaging. Finger thresholds use raw left-aligned ADC counts, with default on/off thresholds 10000/8000.

SpO2 uses AC RMS/DC ratio-of-ratios with the approximate curve `110 - 25 * ratio`. This is an unvalidated educational estimate, not a calibrated instrument reading. Actual module calibration and signal quality matter; `setSpO2Calibration(intercept, slope)` accepts an application-specific linear curve. Values outside 70-100% are unavailable. HR-only mode does not provide SpO2.

## Configuration and temperature

- Heart-rate window: 1-16 beat intervals. SpO2 window: 50-4096 samples. Defaults: 4 and 200. Configuration changes reset the affected processing history.
- Sample rates: 50, 100, 167, 200, 400, 600, 800, 1000 Hz. Pulse widths: 200, 400, 800, 1600 us. Unsupported rate/width/mode combinations return `INVALID_ARGUMENT`; choose a shorter pulse before increasing the rate.
- Integer LED current requests from 0-50 mA use the nearest hardware step. The code-based setter selects an exact datasheet step; its arguments are infrared first, red second.
- `startTemperatureSampling()` starts a nonblocking die-temperature conversion. `update()` or temperature getters advance it; `isTemperatureReady()` reports cached completion. Call the start method for each new conversion. Temperature is in degrees C/F; raw integer bytes are signed two's-complement and fractions are sixteenths of a degree. This is sensor die temperature.
- Interrupt status is accumulated from hardware reads until `clearInterrupts()`. No interrupt handler is installed; poll the driver. Hardware operations belong in the main task, not an interrupt handler.

## Validation

The bundle regression runner is `MYOSA/extras/tests/run_new_sensor_regressions.py`. Tests use mocked I2C and synthetic optical signals. Compilation and these checks do not establish real-module measurement accuracy.

Modifications: 10 September, 2026 by Pegasus Automation  
(as a part of MYOSA Initiative)  
Contact Team MYOSA - Email: myosa.event@gmail.com
