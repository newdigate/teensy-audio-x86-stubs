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

#endif
