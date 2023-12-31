//========= Copyright 2015-2019, HTC Corporation. All rights reserved. ===========

#pragma once
#include "IDecoder.h"
#include <thread>
#include <mutex>
#include <memory>
 
class AVDecoderHandler {
public:
	AVDecoderHandler();
	~AVDecoderHandler();
	
	enum DecoderState {
		INIT_FAIL = -1, UNINITIALIZED, INITIALIZED, DECODING, SEEK, BUFFERING, DECODE_EOF, STOP
	};
	enum BufferState {
		NONE, LOADING, FULL
	};
	enum MediaType {
		VIDEO, AUDIO, SUBTITLE
	};
	DecoderState getDecoderState();

	void init(const char* filePath);
	void startDecoding();
	void stopDecoding();

    void stop();

    bool isDecoderRunning() const;

	void setSeekTime(float sec);
	
	double getVideoFrame(void** frameData);
	double getAudioFrame(uint8_t** outputFrame, int& frameSize, int& nb_channel, size_t& byte_per_sample);
	bool getOtherIndex(MediaType type, int* li, int& count, int& current);
	void freeVideoFrame();
	void freeAudioFrame();
	void setVideoEnable(bool isEnable);
	void setAudioEnable(bool isEnable);
	void setAudioAllChDataEnable(bool isEnable);

	IDecoder::VideoInfo getVideoInfo();
	IDecoder::AudioInfo getAudioInfo();
	IDecoder::SubtitleInfo getSubtitleInfo();
	bool isVideoBufferEmpty();
	bool isVideoBufferFull();

	int getMetaData(char**& key, char**& value);

private:
	DecoderState mDecoderState;
	BufferState mBufferState;
	std::unique_ptr<IDecoder> mIDecoder;
	double mSeekTime;
	
	std::thread mDecodeThread;
	std::thread mBufferThread;
  bool mDecodeThreadRunning = false;
	bool mBufferThreadRunning = false;
};