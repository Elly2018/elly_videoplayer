#include "FFmpegNode.h"
#include "Logger.h"

#include <cstring>

//#include <godot_cpp/classes/audio_stream_generator.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/class_db.hpp>
//#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void FFmpegNode::_init_media() {
	emit_signal("message", "start init media");
	video_playback = nativeIsVideoEnabled(id);
	if (video_playback) {
		first_frame = true;

		nativeGetVideoFormat(id, width, height, video_length);
		data_size = width * height * 3;
	}

	audio_playback = nativeIsAudioEnabled(id);
	if (audio_playback) {
		nativeGetAudioFormat(id, channels, frequency, audio_length);
	}

	state = INITIALIZED;
}

bool FFmpegNode::load_path(String path) {
	emit_signal("message", String("start load path: ") + path);
	int d_state = nativeGetDecoderState(id);
	if (d_state > 1) {
		emit_signal("error", String("Decoder state: ") + String(std::to_string(d_state).c_str()));
		return false;
	}

	CharString utf8 = path.utf8();
	const char *cstr = utf8.get_data();

	nativeCreateDecoder(cstr, id);

	bool is_loaded = nativeGetDecoderState(id) == 1;
	if (is_loaded) {
		_init_media();
	} else {
		emit_signal("message", String("State change to UNINITIALIZED"));
		state = UNINITIALIZED;
	}

	return is_loaded;
}

void FFmpegNode::load_path_async(String path) {
	emit_signal("message", String("start load path: ") + path);
	int d_state = nativeGetDecoderState(id);
	if (d_state > 1) {
		emit_signal("error", String("Decoder state: ") + String(std::to_string(d_state).c_str()));
		emit_signal("async_loaded", false);
		return;
	}

	CharString utf8 = path.utf8();
	const char *cstr = utf8.get_data();

	nativeCreateDecoderAsync(cstr, id);

	emit_signal("message", String("State change to LOADING"));
	state = LOADING;
}

void FFmpegNode::play() {
	if (state != INITIALIZED) {
		emit_signal("message", String("play func failed, because state is not INITIALIZED"));
		return;
	}

	if (paused) {
		paused = false;
	} else {
		nativeStartDecoding(id);
	}

	global_start_time = Time::get_singleton()->get_unix_time_from_system();

	state = DECODING;
}

void FFmpegNode::stop() {
	if (nativeGetDecoderState(id) != DECODING) {
		return;
	}

	nativeDestroyDecoder(id);

	video_current_time = 0.0f;
	audio_current_time = 0.0f;
	paused = false;

	state = INITIALIZED;
}

bool FFmpegNode::is_playing() const {
	return !paused && state == DECODING;
}

void FFmpegNode::set_paused(bool p_paused) {
	paused = p_paused;

	if (paused) {
		hang_time = Time::get_singleton()->get_unix_time_from_system() - global_start_time;
	} else {
		global_start_time = Time::get_singleton()->get_unix_time_from_system() - hang_time;
	}
}

bool FFmpegNode::is_paused() const {
	return paused;
}

Ref<ImageTexture> FFmpegNode::get_video_texture() {
	return texture;
}

float FFmpegNode::get_length() const {
	return video_playback && video_length > audio_length ? video_length : audio_length;
}

void FFmpegNode::set_loop(bool p_enable) {
	looping = p_enable;
}

bool FFmpegNode::has_loop() const {
	return looping;
}

float FFmpegNode::get_playback_position() const {
	return video_playback && video_current_time > audio_current_time ? video_current_time : audio_current_time;
}

void FFmpegNode::seek(float p_time) {
	if (state != DECODING && state != END_OF_FILE) {
		return;
	}

	if (p_time < 0.0f) {
		p_time = 0.0f;
	} else if ((video_playback && p_time > video_length) || (audio_playback && p_time > audio_length)) {
		p_time = video_length;
	}

	nativeSetSeekTime(id, p_time);
	nativeSetVideoTime(id, p_time);

	hang_time = p_time;

	state = SEEK;
}

