# ProximityAndDistance 2.0.0

VL53L0X support for MYOSA, with a bundled core adapted from Pololu and ST.

## Layout and initialization

- `src/ProximityAndDistance.h` is the public interface, with definitions in the matching `.cpp`.
- `src/detail/Vl53l0xCore.*` contains the private initialization, SPAD, timing, and calibration implementation. It needs no separately installed VL53L0X library. License notices are in `LICENSE.txt`.
- Initialize the chosen `TwoWire` bus in the application, then call `begin()`. The driver does not initialize or change the shared bus clock. The default 7-bit address is `0x29`; model ID must be `0xEE`.
- `begin(true)` selects 2.8 V I/O mode; use the mode appropriate to the actual module. A software address change accepts `0x08` through `0x77` and lasts until the device resets. Multiple devices initially at `0x29` need separate XSHUT control while assigning addresses.
- I2C failures and measurement/calibration timeouts invalidate initialization. Call `begin()` to recover. No I2C operation should run in an interrupt handler.

## Measurement results

`readDistance()` and `readRangeSingleMillimeters()` perform one measurement. `startContinuous(periodMs)` starts repeated measurements; zero requests back-to-back operation. `readDistanceContinuous()` and `readRangeContinuousMillimeters()` wait for the next result. `stopContinuous()` stops it. `setContinuousModeInterMeasurementPeriod()` starts or restarts continuous mode with the requested period.

A successful result is a distance in millimeters. `-1` means an invalid result or another error; `-2` means timeout. Inspect `getLastStatus()` and `getRangeStatus()`. The overload with output pointers returns the range status and signal rate in **Q9.7 MCPS**; divide by 128 for MCPS. `getAmbientRate()` uses the same format. `getTotalEvents()` returns the cached **32-bit** return-channel ranging event count.

Cached getters describe the last completed read and do not start a measurement. `isRangeValid()` checks the hardware status. The private core does not implement ST's host-side sigma estimator, DMAX, or cover-glass compensation. `VL53L0X_SIGMA_FAIL` is reserved; hardware status alone cannot establish measurement accuracy. Invalid hardware results retain raw range/rates for inspection; failed transfers clear them.

## Configuration

- Measurement timing budget: 20-1000 ms. A nonzero continuous period must be at least the actual timing budget. Set a timeout longer than the expected measurement/continuous period; default timeout is 500 ms. Timeouts are bounded and `setTimeout(0)` is rejected. `timeoutOccurred()` returns and clears the timeout flag.
- Signal rate limits are in MCPS. VCSEL pre-range periods: 12, 14, 16, 18 PCLKs; final-range: 8, 10, 12, 14 PCLKs.
- Long, short, and default distance methods are software presets for signal limit and VCSEL timing, not distinct hardware modes. They do not guarantee a particular range. Stop continuous mode before changing timing, address, signal limits, or VCSEL periods.
- Interrupt modes are values, not bit masks: disabled 0, low threshold 1, high threshold 2, outside window 3, new sample 4. Default is new sample, active low. Thresholds accept 0-8190 mm, rounded down to the hardware's 2 mm resolution. Set the high threshold before raising the low threshold.
- Reads wait on the configured interrupt condition. Threshold modes can time out when no threshold is crossed. Disabled interrupts prevent measurement read/start calls; select new-sample mode for ordinary polling. No interrupt handler is installed.
- `getLastI2cStatus()` uses Wire transmission codes, with `0xFD` for a short or missing read. An error is latched during each operation so later writes cannot use a failed register read.

## Provenance and validation

The core derives from Pololu's `vl53l0x-arduino` sources, retrieved 10 September, 2026. Local changes add checked transfers, bounded waits, wider timeout arithmetic, and error propagation. Original source hashes are recorded in `SOURCE.txt`; all applicable license text is retained in `LICENSE.txt`.

The bundle regression runner is `MYOSA/extras/tests/run_new_sensor_regressions.py`. Tests use mocked I2C. Validate distance, timing, interrupt behavior, and recovery on the actual hardware before relying on measurements.

Modifications: 10 September, 2026 by Pegasus Automation  
(as a part of MYOSA Initiative)  
Contact Team MYOSA - Email: myosa.event@gmail.com
