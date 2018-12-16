package com.example.autulin.gb28181_android;

import android.Manifest;
import android.support.v4.app.ActivityCompat;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.RelativeLayout;

import com.autulin.gb28181library.MediaOutput;
import com.autulin.gb28181library.MediaRecorderBase;
import com.autulin.gb28181library.MediaRecorderNative;
import com.autulin.gb28181library.utils.DeviceUtils;
import java.io.IOException;

public class DemoActivity extends AppCompatActivity implements
        MediaRecorderBase.OnErrorListener, MediaRecorderBase.OnPreparedListener {

    private Button mButton;
    private SurfaceView mSurfaceView;
    private RelativeLayout bottomLayout;

    private MediaRecorderNative mMediaRecorder;
    private MediaOutput mediaOutput;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON); // 防止锁屏
        setContentView(R.layout.activity_demo);
        ActivityCompat.requestPermissions(this,
                new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE},1);
        initData();
        initView();

        mButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if (mMediaRecorder != null) {
                    if (!mMediaRecorder.mRecording) {
                        mButton.setText("点击结束");
                        mMediaRecorder.startMux();
                    } else{
                        mButton.setText("点击开始");
                        mMediaRecorder.endMux();
                    }
                }
//                new Thread(runnable).start();
            }
        });
        try {
            Log.e("log", "path: "+MediaRecorderBase.getLogOutPutPath());
            Runtime.getRuntime().exec("logcat -f " + MediaRecorderBase.getLogOutPutPath());
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void onResume() {
        super.onResume();

        //初始化
        if (mMediaRecorder == null) {
            initMediaRecorder();
        } else {
            mMediaRecorder.prepare();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        // 释放资源
        mMediaRecorder.release();
    }

    //初始化视频参数用的
    private void initData() {
        // 设置视频的宽高，比特率等
//        MediaRecorderBase.SMALL_VIDEO_HEIGHT = mediaRecorderConfig.getSmallVideoHeight();
//        MediaRecorderBase.SMALL_VIDEO_WIDTH = mediaRecorderConfig.getSmallVideoWidth();
//        MediaRecorderBase. = mediaRecorderConfig.getVideoBitrate();
        MediaRecorderBase.QUEUE_MAX_SIZE = 20;
        Log.i("debug", "SMALL_VIDEO_HEIGHT: " + MediaRecorderBase.SMALL_VIDEO_HEIGHT + ", SMALL_VIDEO_WIDTH:" + MediaRecorderBase.SMALL_VIDEO_WIDTH );
    }

    private void initView() {
        mButton = findViewById(R.id.start_btn);
        mSurfaceView = findViewById(R.id.record_preview);
        bottomLayout = findViewById(R.id.bottom_lt);
    }

    /**
     * 初始化拍摄SDK
     */
    private void initMediaRecorder() {
        mMediaRecorder = new MediaRecorderNative();

        mMediaRecorder.setOnErrorListener(this);
        mMediaRecorder.setOnPreparedListener(this);
//        mMediaRecorder.setCameraFront();   // 设置前置摄像头

        // 设置输出
//        String fileName = String.valueOf(System.currentTimeMillis());
//        String fileName = "tttttt";
//        mediaOutput = mMediaRecorder.setFileOutPut(fileName);  //输出到文件，这里demo是/sdcard/pstest/tttttt.ps
        int ssrc = 1;
//        mediaOutput = mMediaRecorder.setUdpOutPut("10.210.100.69", 8888, ssrc);
        mediaOutput = mMediaRecorder.setTcpOutPut("10.210.100.69", 8888,8088, ssrc);

        mMediaRecorder.setSurfaceHolder(mSurfaceView.getHolder());
        mMediaRecorder.prepare();
    }

    /**
     * 初始化画布
     */
    private void initSurfaceView() {
        final int w = DeviceUtils.getScreenWidth(this);
        // 避免摄像头的转换，只取上面h部分
        ((RelativeLayout.LayoutParams)bottomLayout.getLayoutParams()).topMargin = (int) (w / (MediaRecorderBase.SMALL_VIDEO_HEIGHT / (MediaRecorderBase.SMALL_VIDEO_WIDTH * 1.0f)));
        int width = w;
        int height = (int) (w * (MediaRecorderBase.mSupportedPreviewWidth * 1.0f)) / MediaRecorderBase.SMALL_VIDEO_HEIGHT;
        Log.e("debug", "initSurfaceView: w=" + width + ",h=" + height);
        FrameLayout.LayoutParams lp = (FrameLayout.LayoutParams) mSurfaceView
                .getLayoutParams();
        lp.width = width;
        lp.height = height;
        mSurfaceView.setLayoutParams(lp);
    }

    /**
     * 摄像头初始化完毕，初始化显示
     */
    @Override
    public void onPrepared() {
        initSurfaceView();
    }

    @Override
    public void onVideoError(int what, int extra) {

    }

    @Override
    public void onAudioError(int what, String message) {

    }
}
