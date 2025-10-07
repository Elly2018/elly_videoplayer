//========= Copyright 2015-2019, HTC Corporation. All rights reserved. ===========

#include "AVDecoderHandler.h"
#include "DecoderFFmpeg.h"
#include "Logger.h"

AVDecoderHandler::AVDecoderHandler() {
	mDecoderState = UNINITIALIZED;
	mBufferState = NONE;
	mSeekTime = 0.0;
	mIDecoder = std::make_unique<DecoderFFmpeg>();
}

void AVDecoderHandler::init(const char* filePath) {
	if (mIDecoder == nullptr || !mIDecoder->init(filePath)) {
		mDecoderState = INIT_FAIL;
	} else {
		mDecoderState = INITIALIZED;
	}
}

AVDecoderHandler::DecoderState AVDecoderHandler::getDecoderState() const {
	return mDecoderState;
}

void AVDecoderHandler::stopDecoding() {
	mDecoderState = STOP;
	if (mDecodeThread.joinable()) {
		mDecodeThread.join();
	}

	mIDecoder = nullptr;
	mDecoderState = UNINITIALIZED;
}

void AVDecoderHandler::stop() {
    mDecoderState = STOP;
}

bool AVDecoderHandler::isDecoderRunning() const {
    return mDecodeThreadRunning;
}

double AVDecoderHandler::getVideoFrame(void** frameData) const {
	bool decoder_null = mIDecoder == nullptr;
	bool decoder_disable = !mIDecoder->getVideoInfo().isEnabled;
	bool decoder_seek = mDecoderState == SEEK;
	if (decoder_null || decoder_disable || decoder_seek) {
		LOG_ERROR_VERBOSE("[AVDecoderHandler] Video is not available: ");
		LOG_ERROR_VERBOSE("[AVDecoderHandler] decoder_null: ", decoder_null);
		LOG_ERROR_VERBOSE("[AVDecoderHandler] decoder_disable: ", decoder_disable);
		LOG_ERROR_VERBOSE("[AVDecoderHandler] decoder_seek: ", decoder_seek);
		*frameData = nullptr;
		return -1;
	}

	return mIDecoder->getVideoFrame(frameData);
}

double AVDecoderHandler::getAudioFrame(unsigned char** outputFrame, int& frameSize, int& nb_channel, size_t& byte_per_sample) const {
	bool decoder_null = mIDecoder == nullptr;
	bool decoder_disable = !mIDecoder->getAudioInfo().isEnabled;
	bool decoder_seek = mDecoderState == SEEK;
	if (decoder_null || decoder_disable || decoder_seek) {
		LOG_ERROR_VERBOSE("[AVDecoderHandler] Audio is not available. ");
		LOG_ERROR_VERBOSE("[AVDecoderHandler] decoder_null: ", decoder_null);
		LOG_ERROR_VERBOSE("[AVDecoderHandler] decoder_disable: ", decoder_disable);
		LOG_ERROR_VERBOSE("[AVDecoderHandler] decoder_seek: ", decoder_seek);
		*outputFrame = nullptr;
		return -1;
	}
	
	return mIDecoder->getAudioFrame(outputFrame, frameSize, nb_channel, byte_per_sample);
}

bool AVDecoderHandler::getOtherIndex(MediaType type, int* li, int& count, int& current) const {
	switch (type)
	{
	case AVDecoderHandler::VIDEO:
		{
			IDecoder::VideoInfo info = getVideoInfo();
			count = info.otherIndexCount;
			current = info.currentIndex;
			memcpy(li, info.otherIndex, count * sizeof(int));
			//li = info.otherIndex;
		} return true;
	case AVDecoderHandler::AUDIO:
		{
			IDecoder::AudioInfo info = getAudioInfo();
			count = info.otherIndexCount;
			current = info.currentIndex;
			memcpy(li, info.otherIndex, count * sizeof(int));
			//li = info.otherIndex;
		} return true;
	case AVDecoderHandler::SUBTITLE:
		{
			IDecoder::SubtitleInfo info = getSubtitleInfo();
			count = info.otherIndexCount;
			current = info.currentIndex;
			memcpy(li, info.otherIndex, count * sizeof(int));
			//li = info.otherIndex;
		} return true;
	}

	return false;
}

