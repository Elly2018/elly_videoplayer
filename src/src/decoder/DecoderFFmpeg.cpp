//========= Copyright 2015-2019, HTC Corporation. All rights reserved. ===========
#include "Logger.h"
#include "DecoderFFmpeg.h"
#include "DecodeConfig.h"
#include <fstream>
#include <string>
#include <thread>
#include <future>

extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/hwcontext.h>
}

#ifdef DECODER_HW
static AVBufferRef* hw_device_ctx = NULL;
static enum AVPixelFormat hw_pix_fmt;

static int hw_decoder_init(AVCodecContext* ctx, const enum AVHWDeviceType type)
{
	int err = 0;
	if ((err = av_hwdevice_ctx_create(&hw_device_ctx, type,
		NULL, NULL, 0)) < 0) {
		LOG_ERROR("Failed to create specified HW device.");
		return err;
	}
	ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
	return err;
}

static AVPixelFormat get_hw_format(AVCodecContext* ctx, const enum AVPixelFormat* pix_fmts)
{
	const enum AVPixelFormat* p;
	for (p = pix_fmts; *p != -1; p++) {
		if (*p == hw_pix_fmt)
			return *p;
	}
	LOG_ERROR("Failed to get HW surface format.");
	return AV_PIX_FMT_NONE;
}

#endif

DecoderFFmpeg::DecoderFFmpeg() {
	LOG("[DecoderFFmpeg] Create");
	mAVFormatContext = nullptr;
	mVideoStream = nullptr;
	mAudioStream = nullptr;
	mSubtitleStream = nullptr;
	mVideoCodec = nullptr;
	mAudioCodec = nullptr;
	mSubtitleCodec = nullptr;
	mVideoCodecContext = nullptr;
	mAudioCodecContext = nullptr;
	mSubtitleCodecContext = nullptr;
	mPacket = av_packet_alloc();

	mSwrContext = nullptr;

	mVideoBuffMax = DEFAULT_VIDEO_BUFFER;
	mAudioBuffMax = DEFAULT_AUDIO_BUFFER;

	mVideoPreloadMax = DEFAULT_VIDEO_PRELOAD;
	mAudioPreloadMax = DEFAULT_AUDIO_PRELOAD;

	memset(&mVideoInfo, 0, sizeof(VideoInfo));
	memset(&mAudioInfo, 0, sizeof(AudioInfo));
	mIsInitialized = false;
	mIsAudioAllChEnabled = false;
	mUseTCP = false;
	mIsSeekToAny = false;
}

DecoderFFmpeg::~DecoderFFmpeg() {
	LOG("[DecoderFFmpeg] Destroy");
	destroy();
}

bool DecoderFFmpeg::init(const char* filePath) {
	return init(nullptr, filePath);
}