void FFmpegNode::_process(float delta) {
	if (state > INITIALIZED && state != SEEK && state != END_OF_FILE) {
		// TODO: Implement audio.
		unsigned char *audio_data = nullptr;
		int audio_size = 0;
		double audio_time = nativeGetAudioData(id, &audio_data, audio_size);
		if (audio_time != -1.0f) {
			nativeFreeAudioData(id);
		}
	}

	switch (state) {
		case LOADING: {
			if (nativeGetDecoderState(id) == INITIALIZED) {
				_init_media();
				emit_signal("async_loaded", true);
			} else if (nativeGetDecoderState(id) == -1) {
				state = UNINITIALIZED;
				emit_signal("async_loaded", false);
			}
		} break;

		case BUFFERING: {
			if (nativeIsVideoBufferFull(id) || nativeIsEOF(id)) {
				global_start_time = Time::get_singleton()->get_unix_time_from_system() - hang_time;
				state = DECODING;
			}
		} break;

		case SEEK: {
			if (nativeIsSeekOver(id)) {
				global_start_time = Time::get_singleton()->get_unix_time_from_system() - hang_time;
				state = DECODING;
			}
		} break;

		case DECODING: {
			if (paused) {
				return;
			}

			if (video_playback) {
				void *frame_data = nullptr;
				bool frame_ready = false;

				nativeGrabVideoFrame(id, &frame_data, frame_ready);

				if (frame_ready) {
					PackedByteArray image_data;
					image_data.resize(data_size);
					memcpy(image_data.ptrw(), frame_data, data_size);
					image->create_from_data(width, height, false, Image::FORMAT_RGB8, image_data);

					if (first_frame) {
						texture->create_from_image(image);
						first_frame = false;
					} else {
						texture->update(image);
					}

					nativeReleaseVideoFrame(id);
				}

				video_current_time = Time::get_singleton()->get_unix_time_from_system() - global_start_time;

				if (video_current_time < video_length || video_length == -1.0f) {
					nativeSetVideoTime(id, video_current_time);
				} else {
					if (!nativeIsVideoBufferEmpty(id)) {
						nativeSetVideoTime(id, video_current_time);
					} else {
						state = END_OF_FILE;
					}
				}
			}

			if (nativeIsVideoBufferEmpty(id) && !nativeIsEOF(id)) {
				hang_time = Time::get_singleton()->get_unix_time_from_system() - global_start_time;
				state = BUFFERING;
			}
		} break;

		case END_OF_FILE: {
			if (looping) {
				state = DECODING;
				seek(0.0f);
			}
		} break;
	}
}

// TODO: Implement audio.

// void FFmpegNode::_physics_process(float delta) {
// 	if (!audio_playback || paused || state == UNINITIALIZED || state == EOF) {
// 		return;
// 	}
//
// 	unsigned char *frame_data = nullptr;
// 	int frame_length = 0;
// 	double audio_time = nativeGetAudioData(id, &frame_data, frame_length);
//
// 	if (audio_time > 0.0f) {
// 		if (state != SEEK && frame_length != 0 && frame_data != nullptr) { }
//
// 		nativeFreeAudioData(id);
// 	}
//
// 	if (state == DECODING && frame_data != nullptr && !player->is_playing()) {
// 		audio_current_time = Time::get_singleton()->get_unix_time_from_system() - global_start_time;
// 	}
// }

FFmpegNode::FFmpegNode() {
	texture = Ref<ImageTexture>(memnew(ImageTexture));
	image = Ref<Image>(memnew(Image()));
	auto temp = Logger::instance();
	// TODO: Implement audio.

// 	player = memnew(AudioStreamPlayer);
// 	add_child(player);
// 	Ref<AudioStreamGenerator> generator = Ref<AudioStreamGenerator>(memnew(AudioStreamGenerator));
// 	player->set_stream(generator);
// 	playback = player->get_stream_playback();
}

FFmpegNode::~FFmpegNode() {
	nativeScheduleDestroyDecoder(id);
}

void FFmpegNode::_notification(int p_what)
{
}

void FFmpegNode::_bind_methods() {
	ClassDB::bind_method(D_METHOD("load_path", "path"), &FFmpegNode::load_path);
	ClassDB::bind_method(D_METHOD("load_path_async", "path"), &FFmpegNode::load_path_async);
	ClassDB::bind_method(D_METHOD("play"), &FFmpegNode::play);
	ClassDB::bind_method(D_METHOD("stop"), &FFmpegNode::stop);
	ClassDB::bind_method(D_METHOD("is_playing"), &FFmpegNode::is_playing);
	ClassDB::bind_method(D_METHOD("set_paused", "paused"), &FFmpegNode::set_paused);
	ClassDB::bind_method(D_METHOD("is_paused"), &FFmpegNode::is_paused);
	ClassDB::bind_method(D_METHOD("get_video_texture"), &FFmpegNode::get_video_texture);
	ClassDB::bind_method(D_METHOD("get_length"), &FFmpegNode::get_length);
	ClassDB::bind_method(D_METHOD("set_loop", "enable"), &FFmpegNode::set_loop);
	ClassDB::bind_method(D_METHOD("has_loop"), &FFmpegNode::has_loop);
	ClassDB::bind_method(D_METHOD("get_playback_position"), &FFmpegNode::get_playback_position);
	ClassDB::bind_method(D_METHOD("seek", "time"), &FFmpegNode::seek);

	ADD_SIGNAL(MethodInfo("async_loaded", PropertyInfo(Variant::BOOL, "successful")));
	ADD_SIGNAL(MethodInfo("message", PropertyInfo(Variant::STRING, "message")));
	ADD_SIGNAL(MethodInfo("error", PropertyInfo(Variant::STRING, "message")));
}
