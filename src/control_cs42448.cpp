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
#include "control_cs42448.h"


static const uint8_t default_config[] = {
	0xF4, // CS42448_Functional_Mode = slave mode, MCLK 25.6 MHz max
	0x76, // CS42448_Interface_Formats = TDM mode
	0x1C, // CS42448_ADC_Control_DAC_DeEmphasis = single ended ADC
	0x63, // CS42448_Transition_Control = soft vol control
	0xFF  // CS42448_DAC_Channel_Mute = all outputs mute
};

bool AudioControlCS42448::enable(void)
{
	return true;
}

bool AudioControlCS42448::volumeInteger(uint32_t n)
{
	return true;
}

bool AudioControlCS42448::volumeInteger(int channel, uint32_t n)
{
	return true;
}

bool AudioControlCS42448::inputLevelInteger(int32_t n)
{

	return true;
}

bool AudioControlCS42448::inputLevelInteger(int chnnel, int32_t n)
{

	return true;
}

bool AudioControlCS42448::write(uint32_t address, uint32_t data) {
	return true;
}

bool AudioControlCS42448::write(uint32_t address, const void *data, uint32_t len)
{
	 return true;

}


