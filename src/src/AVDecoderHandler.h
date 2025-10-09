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
	[[nodiscard]] DecoderState getDecoderState() const;

	void init(const char* filePath);
	void startDecoding();
	void stopDecoding();

    void stop();

    [[nodiscard]] bool isDecoderRunning() const;

	void setSeekTime(float sec);
	
	double getVideoFrame(void** frameData) const;
	double getAudioFrame(uint8_t** outputFrame, int& frameSize, int& nb_channel, size_t& byte_per_sample) const;
	bool getOtherIndex(MediaType type, const int* li, int& count, int& current) const;
	void freeVideoFrame() const;
	void freeAudioFrame() const;
	void setVideoEnable(bool isEnable) const;
	void setAudioEnable(bool isEnable) const;
	void setAudioAllChDataEnable(bool isEnable) const;

	[[nodiscard]] IDecoder::VideoInfo getVideoInfo() const;
	[[nodiscard]] IDecoder::AudioInfo getAudioInfo() const;
	[[nodiscard]] IDecoder::SubtitleInfo getSubtitleInfo() const;
	[[nodiscard]] bool isVideoBufferEmpty() const;
	[[nodiscard]] bool isVideoBufferFull() const;

	int getMetaData(char**& key, char**& value) const;

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