bool DecoderFFmpeg::init(const char* format, const char* filePath) {
	int st_index[AVMEDIA_TYPE_NB];

	if (mIsInitialized) {
		LOG("Decoder has been init.");
		return true;
	}

	if (filePath == nullptr) {
		LOG("File path is nullptr.");
		return false;
	}

	LOG("Network init");
	avformat_network_init();
	//av_register_all();

	if (mAVFormatContext == nullptr) {
		mAVFormatContext = avformat_alloc_context();
	}

	int errorCode = 0;

	AVDictionary* opts = nullptr;
	av_dict_set(&opts, "buffer_size", "655360", 0);
	av_dict_set(&opts, "hwaccel", "auto", 0);
	av_dict_set(&opts, "movflags", "faststart", 0);
	av_dict_set(&opts, "refcounted_frames", "1", 0);
	if (mUseTCP) {
		av_dict_set(&opts, "rtsp_transport", "tcp", 0);
	}
	
	if (sizeof(format) == 0) {
		errorCode = avformat_open_input(&mAVFormatContext, filePath, nullptr, &opts);
	}
	else {
		const AVInputFormat* mInputFormat = av_find_input_format(format);
		errorCode = avformat_open_input(&mAVFormatContext, filePath, mInputFormat, &opts);
	}
	av_dict_free(&opts);
	if (errorCode < 0) {
		LOG("avformat_open_input error: ", errorCode);
		printErrorMsg(errorCode);
		return false;
	}

	errorCode = avformat_find_stream_info(mAVFormatContext, nullptr);
	if (errorCode < 0) {
		LOG("avformat_find_stream_info error: ", errorCode);
		printErrorMsg(errorCode);
		return false;
	}

	double ctxDuration = (double)(mAVFormatContext->duration) / AV_TIME_BASE;
#ifdef DECODER_HW
	type = av_hwdevice_iterate_types(type);
#endif
	/* Video initialization */
	LOG("Video initialization  ");
	st_index[AVMEDIA_TYPE_VIDEO] = av_find_best_stream(mAVFormatContext, AVMEDIA_TYPE_VIDEO, st_index[AVMEDIA_TYPE_VIDEO], -1, nullptr, 0);
	if (st_index[AVMEDIA_TYPE_VIDEO] < 0) {
		LOG("video stream not found.");
		mVideoInfo.isEnabled = false;
	} else {
		mVideoInfo.isEnabled = true;
		mVideoStream = mAVFormatContext->streams[st_index[AVMEDIA_TYPE_VIDEO]];
        mVideoCodecContext = avcodec_alloc_context3(NULL);
        mVideoCodecContext->refs = 1;
        avcodec_parameters_to_context(mVideoCodecContext, mVideoStream->codecpar);
#ifdef DECODER_HW
		if (type != AV_HWDEVICE_TYPE_NONE) {
			mVideoCodecContext->get_format = get_hw_format;
			hw_decoder_init(mVideoCodecContext, type);
			LOG("hwaccel: ", type);
		}
#endif
		LOG("Video codec id: ", mVideoCodecContext->codec_id);
		mVideoCodec = avcodec_find_decoder(mVideoCodecContext->codec_id);
		if (mVideoCodec == nullptr) {
			LOG("Video codec not available.");
			return false;
		}
		AVDictionary *autoThread = nullptr;
		av_dict_set(&autoThread, "threads", "auto", 0);
		av_dict_set(&autoThread, "flags", "+copy_opaque", AV_DICT_MULTIKEY);
		mVideoCodecContext->flags2 |= AV_CODEC_FLAG2_FAST;
		errorCode = avcodec_open2(mVideoCodecContext, mVideoCodec, &autoThread);
		av_dict_free(&autoThread);
		if (errorCode < 0) {
			LOG("Could not open video codec: ", errorCode);
			printErrorMsg(errorCode);
			return false;
		}

		//	Save the output video format
		//	Duration / time_base = video time (seconds)
		mVideoInfo.width = mVideoCodecContext->width;
		mVideoInfo.height = mVideoCodecContext->height;
		mVideoInfo.currentIndex = st_index[AVMEDIA_TYPE_VIDEO];
		mVideoInfo.framerate = av_q2d(mVideoCodecContext->framerate);
		mVideoInfo.totalTime = mVideoStream->duration <= 0 ? ctxDuration : mVideoStream->duration * av_q2d(mVideoStream->time_base);
		//mVideoFrames.swap(decltype(mVideoFrames)());
	}

#ifdef DECODER_HW
	for (int i = 0;; i++) {
		const AVCodecHWConfig* config = avcodec_get_hw_config(mVideoCodec, i);
		if (!config) {
			LOG_ERROR("Decoder ", mVideoCodec->name, " does not support device type, ", av_hwdevice_get_type_name(type));
			hw_pix_fmt = AV_PIX_FMT_NONE;
		}
		if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX &&
			config->device_type == type) {
			hw_pix_fmt = config->pix_fmt;
			break;
		}
	}
