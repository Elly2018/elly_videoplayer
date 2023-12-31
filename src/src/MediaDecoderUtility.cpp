﻿//========= Copyright 2015-2019, HTC Corporation. All rights reserved. ===========

#include "MediaDecoderUtility.h"
#include "AVDecoderHandler.h"
#include "Logger.h"
#include <stdio.h>
#include <string>
#include <memory>
#include <thread>
#include <list>
#include <cstring>

#ifndef WIN32
#define strcpy_s(src, size, dest) (strncpy((dest), (src), (size)))
#endif

typedef struct _VideoContext {
	int id = -1;
	std::string path = "";
  std::thread initThread;
  bool initThreadRunning = false;
  bool destroying = false;
  std::unique_ptr<AVDecoderHandler> avhandler = nullptr;
	float progressTime = 0.0f;
	float lastUpdateTime = -1.0f;
  bool videoFrameLocked = false;
	bool audioFrameLocked = false;
	bool isContentReady = false;	//	This flag is used to indicate the period that seek over until first data is got.
									//	Usually used for AV sync problem, in pure audio case, it should be discard.
} VideoContext;

std::list<std::shared_ptr<VideoContext>> videoContexts;
typedef std::list<std::shared_ptr<VideoContext>>::iterator VideoContextIter;

bool getVideoContext(int id, std::shared_ptr<VideoContext>& videoCtx) {
	for (VideoContextIter it = videoContexts.begin(); it != videoContexts.end(); it++) {
		if ((*it)->id == id) {
			videoCtx = *it;
			return true;
		}
	}

	LOG("[ViveMediaDecoder] Decoder does not exist.");
	return false;
}

void removeVideoContext(int id) {
	for (VideoContextIter it = videoContexts.begin(); it != videoContexts.end(); it++) {
		if ((*it)->id == id) {
			videoContexts.erase(it);
			return;
		}
	}
}

void nativeCleanAll() {
    std::list<int> idList;
    for(auto videoCtx : videoContexts) {
        idList.push_back(videoCtx->id);
    }

    for(int id : idList) {
        nativeDestroyDecoder(id);
    }
}

void nativeCleanDestroyedDecoders() {
    std::list<int> idList;
    for(auto videoCtx : videoContexts) {
        if (videoCtx->destroying && !videoCtx->avhandler->isDecoderRunning() && !videoCtx->initThreadRunning) {
            idList.push_back(videoCtx->id);
        }
    }

    for(int id : idList) {
        nativeDestroyDecoder(id);
    }
}

int nativeCreateDecoderAsync(const char* filePath, int& id) {
	LOG("[ViveMediaDecoder] Query available decoder id.");

	int newID = 0;
    std::shared_ptr<VideoContext> videoCtx;
	while (getVideoContext(newID, videoCtx)) { newID++; }

	videoCtx = std::make_shared<VideoContext>();
	videoCtx->avhandler = std::make_unique<AVDecoderHandler>();
	videoCtx->id = newID;
	id = videoCtx->id;
	videoCtx->path = std::string(filePath);
	videoCtx->isContentReady = false;

    videoCtx->initThreadRunning = true;
	videoCtx->initThread = std::thread([videoCtx]() {
		videoCtx->avhandler->init(videoCtx->path.c_str());
        videoCtx->initThreadRunning = false;
	});

	videoContexts.push_back(videoCtx);

	return 0;
}

//	Synchronized init. Used for thumbnail currently.
int nativeCreateDecoder(const char* filePath, int& id) {
	LOG("[ViveMediaDecoder] Query available decoder id.");

	int newID = 0;
    std::shared_ptr<VideoContext> videoCtx;
	while (getVideoContext(newID, videoCtx)) { newID++; }

    videoCtx = std::make_shared<VideoContext>();
    videoCtx->avhandler = std::make_unique<AVDecoderHandler>();
    videoCtx->id = newID;
    id = videoCtx->id;
    videoCtx->path = std::string(filePath);
    videoCtx->isContentReady = false;
	videoCtx->avhandler->init(filePath);

	videoContexts.push_back(videoCtx);

	return 0;
}

