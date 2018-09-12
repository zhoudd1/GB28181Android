
#ifndef GB281818_MUXER_H
#define GB281818_MUXER_H


#include <fstream>

#include "base_include.h"

#include "user_arguments.h"

#include <sys/time.h>

using namespace std;

class GB28181Muxer {
public:
    GB28181Muxer(UserArguments *arg);
public:
    int initMuxer();

    static void* startMux(void *obj);

    int sendVideoFrame(uint8_t *buf);

    int sendAudioFrame(uint8_t *buf);

    void user_end();

    void release();

    int encodeEnd();

    void custom_filter(const GB28181Muxer *gb28181Muxer, const uint8_t *picture_buf,
                       int in_y_size,
                       int format);
    ~GB28181Muxer() {
    }

private:
    int flush_encoder(AVFormatContext *fmt_ctx, unsigned int stream_index);

private:
    UserArguments *arguments;
    int is_end = START_STATE;
    int is_release = RELEASE_FALSE;
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
    AVFrame *pFrame;
    int picture_size;
    int out_y_size;
    int audioFrameCnt = 0;
    int videoFrameCnt = 0;
    long startTime = 0;

    ofstream fout;

    int mux(GB28181Muxer *gb28181Muxer);

    void initOutput();
    void closeOutput();
};

#endif //GB281818_MUXER_H
