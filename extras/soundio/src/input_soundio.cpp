#include "input_soundio.h"
#include <soundio/soundio.h>

audio_block_t * AudioInputSoundIO::block_left = NULL;
audio_block_t * AudioInputSoundIO::block_right = NULL;
bool AudioInputSoundIO::update_responsibility = false;
struct SoundIo *AudioInputSoundIO::soundio = NULL;
struct RecordContext AudioInputSoundIO::rc;

void AudioInputSoundIO::read_callback(struct SoundIoInStream *instream, int frame_count_min, int frame_count_max) {
    struct RecordContext *rc = ( RecordContext *)instream->userdata;
    struct SoundIoChannelArea *areas;
    int err;

    char *write_ptr = soundio_ring_buffer_write_ptr(rc->ring_buffer);
    int free_bytes = soundio_ring_buffer_free_count(rc->ring_buffer);
    int free_count = free_bytes / instream->bytes_per_frame;

    if (free_count < frame_count_min) {
        fprintf(stderr, "ring buffer overflow\n");
        exit(1);
    }

    int write_frames = min(free_count, frame_count_max);
    int frames_left = write_frames;

    for (;;) {
        int frame_count = frames_left;

        if ((err = soundio_instream_begin_read(instream, &areas, &frame_count))) {
            fprintf(stderr, "begin read error: %s", soundio_strerror(err));
            exit(1);
        }

        if (!frame_count)
            break;

        if (!areas) {
            // Due to an overflow there is a hole. Fill the ring buffer with
            // silence for the size of the hole.
            memset(write_ptr, 0, frame_count * instream->bytes_per_frame);
        } else {
            for (int frame = 0; frame < frame_count; frame += 1) {
                for (int ch = 0; ch < instream->layout.channel_count; ch += 1) {
                    memcpy(write_ptr, areas[ch].ptr, instream->bytes_per_sample);
                    areas[ch].ptr += areas[ch].step;
                    write_ptr += instream->bytes_per_sample;
                }
            }
        }

        if ((err = soundio_instream_end_read(instream))) {
            fprintf(stderr, "end read error: %s", soundio_strerror(err));
            exit(1);
        }

        frames_left -= frame_count;
        if (frames_left <= 0)
            break;
    }

    int advance_bytes = write_frames * instream->bytes_per_frame;
    soundio_ring_buffer_advance_write_ptr(rc->ring_buffer, advance_bytes);
}

void AudioInputSoundIO::overflow_callback(struct SoundIoInStream *instream) {
    static int count = 0;
    fprintf(stderr, "overflow %d\n", ++count);
}

void AudioInputSoundIO::begin(void)
{
    soundio = soundio_create();
    if (!soundio) {
        fprintf(stderr, "out of memory\n");
        return;
    }

    int err = soundio_connect(soundio);
    if (err) {
        fprintf(stderr, "error connecting: %s", soundio_strerror(err));
        return;
    }

    soundio_flush_events(soundio);

    struct SoundIoDevice *selected_device = NULL;

    int device_index = soundio_default_input_device_index(soundio);
    selected_device = soundio_get_input_device(soundio, device_index);
    if (!selected_device) {
        fprintf(stderr, "No input devices available.\n");
        return;
    }

    fprintf(stderr, "Device: %s\n", selected_device->name);

    if (selected_device->probe_error) {
        fprintf(stderr, "Unable to probe device: %s\n", soundio_strerror(selected_device->probe_error));
        return;
    }

    soundio_device_sort_channel_layouts(selected_device);

    int sample_rate = 44100;
    double latency = 0.0;
    enum SoundIoFormat fmt = SoundIoFormatS16NE;

    struct SoundIoInStream *instream = soundio_instream_create(selected_device);
    if (!instream) {
        fprintf(stderr, "out of memory\n");
        return;
    }
    instream->format = fmt;
    instream->sample_rate = sample_rate;
    instream->read_callback = read_callback;
    instream->overflow_callback = overflow_callback;
    instream->software_latency = latency;
    instream->userdata = &rc;

    if ((err = soundio_instream_open(instream))) {
        fprintf(stderr, "unable to open input stream: %s", soundio_strerror(err));
        return;
    }

    fprintf(stderr, "%s %dHz %s interleaved\n",
            instream->layout.name, sample_rate, soundio_format_string(fmt));

    const int ring_buffer_duration_seconds = 30;
    int capacity = ring_buffer_duration_seconds * instream->sample_rate * instream->bytes_per_frame;
    rc.ring_buffer = soundio_ring_buffer_create(soundio, capacity);
    if (!rc.ring_buffer) {
        fprintf(stderr, "out of memory\n");
        return;
    }

    if ((err = soundio_instream_start(instream))) {
        fprintf(stderr, "unable to start input device: %s", soundio_strerror(err));
        return;
    }

    update_responsibility = update_setup();

    active = true;
}

void AudioInputSoundIO::update(void)
{
    soundio_flush_events(soundio);
    delay(1);
    int fill_bytes = soundio_ring_buffer_fill_count(rc.ring_buffer);
    if (fill_bytes == 0) return;

    int16_t *read_buf = (int16_t *)soundio_ring_buffer_read_ptr(rc.ring_buffer);

    soundio_ring_buffer_advance_read_ptr(rc.ring_buffer, AUDIO_BLOCK_SAMPLES * 4);

	// allocate 2 new blocks, but if one fails, allocate neither
	block_left = allocate();
	if (block_left != NULL) {
		block_right = allocate();
		if (block_right == NULL) {
			release(block_left);
            block_left = NULL;
		}
	}

    for (int i=0; i<AUDIO_BLOCK_SAMPLES; i++) {
        int16_t leftSample = *read_buf++;
        block_left->data[i] = leftSample;
        int16_t rightSample = *read_buf++;
        block_right->data[i] = rightSample;
    }

    // then transmit the DMA's former blocks
    transmit(block_left, 0);
    release(block_left);
    transmit(block_right, 1);
    release(block_right);
}
