#include "UnityInterface.h"
#include <Logger.h>
#include <unity/FFmpegMediaPlayer.h>
#include <memory>

static IUnityInterfaces* s_UnityInterfaces = 0;
static IUnityGraphics* s_Graphics = 0;

std::list<std::shared_ptr<FFmpegMediaPlayer>> playerContexts;
typedef std::list<std::shared_ptr<FFmpegMediaPlayer>>::iterator PlayerContextIter;

bool getVideoContext(int id, std::shared_ptr<FFmpegMediaPlayer>& playerCtx) {
	for (PlayerContextIter it = playerContexts.begin(); it != playerContexts.end(); it++) {
		if ((*it)->id == id) {
			playerCtx = *it;
			return true;
		}
	}

	LOG("[ViveMediaDecoder] Decoder does not exist.");
	return false;
}

int interfaceCreatePlayer() 
{
  int newID = 0;
  std::shared_ptr<FFmpegMediaPlayer> playerCtx;
  while (getVideoContext(newID, playerCtx)) { newID++; }

	return newID;
}
int interfaceGetPlayerState(int id)
{
	return -1;
}
void interfaceDestroyPlayer(int id)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	playerCtx.reset();
	playerContexts.remove(playerCtx);
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
void interfacePlay(int id)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	playerCtx->set_paused(false);
}
void interfacePause(int id)
{
	std::shared_ptr<FFmpegMediaPlayer> playerCtx;
	if (!getVideoContext(id, playerCtx)) { return; }
	playerCtx->set_paused(true);
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
