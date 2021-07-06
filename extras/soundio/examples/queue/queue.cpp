// Plays a RAW (16-bit signed) PCM audio file at slower or faster rate
// this example plays a sample stored in an array
#include <Arduino.h>
#include <Audio.h>
#include "output_soundio.h"
#include "AudioSampleKick.h"         // http://www.freesound.org/people/DWSD/sounds/171104/
// GUItool: begin automatically generated code
AudioPlayMemory         rraw_a1;        //xy=306,225
AudioRecordQueue         queue1;         //xy=609,267
AudioOutputSoundIO       sio_out1;       //xy=612,224
AudioConnection          patchCord1(rraw_a1, 0, sio_out1, 0);
AudioConnection          patchCord2(rraw_a1, 0, sio_out1, 1);
AudioConnection          patchCord3(rraw_a1, 0, queue1, 0);
// GUItool: end automatically generated code


int16_t buffer[512] = {0};
File frec;

unsigned long lastSamplePlayed = 0;

void setup() {
    Serial.begin(9600);
    AudioMemory(20);

    rraw_a1.play(AudioSampleKick);    
    Serial.println("setup done");

    queue1.begin();
    Serial.println("startRecording");
}

void loop() {
    unsigned currentMillis = millis();
    if (currentMillis > lastSamplePlayed + 1000) {
        if (!rraw_a1.isPlaying()) {
            rraw_a1.play(AudioSampleKick);
        }
        lastSamplePlayed = currentMillis;
    }

    if (queue1.available() >= 1) {   
        memcpy(buffer, queue1.readBuffer(), 256);
        queue1.freeBuffer();
        //Serial.println("r");
    } 
}

int main() {
    initialize_mock_arduino();
    setup();
    while(true){
        loop();
    }
}