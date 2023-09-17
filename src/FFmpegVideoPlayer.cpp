#include "FFmpegVideoPlayer.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "ffmpeg/fftools/ffmpeg.h"

using namespace godot;

void FFmpegVideoPlayer::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("hello_node"), &FFmpegVideoPlayer::hello_node);
}

FFmpegVideoPlayer::FFmpegVideoPlayer()
{
}

FFmpegVideoPlayer::~FFmpegVideoPlayer()
{
}

// Override built-in methods with your own logic. Make sure to declare them in the header as well!

void FFmpegVideoPlayer::_ready()
{
}

void FFmpegVideoPlayer::_process(double delta)
{
}

void FFmpegVideoPlayer::hello_node()
{
	UtilityFunctions::print("Hello GDExtension Node!");
}