#endif

	/* Audio initialization */
	LOG("Audio initialization ");
	st_index[AVMEDIA_TYPE_AUDIO] = av_find_best_stream(mAVFormatContext, AVMEDIA_TYPE_AUDIO, st_index[AVMEDIA_TYPE_AUDIO], st_index[AVMEDIA_TYPE_VIDEO], nullptr, 0);
	if (st_index[AVMEDIA_TYPE_AUDIO] < 0) {
		LOG("audio stream not found. ");
		mAudioInfo.isEnabled = false;
	} else {
		mAudioInfo.isEnabled = true;
		mAudioStream = mAVFormatContext->streams[st_index[AVMEDIA_TYPE_AUDIO]];
		mAudioCodecContext = avcodec_alloc_context3(NULL);
        avcodec_parameters_to_context(mAudioCodecContext, mAudioStream->codecpar);
		LOG("Audio codec id: ", mAudioCodecContext->codec_id);
        mAudioCodec = avcodec_find_decoder(mAudioCodecContext->codec_id);
		if (mAudioCodec == nullptr) {
			LOG("Audio codec not available. ");
			return false;
		}
		AVDictionary* autoThread = nullptr;
		av_dict_set(&autoThread, "threads", "auto", 0);
		av_dict_set(&autoThread, "flags", "+copy_opaque", AV_DICT_MULTIKEY);
		mAudioCodecContext->flags2 |= AV_CODEC_FLAG2_FAST;
		errorCode = avcodec_open2(mAudioCodecContext, mAudioCodec, &autoThread);
		if (errorCode < 0) {
			LOG("Could not open audio codec(%x). ", errorCode);
			printErrorMsg(errorCode);
			return false;
		}

		errorCode = initSwrContext();
		if (errorCode < 0) {
			LOG("Init SwrContext error.(%x) ", errorCode);
			printErrorMsg(errorCode);
			return false;
		}

		//mAudioFrames.swap(decltype(mAudioFrames)());
		mAudioInfo.currentIndex = st_index[AVMEDIA_TYPE_AUDIO];
	}

	std::vector<int> videoIndex = std::vector<int>();
	std::vector<int> audioIndex = std::vector<int>();
	std::vector<int> subtitleIndex = std::vector<int>();
	getListType(mAVFormatContext, videoIndex, audioIndex, subtitleIndex);
	mVideoInfo.otherIndex = videoIndex.data();
	mVideoInfo.otherIndexCount = videoIndex.size();
	mAudioInfo.otherIndex = audioIndex.data();
	mAudioInfo.otherIndexCount = audioIndex.size();
	mSubtitleInfo.otherIndex = subtitleIndex.data();
	mSubtitleInfo.otherIndexCount = subtitleIndex.size();

	LOG("Finished initialization");
	mIsInitialized = true;
	print_stream_maps();
	return true;
}

bool DecoderFFmpeg::decode() {
	if (!mIsInitialized) {
		LOG("Not initialized. ");
		return false;
	}

	if (!isBuffBlocked()) {
		updateVideoFrame();
		updateAudioFrame();
		updateAudioFrame();
		//updateSubtitleFrame();
	}

	return true;
}

bool DecoderFFmpeg::buffering() {
	if (!mIsInitialized) {
		LOG("Not initialized. ");
		return false;
	}

	if (!isPreloadBlocked()) {
		int ret = -1;
		{
			std::lock_guard<std::mutex> lock(mPacketMutex);
			ret = av_read_frame(mAVFormatContext, mPacket);
		}
		if (ret < 0) {
			preloadVideoFrame();
			LOG_VERBOSE("End of file.");
			return true;
		}

		if (mVideoInfo.isEnabled && mVideoStream != nullptr && mPacket->stream_index == mVideoStream->index) {
			preloadVideoFrame();
		}
		else if (mAudioInfo.isEnabled && mAudioStream != nullptr && mPacket->stream_index == mAudioStream->index) {
			preloadAudioFrame();
		}
		else if (mSubtitleInfo.isEnabled && mSubtitleStream != nullptr && mPacket->stream_index == mSubtitleStream->index) {
			//preloadSubtitleFrame();
		}

		av_packet_unref(mPacket);
	}
	return true;
}

IDecoder::VideoInfo DecoderFFmpeg::getVideoInfo() {
	return mVideoInfo;
}

IDecoder::AudioInfo DecoderFFmpeg::getAudioInfo() {
	return mAudioInfo;
}

IDecoder::SubtitleInfo DecoderFFmpeg::getSubtitleInfo()
{
	return mSubtitleInfo;
}

bool DecoderFFmpeg::isBufferingFinish()
{
	return false;
}

void DecoderFFmpeg::setVideoEnable(bool isEnable) {
	if (mVideoStream == nullptr) {
		LOG("Video stream not found. \n");
		return;
	}

	mVideoInfo.isEnabled = isEnable;
}

