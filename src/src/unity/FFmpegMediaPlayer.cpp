#include <Logger.h>
#include "FFmpegMediaPlayer.h"

void FFmpegMediaPlayer::RegisterAudioCallback(SubmitAudioSample func)
{
	audioCallback.push_back(func);
}

void FFmpegMediaPlayer::CleanAudioCallback()
{
	audioCallback.clear();
}

FFmpegMediaPlayer::FFmpegMediaPlayer()
{
	audioCallback = std::list<SubmitAudioSample>();
}

FFmpegMediaPlayer::~FFmpegMediaPlayer()
{
}
