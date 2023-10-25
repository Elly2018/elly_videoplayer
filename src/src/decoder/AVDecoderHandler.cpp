#include "AVDecoderHandler.h"
#include "DecoderFFmpeg.h"
#include <gd/Logger.h>
#include <Windows.h>

AVDecoderHandler::AVDecoderHandler() {
	mDecoderState = UNINITIALIZED;
	mBufferState = NONE;
	mSeekTime = 0.0;
	mIDecoder = std::make_unique<DecoderFFmpeg>();
}

void AVDecoderHandler::init(const char* filePath) {
	bool init_r = mIDecoder->init(filePath);
	if (mIDecoder == nullptr || !init_r) {
		mDecoderState = INIT_FAIL;
		LOG("Init result: INIT_FAIL");
	} else {
		mDecoderState = INITIALIZED;
		LOG("Init result: INITIALIZED");
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
	mBufferState = NONE;
	if (mBufferThread.joinable()) {
		mBufferThread.join();
	}

	freeAllPreloadFrame();
	mIDecoder = nullptr;
	mDecoderState = UNINITIALIZED;
}

void AVDecoderHandler::stop() {
    mDecoderState = STOP;
	mBufferState = NONE;
}

bool AVDecoderHandler::isDecoderRunning() const {
    return mDecodeThreadRunning;
}

bool AVDecoderHandler::isPreloadRunning() const {
	return mBufferThreadRunning;
}

double AVDecoderHandler::getVideoFrame(void** frameData, int& width, int& height) {
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

	return mIDecoder->getVideoFrame(frameData, width, height);
}

double AVDecoderHandler::getNextVideoFrameTime() {
	bool decoder_null = mIDecoder == nullptr;
	bool decoder_disable = !mIDecoder->getVideoInfo().isEnabled;
	bool decoder_seek = mDecoderState == SEEK;
	if (decoder_null || decoder_disable || decoder_seek) {
		LOG_ERROR_VERBOSE("[AVDecoderHandler] Video is not available: ");
		LOG_ERROR_VERBOSE("[AVDecoderHandler] decoder_null: ", decoder_null);
		LOG_ERROR_VERBOSE("[AVDecoderHandler] decoder_disable: ", decoder_disable);
		LOG_ERROR_VERBOSE("[AVDecoderHandler] decoder_seek: ", decoder_seek);
		return -1;
	}

	return mIDecoder->getNextVideoFrameTime();
}

double AVDecoderHandler::getAudioFrame(uint8_t** outputFrame, int& frameSize, int& nb_channel, size_t& byte_per_sample) {
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

double AVDecoderHandler::getNextAudioFrameTime() {
	bool decoder_null = mIDecoder == nullptr;
	bool decoder_disable = !mIDecoder->getAudioInfo().isEnabled;
	bool decoder_seek = mDecoderState == SEEK;
	if (decoder_null || decoder_disable || decoder_seek) {
		LOG_ERROR_VERBOSE("[AVDecoderHandler] Audio is not available: ");
		LOG_ERROR_VERBOSE("[AVDecoderHandler] decoder_null: ", decoder_null);
		LOG_ERROR_VERBOSE("[AVDecoderHandler] decoder_disable: ", decoder_disable);
		LOG_ERROR_VERBOSE("[AVDecoderHandler] decoder_seek: ", decoder_seek);
		return -1;
	}

	return mIDecoder->getNextAudioFrameTime();
}

bool AVDecoderHandler::getOtherIndex(MediaType type, int* li, int& count, int& current)
{
	switch (type)
	{
	case AVDecoderHandler::VIDEO:
		{
			IDecoder::VideoInfo info = getVideoInfo();
			count = info.otherIndexCount;
			current = info.currentIndex;
			//memcpy(li, info.otherIndex, count * sizeof(int));
			li = info.otherIndex;
		} return true;
	case AVDecoderHandler::AUDIO:
		{
			IDecoder::AudioInfo info = getAudioInfo();
			count = info.otherIndexCount;
			current = info.currentIndex;
			//memcpy(li, info.otherIndex, count * sizeof(int));
			li = info.otherIndex;
		} return true;
	case AVDecoderHandler::SUBTITLE:
		{
			IDecoder::SubtitleInfo info = getSubtitleInfo();
			count = info.otherIndexCount;
			current = info.currentIndex;
			//memcpy(li, info.otherIndex, count * sizeof(int));
			li = info.otherIndex;
		} return true;
	}

	return false;
}

void AVDecoderHandler::freeVideoFrame() {
	if (mIDecoder == nullptr || !mIDecoder->getVideoInfo().isEnabled || mDecoderState == SEEK) {
		LOG("[AVDecoderHandler] Video is not available.");
		return;
	}

	mIDecoder->freeVideoFrame();
}

void AVDecoderHandler::freeAudioFrame() {
	if (mIDecoder == nullptr || !mIDecoder->getAudioInfo().isEnabled || mDecoderState == SEEK) {
		LOG("[AVDecoderHandler] Audio is not available.");
		return;
	}

	mIDecoder->freeAudioFrame();
}

void AVDecoderHandler::freeAllPreloadFrame()
{
	if (mIDecoder == nullptr) return;
	mIDecoder->freePreloadFrame();
}

void AVDecoderHandler::freeAllBufferFrame()
{
	if (mIDecoder == nullptr) return;
	mIDecoder->freeBufferFrame();
}

void AVDecoderHandler::startDecoding() {
	if (mIDecoder == nullptr || mDecoderState != INITIALIZED) {
		LOG("[AVDecoderHandler] Not initialized, decode thread would not start.");
		return;
	}

	mBufferThread = std::thread([&]() {
		LOG("[AVDecoderHandler] Buffer thread start running.");
		mBufferThreadRunning = true;
		if (!(mIDecoder->getVideoInfo().isEnabled || mIDecoder->getAudioInfo().isEnabled)) {
			LOG("[AVDecoderHandler] No stream enabled.");
			LOG("[AVDecoderHandler] Decode thread would not start.");
			return;
		}
		mBufferState = LOADING;
		while (mBufferState != NONE) {
			switch (mBufferState) {
			case LOADING:
				if (mDecoderState == SEEK) continue;
				if (!mIDecoder->buffering()) {
					mBufferState = NONE;
				}
				break;
			case NONE:
				break;
			}
		}
		mBufferThreadRunning = false;
	});

	mDecodeThread = std::thread([&]() {
		LOG("[AVDecoderHandler] Decode thread start running.");
		mDecodeThreadRunning = true;
		if (!(mIDecoder->getVideoInfo().isEnabled || mIDecoder->getAudioInfo().isEnabled)) {
			LOG("[AVDecoderHandler] No stream enabled.");
			LOG("[AVDecoderHandler] Decode thread would not start.");
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
		LOG("[AVDecoderHandler] Seek unavaiable.");
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

IDecoder::SubtitleInfo AVDecoderHandler::getSubtitleInfo() {
	return mIDecoder->getSubtitleInfo();
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