void DecoderFFmpeg::setAudioEnable(bool isEnable) {
	if (mAudioStream == nullptr) {
		LOG("Audio stream not found. \n");
		return;
	}

	mAudioInfo.isEnabled = isEnable;
}

void DecoderFFmpeg::setAudioAllChDataEnable(bool isEnable) {
	mIsAudioAllChEnabled = isEnable;
	initSwrContext();
}

int DecoderFFmpeg::initSwrContext() {
	if (mAudioCodecContext == nullptr) {
		LOG("Audio context is null. \n");
		return -1;
	}

	int errorCode = 0;
	int64_t inChannelLayout = av_get_default_channel_layout(mAudioCodecContext->channels);
	uint64_t outChannelLayout = mIsAudioAllChEnabled ? inChannelLayout : AV_CH_LAYOUT_STEREO;
	AVSampleFormat inSampleFormat = mAudioCodecContext->sample_fmt;
	AVSampleFormat outSampleFormat = AV_SAMPLE_FMT_FLT;
	int inSampleRate = mAudioCodecContext->sample_rate;
	int outSampleRate = inSampleRate;

	if (mSwrContext != nullptr) {
		swr_close(mSwrContext);
		swr_free(&mSwrContext);
		mSwrContext = nullptr;
	}

	mSwrContext = swr_alloc();
	swr_alloc_set_opts(mSwrContext,
		outChannelLayout, outSampleFormat, outSampleRate,
		inChannelLayout, inSampleFormat, inSampleRate,
		0, nullptr);

	
	if (swr_is_initialized(mSwrContext) == 0) {
		errorCode = swr_init(mSwrContext);
	}

	//	Save the output audio format
	mAudioInfo.channels = av_get_channel_layout_nb_channels(outChannelLayout);
	mAudioInfo.sampleRate = outSampleRate;
	mAudioInfo.totalTime = mAudioStream->duration <= 0 ? (double)(mAVFormatContext->duration) / AV_TIME_BASE : mAudioStream->duration * av_q2d(mAudioStream->time_base);
	
	return errorCode;
}

double DecoderFFmpeg::getVideoFrame(void** frameData, int& width, int& height) {
	std::lock_guard<std::mutex> lock(mVideoMutex);
	if (!mIsInitialized || mVideoFrames.size() == 0) {
		LOG_VERBOSE("Video frame not available. ");
        *frameData = nullptr;
		return -1;
	}

	AVFrame* frame = mVideoFrames.front();
	*frameData = frame->data[0];
	width = frame->width;
	height = frame->height;

	int64_t timeStamp = frame->pts;
	double timeInSec = av_q2d(mVideoStream->time_base) * timeStamp;
	mVideoInfo.lastTime = timeInSec;

	LOG_VERBOSE("mVideoInfo.lastTime: ", timeInSec);

	return timeInSec;
}

double DecoderFFmpeg::getAudioFrame(unsigned char** outputFrame, int& frameSize, int& nb_channel, size_t& byte_per_sample) {
	std::lock_guard<std::mutex> lock(mAudioMutex);
	if (!mIsInitialized || mAudioFrames.size() == 0) {
		LOG_VERBOSE("Audio frame not available. ");
		*outputFrame = nullptr;
		return -1;
	}

	AVFrame* frame = mAudioFrames.front();
	nb_channel = frame->channels;
	frameSize = frame->nb_samples;
	*outputFrame = frame->data[0];
	byte_per_sample = (size_t)av_get_bytes_per_sample(mAudioCodecContext->sample_fmt);
	int64_t timeStamp = frame->pts;
	double timeInSec = av_q2d(mAudioStream->time_base) * timeStamp;
	mAudioInfo.lastTime = timeInSec;

	LOG_VERBOSE("mAudioInfo.lastTime: ", timeInSec);

	return timeInSec;
}

