package com.autulin.gb28181library;

public class JNIBridge {
    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    public final static int ROTATE_0_CROP_LF=0;
    /**
     * 旋转90度剪裁左上
     */
    public final static int ROTATE_90_CROP_LT =1;
    /**
     * 暂时没处理
     */
    public final static int ROTATE_180=2;
    /**
     * 旋转270(-90)裁剪左上，左右镜像
     */
    public final static int ROTATE_270_CROP_LT_MIRROR_LR=3;


    public final static int UDP = 0;
    public final static int TCP = 1;
    public static final int FILE = 2;


    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public static native String stringFromJNI();

    public static native String getFFmpegConfig();

    /**
     *
     * @param ip 服务器ip
     * @param port 端口
     * @param outType tcp/udp/file
     * @param mediaBasePath 视频存放目录
     * @param mediaName 视频名称
     * @param filter 旋转镜像剪切处理
     * @param in_width 输入视频宽度
     * @param in_height 输入视频高度
     * @param out_height 输出视频高度
     * @param out_width 输出视频宽度
     * @param frameRate 视频帧率
     * @param bit_rate 视频比特率
     * @param audioFrameLen 音频长度
     * @return
     */
    public static native int initMuxer(
            String ip,
            int port,
            int outType,
            String mediaBasePath,
            String mediaName,
            int filter,
            int in_width,
            int in_height,
            int out_width,
            int out_height,
            int frameRate,
            long bit_rate,
            int audioFrameLen,
            int ssrc
    );

    public static native int sendOneVideoFrame(byte[] data);

    public static native int sendOneAudioFrame(byte[] data);

    public static native int endMux();

}