int nativeGetDecoderState(int id) {
    std::shared_ptr<VideoContext> videoCtx;
	if (!getVideoContext(id, videoCtx) || videoCtx->avhandler == nullptr) { return -1; }
		
	return videoCtx->avhandler->getDecoderState();
}

bool nativeStartDecoding(int id) {
    std::shared_ptr<VideoContext> videoCtx;
	if (!getVideoContext(id, videoCtx) || videoCtx->avhandler == nullptr) { return false; }

	if (videoCtx->initThread.joinable()) {
		videoCtx->initThread.join();
	}

	AVDecoderHandler* avhandler = videoCtx->avhandler.get();
	if (avhandler->getDecoderState() >= AVDecoderHandler::DecoderState::INITIALIZED) {
		avhandler->startDecoding();
	}

	if (!avhandler->getVideoInfo().isEnabled) {
		videoCtx->isContentReady = true;
	}

	return true;
}

void nativeScheduleDestroyDecoder(int id) {
    std::shared_ptr<VideoContext> videoCtx;
    if (!getVideoContext(id, videoCtx)) { return; }
    videoCtx->avhandler->stop(); // Async
    videoCtx->destroying = true;
}

void nativeDestroyDecoder(int id) {
  std::shared_ptr<VideoContext> videoCtx;
	if (!getVideoContext(id, videoCtx)) { return; }

	LOG("[ViveMediaDecoder] Destroy decoder.");
	if (videoCtx->initThread.joinable()) {
		videoCtx->initThread.join();
	}
	LOG("[ViveMediaDecoder] Destroy decoder (After join).");

	videoCtx->avhandler.reset();
	LOG("[ViveMediaDecoder] Destroy decoder (After reset).");

	videoCtx->path.clear();
	videoCtx->progressTime = 0.0f;
	videoCtx->lastUpdateTime = 0.0f;

	videoCtx->isContentReady = false;
	removeVideoContext(videoCtx->id);
	LOG("[ViveMediaDecoder] Destroy decoder (After remove video decoder).");
	videoCtx->id = -1;
}

bool nativeGetOtherStreamIndex(int id, int type, int* li, int& count, int& current) {
	std::shared_ptr<VideoContext> videoCtx;
	if (!getVideoContext(id, videoCtx) || videoCtx->avhandler == nullptr) { return false; }

	return videoCtx->avhandler->getOtherIndex((AVDecoderHandler::MediaType)type, li, count, current);
}

//	Video
bool nativeIsVideoEnabled(int id) {
    std::shared_ptr<VideoContext> videoCtx;
	if (!getVideoContext(id, videoCtx)) { return false; }

	if (videoCtx->avhandler->getDecoderState() < AVDecoderHandler::DecoderState::INITIALIZED) {
		LOG("[ViveMediaDecoder] Decoder is unavailable currently.");
		return false;
	}

	bool ret = videoCtx->avhandler->getVideoInfo().isEnabled;
	LOG("[ViveMediaDecoder] nativeIsVideoEnabled: ", ret ? "true" : "false");
	return ret;
}

void nativeGetVideoFormat(int id, int& width, int& height, float& framerate, float& totalTime) {
    std::shared_ptr<VideoContext> videoCtx;
	if (!getVideoContext(id, videoCtx)) { return; }

	if (videoCtx->avhandler->getDecoderState() < AVDecoderHandler::DecoderState::INITIALIZED) {
		LOG("[ViveMediaDecoder] Decoder is unavailable currently.");
		return;
	}

	IDecoder::VideoInfo videoInfo = videoCtx->avhandler->getVideoInfo();
	width = videoInfo.width;
	height = videoInfo.height;
	framerate = videoInfo.framerate;
	totalTime = (float)(videoInfo.totalTime);
}

