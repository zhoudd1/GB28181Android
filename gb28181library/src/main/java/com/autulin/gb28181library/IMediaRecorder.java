package com.autulin.gb28181library;



public interface IMediaRecorder {

	public void startMux();
	/**
	 * 音频错误
	 *
	 * @param what 错误类型
	 * @param message
	 */
	public void onAudioError(int what, String message);

	/**
	 * 接收音频数据
	 *
	 * @param sampleBuffer 音频数据
	 */
	public void receiveAudioData(byte[] sampleBuffer);
}
