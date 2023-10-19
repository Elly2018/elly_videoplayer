#include "FFmpegMediaEncoder.h"
#include "Logger.h"

using namespace godot;

FFmpegMediaEncoder::FFmpegMediaEncoder()
{
	LOG("FFmpegMediaEncoder instance created.");
}

FFmpegMediaEncoder::~FFmpegMediaEncoder()
{
	LOG("FFmpegMediaEncoder instance destroy.");
}

void FFmpegMediaEncoder::push_image(Ref<Image> image)
{
	target = image;
}

void FFmpegMediaEncoder::push_audio(PackedFloat32Array data, int sample_count, int channel) 
{

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

void FFmpegMediaEncoder::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("push_image", "image"), &FFmpegMediaEncoder::push_image);
}