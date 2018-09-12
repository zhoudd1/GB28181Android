#ifndef BASE_INCLUDE_H
#define BASE_INCLUDE_H

extern "C"
{
#include "include/libavcodec/avcodec.h"
#include "include/libavformat/avformat.h"
#include "include/libavcodec/avcodec.h"
#include "include/libavutil/opt.h"
}

#include "threadsafe_queue.cpp"
#include <jni.h>
#include <string>

#define END_STATE 1
#define START_STATE 0

#define RELEASE_TRUE 1
#define RELEASE_FALSE 0

#define ROTATE_0_CROP_LT 0

/**
 * 旋转90度剪裁左上
 */
#define ROTATE_90_CROP_LT 1
/**
 * 暂时没处理
 */
#define ROTATE_180 2
/**
 * 旋转270(-90)裁剪左上，左右镜像
 */
#define ROTATE_270_CROP_LT_MIRROR_LR 3

using namespace std;


static long getCurrentTime() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}


static long bytes2long(char b[]) {
    long temp = 0;
    long res = 0;
    for (int i=0;i<8;i++) {
        res <<= 8;
        temp = b[i] & 0xff;
        res |= temp;
    }
    return res;
}

#endif //BASE_INCLUDE_H
