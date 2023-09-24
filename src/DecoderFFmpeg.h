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

typedef struct RTexture RTexture;
typedef struct MyAVPacketList MyAVPacketList;
typedef struct PacketQueue PacketQueue;
typedef struct AudioParams AudioParams;
typedef struct Clock Clock;
typedef struct FrameData FrameData;
typedef struct Frame Frame;
typedef struct FrameQueue FrameQueue;
typedef struct Decoder Decoder;
typedef struct VideoState VideoState;

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

	int64_t start_time = AV_NOPTS_VALUE;
	int64_t duration = AV_NOPTS_VALUE;
	int eof;
	int paused;
	int muted;
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
	int startup_volume;
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
	int read_thread(void *arg);
	int stream_has_enough_packets(AVStream *st, int stream_id, PacketQueue *queue);
	int frame_queue_nb_remaining(FrameQueue *f);
	void stream_component_close(VideoState *is, int stream_index);
	void stream_close(VideoState *is);
	void printErrorMsg(int errorCode);
};
