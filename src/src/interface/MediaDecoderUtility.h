/*
* The low-level global functions responsible for manage decoders.
*/
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
	bool nativeGetOtherStreamIndex(int id, int type, int* li, int& count, int& current);
	bool nativeIsEOF(int id);
    void nativeGrabVideoFrame(int id, void** frameData, bool& frameReady);
    void nativeReleaseVideoFrame(int id);
	//	Video
	bool nativeIsVideoEnabled(int id);
	void nativeSetVideoEnable(int id, bool isEnable);
	void nativeGetVideoFormat(int id, int& width, int& height, float& framerate, float& totalTime);
	void nativeSetVideoTime(int id, float currentTime);
	bool nativeIsContentReady(int id);
	bool nativeIsVideoBufferFull(int id);
	bool nativeIsVideoBufferEmpty(int id);
	//	Audio
	bool nativeIsAudioEnabled(int id);
	void nativeSetAudioEnable(int id, bool isEnable);
	void nativeSetAudioAllChDataEnable(int id, bool isEnable);
	void nativeGetAudioFormat(int id, int& channel, int& sampleRate, float& totalTime);
	bool nativeSetAudioBufferTime(int id, float time);
	float nativeGetAudioData(int id, bool& frame_ready, unsigned char** audioData, int& frameSize, int& nb_channel, size_t& byte_per_sample);
	void nativeFreeAudioData(int id);
	//	Seek
	void nativeSetSeekTime(int id, float sec);
	bool nativeIsSeekOver(int id);
	//  Utility
	int nativeGetMetaData(const char* filePath, char*** key, char*** value);
}
