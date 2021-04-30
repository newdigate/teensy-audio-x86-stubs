#include <Audio.h>
#include "teensy_main.h"
#include "../../src/output_soundio.h"
#include "../../src/input_soundio.h"

// GUItool: begin automatically generated code
AudioOutputSoundIO       soundIoOut;           //xy=740,266
AudioInputSoundIO        soundIoIn;
AudioConnection connection1(soundIoIn, 0, soundIoOut, 0);
AudioConnection connection2(soundIoIn, 1, soundIoOut, 1);
// GUItool: end automatically generated code

void setup() {
  Serial.begin(57600);
  AudioMemory(24);
}

void loop() {
    delay(1000);
    Serial.println("hello world...");
}

