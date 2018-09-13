package com.autulin.gb28181library;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.RelativeLayout;

import com.autulin.gb28181library.utils.DeviceUtils;

import java.io.File;

public class DemoActivity extends AppCompatActivity implements
        MediaRecorderBase.OnErrorListener, MediaRecorderBase.OnPreparedListener {

    private Button mButton;
    private SurfaceView mSurfaceView;

    private MediaRecorderNative mMediaRecorder;
    private MediaOutput mediaOutput;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON); // 防止锁屏
        setContentView(R.layout.activity_demo);
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
            }
        });
    }

    @Override
    public void onResume() {
        super.onResume();

        if (mMediaRecorder == null) {
            initMediaRecorder();
        } else {
            mMediaRecorder.prepare();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        mMediaRecorder.release();
    }

    private void initData() {
        // 设置视频的宽高，比特率等
//        MediaRecorderBase.SMALL_VIDEO_HEIGHT = mediaRecorderConfig.getSmallVideoHeight();
//        MediaRecorderBase.SMALL_VIDEO_WIDTH = mediaRecorderConfig.getSmallVideoWidth();
//        MediaRecorderBase.mVideoBitrate = mediaRecorderConfig.getVideoBitrate();
    }

    private void initView() {
        mButton = findViewById(R.id.start_btn);
        mSurfaceView = findViewById(R.id.record_preview);
    }

    /**
     * 初始化拍摄SDK
     */
    private void initMediaRecorder() {
        mMediaRecorder = new MediaRecorderNative();

        mMediaRecorder.setOnErrorListener(this);
        mMediaRecorder.setOnPreparedListener(this);

        // 设置输出
//        String fileName = String.valueOf(System.currentTimeMillis());
        String fileName = "tttttt";
        mediaOutput = mMediaRecorder.setFileOutPut(fileName);

        mMediaRecorder.setSurfaceHolder(mSurfaceView.getHolder());
        mMediaRecorder.prepare();
    }

    /**
     * 初始化画布
     */
    private void initSurfaceView() {
        final int w = DeviceUtils.getScreenWidth(this);
        int width = w;
        int height = (int) (w * ((MediaRecorderBase.mSupportedPreviewWidth * 1.0f) / MediaRecorderBase.SMALL_VIDEO_HEIGHT));
        Log.e("debug", "initSurfaceView: w=" + width + ",h=" + height);
        FrameLayout.LayoutParams lp = (FrameLayout.LayoutParams) mSurfaceView
                .getLayoutParams();
        lp.width = width;
        lp.height = height;
        mSurfaceView.setLayoutParams(lp);
    }

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
