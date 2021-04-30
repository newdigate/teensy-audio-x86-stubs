#include <Audio.h>
#include "teensy_main.h"
#include "../../src/output_soundio.h"

// GUItool: begin automatically generated code
AudioSynthWaveformSine      sine1;                 //xy=87,102
AudioOutputSoundIO          i2s1;                  //xy=740,266
AudioAnalyzeRMS             analyzeRms;
AudioAnalyzePeak            analyzePeak;

AudioConnection connection1(sine1, 0, i2s1, 0);
AudioConnection connection2(sine1, 0, i2s1, 1);
AudioConnection connection3(sine1, 0, analyzeRms, 0);
AudioConnection connection4(sine1, 0, analyzePeak, 0);
// GUItool: end automatically generated code

void setup() {
  Serial.begin(57600);
  AudioMemory(24);
  sine1.frequency(120.0);
  sine1.amplitude(0.5);
}

void loop() {
    delay(1000);
    if (analyzeRms.available()) {
        Serial.printf("rms amplitude: %f\n", analyzeRms.read());
    }
    if (analyzePeak.available()) {
        Serial.printf("peak amplitude: %f\n", analyzePeak.read());
    }
}

