
#ifndef GB281818_MUXER_H
#define GB281818_MUXER_H


#include <fstream>

#include "base_include.h"

#include "user_arguments.h"
#include "GB28181_sender.h"

#include <sys/time.h>

using namespace std;

class GB28181Muxer {
public:
    GB28181Muxer(UserArguments *arg);
public:
    int initMuxer();

    static void* startMux(void *obj);
    static void* startEncode(void *obj);

    int sendVideoFrame(uint8_t *buf);

    int sendAudioFrame(uint8_t *buf);

    void sendEndSignal();

    int endMux();

    void custom_filter(const GB28181Muxer *gb28181Muxer, const uint8_t *picture_buf,
                        AVFrame *pFrame);
    ~GB28181Muxer() {
    }

private:
    int flush_encoder(AVFormatContext *fmt_ctx, unsigned int stream_index);

private:
    UserArguments *arguments;
    GB28181_sender *gb28181Sender;
    volatile int is_end = START_STATE;
    volatile int is_release = RELEASE_FALSE;
//    threadsafe_queue<AVFrame *> vFrame_queue;
    threadsafe_queue<uint8_t *> video_queue;
    threadsafe_queue<uint8_t *> audio_queue;
    AVFormatContext *pFormatCtx;
    AVOutputFormat *fmt;
    AVStream *video_st;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVPacket pkt;
    AVPacket nPkt;
    AVPacket *nowPkt;
    AVPacket *nextPkt;
    int picture_size;
    int out_y_size;
    int audioFrameCnt = 0;
    int videoFrameCnt = 0;
    int64_t startTime = 0;
    int in_y_size;

    //合成相关的参数
    int g711aFrameLen = 0;
    int64_t lastPts = 0;
    int64_t frameAppend = 0;
    int scrPerFrame;
    int muxCnt = 0;

    int mux(GB28181Muxer *gb28181Muxer);
    AVFrame * genFrame(uint8_t *rawData);
};

#endif //GB281818_MUXER_H
