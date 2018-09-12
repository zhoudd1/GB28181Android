
#include "gb28181_muxer.h"
#include "log.h"
#include "bits.h"
#include "gb28181_header_maker.c"
#include "g711_coder.c"
#include <pthread.h>

GB28181Muxer::GB28181Muxer(UserArguments *arg) : arguments(arg) {
}

/**
 * 结束编码时刷出还在编码器里面的帧
 * @param fmt_ctx
 * @param stream_index
 * @return
 */
int GB28181Muxer::flush_encoder(AVFormatContext *fmt_ctx, unsigned int stream_index) {
    int ret;
    int got_frame;
    AVPacket enc_pkt;
    if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities &
          CODEC_CAP_DELAY))
        return 0;
    while (1) {
        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);
        ret = avcodec_encode_video2(fmt_ctx->streams[stream_index]->codec, &enc_pkt,
                                    NULL, &got_frame);
        av_frame_free(NULL);
        if (ret < 0)
            break;
        if (!got_frame) {
            ret = 0;
            break;
        }
        LOGI("_Flush Encoder: Succeed to encode 1 frame video!\tsize:%5d\n",
             enc_pkt.size);
        /* mux encoded frame */
        ret = av_write_frame(fmt_ctx, &enc_pkt);
        if (ret < 0)
            break;
    }

    return ret;
}

/**
 * 初始化视频编码器
 * @return
 */
int GB28181Muxer::initMuxer() {
    LOGI("视频编码器初始化开始");

    if (arguments->outType < 2)
        LOGE("ip:%s, port:%d", arguments->ip_addr, arguments->port);

    av_register_all();
    pFormatCtx = avformat_alloc_context();

    video_st = avformat_new_stream(pFormatCtx, 0);
    //video_st->time_base.num = 1;
    //video_st->time_base.den = 25;

    if (video_st == NULL) {
        LOGE("_video_st==null");
        return -1;
    }

    //Param that must set
    pCodecCtx = video_st->codec;
    //pCodecCtx->codec_id =AV_CODEC_ID_HEVC;
    pCodecCtx->codec_id = AV_CODEC_ID_H264;
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    if (arguments->v_custom_format == ROTATE_0_CROP_LT ||
        arguments->v_custom_format == ROTATE_180) {
        pCodecCtx->width = arguments->out_width;
        pCodecCtx->height = arguments->out_height;
    } else {
        pCodecCtx->width = arguments->out_height;
        pCodecCtx->height = arguments->out_width;
    }

    pCodecCtx->bit_rate = arguments->video_bit_rate;
    //这里是设置关键帧的间隔
    pCodecCtx->gop_size = 25;
    pCodecCtx->thread_count = 12;

    pCodecCtx->time_base.num = 1;
    pCodecCtx->time_base.den = arguments->frame_rate;
//    pCodecCtx->me_pre_cmp = 1;
    //H264
    //pCodecCtx->me_range = 16;
    //pCodecCtx->max_qdiff = 4;
    //pCodecCtx->qcompress = 0.6;
    pCodecCtx->qmin = 10;
    pCodecCtx->qmax = 51;

    //Optional Param
    pCodecCtx->max_b_frames = 3;

    // Set Option
    AVDictionary *param = 0;
    //H.264
    if (pCodecCtx->codec_id == AV_CODEC_ID_H264) {
//        av_dict_set(&param, "tune", "animation", 0);
//        av_dict_set(&param, "profile", "baseline", 0);
        av_dict_set(&param, "tune", "zerolatency", 0);
        av_opt_set(pCodecCtx->priv_data, "preset", "ultrafast", 0);
        av_dict_set(&param, "profile", "baseline", 0);
    }

    pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
    if (!pCodec) {
        LOGE("Can not find encoder! \n");
        return -1;
    }
    if (avcodec_open2(pCodecCtx, pCodec, &param) < 0) {
        LOGE("Failed to open encoder! \n");
        return -1;
    }

    initOutput();

    pFrame = av_frame_alloc();
    picture_size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
    LOGI("   picture_size:%d", picture_size);
    uint8_t *buf = (uint8_t *) av_malloc(picture_size);
    avpicture_fill((AVPicture *) pFrame, buf, pCodecCtx->pix_fmt, pCodecCtx->width,
                   pCodecCtx->height);

    av_new_packet(&pkt, picture_size);
    av_new_packet(&nPkt, picture_size);
    nextPkt = &nPkt;

    out_y_size = pCodecCtx->width * pCodecCtx->height;
    is_end = START_STATE;
    pthread_t thread;
    pthread_create(&thread, NULL, GB28181Muxer::startMux, this);
    LOGI("视频编码器初始化完成")

    return 0;
}

