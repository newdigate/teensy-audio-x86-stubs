#ifndef _input_soundio_h_
#define _input_soundio_h_

#include <Arduino.h>
#include <AudioStream.h>
#include <atomic>


struct RecordContext {
    struct SoundIoRingBuffer *ring_buffer;
};

class AudioInputSoundIO : public AudioStream
{
public:
    AudioInputSoundIO(void) : AudioStream(0, NULL) { begin(); }
	virtual void update(void);
	void begin(void);
	// True if begin() failed or a read callback hit a fatal error. Poll from
	// loop(); the realtime callback never exits the process.
	bool hasError() const;
	const char *lastError() const;
	// Count of input batches dropped because the ring buffer was full (the host
	// is consuming slower than the device produces). Recoverable, not fatal.
	unsigned long overflowCount() const;
protected:
	static bool update_responsibility;

private:
	static audio_block_t *block_left;
	static audio_block_t *block_right;

    static struct SoundIo *soundio;
    static std::atomic<int> last_error;            // soundio error code, 0 = none
    static std::atomic<unsigned long> overflow_count;

    static void overflow_callback(struct SoundIoInStream *instream);
    static void read_callback(struct SoundIoInStream *instream, int frame_count_min, int frame_count_max);

    static struct RecordContext rc;
};

#endif