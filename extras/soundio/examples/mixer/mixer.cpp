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
AudioPlayMemory          memory;         //xy=142,223
///*
AudioSynthWaveformSine   sine1;          //xy=172,297
AudioMixer4              mixer1;         //xy=392,291
//*/
AudioRecordQueue         queue1;         //xy=556,320
AudioOutputSoundIO       sio_out1;       //xy=574,249
///*
AudioConnection          patchCord1(memory, 0, mixer1, 0);
AudioConnection          patchCord2(sine1, 0, mixer1, 1);
AudioConnection          patchCord3(mixer1, 0, queue1, 0);
AudioConnection          patchCord4(mixer1, 0, sio_out1, 0);
AudioConnection          patchCord5(mixer1, 0, sio_out1, 1);
//*/

//AudioConnection          patchCord4(memory, 0, sio_out1, 0);
//AudioConnection          patchCord5(memory, 0, sio_out1, 1);
//AudioConnection          patchCord1(memory, 0, queue1, 0);
// GUItool: end automatically generated code

int16_t buffer[512] = {0};
std::ofstream frec;
unsigned long lastSamplePlayed = 0;
bool exitFlag = false;
extern std::mutex critical_section_mutex;
void my_handler(sig_atomic_t i){
    if ( i== SIGINT) {
        exitFlag = true;
        printf("Caught signal %d\n",i);
    } else 
    {
        printf(" - Caught signal %d\n",i);
    }
}

void setup() {
    signal (SIGINT,my_handler);
    Serial.begin(9600);
    AudioMemory(20);

    mixer1.gain(0, 0.25f);
    mixer1.gain(1, 0.25f);
    sine1.amplitude(1.0f);
    sine1.frequency(880.0f);
    //rraw_a1.play((const unsigned int *)kick_raw);    
    Serial.println("setup done");

    queue1.begin();
    Serial.println("startRecording");
    frec.open("RECORD.RAW");
    if (!frec.is_open()){
        Serial.println("couldn't open file for recording...");
        exit(-1);
    }
}

void loop() {
    
    unsigned currentMillis = millis();
    if (currentMillis > lastSamplePlayed + 1000) {
        if (!memory.isPlaying()) {
            memory.play(AudioSampleKick);
            lastSamplePlayed = currentMillis;

            Serial.print("Memory: ");
            Serial.print(AudioMemoryUsage());
            Serial.print(",");
            Serial.print(AudioMemoryUsageMax());
            Serial.print(", mem:");
            Serial.print(memory.memory_used);
            Serial.print(", queue:");
            Serial.print(queue1.memory_used);
            Serial.println();
        }
    }

    if (queue1.available() >= 1) {   
        memcpy(buffer, queue1.readBuffer(), 256);
        queue1.freeBuffer();
        frec.write((char *)buffer, 256);
        frec.flush();
        //Serial.print(".\n");
        //Serial.flush();
    } 
    delay(1);
}

int main() {
    initialize_mock_arduino();
    setup();    
    while(!exitFlag){
        loop();
    }
    arduino_should_exit = true;
    sio_out1.end();
    frec.close();
    Serial.printf("closing app...\n");
}