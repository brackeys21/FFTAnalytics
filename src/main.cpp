#include <Arduino.h>
#include <Wire.h>
#include "MAX30105.h"
#include <arduinoFFT.h>
#include <TFT_eSPI.h>
TFT_eSPI tft;

#define IIC_SDA 2
#define IIC_SCL 1

MAX30105 particleSensor;


const uint16_t SAMPLES = 128;
const byte NATIVE_SAMPLE_RATE = 100;
const uint8_t DECIMATION = 1;
const float SAMPLING_FREQUENCY = (float)NATIVE_SAMPLE_RATE / 4 / DECIMATION;

const float MIN_BPM = 40.0;
const float MAX_BPM = 220.0;

float vReal[SAMPLES];
float vImag[SAMPLES];

ArduinoFFT<float> FFT = ArduinoFFT<float>(
  vReal,
  vImag,
  SAMPLES,
  SAMPLING_FREQUENCY
);

const uint16_t MIN_BIN =
  (uint16_t)((MIN_BPM / 60.0) * SAMPLES / SAMPLING_FREQUENCY) + 1;

const uint16_t MAX_BIN =
  (MAX_BPM / 60.0) * SAMPLES / SAMPLING_FREQUENCY;


void showStatus(const char* msg) {
  Serial.println(msg);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString(msg, 120, 150, 2);
}
void showBPM(float bpm) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("HEART RATE", 120, 90, 2);
  char line[24];
  snprintf(line, sizeof(line), "%.1f BPM", bpm);
  tft.drawCentreString(line, 120, 140, 4);
  Serial.printf("Estimated BPM: %.1f\n", bpm);
  Serial.println("frequency_Hz,magnitude");
  for (uint16_t i = 1; i < SAMPLES / 2; ++i)
    Serial.printf("%.4f,%.3f\n", i * SAMPLING_FREQUENCY / SAMPLES, vReal[i]);
}

void setup() {

  Serial.begin(115200);
  delay(500);
  Wire.begin(IIC_SDA, IIC_SCL);
  tft.init();
  tft.setRotation(0);





  if (particleSensor.begin(Wire, I2C_SPEED_FAST) == false) {

    showStatus("Sensor not found");

    while (1) { delay(1000);
    }

  }


  byte ledBrightness = 60;
  byte sampleAverage = 4;
  byte ledMode = 2;
  int pulseWidth = 411;
  int adcRange = 4096;


  particleSensor.setup(
    ledBrightness,
    sampleAverage,
    ledMode,
    NATIVE_SAMPLE_RATE,
    pulseWidth,
    adcRange
  );


  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);


  particleSensor.clearFIFO();
  showStatus("Place finger; collecting 128 samples at 25 Hz");
}


void loop() {

  uint16_t keptSamples = 0;

  uint16_t rawCounter = 0;

  float irSum = 0;


  while (keptSamples < SAMPLES) {

    const uint32_t started = millis();
    while (particleSensor.available() == false) {

      particleSensor.check();
      if (millis() - started > 2000) {
        showStatus("Sensor timeout; check wiring");
        particleSensor.clearFIFO();
        return;
      }
      delay(1);

    }


    long irValue = particleSensor.getFIFOIR();

    particleSensor.nextSample();


    if (rawCounter % DECIMATION == 0) {

      vReal[keptSamples] = (float)irValue;

      vImag[keptSamples] = 0.0;

      irSum += irValue;

      keptSamples++;

    }


    rawCounter++;

  }


  float irAverage = irSum / SAMPLES;


  if (irAverage < 50000) {

    showStatus("No finger");

    return;

  }


  FFT.dcRemoval();

  FFT.windowing(
    FFTWindow::Hamming,
    FFTDirection::Forward
  );

  FFT.compute(
    FFTDirection::Forward
  );

  FFT.complexToMagnitude();


  uint16_t peakBin = MIN_BIN;

  float peakMagnitude = vReal[MIN_BIN];


  for (
    uint16_t i = MIN_BIN + 1;
    i <= MAX_BIN;
    i++
  ) {

    if (vReal[i] > peakMagnitude) {

      peakMagnitude = vReal[i];

      peakBin = i;

    }

  }


  float peakFrequencyHz =
    (peakBin * SAMPLING_FREQUENCY) /
    (float)SAMPLES;


  float bpm =
    peakFrequencyHz * 60.0;


  showBPM(bpm);

}