void nativeSetVideoTime(int id, float currentTime) {
    std::shared_ptr<VideoContext> videoCtx;
	if (!getVideoContext(id, videoCtx)) { return; }

	videoCtx->progressTime = currentTime;
}

bool nativeIsAudioEnabled(int id) {
    std::shared_ptr<VideoContext> videoCtx;
	if (!getVideoContext(id, videoCtx)) { return false; }

	if (videoCtx->avhandler->getDecoderState() < AVDecoderHandler::DecoderState::INITIALIZED) {
		LOG("[ViveMediaDecoder] Decoder is unavailable currently.");
		return false;
	}

	bool ret = videoCtx->avhandler->getAudioInfo().isEnabled;
	LOG("[ViveMediaDecoder] nativeIsAudioEnabled: ", ret ? "true" : "false");
	return ret;
}

void nativeGetAudioFormat(int id, int& channel, int& sampleRate, float& totalTime) {
    std::shared_ptr<VideoContext> videoCtx;
	if (!getVideoContext(id, videoCtx)) { return; }

	if (videoCtx->avhandler->getDecoderState() < AVDecoderHandler::DecoderState::INITIALIZED) {
		LOG("[ViveMediaDecoder] Decoder is unavailable currently. ");
		return;
	}

	IDecoder::AudioInfo audioInfo = videoCtx->avhandler->getAudioInfo();
	channel = audioInfo.channels;
	sampleRate = audioInfo.sampleRate;
	totalTime = (float)(audioInfo.totalTime);
}

float nativeGetAudioData(int id, unsigned char** audioData, int& frameSize, int& nb_channel, size_t& byte_per_sample) {
  std::shared_ptr<VideoContext> videoCtx;
	if (!getVideoContext(id, videoCtx)) { return -1.0f; }
	if (videoCtx->audioFrameLocked) {
		LOG("[ViveMediaDecoder] Release last audio frame first");
		return -1.0f;
	}

	AVDecoderHandler* localAVDecoderHandler = videoCtx->avhandler.get();
	double curFrameTime = (localAVDecoderHandler->getAudioFrame(audioData, frameSize, nb_channel, byte_per_sample));
	return curFrameTime;
}

void nativeFreeAudioData(int id) {
    std::shared_ptr<VideoContext> videoCtx;
	if (!getVideoContext(id, videoCtx)) { return; }
	//videoCtx->audioFrameLocked = false;
	videoCtx->avhandler->freeAudioFrame();
}

void nativeSetSeekTime(int id, float sec) {
    std::shared_ptr<VideoContext> videoCtx;
	if (!getVideoContext(id, videoCtx)) { return; }

	if (videoCtx->avhandler->getDecoderState() < AVDecoderHandler::DecoderState::INITIALIZED) {
		LOG("[ViveMediaDecoder] Decoder is unavailable currently. ");
		return;
	}

	LOG("[ViveMediaDecoder] nativeSetSeekTime: ", sec);
	videoCtx->avhandler->setSeekTime(sec);
	if (!videoCtx->avhandler->getVideoInfo().isEnabled) {
		videoCtx->isContentReady = true;
	} else {
		videoCtx->isContentReady = false;
	}
}

bool nativeIsSeekOver(int id) {
    std::shared_ptr<VideoContext> videoCtx;
	if (!getVideoContext(id, videoCtx)) { return false; }
	
	return !(videoCtx->avhandler->getDecoderState() == AVDecoderHandler::DecoderState::SEEK);
}

bool nativeIsVideoBufferFull(int id) {
    std::shared_ptr<VideoContext> videoCtx;
	if (!getVideoContext(id, videoCtx)) { return false; }

	return videoCtx->avhandler->isVideoBufferFull();
}

bool nativeIsVideoBufferEmpty(int id) {
    std::shared_ptr<VideoContext> videoCtx;
	if (!getVideoContext(id, videoCtx)) { return false; }
	
	return videoCtx->avhandler->isVideoBufferEmpty();
}