/**
 * 发送一视频帧到编码队列
 * @param buf
 * @return
 */
int GB28181Muxer::sendVideoFrame(uint8_t *buf) {
    int in_y_size = arguments->in_width * arguments->in_height;

    uint8_t *new_buf = (uint8_t *) malloc(in_y_size * 3 / 2);
    memcpy(new_buf, buf, in_y_size * 3 / 2);
    video_queue.push(new_buf);

    return 0;
}

/**
 * 编码并发送一音频帧到编码队列
 * @param buf
 * @return
 */
int GB28181Muxer::sendAudioFrame(uint8_t *buf) {
    uint8_t *encoded_audio = static_cast<uint8_t *>(malloc(arguments->a_frame_len / 2));
    int ret = g711_encode(reinterpret_cast<char *>(buf), reinterpret_cast<char *>(encoded_audio),
                          arguments->a_frame_len, G711_A_LAW);
    if (ret < 0) {
        LOGE("Failed to encode audio! len: %d, data: %d %d %d %d %d\n", arguments->a_frame_len,
             buf[0], buf[1], buf[2], buf[3], buf[4]);
        return -1;
    }
    audio_queue.push(encoded_audio);
    return 0;
}

/**
 * 启动编码线程
 * @param obj
 * @return
 */
void *GB28181Muxer::startMux(void *obj) {
    LOGE("start mux thread");
    GB28181Muxer *gb28181Muxer = (GB28181Muxer *) obj;
    while (!gb28181Muxer->is_end || !gb28181Muxer->video_queue.empty()) {
        if (gb28181Muxer->is_release) {
            //Clean
            if (gb28181Muxer->video_st) {
                avcodec_close(gb28181Muxer->video_st->codec);
                av_free(gb28181Muxer->pFrame);
            }
            avio_close(gb28181Muxer->pFormatCtx->pb);
            avformat_free_context(gb28181Muxer->pFormatCtx);
            delete gb28181Muxer;
            return 0;
        }
        if (gb28181Muxer->video_queue.empty()) {
            continue;
        }
        uint8_t *picture_buf = *gb28181Muxer->video_queue.wait_and_pop().get();
        long v_time = bytes2long(reinterpret_cast<char *>(picture_buf));

        int in_y_size = gb28181Muxer->arguments->in_width * gb28181Muxer->arguments->in_height;

        // 处理视频帧并到 h264_encoder->pFrame 中
        gb28181Muxer->custom_filter(gb28181Muxer, picture_buf + 8, in_y_size,
                                    gb28181Muxer->arguments->v_custom_format);

        long lastPTS = gb28181Muxer->pFrame->pts;
        if (gb28181Muxer->startTime == 0) gb28181Muxer->startTime = v_time;
        gb28181Muxer->pFrame->pts = (v_time - gb28181Muxer->startTime) * 90;
        LOGE("v_time:%ld, now:%ld, lastPTS: %ld, new PTS: %ld, divid: %ld", v_time,
             getCurrentTime(), lastPTS, gb28181Muxer->pFrame->pts,
             gb28181Muxer->pFrame->pts - lastPTS);
        gb28181Muxer->videoFrameCnt++;
        int got_picture = 0;
        //Encode
        int ret = avcodec_encode_video2(gb28181Muxer->pCodecCtx, &gb28181Muxer->pkt,
                                        gb28181Muxer->pFrame, &got_picture);

        if (ret < 0) {
            LOGE("Failed to encode! \n");
        }
        // 前几次调用avcodec_encode_video2可能是拿不到编码好的视频的
        if (got_picture == 1) {
            gb28181Muxer->mux(gb28181Muxer);
        }
        delete (picture_buf);
    }
    if (gb28181Muxer->is_end) {
        gb28181Muxer->encodeEnd();
        delete gb28181Muxer;
    }
    return 0;
}

