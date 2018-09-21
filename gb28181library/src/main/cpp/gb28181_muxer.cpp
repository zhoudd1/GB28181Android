
#include "gb28181_muxer.h"
#include "log.h"
#include "bits.h"
#include "gb28181_header_maker.c"
#include "g711_coder.c"
#include <pthread.h>

GB28181Muxer::GB28181Muxer(UserArguments *arg) : arguments(arg) {
}


/**
 * 初始化视频编码器
 * @return
 */
int GB28181Muxer::initMuxer() {
    LOGI("视频编码器初始化开始");

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

    LOGI("in height: %d, in width: %d.\n out height: %d, out width: %d. \n context height: %d, context width: %d \n video bitrate:%lld, video framerate:%d,\n custom_format %d",
         arguments->in_height,
         arguments->in_width,
         arguments->out_height,
         arguments->out_width,
         pCodecCtx->height,
         pCodecCtx->width,
         arguments->video_bit_rate,
         arguments->video_frame_rate,
         arguments->v_custom_format
    )

    pCodecCtx->bit_rate = arguments->video_bit_rate;
    //这里是设置关键帧的间隔
    pCodecCtx->gop_size = 25;
    pCodecCtx->thread_count = 3;

    pCodecCtx->time_base.num = 1;
    pCodecCtx->time_base.den = arguments->video_frame_rate;
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

    gb28181Sender = new GB28181_sender(arguments);
    gb28181Sender->initSender();

    in_y_size = arguments->in_width * arguments->in_height;
    picture_size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
    LOGI("   picture_size:%d", picture_size);


    av_new_packet(&pkt, picture_size);
    av_new_packet(&nPkt, picture_size);
    nextPkt = &nPkt;
    g711aFrameLen = arguments->a_frame_len / 2;
    scrPerFrame = 90000 / arguments->video_frame_rate;

    out_y_size = pCodecCtx->width * pCodecCtx->height;
    is_end = START_STATE;
    pthread_t thread;
    pthread_create(&thread, NULL, GB28181Muxer::startMux, this);
    pthread_t thread2;
    pthread_create(&thread2, NULL, GB28181Muxer::startEncode, this);
    LOGI("视频编码器初始化完成")

    return 0;
}

/**
 * 发送一视频帧到编码队列
 * @param buf
 * @return
 */
int GB28181Muxer::sendVideoFrame(uint8_t *buf) {
    int64_t st = getCurrentTime();
    AVFrame *pNewFrame = genFrame(buf);
    int64_t st1 = getCurrentTime();
    vFrame_queue.push(pNewFrame);
    int64_t  et = getCurrentTime();
    LOGE("生成帧用时：%lld, 原始帧入队用时：%lld", st1 - st,  et - st1);
    videoFrameCnt++;
    return 0;
}