void DecoderFFmpeg::seek(double time) {
	if (!mIsInitialized) {
		LOG("Not initialized.");
		return;
	}

	uint64_t timeStamp = (uint64_t) time * AV_TIME_BASE;
	std::lock_guard<std::mutex> lock(mPacketMutex);
	int ret = av_seek_frame(mAVFormatContext, -1, timeStamp, AVSEEK_FLAG_FRAME);
	if (ret < 0) {
		LOG("Seek time fail.");
		return;
	}

	if (mVideoInfo.isEnabled) {
		if (mVideoCodecContext != nullptr) {
			avcodec_flush_buffers(mVideoCodecContext);
		}
		flushBuffer(&mVideoFrames, &mVideoMutex);
		freeAllFrame(&mVideoFramesPreload);
		mVideoInfo.lastTime = -1;
	}
	
	if (mAudioInfo.isEnabled) {
		if (mAudioCodecContext != nullptr) {
			avcodec_flush_buffers(mAudioCodecContext);
		}
		flushBuffer(&mAudioFrames, &mAudioMutex);
		freeAllFrame(&mAudioFramesPreload);
		mAudioInfo.lastTime = -1;
	}
}

int DecoderFFmpeg::getMetaData(char**& key, char**& value) {
	if (!mIsInitialized || key != nullptr || value != nullptr) {
		return 0;
	}

	AVDictionaryEntry *tag = nullptr;
	int metaCount = av_dict_count(mAVFormatContext->metadata);

	key = (char**)malloc(sizeof(char*) * metaCount);
	value = (char**)malloc(sizeof(char*) * metaCount);

	for (int i = 0; i < metaCount; i++) {
		tag = av_dict_get(mAVFormatContext->metadata, "", tag, AV_DICT_IGNORE_SUFFIX);
		key[i] = tag->key;
		value[i] = tag->value;
	}

	return metaCount;
}

int DecoderFFmpeg::getStreamCount()
{
	return mAVFormatContext != nullptr ? mAVFormatContext->nb_streams : 0;
}

int DecoderFFmpeg::getStreamType(int index)
{
	AVCodecContext* b = getStreamCodecContext(index);
	if (b == nullptr) return -1;
	int r = b->codec_type;
	freeStreamCodecContext(b);
	return r;
}

void DecoderFFmpeg::destroy() {
	if (mVideoCodecContext != nullptr) {
		avcodec_close(mVideoCodecContext);
		mVideoCodecContext = nullptr;
	}
	
	if (mAudioCodecContext != nullptr) {
		avcodec_close(mAudioCodecContext);
		mAudioCodecContext = nullptr;
	}
	
	if (mAVFormatContext != nullptr) {
		avformat_close_input(&mAVFormatContext);
		avformat_free_context(mAVFormatContext);
		mAVFormatContext = nullptr;
	}
	
	if (mSwrContext != nullptr) {
		swr_close(mSwrContext);
		swr_free(&mSwrContext);
		mSwrContext = nullptr;
	}
	
	flushBuffer(&mVideoFrames, &mVideoMutex);
	flushBuffer(&mAudioFrames, &mAudioMutex);

	freePreloadFrame();
	freeAllFrame(&mVideoFrames);
	freeAllFrame(&mAudioFrames);
	freeAllFrame(&mSubtitleFrames);

	mVideoCodec = nullptr;
	mAudioCodec = nullptr;
	
	mVideoStream = nullptr;
	mAudioStream = nullptr;
	av_packet_unref(mPacket);
	
	memset(&mVideoInfo, 0, sizeof(VideoInfo));
	memset(&mAudioInfo, 0, sizeof(AudioInfo));
	memset(&mSubtitleInfo, 0, sizeof(SubtitleInfo));
	
	mIsInitialized = false;
	mIsAudioAllChEnabled = false;
	mVideoBuffMax = DEFAULT_VIDEO_BUFFER;
	mAudioBuffMax = DEFAULT_AUDIO_BUFFER;
	mUseTCP = false;
	mIsSeekToAny = false;
}

bool DecoderFFmpeg::isBuffBlocked() {
	bool ret = false;
	if (mVideoInfo.isEnabled && mVideoFrames.size() >= mVideoBuffMax) {
		ret = true;
	}

	if (mAudioInfo.isEnabled && mAudioFrames.size() >= mAudioBuffMax) {
		ret = true;
	}

	return ret;
}

bool DecoderFFmpeg::isPreloadBlocked() {
	bool ret = false;
	if (mVideoInfo.isEnabled && mVideoFramesPreload.size() >= mVideoPreloadMax) {
		ret = true;
	}

	if (mAudioInfo.isEnabled && mVideoFramesPreload.size() >= mAudioPreloadMax) {
		ret = true;
	}

	return ret;
}