/**
 * 对视频做一些处理
 * @param gb28181Muxer
 * @param picture_buf
 * @param in_y_size
 * @param format
 */
void
GB28181Muxer::custom_filter(const GB28181Muxer *gb28181Muxer, const uint8_t *picture_buf,
                            int in_y_size, int format) {

    //   y值在H方向开始行
    int y_height_start_index =
            gb28181Muxer->arguments->in_height - gb28181Muxer->arguments->out_height;
    //   uv值在H方向开始行
    int uv_height_start_index = y_height_start_index / 2;

    if (format == ROTATE_90_CROP_LT) {

        for (int i = y_height_start_index; i < gb28181Muxer->arguments->in_height; i++) {

            for (int j = 0; j < gb28181Muxer->arguments->out_width; j++) {

                int index = gb28181Muxer->arguments->in_width * i + j;
                uint8_t value = *(picture_buf + index);
                *(gb28181Muxer->pFrame->data[0] + j * gb28181Muxer->arguments->out_height +
                  (gb28181Muxer->arguments->out_height - (i - y_height_start_index) - 1)) = value;
            }
        }

        for (int i = uv_height_start_index; i < gb28181Muxer->arguments->in_height / 2; i++) {
            for (int j = 0; j < gb28181Muxer->arguments->out_width / 2; j++) {
                int index = gb28181Muxer->arguments->in_width / 2 * i + j;
                uint8_t v = *(picture_buf + in_y_size + index);
                uint8_t u = *(picture_buf + in_y_size * 5 / 4 + index);
                *(gb28181Muxer->pFrame->data[2] + (j * gb28181Muxer->arguments->out_height / 2 +
                                                   (gb28181Muxer->arguments->out_height / 2 -
                                                    (i - uv_height_start_index) -
                                                    1))) = v;
                *(gb28181Muxer->pFrame->data[1] + (j * gb28181Muxer->arguments->out_height / 2 +
                                                   (gb28181Muxer->arguments->out_height / 2 -
                                                    (i - uv_height_start_index) -
                                                    1))) = u;
            }
        }
    } else if (format == ROTATE_0_CROP_LT) {


        for (int i = y_height_start_index; i < gb28181Muxer->arguments->in_height; i++) {

            for (int j = 0; j < gb28181Muxer->arguments->out_width; j++) {

                int index = gb28181Muxer->arguments->in_width * i + j;
                uint8_t value = *(picture_buf + index);

                *(gb28181Muxer->pFrame->data[0] +
                  (i - y_height_start_index) * gb28181Muxer->arguments->out_width +
                  j) = value;
            }
        }


        for (int i = uv_height_start_index; i < gb28181Muxer->arguments->in_height / 2; i++) {
            for (int j = 0; j < gb28181Muxer->arguments->out_width / 2; j++) {

                int index = gb28181Muxer->arguments->in_width / 2 * i + j;
                uint8_t v = *(picture_buf + in_y_size + index);

                uint8_t u = *(picture_buf + in_y_size * 5 / 4 + index);
                *(gb28181Muxer->pFrame->data[2] +
                  ((i - uv_height_start_index) * gb28181Muxer->arguments->out_width / 2 + j)) = v;
                *(gb28181Muxer->pFrame->data[1] +
                  ((i - uv_height_start_index) * gb28181Muxer->arguments->out_width / 2 + j)) = u;
            }
        }
    } else if (format == ROTATE_270_CROP_LT_MIRROR_LR) {

        int y_width_start_index =
                gb28181Muxer->arguments->in_width - gb28181Muxer->arguments->out_width;
        int uv_width_start_index = y_width_start_index / 2;

        for (int i = 0; i < gb28181Muxer->arguments->out_height; i++) {

            for (int j = y_width_start_index; j < gb28181Muxer->arguments->in_width; j++) {

                int index = gb28181Muxer->arguments->in_width *
                            (gb28181Muxer->arguments->out_height - i - 1) + j;
                uint8_t value = *(picture_buf + index);
                *(gb28181Muxer->pFrame->data[0] +
                  (gb28181Muxer->arguments->out_width - (j - y_width_start_index) - 1)
                  * gb28181Muxer->arguments->out_height +
                  i) = value;
            }
        }
        for (int i = 0; i < gb28181Muxer->arguments->out_height / 2; i++) {
            for (int j = uv_width_start_index; j < gb28181Muxer->arguments->in_width / 2; j++) {
                int index = gb28181Muxer->arguments->in_width / 2 *
                            (gb28181Muxer->arguments->out_height / 2 - i - 1) + j;
                uint8_t v = *(picture_buf + in_y_size + index);
                uint8_t u = *(picture_buf + in_y_size * 5 / 4 + index);
                *(gb28181Muxer->pFrame->data[2] +
                  (gb28181Muxer->arguments->out_width / 2 - (j - uv_width_start_index) - 1)
                  * gb28181Muxer->arguments->out_height / 2 +
                  i) = v;
                *(gb28181Muxer->pFrame->data[1] +
                  (gb28181Muxer->arguments->out_width / 2 - (j - uv_width_start_index) - 1)
                  * gb28181Muxer->arguments->out_height / 2 +
                  i) = u;
            }
        }
    }
}

