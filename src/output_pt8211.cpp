/* Audio Library for Teensy 3.X
 * Copyright (c) 2016, Paul Stoffregen, paul@pjrc.com
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

//Adapted to PT8211, Frank Bösing, Ben-Rheinland

#include "output_pt8211.h"

#if !defined(KINETISL)
#include "memcpy_audio.h"

audio_block_t * AudioOutputPT8211::block_left_1st = NULL;
audio_block_t * AudioOutputPT8211::block_right_1st = NULL;
audio_block_t * AudioOutputPT8211::block_left_2nd = NULL;
audio_block_t * AudioOutputPT8211::block_right_2nd = NULL;
uint16_t  AudioOutputPT8211::block_left_offset = 0;
uint16_t  AudioOutputPT8211::block_right_offset = 0;
bool AudioOutputPT8211::update_responsibility = false;
#if defined(AUDIO_PT8211_OVERSAMPLING)
__attribute__((aligned(32))) static uint32_t i2s_tx_buffer[AUDIO_BLOCK_SAMPLES*4];
#else
DMAMEM __attribute__((aligned(32))) static uint32_t i2s_tx_buffer[AUDIO_BLOCK_SAMPLES];
#endif

void AudioOutputPT8211::begin(void)
{

	memset(i2s_tx_buffer, 0, sizeof(i2s_tx_buffer));

	block_left_1st = NULL;
	block_right_1st = NULL;

	// TODO: should we set & clear the I2S_TCSR_SR bit here?
	config_i2s();
#if defined(KINETISK)
	CORE_PIN22_CONFIG = PORT_PCR_MUX(6); // pin 22, PTC1, I2S0_TXD0

	dma.TCD->SADDR = i2s_tx_buffer;
	dma.TCD->SOFF = 2;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
	dma.TCD->NBYTES_MLNO = 2;
	dma.TCD->SLAST = -sizeof(i2s_tx_buffer);
	dma.TCD->DADDR = &I2S0_TDR0;
	dma.TCD->DOFF = 0;
	dma.TCD->CITER_ELINKNO = sizeof(i2s_tx_buffer) / 2;
	dma.TCD->DLASTSGA = 0;
	dma.TCD->BITER_ELINKNO = sizeof(i2s_tx_buffer) / 2;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;

	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_I2S0_TX);
	update_responsibility = update_setup();
	dma.attachInterrupt(isr);
	dma.enable();
	I2S0_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE | I2S_TCSR_FR;
	return;
#elif defined(__IMXRT1052__) || defined(__IMXRT1062__)

#if defined(__IMXRT1052__)
	CORE_PIN6_CONFIG  = 3;  //1:TX_DATA0
#elif defined(__IMXRT1062__)
	arm_dcache_flush_delete(i2s_tx_buffer, sizeof(i2s_tx_buffer));
	CORE_PIN7_CONFIG  = 3;  //1:TX_DATA0
#endif

	dma.TCD->SADDR = i2s_tx_buffer;
	dma.TCD->SOFF = 2;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
	dma.TCD->NBYTES_MLNO = 2;
	dma.TCD->SLAST = -sizeof(i2s_tx_buffer);
	dma.TCD->DOFF = 0;
	dma.TCD->CITER_ELINKNO = sizeof(i2s_tx_buffer) / 2;
	dma.TCD->DLASTSGA = 0;
	dma.TCD->BITER_ELINKNO = sizeof(i2s_tx_buffer) / 2;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.TCD->DADDR = (void *)((uint32_t)&I2S1_TDR0);
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI1_TX);

	I2S1_RCSR |= I2S_RCSR_RE;
	I2S1_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE;

	update_responsibility = update_setup();
	dma.attachInterrupt(isr);
	dma.enable();
	return;
#endif
}

void AudioOutputPT8211::isr(void)
{
}



void AudioOutputPT8211::update(void)
{

	audio_block_t *block;
	block = receiveReadOnly(0); // input 0 = left channel
	if (block) {
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

#if defined(KINETISK)
// MCLK needs to be 48e6 / 1088 * 256 = 11.29411765 MHz -> 44.117647 kHz sample rate
//
#if F_CPU == 96000000 || F_CPU == 48000000 || F_CPU == 24000000
  // PLL is at 96 MHz in these modes
  #define MCLK_MULT 2
  #define MCLK_DIV  17
#elif F_CPU == 72000000
  #define MCLK_MULT 8
  #define MCLK_DIV  51
#elif F_CPU == 120000000
  #define MCLK_MULT 8
  #define MCLK_DIV  85
#elif F_CPU == 144000000
  #define MCLK_MULT 4
  #define MCLK_DIV  51
#elif F_CPU == 168000000
  #define MCLK_MULT 8
  #define MCLK_DIV  119
#elif F_CPU == 180000000
  #define MCLK_MULT 16
  #define MCLK_DIV  255
  #define MCLK_SRC  0
#elif F_CPU == 192000000
  #define MCLK_MULT 1
  #define MCLK_DIV  17
#elif F_CPU == 216000000
  #define MCLK_MULT 12
  #define MCLK_DIV  17
  #define MCLK_SRC  1
#elif F_CPU == 240000000
  #define MCLK_MULT 2
  #define MCLK_DIV  85
  #define MCLK_SRC  0
#elif F_CPU == 256000000
  #define MCLK_MULT 12
  #define MCLK_DIV  17
  #define MCLK_SRC  1
#elif F_CPU == 16000000
  #define MCLK_MULT 12
  #define MCLK_DIV  17
#else
  #error "This CPU Clock Speed is not supported by the Audio library";
#endif

#ifndef MCLK_SRC
#if F_CPU >= 20000000
  #define MCLK_SRC  3  // the PLL
#else
  #define MCLK_SRC  0  // system clock
#endif
#endif
#endif

void AudioOutputPT8211::config_i2s(void)
{
#if defined(KINETISK)
	SIM_SCGC6 |= SIM_SCGC6_I2S;
	SIM_SCGC7 |= SIM_SCGC7_DMA;
	SIM_SCGC6 |= SIM_SCGC6_DMAMUX;

	// if transmitter is enabled, do nothing
	if (I2S0_TCSR & I2S_TCSR_TE) return;


	// enable MCLK output
	I2S0_MCR = I2S_MCR_MICS(MCLK_SRC) | I2S_MCR_MOE;
	while (I2S0_MCR & I2S_MCR_DUF) ;
	I2S0_MDR = I2S_MDR_FRACT((MCLK_MULT-1)) | I2S_MDR_DIVIDE((MCLK_DIV-1));

	// configure transmitter
	I2S0_TMR = 0;
	I2S0_TCR1 = I2S_TCR1_TFW(1);  // watermark at half fifo size
	#if defined(AUDIO_PT8211_OVERSAMPLING)
		I2S0_TCR2 = I2S_TCR2_SYNC(0) | I2S_TCR2_BCP | I2S_TCR2_MSEL(1) | I2S_TCR2_BCD | I2S_TCR2_DIV(0);
	#else
		I2S0_TCR2 = I2S_TCR2_SYNC(0) | I2S_TCR2_BCP | I2S_TCR2_MSEL(1) | I2S_TCR2_BCD | I2S_TCR2_DIV(3);
	#endif
	I2S0_TCR3 = I2S_TCR3_TCE;
//	I2S0_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(15) | I2S_TCR4_MF | I2S_TCR4_FSE | I2S_TCR4_FSP | I2S_TCR4_FSD; //TDA1543
	I2S0_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(15) | I2S_TCR4_MF /*| I2S_TCR4_FSE*/ | I2S_TCR4_FSP | I2S_TCR4_FSD; //PT8211
	I2S0_TCR5 = I2S_TCR5_WNW(15) | I2S_TCR5_W0W(15) | I2S_TCR5_FBT(15);

	// configure pin mux for 3 clock signals
	CORE_PIN23_CONFIG = PORT_PCR_MUX(6); // pin 23, PTC2, I2S0_TX_FS (LRCLK)
	CORE_PIN9_CONFIG  = PORT_PCR_MUX(6); // pin  9, PTC3, I2S0_TX_BCLK
	//CORE_PIN11_CONFIG = PORT_PCR_MUX(6); // pin 11, PTC6, I2S0_MCLK

