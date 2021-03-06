#include <Audio.h>
#include "teensy_main.h"
#include "../../src/output_soundio.h"

// GUItool: begin automatically generated code
AudioSynthWaveformSine   sine1;           //xy=87,102
AudioSynthWaveformSine   sine2;           //xy=87,102
AudioOutputSoundIO           i2s1;           //xy=740,266
AudioConnection connection1(sine1, 0, i2s1, 0);
AudioConnection connection2(sine2, 0, i2s1, 1);
// GUItool: end automatically generated code

void setup() {
  Serial.begin(57600);
  AudioMemory(24);
  sine1.frequency(120.0);
  sine2.frequency(125.0);
}

void loop() {
    delay(1000);
    Serial.println("hello world...");
}