void DecoderFFmpeg::preloadVideoFrame()
{
	int ret = avcodec_send_packet(mVideoCodecContext, mPacket);
	if (ret != 0) {
		LOG_VERBOSE("Video frame update failed: avcodec_send_packet ", ret);
		return;
	}
	do {
		AVFrame* srcFrame = av_frame_alloc();
		ret = avcodec_receive_frame(mVideoCodecContext, srcFrame);
		if (ret != 0) {
			LOG_VERBOSE("Audio frame update failed: avcodec_receive_frame ", ret);
			return;
		}
#ifdef DECODER_HW
		if (srcFrame->format == hw_pix_fmt && hw_pix_fmt != AV_PIX_FMT_NONE) {
			AVFrame* destFrame = av_frame_alloc();
			av_frame_copy_props(destFrame, srcFrame);
			ret = av_hwframe_transfer_data(destFrame, srcFrame, 0);
			destFrame->best_effort_timestamp = srcFrame->best_effort_timestamp;
			destFrame->pts = srcFrame->pts;
			destFrame->pkt_dts = srcFrame->pkt_dts;
			destFrame->width = srcFrame->width;
			destFrame->height = srcFrame->height;
			if (ret < 0) {
				LOG_ERROR("av_hwframe_transfer_data error: ", ret);
			}
			av_frame_free(&srcFrame);
			mVideoFramesPreload.push(destFrame);
		}
		else {
			mVideoFramesPreload.push(srcFrame);
		}
#else
		mVideoFramesPreload.push(srcFrame);
#endif
	} while (ret != AVERROR(EAGAIN));
}

void DecoderFFmpeg::preloadAudioFrame()
{
	int ret = avcodec_send_packet(mAudioCodecContext, mPacket);
	if (ret != 0) {
		LOG_VERBOSE("Audio frame update failed: avcodec_send_packet ", ret);
		return;
	}
	do {
		AVFrame* srcFrame = av_frame_alloc();
		ret = avcodec_receive_frame(mAudioCodecContext, srcFrame);
		if (ret != 0) {
			LOG_VERBOSE("Audio frame update failed: avcodec_receive_frame ", ret);
			return;
		}
		mAudioFramesPreload.push(srcFrame);
	} while (ret != AVERROR(EAGAIN));
}

void DecoderFFmpeg::preloadSubtitleFrame()
{
}

void DecoderFFmpeg::updateVideoFrame() {
	if (mVideoFramesPreload.size() <= 0) return;
	AVFrame* srcFrame = mVideoFramesPreload.front();
	mVideoFramesPreload.pop();

	clock_t start = clock();

	int width = srcFrame->width;
	int height = srcFrame->height;

	const AVPixelFormat dstFormat = AV_PIX_FMT_RGB24;
	LOG_VERBOSE("Video format. w: ", width, ", h: ", height, ", f: ", dstFormat);
	AVFrame* dstFrame = av_frame_alloc();
	av_frame_copy_props(dstFrame, srcFrame);

	dstFrame->format = dstFormat;
	dstFrame->pts = srcFrame->best_effort_timestamp;
	
	//av_image_alloc(dstFrame->data, dstFrame->linesize, dstFrame->width, dstFrame->height, dstFormat, 0)
	int numBytes = av_image_get_buffer_size(dstFormat, width, height, 1);
	LOG_VERBOSE("Number of bytes: ", numBytes);
	AVBufferRef* buffer = av_buffer_alloc(numBytes * sizeof(uint8_t));
	//avpicture_fill((AVPicture *)dstFrame,buffer->data,dstFormat,width,height);
	if (buffer == nullptr) {
		LOG_VERBOSE("The video frame buffer is nullptr");
		return;
	}

	av_image_fill_arrays(dstFrame->data, dstFrame->linesize, buffer->data, dstFormat, width, height, 1);
	dstFrame->buf[0] = buffer;

	SwsContext* conversion = sws_getContext(width,
		height,
		(AVPixelFormat)srcFrame->format,
		width,
		height,
		dstFormat,
		SWS_POINT | SWS_FULL_CHR_H_INT | SWS_ACCURATE_RND,
		nullptr,
		nullptr,
		nullptr);
	
	sws_scale(conversion, srcFrame->data, srcFrame->linesize, 0, height, dstFrame->data, dstFrame->linesize);
	sws_freeContext(conversion);
	av_frame_copy_props(dstFrame, srcFrame);

	dstFrame->format = dstFormat;
	dstFrame->width = srcFrame->width;
	dstFrame->height = srcFrame->height;

	av_frame_free(&srcFrame);

	LOG_VERBOSE("updateVideoFrame = ", (float)(clock() - start) / CLOCKS_PER_SEC);

	std::lock_guard<std::mutex> lock(mVideoMutex);
	mVideoFrames.push(dstFrame);
	updateBufferState();
}

