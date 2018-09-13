
#ifndef USER_ARGUMENTS_H
#define USER_ARGUMENTS_H


#include "jni.h"
class JNIHandler;
typedef struct UserArguments {
    const char *ip_addr;
    int port;
    int outType;
    const char *media_base_path; //文件储存地址
    const char *media_name; // 文件命令前缀
    char *media_path; //合成后的MP4储存地址
    int in_width; //输出宽度
    int in_height; //输入高度
    int out_height; //输出高度
    int out_width; //输出宽度
    int video_frame_rate; //视频帧率控制
    int64_t video_bit_rate; //视频比特率控制
    int v_custom_format; //一些滤镜操作控制
    int a_frame_len;
    JNIEnv *env; //env全局指针
    JavaVM *javaVM; //jvm指针
    jclass java_class; //java接口类的calss对象
    JNIHandler *handler; // 一个全局处理对象的指针
} ;
#endif //USER_ARGUMENTS_H
