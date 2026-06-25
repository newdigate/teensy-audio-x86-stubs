// Unit tests for the dependency-free libsoundio bridge decision logic.
//
// These cover the bounds/underrun bugs in input_soundio.cpp / output_soundio.cpp
// without needing libsoundio or the Teensy stubs, so they run anywhere with a
// C++ compiler:
//
//     g++ -std=c++11 -I../src soundio_helpers_test.cpp -o soundio_helpers_test && ./soundio_helpers_test
//
// AUDIO_BLOCK_SAMPLES on Teensy is 128; a stereo s16 block is 128 * 2ch * 2bytes
// = 512 bytes. The tests use those concrete numbers.

#include "soundio_helpers.h"
#include <cstdio>

static int failures = 0;

#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::printf("FAIL: %s  (line %d)\n", #cond, __LINE__);             \
            ++failures;                                                        \
        }                                                                      \
    } while (0)

int main() {
    const int BLOCK = 128; // AUDIO_BLOCK_SAMPLES

    // Bug #1: never write more frames than soundio granted.
    // A pass must be clamped to the frames remaining in the granted region.
    CHECK(audio_soundio_frames_this_pass(BLOCK, BLOCK) == BLOCK);   // full grant
    CHECK(audio_soundio_frames_this_pass(BLOCK, 1) == 1);           // tiny grant
    CHECK(audio_soundio_frames_this_pass(BLOCK, 64) == 64);         // partial grant
    CHECK(audio_soundio_frames_this_pass(BLOCK, 0) == 0);           // nothing granted
    CHECK(audio_soundio_frames_this_pass(BLOCK, 200) == BLOCK);     // grant > block
    for (int remaining = 0; remaining <= 4 * BLOCK; ++remaining)
        CHECK(audio_soundio_frames_this_pass(BLOCK, remaining) <= remaining);

    // Bug #2: clamp the output area write to the 2-channel interleave buffer.
    CHECK(audio_soundio_output_channels(2) == 2);   // stereo, unchanged
    CHECK(audio_soundio_output_channels(1) == 1);   // mono, unchanged
    CHECK(audio_soundio_output_channels(0) == 0);
    CHECK(audio_soundio_output_channels(6) == 2);   // 5.1 layout must clamp to 2
    CHECK(audio_soundio_output_channels(8) == 2);   // 7.1 layout must clamp to 2

    // Bug #3a: only consume a block when a whole one is buffered; a partial
    // fill must not advance the read pointer.
    const int BLOCK_BYTES = BLOCK * 2 /*ch*/ * 2 /*bytes*/; // 512
    CHECK(audio_soundio_input_block_ready(BLOCK_BYTES, BLOCK_BYTES) == true);
    CHECK(audio_soundio_input_block_ready(BLOCK_BYTES + 100, BLOCK_BYTES) == true);
    CHECK(audio_soundio_input_block_ready(0, BLOCK_BYTES) == false);
    CHECK(audio_soundio_input_block_ready(BLOCK_BYTES - 1, BLOCK_BYTES) == false); // partial
    CHECK(audio_soundio_input_block_ready(4, BLOCK_BYTES) == false);               // partial

    if (failures == 0) {
        std::printf("OK: all soundio helper checks passed\n");
        return 0;
    }
    std::printf("%d check(s) failed\n", failures);
    return 1;
}