void DecoderFFmpeg::updateAudioFrame() {
	if (mAudioFramesPreload.size() <= 0) return;
	AVFrame* srcFrame = mAudioFramesPreload.front();
	mAudioFramesPreload.pop();
	clock_t start = clock();
	AVFrame* frame = av_frame_alloc();
	frame->sample_rate = srcFrame->sample_rate;
	frame->channel_layout = av_get_default_channel_layout(mAudioInfo.channels);
	frame->format = AV_SAMPLE_FMT_FLT;	//	For Unity format.
	frame->best_effort_timestamp = srcFrame->best_effort_timestamp;
	frame->pts = frame->best_effort_timestamp;

	int ret = swr_convert_frame(mSwrContext, frame, srcFrame);
	if (ret != 0) {
		LOG_VERBOSE("Audio update failed ", ret);
	}

	av_frame_free(&srcFrame);

	LOG_VERBOSE("updateAudioFrame. linesize: ", frame->linesize[0]);
	std::lock_guard<std::mutex> lock(mAudioMutex);
	mAudioFrames.push(frame);
	updateBufferState();
}

void DecoderFFmpeg::updateSubtitleFrame()
{
	AVFrame* frameDecoded = av_frame_alloc();
	int ret = avcodec_send_packet(mSubtitleCodecContext, mPacket);
	if (ret != 0) {
		LOG("Subtitle frame update failed: avcodec_send_packet ", ret);
		return;
	}
	ret = avcodec_receive_frame(mSubtitleCodecContext, frameDecoded);
	if (ret != 0) {
		LOG("Subtitle frame update failed: avcodec_receive_frame ", ret);
		return;
	}

	AVFrame* frame = av_frame_alloc();

	std::lock_guard<std::mutex> lock(mSubtitleMutex);
	mSubtitleFrames.push(frame);
	updateBufferState();
	av_frame_free(&frameDecoded);
}

void DecoderFFmpeg::freeVideoFrame() {
	freeFrontFrame(&mVideoFrames, &mVideoMutex);
}

void DecoderFFmpeg::freeAudioFrame() {
	freeFrontFrame(&mAudioFrames, &mAudioMutex);
}

void DecoderFFmpeg::freePreloadFrame()
{
	freeAllFrame(&mVideoFramesPreload);
	freeAllFrame(&mAudioFramesPreload);
	freeAllFrame(&mSubtitleFramesPreload);
}

void DecoderFFmpeg::freeBufferFrame() 
{
	freeAllFrame(&mVideoFrames);
	freeAllFrame(&mAudioFrames);
	freeAllFrame(&mSubtitleFrames);
}

void DecoderFFmpeg::print_stream_maps()
{
	LOG("Stream mapping:");
	LOG("  Video info:");
	if (mVideoCodecContext != nullptr) {
		LOG("    Width: ", mVideoCodecContext->width);
		LOG("    Height: ", mVideoCodecContext->height);
		LOG("    Bitrate: ", mVideoCodecContext->bit_rate);
	}
	if (mVideoCodecContext != nullptr) {
		LOG("    Codec_id: ", mVideoCodec->id);
		LOG("    Codec_name: ", mVideoCodec->name);
		LOG("    Codec_long_name: ", mVideoCodec->long_name);
	}
	if (mVideoCodecContext != nullptr) {
		LOG("    Codec_width: ", mVideoCodecContext->coded_width);
		LOG("    Codec_height: ", mVideoCodecContext->coded_height);
	}
	LOG("  Audio info: ");
	if (mAudioCodecContext != nullptr) {
		LOG("    Channel_count: ", mAudioCodecContext->channels);
		LOG("    Bitrate: ", mAudioCodecContext->bit_rate);
		LOG("    Codec_id: ", mAudioCodec->id);
	}
	if (mAudioInfo.isEnabled) {
		LOG("    Sample_rate: ", mAudioInfo.sampleRate);
	}
	if (mAudioCodec != nullptr) {
		LOG("    Codec_name: ", mAudioCodec->name);
		LOG("    Codec_long_name: ", mAudioCodec->long_name);
	}
}

