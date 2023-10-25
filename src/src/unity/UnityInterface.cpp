#include "UnityInterface.h"
#include <Logger.h>
#include <unity/FFmpegMediaPlayer.h>
#include <memory>

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

}
void interfaceDestroyPlayer(int id)
{

}
void interfacePlay(int id)
{

}
void interfacePause(int id)
{

}
void interfaceStop(int id)
{

}
void interfaceSeek(int id, double time)
{

}
double interfaceGetCurrentTime(int id)
{

}
double interfaceMediaLength(int id)
{

}
void interfaceLoadPath(int id, const char* path)
{

}
void interfaceLoadPathAsync(int id, const char* path)
{

}