void AVDecoderHandler::freeVideoFrame() const {
	if (mIDecoder == nullptr || !mIDecoder->getVideoInfo().isEnabled || mDecoderState == SEEK) {
		LOG("[AVDecoderHandler] Video is not available.");
		return;
	}

	mIDecoder->freeVideoFrame();
}

void AVDecoderHandler::freeAudioFrame() const {
	if (mIDecoder == nullptr || !mIDecoder->getAudioInfo().isEnabled || mDecoderState == SEEK) {
		LOG("[AVDecoderHandler] Audio is not available.");
		return;
	}

	mIDecoder->freeAudioFrame();
}

void AVDecoderHandler::startDecoding() {
	if (mIDecoder == nullptr || mDecoderState != INITIALIZED) {
		LOG("[AVDecoderHandler] Not initialized, decode thread would not start.");
		return;
	}

	mDecodeThread = std::thread([&]() {
    mDecodeThreadRunning = true;
		if (!(mIDecoder->getVideoInfo().isEnabled || mIDecoder->getAudioInfo().isEnabled)) {
			LOG("[AVDecoderHandler] No stream enabled.");
			LOG("[AVDecoderHandler] Decode thread would not start.");
			return;
		}

		mDecoderState = DECODING;
		while (mDecoderState != STOP) {
			switch (mDecoderState) {
				default:
					break;
			case DECODING:
				if (!mIDecoder->decode()) {
					mDecoderState = DECODE_EOF;
				}
				break;
			case SEEK:
				mIDecoder->seek(mSeekTime);
				mDecoderState = DECODING;
				break;
			}
		}
        mDecodeThreadRunning = false;
	});
}

AVDecoderHandler::~AVDecoderHandler() {
	stopDecoding();
}

void AVDecoderHandler::setSeekTime(float sec) {
	if (mDecoderState < INITIALIZED || mDecoderState == SEEK) {
		LOG("[AVDecoderHandler] Seek unavaiable.");
		return;
	} 

	mSeekTime = sec;
	mDecoderState = SEEK;
}

IDecoder::VideoInfo AVDecoderHandler::getVideoInfo() const {
	return mIDecoder->getVideoInfo();
}

IDecoder::AudioInfo AVDecoderHandler::getAudioInfo() const {
	return mIDecoder->getAudioInfo();
}

IDecoder::SubtitleInfo AVDecoderHandler::getSubtitleInfo() const {
	return mIDecoder->getSubtitleInfo();
}

bool AVDecoderHandler::isVideoBufferEmpty() const {
	IDecoder::VideoInfo videoInfo = mIDecoder->getVideoInfo();
	IDecoder::BufferState EMPTY = IDecoder::BufferState::EMPTY;
	return videoInfo.isEnabled && videoInfo.bufferState == EMPTY;
}

bool AVDecoderHandler::isVideoBufferFull() const {
	IDecoder::VideoInfo videoInfo = mIDecoder->getVideoInfo();
	IDecoder::BufferState FULL = IDecoder::BufferState::FULL;
	return videoInfo.isEnabled && videoInfo.bufferState == FULL;
}

int AVDecoderHandler::getMetaData(char**& key, char**& value) const {
	if (mIDecoder == nullptr ||mDecoderState <= UNINITIALIZED) {
		return 0;
	}
	
	return mIDecoder->getMetaData(key, value);
}

void AVDecoderHandler::setVideoEnable(bool isEnable) const {
	if (mIDecoder == nullptr) {
		return;
	}

	mIDecoder->setVideoEnable(isEnable);
}

void AVDecoderHandler::setAudioEnable(bool isEnable) const {
	if (mIDecoder == nullptr) {
		return;
	}

	mIDecoder->setAudioEnable(isEnable);
}

void AVDecoderHandler::setAudioAllChDataEnable(bool isEnable) const {
	if (mIDecoder == nullptr) {
		return;
	}

	mIDecoder->setAudioAllChDataEnable(isEnable);
}