/**
 * 视频编码结束
 * @return
 */
int GB28181Muxer::encodeEnd() {
    //Flush Encoder
    int ret_1 = flush_encoder(pFormatCtx, 0);
    if (ret_1 < 0) {
        LOGE("Flushing encoder failed\n");
        return -1;
    }

    closeOutput();

    //Clean
    if (video_st) {
        avcodec_close(video_st->codec);
        av_free(pFrame);
//        av_free(picture_buf);
    }
    avio_close(pFormatCtx->pb);
    avformat_free_context(pFormatCtx);
    LOGI("视频编码结束")
    return 1;
}

/**
 * 用户中断
 */
void GB28181Muxer::user_end() {
    is_end = END_STATE;
}

void GB28181Muxer::release() {
    is_release = RELEASE_TRUE;
}


int GB28181Muxer::mux(GB28181Muxer *gb28181Muxer) {
    long lastPts = gb28181Muxer->pkt.pts;
    gb28181Muxer->nowPkt = &gb28181Muxer->pkt;
    int append = 0;
    int cnt = 0;
    while (!gb28181Muxer->is_end || !gb28181Muxer->video_queue.empty()) {
        char szTempPacketHead[256];
        int nSizePos = 0;
        int nSize = 0;
        memset(szTempPacketHead, 0, 256);

        // read next frame
        uint8_t *picture_buf = *gb28181Muxer->video_queue.wait_and_pop().get();
        long v_time = bytes2long(reinterpret_cast<char *>(picture_buf));
        int in_y_size = gb28181Muxer->arguments->in_width * gb28181Muxer->arguments->in_height;
        gb28181Muxer->custom_filter(gb28181Muxer, picture_buf + 8, in_y_size,
                                    gb28181Muxer->arguments->v_custom_format);
        gb28181Muxer->pFrame->pts = (v_time - gb28181Muxer->startTime) * 90;
        LOGE("get a pts:%ld", gb28181Muxer->pFrame->pts);

        int got_picture;
        int ret = avcodec_encode_video2(gb28181Muxer->pCodecCtx, gb28181Muxer->nextPkt,
                                        gb28181Muxer->pFrame, &got_picture);
        if (ret < 0) {
            LOGE("Failed to encode!11111111111111 \n");
        }

        // 读到了下一帧
        long newPts = gb28181Muxer->nextPkt->pts;
        // 计算写入音频的个数
        int frameDiv = newPts - lastPts;
        append = (frameDiv + append) % 3600;
        int audioCnt = (frameDiv + append) / 3600;
        LOGE("now pts:%ld.|%ld| next pts:%ld,audio count:%d",
             gb28181Muxer->nowPkt->pts,
             gb28181Muxer->nextPkt->pts - gb28181Muxer->nowPkt->pts,
             gb28181Muxer->nextPkt->pts, audioCnt);

        // 0 rtp header
//        gb28181_make_rtp_header(szTempPacketHead + nSizePos, cnt++, lastPts, 1);
//        nSizePos += RTP_HDR_LEN;
        // 1 package for ps header
        gb28181_make_ps_header(szTempPacketHead + nSizePos, lastPts);
        nSizePos += PS_HDR_LEN;
        //2 system header
        if (gb28181Muxer->nowPkt->flags == 1) {
            // 如果是I帧的话，则添加系统头
            gb28181_make_sys_header(szTempPacketHead + nSizePos, audioCnt);
            nSizePos += SYS_HDR_LEN;
            //这个地方我是不管是I帧还是p帧都加上了map的，貌似只是I帧加也没有问题
            gb28181_make_psm_header(szTempPacketHead + nSizePos);
            nSizePos += PSM_HDR_LEN;
        }
        nSize = gb28181Muxer->nowPkt->size;
        // video psm
        gb28181_make_pes_header(szTempPacketHead + nSizePos, 0xE0, nSize, lastPts, lastPts);
        nSizePos += PES_HDR_LEN;
        gb28181Muxer->fout.write(szTempPacketHead, nSizePos);
        gb28181Muxer->fout.write(reinterpret_cast<const char *>(gb28181Muxer->nowPkt->data), nSize);
        lastPts = gb28181Muxer->nextPkt->pts;

//        LOGE("orgin frame:%ld", h264_encoder->nowPkt->pts);
        gb28181Muxer->nowPkt->stream_index = gb28181Muxer->video_st->index;
        AVPacket *t = gb28181Muxer->nowPkt;
        av_free_packet(gb28181Muxer->nowPkt);
        gb28181Muxer->nowPkt = gb28181Muxer->nextPkt;
        gb28181Muxer->nextPkt = t;
//        LOGE("now frame:%ld", h264_encoder->nowPkt->pts);



        while (audioCnt > 0) {
            uint8_t *audioFrame = *gb28181Muxer->audio_queue.wait_and_pop().get();
            long audioPts = gb28181Muxer->audioFrameCnt * (90000 / gb28181Muxer->arguments->frame_rate);
            int aFrameLen = gb28181Muxer->arguments->a_frame_len / 2;
            gb28181_make_pes_header(szTempPacketHead + nSizePos, 0xC0, aFrameLen, audioPts,
                                    audioPts);
            fout.write(szTempPacketHead + nSizePos, PES_HDR_LEN);
            nSizePos += PES_HDR_LEN;
            fout.write(reinterpret_cast<char *>(audioFrame), aFrameLen);
            gb28181Muxer->audioFrameCnt++;
            audioCnt--;
            delete (audioFrame);
        }
    }
}

void GB28181Muxer::initOutput() {
    switch (arguments->outType) {
        case 0: // udp
            break;
        case 1: // tcp
            break;
        case 2: // file
            //打开ps文件
            LOGE("media path: %s", arguments->media_path);
            fout.open(arguments->media_path, ios::binary);
            break;
        default:
            break;
    }
}


void GB28181Muxer::closeOutput() {
    switch (arguments->outType) {
        case 0: // udp
            break;
        case 1: // tcp
            break;
        case 2: // file
            fout.close();
            break;
        default:
            break;
    }
}