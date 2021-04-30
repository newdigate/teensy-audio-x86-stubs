#ifndef _input_soundio_h_
#define _input_soundio_h_

#include <Arduino.h>
#include <AudioStream.h>
#include "input_soundio.h"


struct RecordContext {
    struct SoundIoRingBuffer *ring_buffer;
};

class AudioInputSoundIO : public AudioStream
{
public:
    AudioInputSoundIO(void) : AudioStream(0, NULL) { begin(); }
	virtual void update(void);
	void begin(void);
protected:
	static bool update_responsibility;

private:
	static audio_block_t *block_left;
	static audio_block_t *block_right;

    static struct SoundIo *soundio;

    static void overflow_callback(struct SoundIoInStream *instream);
    static void read_callback(struct SoundIoInStream *instream, int frame_count_min, int frame_count_max);

    static struct RecordContext rc;
};

#endif