#elif ( defined(__IMXRT1052__) || defined(__IMXRT1062__) )

	CCM_CCGR5 |= CCM_CCGR5_SAI1(CCM_CCGR_ON);
//PLL:
	int fs = AUDIO_SAMPLE_RATE_EXACT;
	// PLL between 27*24 = 648MHz und 54*24=1296MHz
	int n1 = 4; //SAI prescaler 4 => (n1*n2) = multiple of 4
	int n2 = 1 + (24000000 * 27) / (fs * 256 * n1);

	double C = ((double)fs * 256 * n1 * n2) / 24000000;
	int c0 = C;
	int c2 = 10000;
	int c1 = C * c2 - (c0 * c2);
	set_audioClock(c0, c1, c2);

	// clear SAI1_CLK register locations
	CCM_CSCMR1 = (CCM_CSCMR1 & ~(CCM_CSCMR1_SAI1_CLK_SEL_MASK))
		   | CCM_CSCMR1_SAI1_CLK_SEL(2); // &0x03 // (0,1,2): PLL3PFD0, PLL5, PLL4
	CCM_CS1CDR = (CCM_CS1CDR & ~(CCM_CS1CDR_SAI1_CLK_PRED_MASK | CCM_CS1CDR_SAI1_CLK_PODF_MASK))
		   | CCM_CS1CDR_SAI1_CLK_PRED(n1-1) // &0x07
		   | CCM_CS1CDR_SAI1_CLK_PODF(n2-1); // &0x3f

	IOMUXC_GPR_GPR1 = (IOMUXC_GPR_GPR1 & ~(IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL_MASK))
			| (IOMUXC_GPR_GPR1_SAI1_MCLK_DIR | IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL(0));	//Select MCLK

	if (I2S1_TCSR & I2S_TCSR_TE) return;

