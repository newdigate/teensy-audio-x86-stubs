/* Audio Library for Teensy 3.X
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <Arduino.h>
#include "output_i2s.h"

#include "memcpy_audio.h"

// high-level explanation of how this I2S & DMA code works:
// https://forum.pjrc.com/threads/65229?p=263104&viewfull=1#post263104


audio_block_t * AudioOutputI2S::block_left_1st = NULL;
audio_block_t * AudioOutputI2S::block_right_1st = NULL;
audio_block_t * AudioOutputI2S::block_left_2nd = NULL;
audio_block_t * AudioOutputI2S::block_right_2nd = NULL;
uint16_t  AudioOutputI2S::block_left_offset = 0;
uint16_t  AudioOutputI2S::block_right_offset = 0;
bool AudioOutputI2S::update_responsibility = false;
__attribute__((aligned(32))) static uint32_t i2s_tx_buffer[AUDIO_BLOCK_SAMPLES];

#if defined(__IMXRT1062__)
#include "utility/imxrt_hw.h"
#endif

void AudioOutputI2S::begin(void)
{
	//dma.begin(true); // Allocate the DMA channel first

	block_left_1st = NULL;
	block_right_1st = NULL;

	config_i2s();

	update_responsibility = update_setup();
}


void AudioOutputI2S::isr(void)
{
    int16_t *dest;
    audio_block_t *blockL, *blockR;
    uint32_t saddr, offsetL, offsetR;

    dest = (int16_t *)&i2s_tx_buffer[0];
    if (AudioOutputI2S::update_responsibility) AudioStream::update_all();

    blockL = AudioOutputI2S::block_left_1st;
    blockR = AudioOutputI2S::block_right_1st;
    offsetL = AudioOutputI2S::block_left_offset;
    offsetR = AudioOutputI2S::block_right_offset;

    /*
    if (blockL && blockR) {
        memcpy_tointerleaveLR(dest, blockL->data + offsetL, blockR->data + offsetR);
        offsetL += AUDIO_BLOCK_SAMPLES / 2;
        offsetR += AUDIO_BLOCK_SAMPLES / 2;
    } else if (blockL) {
        memcpy_tointerleaveL(dest, blockL->data + offsetL);
        offsetL += AUDIO_BLOCK_SAMPLES / 2;
    } else if (blockR) {
        memcpy_tointerleaveR(dest, blockR->data + offsetR);
        offsetR += AUDIO_BLOCK_SAMPLES / 2;
    } else {
        memset(dest,0,AUDIO_BLOCK_SAMPLES * 2);
    }
     */

    if (offsetL < AUDIO_BLOCK_SAMPLES) {
        AudioOutputI2S::block_left_offset = offsetL;
    } else {
        AudioOutputI2S::block_left_offset = 0;
        AudioStream::release(blockL);
        AudioOutputI2S::block_left_1st = AudioOutputI2S::block_left_2nd;
        AudioOutputI2S::block_left_2nd = NULL;
    }
    if (offsetR < AUDIO_BLOCK_SAMPLES) {
        AudioOutputI2S::block_right_offset = offsetR;
    } else {
        AudioOutputI2S::block_right_offset = 0;
        AudioStream::release(blockR);
        AudioOutputI2S::block_right_1st = AudioOutputI2S::block_right_2nd;
        AudioOutputI2S::block_right_2nd = NULL;
    }
}




void AudioOutputI2S::update(void)
{
	// null audio device: discard all incoming data
	//if (!active) return;
	//audio_block_t *block = receiveReadOnly();
	//if (block) release(block);

	audio_block_t *block;
	block = receiveReadOnly(0); // input 0 = left channel
	if (block) {
        /*
	    for (int i=0; i<AUDIO_BLOCK_SAMPLES;i++) {
	        Serial.printf("[%d]\n", block->data[i]);
	    }
        Serial.print("fff \n"); */
		__disable_irq();
		if (block_left_1st == NULL) {
			block_left_1st = block;
			block_left_offset = 0;
			__enable_irq();
		} else if (block_left_2nd == NULL) {
			block_left_2nd = block;
			__enable_irq();
		} else {
			audio_block_t *tmp = block_left_1st;
			block_left_1st = block_left_2nd;
			block_left_2nd = block;
			block_left_offset = 0;
			__enable_irq();
			release(tmp);
		}
	}
	block = receiveReadOnly(1); // input 1 = right channel
	if (block) {
		__disable_irq();
		if (block_right_1st == NULL) {
			block_right_1st = block;
			block_right_offset = 0;
			__enable_irq();
		} else if (block_right_2nd == NULL) {
			block_right_2nd = block;
			__enable_irq();
		} else {
			audio_block_t *tmp = block_right_1st;
			block_right_1st = block_right_2nd;
			block_right_2nd = block;
			block_right_offset = 0;
			__enable_irq();
			release(tmp);
		}
	}
}

void AudioOutputI2S::config_i2s(bool only_bclk)
{
}


/******************************************************************/

void AudioOutputI2Sslave::begin(void)
{

	block_left_1st = NULL;
	block_right_1st = NULL;

	AudioOutputI2Sslave::config_i2s();


	update_responsibility = update_setup();
}

void AudioOutputI2Sslave::config_i2s(void)
{
}