int nativeGetMetaData(const char* filePath, char*** key, char*** value) {
    std::unique_ptr<AVDecoderHandler> avHandler = std::make_unique<AVDecoderHandler>();
	avHandler->init(filePath);

	char** metaKey = nullptr;
	char** metaValue = nullptr;
	int metaCount = avHandler->getMetaData(metaKey, metaValue);

	*key = (char**)malloc(sizeof(char*) * metaCount);
	*value = (char**)malloc(sizeof(char*) * metaCount);

	for (int i = 0; i < metaCount; i++) {
		(*key)[i] = (char*)malloc(strlen(metaKey[i]) + 1);
		(*value)[i] = (char*)malloc(strlen(metaValue[i]) + 1);
		strcpy_s((*key)[i], strlen(metaKey[i]) + 1, metaKey[i]);
		strcpy_s((*value)[i], strlen(metaValue[i]) + 1, metaValue[i]);
	}

	free(metaKey);
	free(metaValue);

	return metaCount;
}

bool nativeIsContentReady(int id) {
    std::shared_ptr<VideoContext> videoCtx;
	if (!getVideoContext(id, videoCtx)) { return false; }

	return videoCtx->isContentReady;
}

void nativeSetVideoEnable(int id, bool isEnable) {
    std::shared_ptr<VideoContext> videoCtx;
	if (!getVideoContext(id, videoCtx)) { return; }

	videoCtx->avhandler->setVideoEnable(isEnable);
}

void nativeSetAudioEnable(int id, bool isEnable) {
    std::shared_ptr<VideoContext> videoCtx;
	if (!getVideoContext(id, videoCtx)) { return; }

	videoCtx->avhandler->setAudioEnable(isEnable);
}

void nativeSetAudioAllChDataEnable(int id, bool isEnable) {
    std::shared_ptr<VideoContext> videoCtx;
	if (!getVideoContext(id, videoCtx)) { return; }

	videoCtx->avhandler->setAudioAllChDataEnable(isEnable);
}

void nativeGrabVideoFrame(int id, void** frameData, bool& frameReady) {
    frameReady = false;
    std::shared_ptr<VideoContext> videoCtx;
    if (!getVideoContext(id, videoCtx) || videoCtx->avhandler == nullptr) { return; }
    if (videoCtx->videoFrameLocked) {
        LOG_VERBOSE("[ViveMediaDecoder] Release last video frame first");
        return;
    }

	AVDecoderHandler* localAVDecoderHandler = videoCtx->avhandler.get();

  if (localAVDecoderHandler != nullptr && 
	localAVDecoderHandler->getDecoderState() >= AVDecoderHandler::DecoderState::INITIALIZED && 
	localAVDecoderHandler->getVideoInfo().isEnabled) {
      double videoDecCurTime = localAVDecoderHandler->getVideoInfo().lastTime;
      if (videoDecCurTime <= videoCtx->progressTime) {

          double curFrameTime = localAVDecoderHandler->getVideoFrame(frameData);
          if (frameData != nullptr && 
			curFrameTime != -1 && 
			videoCtx->lastUpdateTime != curFrameTime) {
              frameReady = true;
              videoCtx->lastUpdateTime = (float)curFrameTime;
              videoCtx->isContentReady = true;
              videoCtx->videoFrameLocked = true;
          }
      }
  }
}

void nativeReleaseVideoFrame(int id) {
    std::shared_ptr<VideoContext> videoCtx;
    if (!getVideoContext(id, videoCtx) || videoCtx->avhandler == nullptr) { return; }
    videoCtx->avhandler->freeVideoFrame();
    videoCtx->videoFrameLocked = false;
}

bool nativeIsEOF(int id) {
    std::shared_ptr<VideoContext> videoCtx;
	if (!getVideoContext(id, videoCtx) || videoCtx->avhandler == nullptr) { return true; }

	return videoCtx->avhandler->getDecoderState() == AVDecoderHandler::DecoderState::DECODE_EOF;
}