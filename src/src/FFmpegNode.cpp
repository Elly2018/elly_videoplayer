#include "FFmpegNode.h"
#include "Logger.h"

#include <cstring>
#include <math.h>

#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void FFmpegNode::_init_media() {
	emit_signal("message", String("start init media"));
	video_playback = nativeIsVideoEnabled(id);
	if (video_playback) {
		first_frame = true;

		nativeGetVideoFormat(id, width, height, video_length);
		data_size = width * height * 3;
	}

	audio_playback = nativeIsAudioEnabled(id);
	if (audio_playback) {
		nativeGetAudioFormat(id, channels, frequency, audio_length);
		generator->set_mix_rate(frequency);
		LOG("Audio info: %d %d %d \n", channels, frequency, audio_length);
		player->play();
	}

	state = INITIALIZED;
	emit_signal("message", String("start change to INITIALIZED"));
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
		emit_signal("message", String("nativeGetDecoderState is false"));
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
		player->play();
	}

	global_start_time = Time::get_singleton()->get_unix_time_from_system();

	emit_signal("message", String("start change to Decoding"));
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
	player->stop();

	emit_signal("message", String("start change to INITIALIZED"));
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
	player->set_stream_paused(p_paused);
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
		unsigned char* raw_audio_data = nullptr;
		int audio_size = 0;
		int channel = 0;
		size_t byte_per_sample = 0;
		/*
		* AV_SAMPLE_FMT_FLT will usually give us byte_per_sample = 4
		*/
		double audio_time = nativeGetAudioData(id, &raw_audio_data, audio_size, channel, byte_per_sample);
		if (playback == nullptr) {
			nativeFreeAudioData(id);
		}
		else {
			int c = playback->get_frames_available();
			LOG("get_frames_available: %d \n", c);
			if (audio_time != -1.0f) {
				PackedByteArray audio_data = PackedByteArray();
				audio_data.resize(audio_size * byte_per_sample * channel);
				memcpy(audio_data.ptrw(), raw_audio_data, audio_size * byte_per_sample);
				nativeFreeAudioData(id);
				//audio_data.resize(audio_size);
				LOG("Audio info, sample size: %d, channel: %d, byte per sample: %d \n", audio_size, channel, byte_per_sample);
				union {
					float result;
					signed char a[4];
				} u_data;
				u_data.result = 0;

				for (int i = 0; i < audio_size * byte_per_sample * channel; i += (byte_per_sample * channel)) {
					float* out = new float[channel];
					for (int j = 0; j < channel; j++) { // j have byte per sample padding for each sample
						for (int k = 0; k < byte_per_sample; k++) { // read four byte
							u_data.a[k] = audio_data[i + (j * byte_per_sample) + k];
						}
						out[j] = u_data.result;
					}
					float left = out[0];
					float right;
					if (channel <= 1) {
						right = out[0];
					}
					else {
						right = out[1];
					}
					//float increment_x = left / 44100.0f;
					//float increment_y = right / 44100.0f;
					//float si_x = Math::sin(phase.x * (float)Math_TAU);
					//float si_y = Math::sin(phase.y * (float)Math_TAU);
					lastframe = Vector2(left, right);
					//const Vector2 f = Vector2(left, right);
					audioFrame.push_back(Vector2(left, right));
					playback->push_frame(lastframe);
					//phase = Vector2(Math::fmod(phase.x + increment_x, 1.0f), Math::fmod(phase.y + increment_y, 1.0f));
					LOG("Push frame, out: %f, sin: [%f, %f], frame: [%f, %f] \n", out, left, right, lastframe.x, lastframe.y);
					delete[] out;
				}
			}

			while (c > 0 && audioFrame.size() > 0) {
				Vector2 element = audioFrame.front()->get();
				playback->push_frame(element);
				audioFrame.pop_front();
				c -= 1;
			}
		}
	}
passout:
	switch (state) {
		case LOADING: {
			if (nativeGetDecoderState(id) == INITIALIZED) {
				_init_media();
				emit_signal("async_loaded", true);
				emit_signal("message", String("Loading successful"));
			} else if (nativeGetDecoderState(id) == -1) {
				state = UNINITIALIZED;
				emit_signal("error", String("Main loop, async loading failed, nativeGetDecoderState == -1"));
				emit_signal("error", String("Init failed"));
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
					LOG("data size: %d \n", data_size);
					image_data.resize(data_size);
					memcpy(image_data.ptrw(), frame_data, data_size);
					LOG("actual data size: %d \n", image_data.size());
					image->call_deferred("set_data", width, height, false, Image::FORMAT_RGB8, image_data);
					texture->set_deferred("image", image);
					first_frame = false;

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

void FFmpegNode::_physics_process(float delta) {
	/*
  	if (!audio_playback || paused || state == UNINITIALIZED || state == EOF) {
  		return;
  	}
 
  	unsigned char *frame_data = nullptr;
  	int frame_length = 0;
  	double audio_time = nativeGetAudioData(id, &frame_data, frame_length);
 
  	if (audio_time > 0.0f) {
  		if (state != SEEK && frame_length != 0 && frame_data != nullptr) { }
 
  		nativeFreeAudioData(id);
  	}
 
  	if (state == DECODING && frame_data != nullptr && !player->is_playing()) {
  		audio_current_time = Time::get_singleton()->get_unix_time_from_system() - global_start_time;
  	}
	*/
  }

void FFmpegNode::set_player(AudioStreamPlayer* _player)
{
	player = _player;
	player->set_autoplay(true);
}

AudioStreamPlayer* FFmpegNode::get_player() const
{
	return player;
}

void FFmpegNode::set_gen_streamer(AudioStreamGenerator* _gen)
{
	generator = _gen;
	if (player != nullptr) {
		player->set_stream(generator);
	}
}

AudioStreamGenerator* FFmpegNode::get_gen_streamer() const
{
	return generator;
}

void FFmpegNode::set_gen_streamer_playback(AudioStreamGeneratorPlayback* _gen)
{
	playback = _gen;
}

AudioStreamGeneratorPlayback* FFmpegNode::get_gen_streamer_playback() const
{
	return playback;
}
FFmpegNode::FFmpegNode() {
	image = Image::create(1,1,false, Image::FORMAT_RGB8);
	texture = ImageTexture::create_from_image(image);
	audioFrame = List<Vector2>();

	auto temp = Logger::instance();
	LOG("FFmpegNode instance created. \n");
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
	ClassDB::bind_method(D_METHOD("set_player", "player"), &FFmpegNode::set_player);
	ClassDB::bind_method(D_METHOD("get_player"), &FFmpegNode::get_player);
	ClassDB::bind_method(D_METHOD("set_gen_streamer", "streamer"), &FFmpegNode::set_gen_streamer);
	ClassDB::bind_method(D_METHOD("get_gen_streamer"), &FFmpegNode::get_gen_streamer);
	ClassDB::bind_method(D_METHOD("set_gen_streamer_playback", "streamer"), &FFmpegNode::set_gen_streamer_playback);
	ClassDB::bind_method(D_METHOD("get_gen_streamer_playback"), &FFmpegNode::get_gen_streamer_playback);

	ADD_SIGNAL(MethodInfo("async_loaded", PropertyInfo(Variant::BOOL, "successful")));
	ADD_SIGNAL(MethodInfo("audio", PropertyInfo(Variant::VECTOR2, "v2")));
	ADD_SIGNAL(MethodInfo("message", PropertyInfo(Variant::STRING, "message")));
	ADD_SIGNAL(MethodInfo("error", PropertyInfo(Variant::STRING, "message")));
}
