#include "FFmpegMediaEncoder.h"

using namespace godot;

void FFmpegMediaEncoder::_bind_methods()
{

}

void FFmpegMediaEncoder::register_camera(Ref<CameraTexture> tex)
{
	target = tex;
}

void FFmpegMediaEncoder::start()
{

}

void FFmpegMediaEncoder::set_pause(bool pau)
{
	is_pause = pau;
}

bool FFmpegMediaEncoder::get_pause()
{
	return is_pause;
}

void FFmpegMediaEncoder::stop()
{

}

FFmpegMediaEncoder::FFmpegMediaEncoder() 
{
	
}

FFmpegMediaEncoder::~FFmpegMediaEncoder() 
{

}