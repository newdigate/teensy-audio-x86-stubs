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
#include "mixer.h"

#define MULTI_UNITYGAIN 256

static void applyGain(int16_t *data, int32_t mult)
{
	uint32_t *p = (uint32_t *)data;
	const uint32_t *end = (uint32_t *)(data + AUDIO_BLOCK_SAMPLES);

	do {
		uint32_t tmp32 = *p; // read 2 samples from *data
		int32_t val1 = signed_multiply_32x16b(mult, tmp32);
		int32_t val2 = signed_multiply_32x16t(mult, tmp32);
		val1 = signed_saturate_rshift(val1, 16, 0);
		val2 = signed_saturate_rshift(val2, 16, 0);
		*p++ = pack_16b_16b(val2, val1);
	} while (p < end);
}

static void applyGainThenAdd(int16_t *dst, const int16_t *src, int32_t mult)
{
	const int16_t *end = dst + AUDIO_BLOCK_SAMPLES;

	if (mult == MULTI_UNITYGAIN) {
		do {
			int32_t val = *dst + *src++;
			*dst++ = val;
		} while (dst < end);
	} else {
		do {
			int16_t postgain = ((*src++ * mult) >> 8);
			int32_t val = *dst + postgain; // overflow possible??
			int16_t ddd = max(-32768, min(val, 32767));
			*dst++ = ddd;
		} while (dst < end);
	}
}

void AudioMixer4::update(void)
{
	audio_block_t *in, *out=NULL;
	unsigned int channel;

	for (channel=0; channel < 4; channel++) {
		if (!out) {
			out = receiveWritable(channel);
			if (out) {
				int32_t mult = multiplier[channel];
				if (mult != MULTI_UNITYGAIN) applyGain(out->data, mult);
			}
		} else {
			in = receiveReadOnly(channel);
			if (in) {
				applyGainThenAdd(out->data, in->data, multiplier[channel]);
				release(in);
			}
		}
	}
	if (out) {
		transmit(out);
		release(out);
	}
}

void AudioAmplifier::update(void)
{
	audio_block_t *block;
	int32_t mult = multiplier;

	if (mult == 0) {
		// zero gain, discard any input and transmit nothing
		block = receiveReadOnly(0);
		if (block) release(block);
	} else if (mult == MULTI_UNITYGAIN) {
		// unity gain, pass input to output without any change
		block = receiveReadOnly(0);
		if (block) {
			transmit(block);
			release(block);
		}
	} else {
		// apply gain to signal
		block = receiveWritable(0);
		if (block) {
			applyGain(block->data, mult);
			transmit(block);
			release(block);
		}
	}
}