//	CORE_PIN23_CONFIG = 3;  //1:MCLK
	CORE_PIN21_CONFIG = 3;  //1:RX_BCLK
	CORE_PIN20_CONFIG = 3;  //1:RX_SYNC
//	CORE_PIN6_CONFIG  = 3;  //1:TX_DATA0
//	CORE_PIN7_CONFIG  = 3;  //1:RX_DATA0

	int rsync = 0;
	int tsync = 1;
	#if defined(AUDIO_PT8211_OVERSAMPLING)
	int div = 0;
	#else
	int div = 3;
	#endif
	// configure transmitter
	I2S1_TMR = 0;
	I2S1_TCR1 = I2S_TCR1_RFW(0);
	I2S1_TCR2 = I2S_TCR2_SYNC(tsync) | I2S_TCR2_BCP | I2S_TCR2_MSEL(1) | I2S_TCR2_BCD | I2S_TCR2_DIV(div);
	I2S1_TCR3 = I2S_TCR3_TCE;
//	I2S1_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(15) | I2S_TCR4_MF | I2S_TCR4_FSE | I2S_TCR4_FSP | I2S_TCR4_FSD; //TDA1543
	I2S1_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(15) | I2S_TCR4_MF /*| I2S_TCR4_FSE*/ | I2S_TCR4_FSP | I2S_TCR4_FSD; //PT8211
	I2S1_TCR5 = I2S_TCR5_WNW(15) | I2S_TCR5_W0W(15) | I2S_TCR5_FBT(15);

	I2S1_RMR = 0;
	//I2S1_RCSR = (1<<25); //Reset
	I2S1_RCR1 = I2S_RCR1_RFW(0);
	I2S1_RCR2 = I2S_RCR2_SYNC(rsync) | I2S_RCR2_BCP | I2S_RCR2_MSEL(1) | I2S_TCR2_BCD | I2S_TCR2_DIV(div);
	I2S1_RCR3 = I2S_RCR3_RCE;
