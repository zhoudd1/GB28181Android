#include <jni.h>
#include <string>

#include "log.h"
#include "gb28181_muxer.h"

#define MEDIA_FORMAT ".ps"

GB28181Muxer *gb28181Muxer;

extern "C" JNIEXPORT jstring
JNICALL
Java_com_autulin_gb28181library_JNIBridge_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_autulin_gb28181library_JNIBridge_getFFmpegConfig(JNIEnv *env, jclass type) {
    char info[10000] = {0};
    sprintf(info, "%s\n", avcodec_configuration());
    return env->NewStringUTF(info);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_autulin_gb28181library_JNIBridge_initMuxer(JNIEnv *env, jclass type, jstring ip_,
                                                    jint port, jint outType,
                                                    jstring mediaBasePath_, jstring mediaName_,
                                                    jint filter, jint in_width, jint in_height,
                                                    jint out_width, jint out_height, jint frameRate,
                                                    jlong bit_rate, jint audioFrameLen) {
    const char *ip = env->GetStringUTFChars(ip_, 0);
    const char *mediaBasePath = env->GetStringUTFChars(mediaBasePath_, 0);
    const char *mediaName = env->GetStringUTFChars(mediaName_, 0);

    jclass global_class = (jclass) env->NewGlobalRef(type);
    UserArguments *arguments = (UserArguments *) malloc(sizeof(UserArguments));

    arguments->ip_addr = ip;
    arguments->port = port;
    arguments->outType = outType;
    arguments->media_base_path = mediaBasePath;
    arguments->media_name = mediaName;

    size_t m_path_size = strlen(mediaBasePath) + strlen(mediaName) + strlen(MEDIA_FORMAT) + 1;
    arguments->media_path = (char *) malloc(m_path_size + 1);

    strcpy(arguments->media_path, mediaBasePath);
    strcat(arguments->media_path, "/");
    strcat(arguments->media_path, mediaName);
    strcat(arguments->media_path, MEDIA_FORMAT);

    arguments->video_bit_rate = bit_rate;
    arguments->video_frame_rate = frameRate;
    arguments->in_width = in_width;
    arguments->in_height = in_height;
    arguments->out_height = out_height;
    arguments->out_width = out_width;
    arguments->v_custom_format = filter;
    arguments->a_frame_len = audioFrameLen;
    arguments->env = env;
    arguments->java_class = global_class;
    arguments->env->GetJavaVM(&arguments->javaVM);

    env->ReleaseStringUTFChars(ip_, ip);
    env->ReleaseStringUTFChars(mediaBasePath_, mediaBasePath);
    env->ReleaseStringUTFChars(mediaName_, mediaName);

    gb28181Muxer = new GB28181Muxer(arguments);
    return gb28181Muxer->initMuxer();
}


extern "C"
JNIEXPORT jint JNICALL
Java_com_autulin_gb28181library_JNIBridge_sendOneVideoFrame(JNIEnv *env, jclass type,
                                                            jbyteArray data_) {
    jbyte *data = env->GetByteArrayElements(data_, NULL);
    int i = gb28181Muxer->sendVideoFrame(reinterpret_cast<uint8_t *>(data));
    env->ReleaseByteArrayElements(data_, data, 0);
    return i;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_autulin_gb28181library_JNIBridge_sendOneAudioFrame(JNIEnv *env, jclass type,
                                                            jbyteArray data_) {
    jbyte *data = env->GetByteArrayElements(data_, NULL);
    int i = gb28181Muxer->sendAudioFrame(reinterpret_cast<uint8_t *>(data));
    env->ReleaseByteArrayElements(data_, data, 0);
    return i;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_autulin_gb28181library_JNIBridge_endMux(JNIEnv *env, jclass type) {

    if (gb28181Muxer != NULL) {
        gb28181Muxer->user_end();
        gb28181Muxer = NULL;
    }
    return 0;
}
