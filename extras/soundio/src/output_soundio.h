#ifndef output_soundio_h_
#define output_soundio_h_

#include <Arduino.h>
#include <AudioStream.h>
#include <soundio/soundio.h>
#include <thread>
#include <atomic>
class AudioOutputSoundIO : public AudioStream
{
public:
    AudioOutputSoundIO(void) : AudioStream(2, inputQueueArray) { begin(); }
	virtual void update(void);
	void begin(void);
	void end(void);
	// True if begin() failed or the stream hit a fatal error in its callback.
	// Poll from loop() and react (report / end() / restart) — the realtime
	// callback never exits the process.
	bool hasError() const;
	const char *lastError() const;
protected:
	static audio_block_t *block_left_1st;
	static audio_block_t *block_right_1st;
	static bool update_responsibility;
	static void isr(void);

	struct SoundIo *soundio;
	struct SoundIoDevice *device;
	struct SoundIoOutStream *outstream;
private:
	static audio_block_t *block_left_2nd;
	static audio_block_t *block_right_2nd;
	static uint16_t block_left_offset;
	static uint16_t block_right_offset;
	audio_block_t *inputQueueArray[2];
    static double seconds_offset;
    static void write_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max);
    static volatile bool want_pause;
    static std::atomic<int> last_error; // soundio error code, 0 = none
    static void underflow_callback(struct SoundIoOutStream *outstream);

/* Teensyduino Audio Memcpy
 * Copyright (c) 2016 Frank Bösing
 */

	static void tointerleaveLR(int16_t *dst, const int16_t *srcL, const int16_t *srcR);
	static void tointerleaveL(int16_t *dst, const int16_t *srcL);
	static void tointerleaveR(int16_t *dst, const int16_t *srcR);
};


#endif
