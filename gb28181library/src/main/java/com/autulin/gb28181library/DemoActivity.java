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
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.UnknownHostException;

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
//                new Thread(runnable).start();
            }
        });
    }

    private Runnable runnable = new Runnable() {
        @Override
        public void run() {
            try {
                DatagramSocket socket = new DatagramSocket(8888);
                InetAddress serverAddress = InetAddress.getByName("10.112.181.160");
                String str = "hello";
                DatagramPacket pkt = new DatagramPacket (str.getBytes() , str.getBytes().length , serverAddress , 8888);
                socket.send(pkt);
                socket.close();
            } catch (SocketException e) {
                e.printStackTrace();
            } catch (UnknownHostException e) {
                e.printStackTrace();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    };

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
//        MediaRecorderBase.mVideoBitrate = mediaRecorderConfig.getVideoBitrate();
        Log.i("debug", "SMALL_VIDEO_HEIGHT: " + MediaRecorderBase.SMALL_VIDEO_HEIGHT + ", SMALL_VIDEO_WIDTH:" + MediaRecorderBase.SMALL_VIDEO_WIDTH );
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
        mediaOutput = mMediaRecorder.setFileOutPut(fileName);  //输出到文件，这里demo是/sdcard/DCIM/pstest/tttttt.ps
//        int ssrc = 1;
//        mediaOutput = mMediaRecorder.setUdpOutPut("10.112.181.160", 8888, ssrc);

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
