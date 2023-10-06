//========= Copyright 2015-2019, HTC Corporation. All rights reserved. ===========

#pragma once

extern "C" {
    // Utils
    void nativeCleanAll();
    void nativeCleanDestroyedDecoders();
	//	Decoder
	int nativeCreateDecoder(const char* filePath, int& id);
	int nativeCreateDecoderAsync(const char* filePath, int& id);
	int nativeGetDecoderState(int id);
	bool nativeStartDecoding(int id);
    void nativeScheduleDestroyDecoder(int id);
	void nativeDestroyDecoder(int id);
	bool nativeIsEOF(int id);
    void nativeGrabVideoFrame(int id, void** frameData, bool& frameReady);
    void nativeReleaseVideoFrame(int id);
	//	Video
	bool nativeIsVideoEnabled(int id);
	void nativeSetVideoEnable(int id, bool isEnable);
	void nativeGetVideoFormat(int id, int& width, int& height, float& totalTime);
	void nativeSetVideoTime(int id, float currentTime);
	bool nativeIsContentReady(int id);
	bool nativeIsVideoBufferFull(int id);
	bool nativeIsVideoBufferEmpty(int id);
	//	Audio
	bool nativeIsAudioEnabled(int id);
	void nativeSetAudioEnable(int id, bool isEnable);
	void nativeSetAudioAllChDataEnable(int id, bool isEnable);
	void nativeGetAudioFormat(int id, int& channel, int& sampleRate, float& totalTime);
	float nativeGetAudioData(int id, unsigned char** audioData, int& frameSize, int& nb_channel, size_t& byte_per_sample);
	void nativeFreeAudioData(int id);
	//	Seek
	void nativeSetSeekTime(int id, float sec);
	bool nativeIsSeekOver(int id);
	//  Utility
	int nativeGetMetaData(const char* filePath, char*** key, char*** value);
}
