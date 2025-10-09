//========= Copyright 2015-2019, HTC Corporation. All rights reserved. ===========

#pragma once
#include "IDecoder.h"
#include <queue>
#include <mutex>

extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}

class DecoderFFmpeg : public virtual IDecoder
{
public:
	DecoderFFmpeg();
	virtual ~DecoderFFmpeg();

	bool init(const char* filePath) override;
	bool init(const char* format, const char* filePath);
	bool decode() override;
	bool buffering() override;
	void seek(double time) override;
	void destroy() override;

	VideoInfo getVideoInfo() override;
	AudioInfo getAudioInfo() override;
	SubtitleInfo getSubtitleInfo() override;
	bool isBufferingFinish() override;
	void setVideoEnable(bool isEnable) override;
	void setAudioEnable(bool isEnable) override;
	void setAudioAllChDataEnable(bool isEnable) override;
	double getVideoFrame(void** frameData) override;
	double getAudioFrame(unsigned char** outputFrame, int& frameSize, int& nb_channel, size_t& byte_per_sample) override;
	void freeVideoFrame() override;
	void freeAudioFrame() override;
	void print_stream_maps();

	int getMetaData(char**& key, char**& value) override;
	int getStreamCount();
	/**
	 * 
	 * Get the type from streams by index.
	 *
	 * @return -1: Fail, 0: Video, 1: Audio, 2: Data, 3: Subtitle
	 * 
	 */
	int getStreamType(int index);


private:
	bool mIsInitialized;
	bool mIsAudioAllChEnabled;
	bool mUseTCP;				//	For RTSP stream.

	AVFormatContext* mAVFormatContext;
	AVStream*		mVideoStream;
	AVStream*		mAudioStream;
	AVStream*		mSubtitleStream{};
	const AVCodec*		mVideoCodec;
	const AVCodec*		mAudioCodec;
	const AVCodec*		mSubtitleCodec{};
	AVCodecContext*	mVideoCodecContext;
	AVCodecContext*	mAudioCodecContext;
	AVCodecContext*	mSubtitleCodecContext{};

	AVPacket*	mPacket;
	std::queue<AVFrame*> mVideoFrames;
	std::queue<AVFrame*> mAudioFrames;
	std::queue<AVFrame*> mSubtitleFrames;
	unsigned int mVideoBuffMax;
	unsigned int mAudioBuffMax;
	unsigned int mSubtitleBuffMax{};

	SwrContext*	mSwrContext;
	int initSwrContext();

	VideoInfo	mVideoInfo{};
	AudioInfo	mAudioInfo{};
	SubtitleInfo	mSubtitleInfo{};
	void updateBufferState();

	int mFrameBufferNum{};

	bool isBuffBlocked();
	void updateVideoFrame();
	void updateAudioFrame();
	void updateSubtitleFrame();
	void freeFrontFrame(std::queue<AVFrame*>* frameBuff, std::mutex* mutex);
	void flushBuffer(std::queue<AVFrame*>* frameBuff, std::mutex* mutex);
	AVCodecContext* getStreamCodecContext(int index);
	void freeStreamCodecContext(AVCodecContext* codec);
	void getListType(AVFormatContext* format, std::vector<int>& v, std::vector<int>& a, std::vector<int>& s);
	std::mutex mVideoMutex;
	std::mutex mAudioMutex;
	std::mutex mSubtitleMutex;

	bool mIsSeekToAny;

	int loadConfig();
	void printErrorMsg(int errorCode);
};
