package com.autulin.gb28181library;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.util.Log;

/**
 * 音频录制
 * 
 */
public class AudioCollector extends Thread {

	private AudioRecord mAudioRecord = null;
	/** 采样率 */
	private int mSampleRate = MediaRecorderNative.AUDIO_SAMPLE_RATE;
	private IMediaRecorder mMediaRecorder;

	public AudioCollector(IMediaRecorder mediaRecorder) {
		this.mMediaRecorder = mediaRecorder;
	}

	public int getFrameLen() {
		return AudioRecord.getMinBufferSize(mSampleRate, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT);
	}
	@Override
	public void run() {
		if (mSampleRate != 8000 && mSampleRate != 16000 && mSampleRate != 22050 && mSampleRate != 44100) {
			mMediaRecorder.onAudioError(MediaRecorderBase.AUDIO_RECORD_ERROR_SAMPLERATE_NOT_SUPPORT, "sampleRate not support.");
			return;
		}

		// 8000Hz -> 640。 25fps
		final int mMinBufferSize = AudioRecord.getMinBufferSize(mSampleRate, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT); // 一帧的大小

		if (AudioRecord.ERROR_BAD_VALUE == mMinBufferSize) {
			mMediaRecorder.onAudioError(MediaRecorderBase.AUDIO_RECORD_ERROR_GET_MIN_BUFFER_SIZE_NOT_SUPPORT, "parameters are not supported by the hardware.");
			return;
		}
		Log.e("AudioCollector", "sample rate:" + mSampleRate + ",mMinBufferSize: " + mMinBufferSize);

		mAudioRecord = new AudioRecord(android.media.MediaRecorder.AudioSource.MIC, mSampleRate, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT, mMinBufferSize); // mMinBufferSize这个参数必须大于等于一帧
		if (null == mAudioRecord) {
			mMediaRecorder.onAudioError(MediaRecorderBase.AUDIO_RECORD_ERROR_CREATE_FAILED, "new AudioRecord failed.");
			return;
		}
		try {
			mAudioRecord.startRecording();
		} catch (IllegalStateException e) {
			mMediaRecorder.onAudioError(MediaRecorderBase.AUDIO_RECORD_ERROR_UNKNOWN, "startRecording failed.");
			return;
		}

		byte[] sampleBuffer = new byte[mMinBufferSize];

		try {
			while (!Thread.currentThread().isInterrupted()) {

				int result = mAudioRecord.read(sampleBuffer, 0, mMinBufferSize);
				if (result > 0) {
					mMediaRecorder.receiveAudioData(sampleBuffer);
				}
			}
		} catch (Exception e) {
			String message = "";
			if (e != null)
				message = e.getMessage();
			mMediaRecorder.onAudioError(MediaRecorderBase.AUDIO_RECORD_ERROR_UNKNOWN, message);
		}

		mAudioRecord.release();
		mAudioRecord = null;
	}
}
