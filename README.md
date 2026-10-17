# FFTAnalytics — Freenove FNK0086

ESP32-S3 Arduino firmware for FFT analysis of MAX30105 infrared signals. MAX30102 sensors supplied with the Freenove kit use the same SparkFun library. Displays estimated BPM on the ST7789 screen and prints the positive-frequency spectrum as CSV at 115200 baud.

## Visual Studio Code

Open `esp32-s3-max30105-fft.code-workspace`, install the recommended **PlatformIO IDE** extension, then use **PlatformIO: Build**, **Upload**, and **Serial Monitor**. Dependencies are pinned in `platformio.ini` and download automatically. Use the USB-to-UART connection; native USB CDC is not enabled. If auto-reset fails during upload, hold BOOT, tap RESET, start upload and release BOOT.

Command line alternative:

```sh
python -m pip install platformio==6.1.18
python -m platformio run
python -m platformio run -t upload
python -m platformio device monitor -b 115200
```

The compatible ESP32-S3 DevKitC-1 MCU target is used; PSRAM is unnecessary for this firmware. Close the serial monitor before uploading.

## Connections

Sensor SDA = GPIO2; SCL = GPIO1; common GND. Power the breakout according to its specifications; ESP32 GPIO uses 3.3V logic. Use the shield I2C connector for the kit sensor.

The display configuration in `include/freenove_tft.h` follows Freenove TFT_eSPI_Setups v1.3 FNK0086A CFG1: MOSI=20, clock=21, DC=0, CS=-1, reset=-1. Verify against your kit revision before flashing; revisions may require different pins. Fit the screen to the shield. For inverted colors use TFT_INVERSION_ON and TFT_RGB instead of TFT_INVERSION_OFF and TFT_BGR (CFG2). Touch is unused.

References:
- https://github.com/Freenove/Freenove_Development_Kit_for_ESP32_S3
- https://docs.freenove.com/projects/fnk0086/en/latest/fnk0086/codes/tutorial/7_Drving_Freenove_2.8-Inch_Screen.html
- https://github.com/sparkfun/SparkFun_MAX3010x_Sensor_Library

## Signal processing

100 sensor conversions/s with FIFO averaging of 4 produces 25 FIFO samples/s, with no further decimation. Each 128-point window spans 5.12 seconds. DC removal and a Hamming window precede the FFT; the strongest bin center within 40–220 BPM determines the estimate. Frequency resolution is 0.1953125 Hz, equivalent to 11.71875 BPM. Decimal formatting does not imply sub-bin precision. Motion and harmonics can cause incorrect estimates. SpO2 is not calculated.

Corrections to the supplied sketch: pulse width 411 now uses an int instead of an overflowing byte; sampling frequency accounts for hardware averaging; getFIFOIR reads queued samples in order; sampling waits yield and time out; text is white on black. The inherited finger threshold of 50000 may need tuning.

The original source is preserved in `original/arduinocode.txt`. Compilation is a software check only; physical sensor and display behavior still require hardware testing.

## Repository

https://github.com/brackeys21/FFTAnalytics

Clone with VS Code's **Git: Clone** command or download the repository ZIP. A GitHub Actions workflow builds the firmware on pushes and pull requests.
