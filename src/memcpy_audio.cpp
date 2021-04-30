#include <AudioStream.h>
#include "memcpy_audio.h"

void memcpy_tointerleaveLR(int16_t *dst, const int16_t *srcL, const int16_t *srcR) {
    for (int i=0; i< AUDIO_BLOCK_SAMPLES; i++) {
        *dst++ = *srcL++;
        *dst++ = *srcR++;
    }
}
void memcpy_tointerleaveL(int16_t *dst, const int16_t *srcL) {
    for (int i=0; i< AUDIO_BLOCK_SAMPLES; i++) {
        *dst++ = *srcL++;
        dst++;
    }
}
void memcpy_tointerleaveR(int16_t *dst, const int16_t *srcR) {
    for (int i=0; i< AUDIO_BLOCK_SAMPLES; i++) {
        dst++;
        *dst++ = *srcR++;
    }
}
void memcpy_tointerleaveQuad(int16_t *dst, const int16_t *src1, const int16_t *src2,
                             const int16_t *src3, const int16_t *src4) {
 // TODO: this...
}