package com.autulin.gb28181library;

import android.hardware.Camera;
import android.media.MediaRecorder;
import android.util.Log;

public class MediaRecorderNative extends MediaRecorderBase implements MediaRecorder.OnErrorListener {

    /**
     * 停止录制
     */
    @Override
    public void endMux() {
        Log.e("MediaRecorderNative", "endMux: ");
        super.endMux();
        JNIBridge.endMux();
    }

    /**
     * 视频数据回调
     */
    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        if (mRecording) {
            JNIBridge.sendOneVideoFrame(data);
            mPreviewFrameCallCount++;
        }
        super.onPreviewFrame(data, camera);
    }

    /**
     * 预览成功，设置视频输入输出参数
     */
    @Override
    protected void onStartPreviewSuccess() {
//        if (mCameraId == Camera.CameraInfo.CAMERA_FACING_BACK) {
//            UtilityAdapter.RenderInputSettings(mSupportedPreviewWidth, SMALL_VIDEO_WIDTH, 0, UtilityAdapter.FLIPTYPE_NORMAL);
//        } else {
//            UtilityAdapter.RenderInputSettings(mSupportedPreviewWidth, SMALL_VIDEO_WIDTH, 180, UtilityAdapter.FLIPTYPE_HORIZONTAL);
//        }
//        UtilityAdapter.RenderOutputSettings(SMALL_VIDEO_WIDTH, SMALL_VIDEO_HEIGHT, mFrameRate, UtilityAdapter.OUTPUTFORMAT_YUV | UtilityAdapter.OUTPUTFORMAT_MASK_MP4/*| UtilityAdapter.OUTPUTFORMAT_MASK_HARDWARE_ACC*/);
    }

    @Override
    public void onError(MediaRecorder mr, int what, int extra) {
        try {
            if (mr != null)
                mr.reset();
        } catch (IllegalStateException e) {
            Log.w("jianxi", "endMux", e);
        } catch (Exception e) {
            Log.w("jianxi", "endMux", e);
        }
        if (mOnErrorListener != null)
            mOnErrorListener.onVideoError(what, extra);
    }

    @Override
    public void startMux() {
        int vCustomFormat;
        if (mCameraId == Camera.CameraInfo.CAMERA_FACING_BACK) {
            vCustomFormat = JNIBridge.ROTATE_90_CROP_LT;
        } else {
            vCustomFormat = JNIBridge.ROTATE_270_CROP_LT_MIRROR_LR;
        }

        if (mAudioCollector == null) {
            mAudioCollector = new AudioCollector(this);
        }

        JNIBridge.initMuxer(
                mediaOutput.getIp(),
                mediaOutput.getPort(),
                mediaOutput.getOutputType(),
                mediaOutput.getOutputDir(),
                mediaOutput.getOutputName(),
                vCustomFormat,
                mSupportedPreviewWidth,
                SMALL_VIDEO_HEIGHT,
                SMALL_VIDEO_WIDTH,
                SMALL_VIDEO_HEIGHT,
                mFrameRate,
                mVideoBitrate,
                mAudioCollector.getFrameLen()
        );


        mAudioCollector.start();

        mRecording = true;

    }

    /**
     * 接收音频数据，传递到底层
     */
    @Override
    public void receiveAudioData(byte[] sampleBuffer) {
        if (mRecording) {
            JNIBridge.sendOneAudioFrame(sampleBuffer);
        }
    }
}
