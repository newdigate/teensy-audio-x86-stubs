// Plays a RAW (16-bit signed) PCM audio file at slower or faster rate
// this example plays a sample stored in an array
#include <Arduino.h>
#include <Audio.h>
#include <iostream>
#include <fstream>
#include "../../src/output_soundio.h"
#include <signal.h>
#include "AudioSampleKick.h"

// GUItool: begin automatically generated code
AudioPlayMemory          memory;        //xy=306,225
AudioRecordQueue         queue1;         //xy=609,267
AudioOutputSoundIO       sio_out1;       //xy=612,224
AudioConnection          patchCord1(memory, 0, sio_out1, 0);
AudioConnection          patchCord2(memory, 0, sio_out1, 1);
AudioConnection          patchCord3(memory, 0, queue1, 0);
// GUItool: end automatically generated code

int16_t buffer[512] = {0};
std::ofstream frec;
unsigned long lastSamplePlayed = 0;
void my_handler(sig_atomic_t i){
    if ( i== SIGINT) {
        frec.close();
        printf("Caught signal %d\n",i);
        exit(1); 
    }
}

void setup() {
    signal (SIGINT,my_handler);
    Serial.begin(9600);
    AudioMemory(20);

    memory.play(AudioSampleKick);
    
    //rraw_a1.play((const unsigned int *)kick_raw);    
    Serial.println("setup done");

    queue1.begin();
    Serial.println("startRecording");
    frec.open("./RECORD.RAW");
    if (!frec.is_open()){
        Serial.println("couldn't open file for recording...");
        exit(-1);
    }
}

void loop() {
    /*
    unsigned currentMillis = millis();
    if (currentMillis > lastSamplePlayed + 1000) {
        if (!memory.isPlaying()) {
            memory.play(AudioSampleKick);
        }
        lastSamplePlayed = currentMillis;
    }
    */

    if (queue1.available() >= 1) {   
        memcpy(buffer, queue1.readBuffer(), 256);
        queue1.freeBuffer();
        frec.write((char *)buffer, 256);
        frec.flush();
        //Serial.print(".\n");
        //Serial.flush();
    } 
}

int main() {
    initialize_mock_arduino();
    setup();    
    while(true){
        loop();
    }
}