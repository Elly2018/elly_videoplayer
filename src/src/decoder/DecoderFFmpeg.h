//========= Copyright 2015-2019, HTC Corporation. All rights reserved. ===========

#pragma once
#include <decoder/IDecoder.h>
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
	~DecoderFFmpeg();

	bool init(const char* filePath);
	bool init(const char* format, const char* filePath);
	bool decode();
	bool buffering();
	void seek(double time);
	void destroy();

	VideoInfo getVideoInfo();
	AudioInfo getAudioInfo();
	SubtitleInfo getSubtitleInfo();
	bool isBufferingFinish();
	void setVideoEnable(bool isEnable);
	void setAudioEnable(bool isEnable);
	void setAudioAllChDataEnable(bool isEnable);
	double getVideoFrame(void** frameData);
	double getAudioFrame(unsigned char** outputFrame, int& frameSize, int& nb_channel, size_t& byte_per_sample);
	void freeVideoFrame();
	void freeAudioFrame();
	void freePreloadFrame();
	void freeBufferFrame();
	void print_stream_maps();

	int getMetaData(char**& key, char**& value);
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
	AVStream*		mSubtitleStream;
	const AVCodec*		mVideoCodec;
	const AVCodec*		mAudioCodec;
	const AVCodec*		mSubtitleCodec;
	AVCodecContext*	mVideoCodecContext;
	AVCodecContext*	mAudioCodecContext;
	AVCodecContext*	mSubtitleCodecContext;

	AVPacket*	mPacket;
	std::queue<AVFrame*> mVideoFrames;
	std::queue<AVFrame*> mAudioFrames;
	std::queue<AVFrame*> mSubtitleFrames;
	unsigned int mVideoBuffMax;
	unsigned int mAudioBuffMax;
	unsigned int mSubtitleBuffMax;

	std::queue<AVFrame*> mVideoFramesPreload;
	std::queue<AVFrame*> mAudioFramesPreload;
	std::queue<AVFrame*> mSubtitleFramesPreload;
	unsigned int mVideoPreloadMax;
	unsigned int mAudioPreloadMax;
	unsigned int mSubtitlePreloadMax;

	SwrContext*	mSwrContext;
	int initSwrContext();

	VideoInfo	mVideoInfo;
	AudioInfo	mAudioInfo;
	SubtitleInfo	mSubtitleInfo;
	void updateBufferState();

	int mFrameBufferNum;

	bool isBuffBlocked();
	bool isPreloadBlocked();
	void preloadVideoFrame();
	void preloadAudioFrame();
	void preloadSubtitleFrame();
	void updateVideoFrame();
	void updateAudioFrame();
	void updateSubtitleFrame();
	void freeFrontFrame(std::queue<AVFrame*>* frameBuff, std::mutex* mutex);
	void freeAllFrame(std::queue<AVFrame*>* frameBuff);
	void flushBuffer(std::queue<AVFrame*>* frameBuff, std::mutex* mutex);
	AVCodecContext* getStreamCodecContext(int index);
	void freeStreamCodecContext(AVCodecContext* codec);
	void getListType(AVFormatContext* format, std::vector<int>& v, std::vector<int>& a, std::vector<int>& s);
	std::mutex mVideoMutex;
	std::mutex mAudioMutex;
	std::mutex mSubtitleMutex;

	bool mIsSeekToAny;

	void printErrorMsg(int errorCode);
};