void DecoderFFmpeg::freeFrontFrame(std::queue<AVFrame*>* frameBuff, std::mutex* mutex) {
	std::lock_guard<std::mutex> lock(*mutex);
	if (!mIsInitialized || frameBuff->size() == 0) {
		LOG_VERBOSE("Not initialized or buffer empty. ");
		return;
	}

	AVFrame* frame = frameBuff->front();
	av_frame_free(&frame);
	frameBuff->pop();
	updateBufferState();
}

void DecoderFFmpeg::freeAllFrame(std::queue<AVFrame*>* frameBuff)
{
	while (frameBuff->size() > 0) {
		AVFrame* f = frameBuff->front();
		av_frame_free(&f);
		frameBuff->pop();
	}
}

//	frameBuff.clear would only clean the pointer rather than whole resources. So we need to clear frameBuff by ourself.
void DecoderFFmpeg::flushBuffer(std::queue<AVFrame*>* frameBuff, std::mutex* mutex) {
	std::lock_guard<std::mutex> lock(*mutex);
	while (!frameBuff->empty()) {
		av_frame_free(&(frameBuff->front()));
		frameBuff->pop();
	}
}

AVCodecContext* DecoderFFmpeg::getStreamCodecContext(int index)
{
	if (index < 0 || index > mAVFormatContext->nb_streams) {
		LOG_ERROR("Index out of range: getStreamsCodecContext");
		return nullptr;
	}
	AVCodecContext* buffer = avcodec_alloc_context3(NULL);
	avcodec_parameters_to_context(buffer, mAVFormatContext->streams[index]->codecpar);
	return buffer;
}

void DecoderFFmpeg::freeStreamCodecContext(AVCodecContext* codec) {
	if (codec != nullptr)
	{
		avcodec_close(codec);
	}
}

void DecoderFFmpeg::getListType(AVFormatContext* format, std::vector<int>& v, std::vector<int>& a, std::vector<int>& s) {
	v.clear();
	a.clear();
	s.clear();
	for (int i = 0; i < format->nb_streams; i++) {
		int type = getStreamType(i);
		if (type == AVMEDIA_TYPE_VIDEO) v.push_back(i);
		else if (type == AVMEDIA_TYPE_AUDIO) a.push_back(i);
		else if (type == AVMEDIA_TYPE_SUBTITLE) s.push_back(i);
	}
}

//	Record buffer state either FULL or EMPTY. It would be considered by ViveMediaDecoder.cs for buffering judgement.
void DecoderFFmpeg::updateBufferState() {
	if (mVideoInfo.isEnabled) {
		if (mVideoFrames.size() >= mVideoBuffMax) {
			mVideoInfo.bufferState = BufferState::FULL;
		} else if(mVideoFrames.size() == 0) {
			mVideoInfo.bufferState = BufferState::EMPTY;
		} else {
			mVideoInfo.bufferState = BufferState::NORMAL;
		}
	}

	if (mAudioInfo.isEnabled) {
		if (mAudioFrames.size() >= mAudioBuffMax) {
			mAudioInfo.bufferState = BufferState::FULL;
		} else if (mAudioFrames.size() == 0) {
			mAudioInfo.bufferState = BufferState::EMPTY;
		} else {
			mAudioInfo.bufferState = BufferState::NORMAL;
		}
	}
}

void DecoderFFmpeg::printErrorMsg(int errorCode) {
	char msg[500];
	av_strerror(errorCode, msg, sizeof(msg));
	LOG("Error massage: ", msg);
}