//	I2S1_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(15) | I2S_TCR4_MF | I2S_TCR4_FSE | I2S_TCR4_FSP | I2S_TCR4_FSD; //TDA1543
	I2S1_RCR4 = I2S_RCR4_FRSZ(1) | I2S_RCR4_SYWD(15) | I2S_RCR4_MF /*| I2S_RCR4_FSE*/ | I2S_RCR4_FSP | I2S_RCR4_FSD; //PT8211
	I2S1_RCR5 = I2S_RCR5_WNW(15) | I2S_RCR5_W0W(15) | I2S_RCR5_FBT(15);

#endif
}

#elif defined(KINETISL)
/**************************************************************************************
*       Teensy LC
***************************************************************************************/
// added jan 2021, Frank Bösing

audio_block_t * AudioOutputPT8211::block_left = NULL;
audio_block_t * AudioOutputPT8211::block_right = NULL;
bool AudioOutputPT8211::update_responsibility = false;

#define NUM_SAMPLES (AUDIO_BLOCK_SAMPLES / 2)

DMAMEM static int16_t i2s_tx_buffer1[NUM_SAMPLES*2];
DMAMEM static int16_t i2s_tx_buffer2[NUM_SAMPLES*2];
DMAChannel AudioOutputPT8211::dma1(false);
DMAChannel AudioOutputPT8211::dma2(false);


void AudioOutputPT8211::begin(void)
{

	memset(i2s_tx_buffer1, 0, sizeof( i2s_tx_buffer1 ) );
	memset(i2s_tx_buffer2, 0, sizeof( i2s_tx_buffer2 ) );

	dma1.begin(true); // Allocate the DMA channel first
	dma2.begin(true);

	SIM_SCGC6 |= SIM_SCGC6_I2S;//Enable I2S periphal

	// enable MCLK
	I2S0_MCR = I2S_MCR_MICS(0) | I2S_MCR_MOE;
	//MDR is not available on Teensy LC

	// configure transmitter
	I2S0_TMR = 0;
	I2S0_TCR2 = I2S_TCR2_SYNC(0) | I2S_TCR2_BCP | I2S_TCR2_MSEL(1) | I2S_TCR2_BCD | I2S_TCR2_DIV(16);
	I2S0_TCR3 = I2S_TCR3_TCE;
	I2S0_TCR4 = I2S_TCR4_FRSZ(1) | I2S_TCR4_SYWD(15) | I2S_TCR4_MF /*| I2S_TCR4_FSE*/ | I2S_TCR4_FSP | I2S_TCR4_FSD;
	I2S0_TCR5 = I2S_TCR5_WNW(15) | I2S_TCR5_W0W(15) | I2S_TCR5_FBT(15);

	// configure pin mux
	CORE_PIN22_CONFIG = PORT_PCR_MUX(6); // pin 22, PTC1, I2S0_TXD0
	CORE_PIN23_CONFIG = PORT_PCR_MUX(6); // pin 23, PTC2, I2S0_TX_FS (LRCLK)
	CORE_PIN9_CONFIG  = PORT_PCR_MUX(6); // pin  9, PTC3, I2S0_TX_BCLK
	//CORE_PIN11_CONFIG = PORT_PCR_MUX(6); // pin 11, PTC6, I2S0_MCLK

	//configure both DMA channels
	dma1.sourceBuffer(i2s_tx_buffer1, sizeof(i2s_tx_buffer1));
	dma1.destination(*(int16_t *)&I2S0_TDR0);
	dma1.triggerAtHardwareEvent(DMAMUX_SOURCE_I2S0_TX);
	dma1.interruptAtCompletion();
	dma1.disableOnCompletion();
	dma1.attachInterrupt(isr1);

	dma2.destination(*(int16_t *)&I2S0_TDR0);
	dma2.sourceBuffer(i2s_tx_buffer2, sizeof(i2s_tx_buffer2));
	dma2.interruptAtCompletion();
	dma2.disableOnCompletion();
	dma2.attachInterrupt(isr2);

	update_responsibility = update_setup();
	dma1.enable();

	I2S0_TCSR = I2S_TCSR_SR;
	I2S0_TCSR = I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FWDE;

}