AVFrame *GB28181Muxer::genFrame(uint8_t *rawData) {
    uint8_t *new_buf = (uint8_t *) malloc(in_y_size * 3 / 2);
    memcpy(new_buf, rawData, in_y_size * 3 / 2);

    AVFrame *pNewFrame = av_frame_alloc();
    uint8_t *buf = (uint8_t *) av_malloc(picture_size);
    avpicture_fill((AVPicture *) pNewFrame, buf, pCodecCtx->pix_fmt, pCodecCtx->width,
                   pCodecCtx->height);
    custom_filter(this, new_buf, pNewFrame);
    if (startTime == 0) {
        startTime = getCurrentTime();
        pNewFrame->pts = 0;
    } else {
        pNewFrame->pts = (getCurrentTime() - startTime) * 90;
    }
    LOGI("new Frame pts:%lld(%d)",
         pNewFrame->pts, videoFrameCnt);
    return pNewFrame;
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



void *GB28181Muxer::startEncode(void *obj) {
    LOGE("start encode thread");
    GB28181Muxer *gb28181Muxer = (GB28181Muxer *) obj;
    while (!gb28181Muxer->is_end) {
        int64_t st = getCurrentTime();
        AVFrame * pFrame = *gb28181Muxer->vFrame_queue.wait_and_pop();
        int64_t et1 = getCurrentTime();
        int ret = avcodec_send_frame(gb28181Muxer->pCodecCtx, pFrame);
        while (ret == AVERROR(EAGAIN)) {
//            LOGE("送入FFmpeg错误：%d.（重试）", ret);
            usleep(5000);
            ret = avcodec_send_frame(gb28181Muxer->pCodecCtx, pFrame);
        }
        int64_t et2 = getCurrentTime();
        LOGE("fetch queue time：%lld（frame left：%d），in FFmpeg time：%lld.", et1 - st, gb28181Muxer->vFrame_queue.size(), et2 - et1);
        if (ret < 0) {
            LOGE("send FFmpeg error：%d.", ret);
        }
    }
}
/**
 * 启动编码线程
 * @param obj
 * @return
 */
void *GB28181Muxer::startMux(void *obj) {
    LOGE("start mux thread");
    GB28181Muxer *gb28181Muxer = (GB28181Muxer *) obj;
    while (!gb28181Muxer->is_end) {
        int ret;
        int64_t st = getCurrentTime();
        if (gb28181Muxer->muxCnt == 0) { // 获得首帧
            ret = avcodec_receive_packet(gb28181Muxer->pCodecCtx, &gb28181Muxer->pkt);
            if (ret < 0) {
                usleep(5000);
            } else{
                gb28181Muxer->nowPkt = &gb28181Muxer->pkt;
                LOGI("got first encoded pkt!(pts:%lld, queue size: %d) \n",
                     gb28181Muxer->nowPkt->pts, gb28181Muxer->video_queue.size());
                gb28181Muxer->lastPts = gb28181Muxer->nowPkt->pts;
                gb28181Muxer->muxCnt++;
            }
        } else {
            ret = avcodec_receive_packet(gb28181Muxer->pCodecCtx, gb28181Muxer->nextPkt);
            if (ret < 0) {
//            LOGE("Failed to send encode!(%d) \n", ret);
                usleep(5000);
            } else {
//                LOGI("got encoded pkt!(pts:%lld, queue size: %d) \n", gb28181Muxer->nextPkt->pts,
//                     gb28181Muxer->video_queue.size());
                gb28181Muxer->mux(gb28181Muxer);
            }
        }
        int64_t et = getCurrentTime();
        if (ret >= 0)
            LOGE("mux one time:%lld", et - st);
    }
    if (gb28181Muxer->is_end) {
        gb28181Muxer->endMux();
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
                            AVFrame *pFrame) {

    int format = arguments->v_custom_format;
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
                *(pFrame->data[0] + j * gb28181Muxer->arguments->out_height +
                  (gb28181Muxer->arguments->out_height - (i - y_height_start_index) - 1)) = value;
            }
        }

        for (int i = uv_height_start_index; i < gb28181Muxer->arguments->in_height / 2; i++) {
            for (int j = 0; j < gb28181Muxer->arguments->out_width / 2; j++) {
                int index = gb28181Muxer->arguments->in_width / 2 * i + j;
                uint8_t v = *(picture_buf + in_y_size + index);
                uint8_t u = *(picture_buf + in_y_size * 5 / 4 + index);
                *(pFrame->data[2] + (j * gb28181Muxer->arguments->out_height / 2 +
                                     (gb28181Muxer->arguments->out_height / 2 -
                                      (i - uv_height_start_index) -
                                      1))) = v;
                *(pFrame->data[1] + (j * gb28181Muxer->arguments->out_height / 2 +
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

                *(pFrame->data[0] +
                  (i - y_height_start_index) * gb28181Muxer->arguments->out_width +
                  j) = value;
            }
        }


        for (int i = uv_height_start_index; i < gb28181Muxer->arguments->in_height / 2; i++) {
            for (int j = 0; j < gb28181Muxer->arguments->out_width / 2; j++) {

                int index = gb28181Muxer->arguments->in_width / 2 * i + j;
                uint8_t v = *(picture_buf + in_y_size + index);

                uint8_t u = *(picture_buf + in_y_size * 5 / 4 + index);
                *(pFrame->data[2] +
                  ((i - uv_height_start_index) * gb28181Muxer->arguments->out_width / 2 + j)) = v;
                *(pFrame->data[1] +
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
                *(pFrame->data[0] +
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
                *(pFrame->data[2] +
                  (gb28181Muxer->arguments->out_width / 2 - (j - uv_width_start_index) - 1)
                  * gb28181Muxer->arguments->out_height / 2 +
                  i) = v;
                *(pFrame->data[1] +
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
int GB28181Muxer::endMux() {
    LOGE("endMux");
    gb28181Sender->sendCloseSignal();

    LOGE("aduio queue left num: %d, video queue left num: %d", audio_queue.size(),
         video_queue.size());
    audio_queue.clear();
    video_queue.clear();

    //Clean
    if (video_st) {
        avcodec_close(video_st->codec);
//        av_free(pFrame);
//        av_free(picture_buf);
    }
//    avio_close(pFormatCtx->pb);
    avformat_free_context(pFormatCtx);
    LOGI("视频编码结束")
//    gb28181Sender->closeSender(); //再发一次防止之前没关掉
    return 1;
}

/**
 * 用户结束
 */
void GB28181Muxer::user_end() {
    LOGE("call user end");
    is_end = END_STATE;
}

int GB28181Muxer::mux(GB28181Muxer *gb28181Muxer) {

    //init header buffer
    char gb28181headerBuf[256];
    int nSizePos = 0;
    int nSize = 0;
    memset(gb28181headerBuf, 0, 256);

    int64_t newPts = gb28181Muxer->nextPkt->pts;

    // 计算写入音频的个数
    int64_t frameDiv = newPts - gb28181Muxer->lastPts;
    gb28181Muxer->frameAppend = (frameDiv + gb28181Muxer->frameAppend) % 3600;
    int audioCnt = (frameDiv + gb28181Muxer->frameAppend) / 3600;
    LOGI("now pts:%lld.|%lld| next pts:%lld,audio count:%d",
         gb28181Muxer->nowPkt->pts,
         gb28181Muxer->nextPkt->pts - gb28181Muxer->nowPkt->pts,
         gb28181Muxer->nextPkt->pts, audioCnt);

    // 0 rtp header
    gb28181_make_rtp_header(gb28181headerBuf + nSizePos, gb28181Muxer->muxCnt++,
                            gb28181Muxer->lastPts, gb28181Muxer->arguments->ssrc, RTP_PKT_END);
    nSizePos += RTP_HDR_LEN;
    // 1 package for ps header
    gb28181_make_ps_header(gb28181headerBuf + nSizePos, gb28181Muxer->lastPts);
    nSizePos += PS_HDR_LEN;
    //2 system header
    if (gb28181Muxer->nowPkt->flags == 1) {
        // 如果是I帧的话，则添加系统头
        gb28181_make_sys_header(gb28181headerBuf + nSizePos, audioCnt);
        nSizePos += SYS_HDR_LEN;
        gb28181_make_psm_header(gb28181headerBuf + nSizePos);
        nSizePos += PSM_HDR_LEN;
    }
    nSize = gb28181Muxer->nowPkt->size;
    // video psm
    gb28181_make_pes_header(gb28181headerBuf + nSizePos, 0xE0, nSize, gb28181Muxer->lastPts,
                            gb28181Muxer->lastPts);
    nSizePos += PES_HDR_LEN;
    gb28181Muxer->lastPts = gb28181Muxer->nextPkt->pts;

    uint16_t pktPos = 0;
    uint16_t pkt_len = (uint16_t) (nSizePos + nSize +
                                   audioCnt * (PES_HDR_LEN + gb28181Muxer->g711aFrameLen));
    uint8_t *pkt_full = (uint8_t *) malloc(pkt_len + 2);
    memcpy(pkt_full, short2Bytes(pkt_len), 2);
    pktPos += 2;
    memcpy(pkt_full + pktPos, gb28181headerBuf, nSizePos);
    pktPos += nSizePos;
    memcpy(pkt_full + pktPos, gb28181Muxer->nowPkt->data, nSize);
    pktPos += nSize;

    // next packet
    gb28181Muxer->nowPkt->stream_index = gb28181Muxer->video_st->index;
    AVPacket *t = gb28181Muxer->nowPkt;
    av_free_packet(gb28181Muxer->nowPkt);
    gb28181Muxer->nowPkt = gb28181Muxer->nextPkt;
    gb28181Muxer->nextPkt = t;

    // audio part
    while (audioCnt > 0) {
        uint8_t *audioFrame = *gb28181Muxer->audio_queue.wait_and_pop().get();
        int64_t audioPts = gb28181Muxer->audioFrameCnt * 3600; // 音频默认25帧，90000/25=3600
        gb28181_make_pes_header(gb28181headerBuf + nSizePos, 0xC0, gb28181Muxer->g711aFrameLen,
                                audioPts,
                                audioPts);
        memcpy(pkt_full + pktPos, gb28181headerBuf + nSizePos, PES_HDR_LEN);
        pktPos += PES_HDR_LEN;
        nSizePos += PES_HDR_LEN;
        memcpy(pkt_full + pktPos, audioFrame, gb28181Muxer->g711aFrameLen);
        pktPos += gb28181Muxer->g711aFrameLen;
        gb28181Muxer->audioFrameCnt++;
        audioCnt--;
        delete (audioFrame);
    }
    gb28181Sender->addPkt(pkt_full);

    LOGI("mux one pkt over!(video queue size: %d, audio queue size: %d)",
         gb28181Muxer->video_queue.size(), gb28181Muxer->audio_queue.size());
    return 0;
}