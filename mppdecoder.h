#ifndef MPPDECODER_H
#define MPPDECODER_H
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "rockchip/rk_type.h"

/*
 * Usually hardware decode one
 * frame which resolution is 1080p needs 2 ms
 */
#define MPP_H264_DECODE_TIMEOUT   (3)  // milliseconds

/*
 * Maxium frames in the queue
 */
#define MAX_BUFFER_FRAMES         (30)
#define FRAME_SIZE                (1920*1088*2) /* h_stride*v_stride*2 */

typedef struct _frame_st {
    uint64_t cap_ms;
    uint32_t width;
    uint32_t height;
    uint32_t h_stride;
    uint32_t v_stride;
    uint8_t  data[FRAME_SIZE];
} frame_st;


int mppDecoder();

void decoder_routine();

float decoder_fps();

/*
 * Get a frame from decoder
 * DON'T free the memory outside
 */
frame_st* decoder_frame();


#endif // MPPDECODER_H
