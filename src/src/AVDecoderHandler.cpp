//========= Copyright 2015-2019, HTC Corporation. All rights reserved. ===========

#include "AVDecoderHandler.h"
#include "DecoderFFmpeg.h"
#include "Logger.h"

AVDecoderHandler::AVDecoderHandler() {
	mDecoderState = UNINITIALIZED;
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

AVDecoderHandler::DecoderState AVDecoderHandler::getDecoderState() {
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

double AVDecoderHandler::getVideoFrame(void** frameData) {
	bool decoder_null = mIDecoder == nullptr;
	bool decoder_disable = !mIDecoder->getVideoInfo().isEnabled;
	bool decoder_seek = mDecoderState == SEEK;
	if (decoder_null || decoder_disable || decoder_seek) {
		LOG("Video is not available. %d %d %d \n", decoder_null, decoder_disable, decoder_seek);
		*frameData = nullptr;
		return -1;
	}

	return mIDecoder->getVideoFrame(frameData);
}

double AVDecoderHandler::getAudioFrame(unsigned char** outputFrame, int& frameSize, int& nb_channel, size_t& byte_per_sample) {
	bool decoder_null = mIDecoder == nullptr;
	bool decoder_disable = !mIDecoder->getAudioInfo().isEnabled;
	bool decoder_seek = mDecoderState == SEEK;
	if (decoder_null || decoder_disable || decoder_seek) {
		LOG("Audio is not available. %d %d %d \n", decoder_null, decoder_disable, decoder_seek);
		*outputFrame = nullptr;
		return -1;
	}
	
	return mIDecoder->getAudioFrame(outputFrame, frameSize, nb_channel, byte_per_sample);
}

void AVDecoderHandler::freeVideoFrame() {
	if (mIDecoder == nullptr || !mIDecoder->getVideoInfo().isEnabled || mDecoderState == SEEK) {
		LOG("Video is not available. \n");
		return;
	}

	mIDecoder->freeVideoFrame();
}

void AVDecoderHandler::freeAudioFrame() {
	if (mIDecoder == nullptr || !mIDecoder->getAudioInfo().isEnabled || mDecoderState == SEEK) {
		LOG("Audio is not available. \n");
		return;
	}

	mIDecoder->freeAudioFrame();
}

void AVDecoderHandler::startDecoding() {
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

AVDecoderHandler::~AVDecoderHandler() {
	stopDecoding();
}

void AVDecoderHandler::setSeekTime(float sec) {
	if (mDecoderState < INITIALIZED || mDecoderState == SEEK) {
		LOG("Seek unavaiable.");
		return;
	} 

	mSeekTime = sec;
	mDecoderState = SEEK;
}

IDecoder::VideoInfo AVDecoderHandler::getVideoInfo() {
	return mIDecoder->getVideoInfo();
}

IDecoder::AudioInfo AVDecoderHandler::getAudioInfo() {
	return mIDecoder->getAudioInfo();
}

bool AVDecoderHandler::isVideoBufferEmpty() {
	IDecoder::VideoInfo videoInfo = mIDecoder->getVideoInfo();
	IDecoder::BufferState EMPTY = IDecoder::BufferState::EMPTY;
	return videoInfo.isEnabled && videoInfo.bufferState == EMPTY;
}

bool AVDecoderHandler::isVideoBufferFull() {
	IDecoder::VideoInfo videoInfo = mIDecoder->getVideoInfo();
	IDecoder::BufferState FULL = IDecoder::BufferState::FULL;
	return videoInfo.isEnabled && videoInfo.bufferState == FULL;
}

int AVDecoderHandler::getMetaData(char**& key, char**& value) {
	if (mIDecoder == nullptr ||mDecoderState <= UNINITIALIZED) {
		return 0;
	}
	
	return mIDecoder->getMetaData(key, value);
}

void AVDecoderHandler::setVideoEnable(bool isEnable) {
	if (mIDecoder == nullptr) {
		return;
	}

	mIDecoder->setVideoEnable(isEnable);
}

void AVDecoderHandler::setAudioEnable(bool isEnable) {
	if (mIDecoder == nullptr) {
		return;
	}

	mIDecoder->setAudioEnable(isEnable);
}

void AVDecoderHandler::setAudioAllChDataEnable(bool isEnable) {
	if (mIDecoder == nullptr) {
		return;
	}

	mIDecoder->setAudioAllChDataEnable(isEnable);
}