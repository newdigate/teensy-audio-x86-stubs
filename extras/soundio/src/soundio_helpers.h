#ifndef _soundio_helpers_h_
#define _soundio_helpers_h_

// Dependency-free decision logic shared by the libsoundio input/output
// AudioStream nodes. Kept out of the .cpp files (which pull in <soundio/...>
// and the Teensy stubs) so it can be unit tested on its own. See
// extras/soundio/test/soundio_helpers_test.cpp.

// Frames to copy into the output area in a single pass. soundio_outstream_
// begin_write() may grant FEWER frames than requested, so a pass must never
// advance past the granted region. Returns min(block_samples, frames_remaining),
// clamped at 0. (Bug: writing a fixed block size regardless of the grant
// overflowed libsoundio's buffer.)
static inline int audio_soundio_frames_this_pass(int block_samples, int frames_remaining) {
    if (frames_remaining < 0) return 0;
    return frames_remaining < block_samples ? frames_remaining : block_samples;
}

// Channels to write per frame into the interleaved output area. The interleave
// buffer only holds 2 (stereo) channels, so a device layout with more channels
// must not be read past. Clamped to [0, 2]. (Bug: looping over the device's
// raw channel_count over-read the 2-channel buffer on surround layouts.)
static inline int audio_soundio_output_channels(int layout_channel_count) {
    if (layout_channel_count < 0) return 0;
    return layout_channel_count < 2 ? layout_channel_count : 2;
}

// Whether a whole block can be consumed from the input ring buffer. The reader
// pulls a fixed block_bytes every update; a partial fill must NOT advance the
// read pointer or it would read uninitialised/garbage data. (Bug: the guard
// only rejected a completely empty buffer.)
static inline bool audio_soundio_input_block_ready(int fill_bytes, int block_bytes) {
    return fill_bytes >= block_bytes;
}

// Whether a soundio error code from a stream read/write callback should tear
// the stream down. A realtime audio callback must never exit()/throw/block, so
// errors are reported via shared state instead; transient codes are not even
// failures. err == 0 and the two caller-supplied recoverable codes (e.g.
// SoundIoErrorUnderflow, SoundIoErrorInterrupted) are non-fatal; every other
// non-zero code is fatal. The recoverable codes are passed in rather than
// referenced here so this header stays free of the <soundio/soundio.h>
// dependency.
static inline bool audio_soundio_error_is_fatal(int err, int recoverable_a, int recoverable_b) {
    if (err == 0) return false;
    return err != recoverable_a && err != recoverable_b;
}

#endif
