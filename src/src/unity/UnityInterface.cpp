#include "UnityInterface.h"
#include <Logger.h>
#include <unity/FFmpegMediaPlayer.h>
#include <memory>

static IUnityInterfaces* s_UnityInterfaces = 0;
static IUnityGraphics* s_Graphics = 0;

std::vector<std::shared_ptr<FFmpegMediaPlayer>> playerContexts;

bool getVideoContext(int id, std::shared_ptr<FFmpegMediaPlayer>& playerCtx) {
	for (int it = 0; it < playerContexts.size(); it++) {
		if ((playerContexts.at(it))->id == id) {
			playerCtx = playerContexts.at(it);
			return true;
		}
	}

	LOG("[UnityInterface] Player does not exist.");
	return false;
}
int getVideoContextIndex(std::shared_ptr<FFmpegMediaPlayer> playerCtx) 
{
	for (int it = 0; it < playerContexts.size(); it++) {
		if ((playerContexts.at(it))->id == playerCtx->id) {
			return it;
		}
	}
	return -1;
}

int interfaceCreatePlayer() 
{
	int newID = 0;
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	while (getVideoContext(newID, playerCtx)) { newID++; }
	bool haveEmpty = getVideoContext(-1, playerCtx);
	if (!haveEmpty) {
		playerCtx = std::make_shared<FFmpegMediaPlayer>();
	}
	playerCtx->id = newID;
	playerContexts.push_back(playerCtx);
	LOG("[UnityInterface] Player create: ", newID);
	return newID;
}
void interfaceUpdate(int id)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	playerCtx->_Update();
}
void interfaceFixedUpdate(int id)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	playerCtx->_FixedUpdate();
}
int interfaceGetPlayerState(int id)
{
	return -1;
}
void interfaceDestroyPlayer(int id)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	int index = getVideoContextIndex(playerCtx);
	playerContexts.erase(playerContexts.begin() + index);
	playerCtx.reset();
	LOG("[UnityInterface] Player destroy: ", id);
}
void interfaceAudioSampleCallback(int id, SubmitAudioSample func)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	playerCtx->RegisterAudioCallback(func);
}
void interfaceAudioSampleCallback_Clean(int id)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	playerCtx->CleanAudioCallback();
}
void interfaceAudioFormatCallback(int id, SubmitAudioFormat func)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	playerCtx->RegisterAudioFormatCallback(func);
}

void interfaceAudioFormatCallback_Clean(int id)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	playerCtx->CleanAudioFormatCallback();
}
void interfaceVideoSampleCallback(int id, SubmitVideoSample func)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	playerCtx->RegisterVideoCallback(func);
}
void interfaceVideoSampleCallback_Clean(int id)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	playerCtx->CleanVideoCallback();
}
void interfaceVideoFormatCallback(int id, SubmitVideoFormat func)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	playerCtx->RegisterVideoFormatCallback(func);
}
void interfaceVideoFormatCallback_Clean(int id)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	playerCtx->CleanVideoFormatCallback();
}
void interfaceStateChangedCallback(int id, StateChanged func)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	playerCtx->RegisterStateChangedCallback(func);
}
void interfaceStateChangedCallback_Clean(int id)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	playerCtx->CleanStateChangedCallback();
}
void interfaceGlobalTimeCallback(int id, GetGlobalTime func)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	playerCtx->RegisterGlobalTimeCallback(func);
}
void interfaceAsyncLoadCallback(int id, AsyncLoad func)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	playerCtx->RegisterAsyncLoadCallback(func);
}
void interfaceAudioBufferCountCallback(int id, AudioBufferCount func)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	playerCtx->RegisterAudioBufferCountCallback(func);
}
void interfaceAudioControlCallback(int id, AudioControl func)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	playerCtx->RegisterAudioControlCallback(func);
}
void interfacePlay(int id)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	playerCtx->play();
}
void interfacePause(int id, bool pause)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	playerCtx->set_paused(pause);
}
void interfaceStop(int id)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	playerCtx->stop();
}
void interfaceSeek(int id, double time)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	playerCtx->seek(time);
}
void interfaceLoad(int id)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	playerCtx->load();
}
void interfaceLoadAsync(int id)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	playerCtx->load_async();
}
double interfaceGetCurrentTime(int id)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return -1.0; }
	return playerCtx->get_playback_position();
}
double interfaceMediaLength(int id)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return -1.0; }
	return playerCtx->get_length();
}
void interfaceLoadPath(int id, const char* path)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	playerCtx->load_path(path);
}
void interfaceLoadPathAsync(int id, const char* path)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	playerCtx->load_path_async(path);
}

void interfaceSetPath(int id, const char* path)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	playerCtx->set_path(path);
}

const char* interfaceGetPath(int id)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return nullptr; }
	return playerCtx->get_path();
}

void interfaceSetFormat(int id, const char* format)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	playerCtx->set_format(format);
}

const char* interfaceGetFormat(int id)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return nullptr; }
	return playerCtx->get_format();
}
