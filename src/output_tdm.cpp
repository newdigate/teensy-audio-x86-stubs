/* Audio Library for Teensy 3.X
 * Copyright (c) 2017, Paul Stoffregen, paul@pjrc.com
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

#include "output_tdm.h"
#include "memcpy_audio.h"

audio_block_t * AudioOutputTDM::block_input[16] = {
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
};
bool AudioOutputTDM::update_responsibility = false;
//DMAChannel AudioOutputTDM::dma(false);
__attribute__((aligned(32)))
static uint32_t zeros[AUDIO_BLOCK_SAMPLES/2];
__attribute__((aligned(32)))
static uint32_t tdm_tx_buffer[AUDIO_BLOCK_SAMPLES*16];


void AudioOutputTDM::begin(void)
{

	for (int i=0; i < 16; i++) {
		block_input[i] = nullptr;
	}
	memset(zeros, 0, sizeof(zeros));
	memset(tdm_tx_buffer, 0, sizeof(tdm_tx_buffer));

	config_tdm();

}

// TODO: needs optimization...
static void memcpy_tdm_tx(uint32_t *dest, const uint32_t *src1, const uint32_t *src2)
{
	uint32_t i, in1, in2, out1, out2;

	for (i=0; i < AUDIO_BLOCK_SAMPLES/4; i++) {

		in1 = *src1++;
		in2 = *src2++;
		out1 = (in1 << 16) | (in2 & 0xFFFF);
		out2 = (in1 & 0xFFFF0000) | (in2 >> 16);
		*dest = out1;
		*(dest + 8) = out2;

		in1 = *src1++;
		in2 = *src2++;
		out1 = (in1 << 16) | (in2 & 0xFFFF);
		out2 = (in1 & 0xFFFF0000) | (in2 >> 16);
		*(dest + 16)= out1;
		*(dest + 24) = out2;

		dest += 32;
	}
}

void AudioOutputTDM::isr(void)
{

}


void AudioOutputTDM::update(void)
{
	audio_block_t *prev[16];
	unsigned int i;

	__disable_irq();
	for (i=0; i < 16; i++) {
		prev[i] = block_input[i];
		block_input[i] = receiveReadOnly(i);
	}
	__enable_irq();
	for (i=0; i < 16; i++) {
		if (prev[i]) release(prev[i]);
	}
}


void AudioOutputTDM::config_tdm(void)
{
}
