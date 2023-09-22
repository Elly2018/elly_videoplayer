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
	~DecoderFFmpeg();

	bool init(const char* filePath);
	bool decode();
	void seek(double time);
	void destroy();

	VideoInfo getVideoInfo();
	AudioInfo getAudioInfo();
	void setVideoEnable(bool isEnable);
	void setAudioEnable(bool isEnable);
	void setAudioAllChDataEnable(bool isEnable);
	double getVideoFrame(void** frameData);
	double getAudioFrame(unsigned char** outputFrame, int& frameSize);
	void freeVideoFrame();
	void freeAudioFrame();

	int getMetaData(char**& key, char**& value);

private:
	bool mIsInitialized;
	bool mIsAudioAllChEnabled;
	bool mUseTCP;				//	For RTSP stream.

	AVFormatContext* mAVFormatContext;
	AVStream*		mVideoStream;
	AVStream*		mAudioStream;
	AVStream*		mSubtitleStream;
	//AVCodec*		mVideoCodec;
	//AVCodec*		mAudioCodec;
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

	SwrContext*	mSwrContext;
	int initSwrContext();

	VideoInfo	mVideoInfo;
	AudioInfo	mAudioInfo;
	void updateBufferState();

	int mFrameBufferNum;
	float seek_interval;
	int audio_disable;
	int video_disable;
	int subtitle_disable;
	int av_sync_type;
	int fast;
	int genpts;
	int decoder_reorder_pts;
	int framedrop;
	int infinite_buffer;
	const char* audio_codec_name;
	const char* subtitle_codec_name;
	const char* video_codec_name;
	int nb_vtiler;
	char *afilters;
	int find_stream_info;
	int filter_nbthreads;

	bool isBuffBlocked();
	void updateVideoFrame();
	void updateAudioFrame();
	void freeFrontFrame(std::queue<AVFrame*>* frameBuff, std::mutex* mutex);
	void flushBuffer(std::queue<AVFrame*>* frameBuff, std::mutex* mutex);
	std::mutex mVideoMutex;
	std::mutex mAudioMutex;
	std::mutex mSubtitleMutex;

	VideoState *is;

	bool mIsSeekToAny;

	int loadConfig();
	void printErrorMsg(int errorCode);
};