void AudioOutputPT8211::update(void)
{
	if (!block_left)  block_left  = receiveReadOnly(0);// input 0 = left channel
	if (!block_right) block_right = receiveReadOnly(1);// input 1 = right channel
}

inline __attribute__((always_inline, hot))
void interleave(const int16_t *dest,const audio_block_t *block_left, const audio_block_t *block_right, const size_t offset)
{

	uint32_t *p = (uint32_t*)dest;
	uint32_t *end = p + NUM_SAMPLES;

	if (block_left != nullptr && block_right != nullptr) {
		uint16_t *l = (uint16_t*)&block_left->data[offset];
		uint16_t *r = (uint16_t*)&block_right->data[offset];
		do {
			*p++ = (((uint32_t)(*l++)) << 16)  | (uint32_t)(*r++);
			*p++ = (((uint32_t)(*l++)) << 16)  | (uint32_t)(*r++);
			*p++ = (((uint32_t)(*l++)) << 16)  | (uint32_t)(*r++);
			*p++ = (((uint32_t)(*l++)) << 16)  | (uint32_t)(*r++);
		} while (p < end);
		return;
	}

	if (block_left != nullptr) {
		uint16_t *l = (uint16_t*)&block_left->data[offset];
		do {
			*p++ = (uint32_t)(*l++) << 16;
			*p++ = (uint32_t)(*l++) << 16;
			*p++ = (uint32_t)(*l++) << 16;
			*p++ = (uint32_t)(*l++) << 16;
		} while (p < end);
		return;
	}

	if (block_right != nullptr) {
		uint16_t *r = (uint16_t*)&block_right->data[offset];
		do {
			*p++ =(uint32_t)(*r++);
			*p++ =(uint32_t)(*r++);
			*p++ =(uint32_t)(*r++);
			*p++ =(uint32_t)(*r++);
		} while (p < end);
		return;
	}

	do {
		*p++ = 0;
		*p++ = 0;
	} while (p < end);

}

void AudioOutputPT8211::isr1(void)
{	//DMA Channel 1 Interrupt

	//Start Channel 2:
	dma2.triggerAtHardwareEvent(DMAMUX_SOURCE_I2S0_TX);
	dma2.enable();
	
	//Reset & Copy Data Channel 1
	dma1.clearInterrupt();
	dma1.sourceBuffer(i2s_tx_buffer1, sizeof(i2s_tx_buffer1));
	interleave(&i2s_tx_buffer1[0], AudioOutputPT8211::block_left, AudioOutputPT8211::block_right, 0);
}

void AudioOutputPT8211::isr2(void) 
{	//DMA Channel 2 Interrupt

	//Start Channel 1:
	dma1.triggerAtHardwareEvent(DMAMUX_SOURCE_I2S0_TX);
	dma1.enable();

	//Reset & Copy Data Channel 2
	dma2.clearInterrupt();
	dma2.sourceBuffer(i2s_tx_buffer2, sizeof(i2s_tx_buffer2));

	audio_block_t *block_left = AudioOutputPT8211::block_left;
	audio_block_t *block_right = AudioOutputPT8211::block_right;

	interleave(&i2s_tx_buffer2[0], block_left, block_right, NUM_SAMPLES);

	if (block_left) AudioStream::release(block_left);
	if (block_right) AudioStream::release(block_right);

	AudioOutputPT8211::block_left = nullptr;
	AudioOutputPT8211::block_right = nullptr;

	if (AudioOutputPT8211::update_responsibility) AudioStream::update_all();
}

#else
//#error Output PT8211: No code for this CPU
#endif
