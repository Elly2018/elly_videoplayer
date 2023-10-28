#include <Logger.h>
#include "FFmpegMediaPlayer.h"

void FFmpegMediaPlayer::RegisterAudioFormatCallback(SubmitAudioFormat func)
{
	audioFormatCallback.push_back(func);
}

void FFmpegMediaPlayer::CleanAudioFormatCallback()
{
	audioFormatCallback.clear();
}

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
