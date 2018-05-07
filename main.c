#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "tools.h"
#include "rtspprotocolutil.h"
#include "mppdecoder.h"
#include "rkdrm.h"

#if 0
// 将一个YUV帧存入文件，便于使用外部解码器来检验解码的正确性
static void save_frame(frame_st* frame) 
{
    static d = 0;
    if (d++ > 10) return;
    char fn[256];
    sprintf(fn, "frame-%04d.yuv", d);
    FILE* fp = fopen(fn, "wb");
    uint32_t i;
    uint8_t *base_y = frame->data;
    uint8_t *base_c = frame->data + frame->h_stride * frame->v_stride;

    for (i = 0; i < frame->height; i++, base_y += frame->h_stride) {
        fwrite(base_y, 1, frame->width, fp);
    }
    for (i = 0; i < frame->height / 2; i++, base_c += frame->h_stride) {
        fwrite(base_c, 1, frame->width, fp);
    }
    fclose(fp);
}

static void _copy_frame(frame_st* frame)
{
    extern uint8_t* yuv_y;
    extern uint8_t* yuv_uv;
    if (NULL == yuv_y || NULL == yuv_uv) return;
    uint8_t* wp = NULL;
    uint32_t i;

    uint8_t *base_y = frame->data;
    uint8_t *base_c = frame->data + frame->h_stride * frame->v_stride;

    wp = yuv_y;
    for (i = 0; i < frame->height; i++, base_y += frame->h_stride, wp += frame->width) {
        memcpy(wp, base_y, frame->width);
    }
    wp = yuv_uv;
    for (i = 0; i < frame->height / 2; i++, base_c += frame->h_stride, wp += frame->width) {
        memcpy(wp, base_c, frame->width);
    }
}
#endif

static void *
       thread_rtsp_mpp(void *arg)
{
    (void)(arg);
    while (1) {
        // Check the RTSP connection
        if (!isStart()) {
            msleep(1000);
            continue;
        }
        // 尝试从socket获取数据
        rtsp_read();
        // 获取RTSP中的H264数据
        if (rtsp_packet() > 0) {
            // 解码
            decoder_routine();
        }
        else {
            msleep(8);
        }  
    }
    return NULL;
}

int main()
{
    //unsigned long long int ldt = current_ms();
    //unsigned long long int count = 0, miss = 0;
    // Initialise RTSP client
    if (RtspProtocolUtil_init("rtsp://192.168.199.30:554/h264/ch1/main/av_stream")) {
        fprintf(stderr, "RTSP initialise error.\n");
        exit(-1);
    }

    // Initialise MPP decoder
    mppDecoder();

    rkdrm_init();
#if 0
    // Initialise EGL display
    if (initWindow()) {
        printf("Initialise windows error.\n");
        return -1;
    }
    if (initShader()) {
        printf("Initialise OpenGL shader error.\n");
        return -1;
    }
#endif
    pthread_t thread;
    pthread_create(&thread, NULL, &thread_rtsp_mpp, NULL);
    //unsigned long long int espc = 0, espd = 0, disp = 0;
    while (1) {
#if 0
        // Try to get a YUV frame
        count++;
        frame_st* frame = decoder_frame();
        if (NULL == frame) {
            miss++;
            msleep(8);
            continue;
        }
unsigned long long int b1 = current_ms();
        // Copy frame to buffer
        _copy_frame(frame);
unsigned long long int b2 = current_ms();
        // Call OpenGL ES shader to display the frame
        onDrawFrame();
unsigned long long int b3 = current_ms();
        
        // Debug
        // save_frame(frame, debug);
        espc += (b2 - b1);
        espd += (b3 - b2);
        disp++;
        unsigned long long int now = current_ms();
        if (now - ldt > 5000) {
            printf("Try get frame %llu times, miss %llu.\n", count, miss);
            printf("Frames: %llu, memory copy time: %llu, display time: %llu, FPS: %llu.\n", disp, (espc/disp), (espd/disp), (disp*1000/(now-ldt)));
            ldt = now;
            espc = espd = disp = 0;
        }
#else
        msleep(1000);
#endif
    }

    rkdrm_fini();

    return 0;
}

