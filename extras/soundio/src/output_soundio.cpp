#include <Arduino.h>
#include "output_soundio.h"
#include "soundio_helpers.h"

audio_block_t * AudioOutputSoundIO::block_left_1st = NULL;
audio_block_t * AudioOutputSoundIO::block_right_1st = NULL;
audio_block_t * AudioOutputSoundIO::block_left_2nd = NULL;
audio_block_t * AudioOutputSoundIO::block_right_2nd = NULL;
uint16_t  AudioOutputSoundIO::block_left_offset = 0;
uint16_t  AudioOutputSoundIO::block_right_offset = 0;
bool AudioOutputSoundIO::update_responsibility = false;
double  AudioOutputSoundIO::seconds_offset = 0.0;
volatile bool AudioOutputSoundIO::want_pause = false;
std::atomic<int> AudioOutputSoundIO::last_error{0};
__attribute__((aligned(32))) static int16_t i2s_tx_buffer[AUDIO_BLOCK_SAMPLES * 2]; // * 2 becuase its stereo

void AudioOutputSoundIO::write_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max) {
    struct SoundIoChannelArea *areas;
    int err;

    const struct SoundIoChannelLayout *layout = &outstream->layout;
    const int channel_count = layout->channel_count;
    // i2s_tx_buffer only holds 2 (stereo) channels; clamp so a device layout
    // with more channels never reads past it.
    const int src_channels = audio_soundio_output_channels(channel_count);

    int frames_left = frame_count_max;

    while (frames_left > 0) {
        if (arduino_should_exit)
            break;

        // begin_write may grant FEWER frames than requested; only ever write
        // into the region soundio actually handed back.
        int frame_count = frames_left;
        if ((err = soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
            // Realtime thread: never exit(). Record the error and bail; the
            // control thread observes it via hasError() and tears down.
            last_error.store(err, std::memory_order_relaxed);
            return;
        }
        if (!frame_count)
            break;

        int written = 0;
        while (written < frame_count) {
            isr(); // generates one AUDIO_BLOCK_SAMPLES block into i2s_tx_buffer
            int n = audio_soundio_frames_this_pass(AUDIO_BLOCK_SAMPLES, frame_count - written);
            int16_t *sample = i2s_tx_buffer; // interleaved stereo: L0,R0,L1,R1,...
            for (int frame = 0; frame < n; frame += 1) {
                for (int channel = 0; channel < channel_count; channel += 1) {
                    int16_t *buf = (int16_t *)areas[channel].ptr;
                    *buf = (channel < src_channels) ? sample[channel] : 0;
                    areas[channel].ptr += areas[channel].step;
                }
                sample += 2;
            }
            written += n;
        }

        if ((err = soundio_outstream_end_write(outstream))) {
            // Underflow/interrupted are transient (the host just couldn't keep
            // up); not a failure. Anything else is fatal to the stream.
            if (audio_soundio_error_is_fatal(err, SoundIoErrorUnderflow, SoundIoErrorInterrupted))
                last_error.store(err, std::memory_order_relaxed);
            return;
        }

        frames_left -= frame_count;
    }

    soundio_outstream_pause(outstream, want_pause);
}

void AudioOutputSoundIO::begin(void)
{
    char *device_id = NULL;
    bool raw = false;
    char *stream_name = NULL;
    double latency = 0.0;
    int sample_rate = 44100;

    // Pessimistic until the stream is fully started; cleared on success. Any
    // early return below therefore leaves hasError() == true.
    last_error.store(SoundIoErrorInitAudioBackend, std::memory_order_relaxed);

    soundio = soundio_create();
    if (!soundio) {
        fprintf(stderr, "out of memory\n");
        return;
    }

    int err = soundio_connect(soundio);
    if (err) {
        fprintf(stderr, "Unable to connect to backend: %s\n", soundio_strerror(err));
        return;
    }

    fprintf(stderr, "Backend: %s\n", soundio_backend_name(soundio->current_backend));

    soundio_flush_events(soundio);

    int selected_device_index = soundio_default_output_device_index(soundio);
    if (selected_device_index < 0) {
        fprintf(stderr, "Output device not found\n");
        return;
    }

    device = soundio_get_output_device(soundio, selected_device_index);
    if (!device) {
        fprintf(stderr, "out of memory\n");
        return;
    }

    fprintf(stderr, "Output device: %s\n", device->name);
    if (device->probe_error) {
        fprintf(stderr, "Cannot probe device: %s\n", soundio_strerror(device->probe_error));
        return;
    }

    outstream = soundio_outstream_create(device);
    if (!outstream) {
        fprintf(stderr, "out of memory\n");
        return;
    }

    outstream->write_callback = write_callback;
    outstream->underflow_callback = underflow_callback;
    outstream->name = stream_name;
    outstream->software_latency = latency;
    outstream->sample_rate = sample_rate;

    if (soundio_device_supports_format(device, SoundIoFormatS16NE)) {
        outstream->format = SoundIoFormatS16NE;
    } else {
        fprintf(stderr, "No suitable device format available.\n");
        return;
    }

    if ((err = soundio_outstream_open(outstream))) {
        fprintf(stderr, "unable to open device: %s", soundio_strerror(err));
        return;
    }

    fprintf(stderr, "Software latency: %f\n", outstream->software_latency);
    fprintf(stderr, "sample_rate: %i\n", outstream->sample_rate);

    if (outstream->layout_error)
        fprintf(stderr, "unable to set channel layout: %s\n", soundio_strerror(outstream->layout_error));

    if ((err = soundio_outstream_start(outstream))) {
        fprintf(stderr, "unable to start device: %s\n", soundio_strerror(err));
        return;
    }

    block_left_1st = NULL;
	block_right_1st = NULL;

	update_responsibility = update_setup();
    active = true;
    last_error.store(0, std::memory_order_relaxed); // started cleanly
}

bool AudioOutputSoundIO::hasError() const {
    return last_error.load(std::memory_order_relaxed) != 0;
}

const char *AudioOutputSoundIO::lastError() const {
    return soundio_strerror(last_error.load(std::memory_order_relaxed));
}


void AudioOutputSoundIO::isr(void)
{
    int16_t *dest;
    audio_block_t *blockL, *blockR;
    uint32_t saddr, offsetL, offsetR;

    dest = (int16_t *)&i2s_tx_buffer[0];
    if (AudioOutputSoundIO::update_responsibility) AudioStream::update_all();

    blockL = AudioOutputSoundIO::block_left_1st;
    blockR = AudioOutputSoundIO::block_right_1st;
    offsetL = AudioOutputSoundIO::block_left_offset;
    offsetR = AudioOutputSoundIO::block_right_offset;


    if (blockL && blockR) {
        tointerleaveLR(dest, blockL->data, blockR->data);
        //offsetL += AUDIO_BLOCK_SAMPLES / 2;
        //offsetR += AUDIO_BLOCK_SAMPLES / 2;
    } else if (blockL) {
        tointerleaveL(dest, blockL->data + offsetL);
        //offsetL += AUDIO_BLOCK_SAMPLES / 2;
    } else if (blockR) {
        tointerleaveR(dest, blockR->data + offsetR);
        //offsetR += AUDIO_BLOCK_SAMPLES / 2;
    } else {
        memset(dest,0,AUDIO_BLOCK_SAMPLES * 4);
    }


    if (offsetL < AUDIO_BLOCK_SAMPLES) {
        AudioOutputSoundIO::block_left_offset = offsetL;
    } else {
        AudioOutputSoundIO::block_left_offset = 0;
        AudioStream::release(blockL);
        AudioOutputSoundIO::block_left_1st = AudioOutputSoundIO::block_left_2nd;
        AudioOutputSoundIO::block_left_2nd = NULL;
    }
    if (offsetR < AUDIO_BLOCK_SAMPLES) {
        AudioOutputSoundIO::block_right_offset = offsetR;
    } else {
        AudioOutputSoundIO::block_right_offset = 0;
        AudioStream::release(blockR);
        AudioOutputSoundIO::block_right_1st = AudioOutputSoundIO::block_right_2nd;
        AudioOutputSoundIO::block_right_2nd = NULL;
    }
}




void AudioOutputSoundIO::update(void)
{
	// null audio device: discard all incoming data
	//if (!active) return;
	//audio_block_t *block = receiveReadOnly();
	//if (block) release(block);

	audio_block_t *block;
	block = receiveReadOnly(0); // input 0 = left channel
    if (!block) {
        block = allocate();
        if (block) memset(block->data, 0, AUDIO_BLOCK_SAMPLES * 2);
    }
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
    if (!block) {
        block = allocate();
        if (block) memset(block->data, 0, AUDIO_BLOCK_SAMPLES * 2);
    }
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

void AudioOutputSoundIO::underflow_callback(struct SoundIoOutStream *outstream) {
    static int count = 0;
    fprintf(stderr, "underflow %d\n", count++);
}

void AudioOutputSoundIO::end(void) {
    arduino_should_exit = true;
    Serial.println("releasing audio");
    soundio_outstream_destroy(outstream);
    soundio_device_unref(device);
    soundio_destroy(soundio);
}
/* Teensyduino Audio Memcpy
 * Copyright (c) 2016 Frank Bösing
 */

void AudioOutputSoundIO::tointerleaveLR(int16_t *dst, const int16_t *srcL, const int16_t *srcR){
    for (int i=0; i< AUDIO_BLOCK_SAMPLES; i++) {
        *dst++ = *srcL++;
        *dst++ = *srcR++;
    }
}
void AudioOutputSoundIO::tointerleaveL(int16_t *dst, const int16_t *srcL){
    for (int i=0; i< AUDIO_BLOCK_SAMPLES; i++) {
        *dst++ = *srcL++;
        dst++;
    }
}
void AudioOutputSoundIO::tointerleaveR(int16_t *dst, const int16_t *srcR){
    for (int i=0; i< AUDIO_BLOCK_SAMPLES; i++) {
        dst++;
        *dst++ = *srcR++;
    }
}