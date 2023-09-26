//========= Copyright 2015-2019, HTC Corporation. All rights reserved. ===========

#include "AVHandler.h"
#include "DecoderFFmpeg.h"
#include "Logger.h"

AVHandler::AVHandler() {
	mDecoderState = UNINITIALIZED;
	mSeekTime = 0.0;
	mIDecoder = std::make_unique<DecoderFFmpeg>();
}

void AVHandler::init(const char* filePath) {
	if (mIDecoder == nullptr || !mIDecoder->init(filePath)) {
		mDecoderState = INIT_FAIL;
	} else {
		mDecoderState = INITIALIZED;
	}
}

AVHandler::DecoderState AVHandler::getDecoderState() {
	return mDecoderState;
}

void AVHandler::stopDecoding() {
	mDecoderState = STOP;
	if (mDecodeThread.joinable()) {
		mDecodeThread.join();
	}

	mIDecoder = nullptr;
	mDecoderState = UNINITIALIZED;
}

void AVHandler::stop() {
    mDecoderState = STOP;
}

bool AVHandler::isDecoderRunning() const {
    return mDecodeThreadRunning;
}

double AVHandler::getVideoFrame(void** frameData) {
	if (mIDecoder == nullptr || !mIDecoder->getVideoInfo().isEnabled || mDecoderState == SEEK) {
		LOG("Video is not available. \n");
		*frameData = nullptr;
		return -1;
	}

	return mIDecoder->getVideoFrame(frameData);
}

double AVHandler::getAudioFrame(uint8_t** outputFrame, int& frameSize) {
	if (mIDecoder == nullptr || !mIDecoder->getAudioInfo().isEnabled || mDecoderState == SEEK) {
		LOG("Audio is not available. \n");
		*outputFrame = nullptr;
		return -1;
	}
	
	return mIDecoder->getAudioFrame(outputFrame, frameSize);
}

void AVHandler::freeVideoFrame() {
	if (mIDecoder == nullptr || !mIDecoder->getVideoInfo().isEnabled || mDecoderState == SEEK) {
		LOG("Video is not available. \n");
		return;
	}

	mIDecoder->freeVideoFrame();
}

void AVHandler::freeAudioFrame() {
	if (mIDecoder == nullptr || !mIDecoder->getAudioInfo().isEnabled || mDecoderState == SEEK) {
		LOG("Audio is not available. \n");
		return;
	}

	mIDecoder->freeAudioFrame();
}

void AVHandler::startDecoding() {
	if (mIDecoder == nullptr || mDecoderState != INITIALIZED) {
		LOG("Not initialized, decode thread would not start. \n");
		return;
	}

	mDecodeThread = std::thread([&]() {
        mDecodeThreadRunning = true;
		if (!(mIDecoder->getVideoInfo().isEnabled || mIDecoder->getAudioInfo().isEnabled)) {
			LOG("No stream enabled. \n");
			LOG("Decode thread would not start. \n");
			return;
		}

		mDecoderState = DECODING;
		while (mDecoderState != STOP) {
			switch (mDecoderState) {
			case DECODING:
				if (!mIDecoder->decode()) {
					mDecoderState = DECODE_EOF;
				}
				break;
			case SEEK:
				mIDecoder->seek(mSeekTime);
				mDecoderState = DECODING;
				break;
			case DECODE_EOF:
				break;
			}
		}
        mDecodeThreadRunning = false;
	});
}

AVHandler::~AVHandler() {
	stopDecoding();
}

void AVHandler::setSeekTime(float sec) {
	if (mDecoderState < INITIALIZED || mDecoderState == SEEK) {
		LOG("Seek unavaiable.");
		return;
	} 

	mSeekTime = sec;
	mDecoderState = SEEK;
}

IDecoder::VideoInfo AVHandler::getVideoInfo() {
	return mIDecoder->getVideoInfo();
}

IDecoder::AudioInfo AVHandler::getAudioInfo() {
	return mIDecoder->getAudioInfo();
}

bool AVHandler::isVideoBufferEmpty() {
	IDecoder::VideoInfo videoInfo = mIDecoder->getVideoInfo();
	IDecoder::BufferState EMPTY = IDecoder::BufferState::EMPTY;
	return videoInfo.isEnabled && videoInfo.bufferState == EMPTY;
}

bool AVHandler::isVideoBufferFull() {
	IDecoder::VideoInfo videoInfo = mIDecoder->getVideoInfo();
	IDecoder::BufferState FULL = IDecoder::BufferState::FULL;
	return videoInfo.isEnabled && videoInfo.bufferState == FULL;
}

int AVHandler::getMetaData(char**& key, char**& value) {
	if (mIDecoder == nullptr ||mDecoderState <= UNINITIALIZED) {
		return 0;
	}
	
	return mIDecoder->getMetaData(key, value);
}

void AVHandler::setVideoEnable(bool isEnable) {
	if (mIDecoder == nullptr) {
		return;
	}

	mIDecoder->setVideoEnable(isEnable);
}

void AVHandler::setAudioEnable(bool isEnable) {
	if (mIDecoder == nullptr) {
		return;
	}

	mIDecoder->setAudioEnable(isEnable);
}

void AVHandler::setAudioAllChDataEnable(bool isEnable) {
	if (mIDecoder == nullptr) {
		return;
	}

	mIDecoder->setAudioAllChDataEnable(isEnable);
}