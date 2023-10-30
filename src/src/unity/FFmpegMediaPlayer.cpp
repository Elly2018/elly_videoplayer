#include <Logger.h>
#include "FFmpegMediaPlayer.h"
#include <RenderAPI.h>
#include <interface/MediaDecoderUtility.h>
#include <chrono>

void FFmpegMediaPlayer::_init_media() {
	int* li = nullptr;
	int count = 0;
	int current = 0;
	LOG("start init media");
	video_playback = nativeIsVideoEnabled(id);
	if (video_playback) {
		first_frame_v = true;
		nativeGetVideoFormat(id, width, height, framerate, video_length);
		nativeGetOtherStreamIndex(id, 0, li, count, current);
		LOG("Video info:");
		LOG("\tStream Count: ", count);
		LOG("\tCurrent Index: ", current);
		LOG("\tWidth: ", width);
		LOG("\tHeight: ", height);
		LOG("\tFramerate: ", framerate);
	}

	audio_playback = nativeIsAudioEnabled(id);
	if (audio_playback) {
		first_frame_a = true;
		nativeGetAudioFormat(id, channels, sampleRate, audio_length);
		nativeGetOtherStreamIndex(id, 1, li, count, current);
		LOG("Audio info:");
		LOG("\tStream Count: ", count);
		LOG("\tCurrent Index: ", current);
		LOG("\tChannel: ", channels);
		LOG("\tSamplerate: ", sampleRate);
		LOG("\tFLength: ", audio_length);
		LOG("Audio info. channel: ", channels, ", samplerate: ", sampleRate, ", audio_length: ", audio_length);
		nativeSetAudioBufferTime(id, sampleRate * 2);
		for (SubmitAudioFormat& submitAudioFormat : audioFormatCallback) {
			submitAudioFormat(channels, sampleRate);
		}
	}

	clock = nativeGetClock(id);
	LOG("Current clock: ", clock);
	state = INITIALIZED;
	LOG("start change to INITIALIZED");
}


void FFmpegMediaPlayer::load()
{
	load_path(path.c_str());
}

void FFmpegMediaPlayer::load_async()
{
	load_path_async(path.c_str());
}

bool FFmpegMediaPlayer::load_path(const char* p) {
	path = p;
	LOG("start load path: ", path);
	if (player == nullptr) {
		LOG_ERROR("You must register the player instance first");
		return false;
	}

	int d_state = nativeGetDecoderState(id);
	if (d_state > 1) {
		stop();
		//LOG_ERROR("Decoder state: ") + String(std::to_string(d_state).c_str()));
		//return false;
	}

	nativeCreateDecoder(path.c_str(), id);

	bool is_loaded = nativeGetDecoderState(id) == 1;
	if (is_loaded) {
		_init_media();
	}
	else {
		LOG("nativeGetDecoderState is false");
		LOG("State change to UNINITIALIZED");
		state = UNINITIALIZED;
	}

	return is_loaded;
}

void FFmpegMediaPlayer::load_path_async(const char* p) {
	path = p;
	LOG("start load path: ", path);
	int d_state = nativeGetDecoderState(id);
	if (d_state > 1) {
		LOG_ERROR("Decoder state: ", d_state);
		return;
	}

	LOG("State change to LOADING");
	state = LOADING;

	nativeCreateDecoderAsync(path.c_str(), id);
}

void FFmpegMediaPlayer::play() {
	if (state != INITIALIZED) {
		LOG("play func failed, because state is not INITIALIZED");
		return;
	}

	if (paused) {
		paused = false;
	}
	else {
		nativeStartDecoding(id);
		player->play();
	}

	global_start_time = globalTime();

	LOG("start change to Decoding");
	state = DECODING;
	audio_init();
}

void FFmpegMediaPlayer::stop() {
	if (state < State::DECODING) {
		LOG("Stop failed, decoder state currently is: ", state);
		return;
	}

	nativeDestroyDecoder(id);

	video_current_time = 0.0f;
	audio_current_time = 0.0f;
	paused = false;
	player->stop();
	audioFrame.clear();

	PackedByteArray empty = PackedByteArray();
	empty.append(0); empty.append(0); empty.append(0);
	image->call_deferred("set_data", 1, 1, false, Image::FORMAT_RGB8, empty);
	texture->set_deferred("image", image);
	emit_signal("video_update", texture, Vector2i(1, 1));


	LOG("start change to INITIALIZED");
	state = INITIALIZED;
}

bool FFmpegMediaPlayer::is_playing() const {
	return !paused && state == DECODING;
}

void FFmpegMediaPlayer::set_paused(bool p_paused) {
	paused = p_paused;

	if (paused) {
		hang_time = GetGlobalTime() - global_start_time;
	}
	else {
		global_start_time = GetGlobalTime() - hang_time;
	}
	player->set_stream_paused(p_paused);
}

bool FFmpegMediaPlayer::is_paused() const {
	return paused;
}

float FFmpegMediaPlayer::get_length() const {
	return video_playback && video_length > audio_length ? video_length : audio_length;
}

void FFmpegMediaPlayer::set_loop(bool p_enable) {
	looping = p_enable;
}

bool FFmpegMediaPlayer::has_loop() const {
	return looping;
}

float FFmpegMediaPlayer::get_playback_position() const {
	return video_playback && video_current_time > audio_current_time ? video_current_time : audio_current_time;
}

void FFmpegMediaPlayer::seek(float p_time) {
	if (state != DECODING && state != END_OF_FILE) {
		return;
	}

	if (p_time < 0.0f) {
		p_time = 0.0f;
	}
	else if ((video_playback && p_time > video_length) || (audio_playback && p_time > audio_length)) {
		p_time = video_length;
	}

	nativeSetSeekTime(id, p_time);
	nativeSetVideoTime(id, p_time);

	hang_time = p_time;

	audioFrame.clear();

	state = SEEK;
}


void FFmpegMediaPlayer::set_path(const char* _path)
{
}

char* FFmpegMediaPlayer::get_path() const
{
	return nullptr;
}

void FFmpegMediaPlayer::set_format(const char* _format)
{
}

char* FFmpegMediaPlayer::get_format() const
{
	return nullptr;
}

void FFmpegMediaPlayer::RegisterGlobalTimeCallback(GetGlobalTime func)
{
	globalTime = func;
}

void FFmpegMediaPlayer::RegisterAudioFormatCallback(SubmitAudioFormat func)
{
	audioFormatCallback.push_back(func);
}

void FFmpegMediaPlayer::CleanAudioFormatCallback()
{
	audioFormatCallback.clear();
}

void FFmpegMediaPlayer::RegisterAsyncLoadCallback(AsyncLoad func)
{
	asyncLoad = func;
}

void FFmpegMediaPlayer::RegisterAudioCallback(SubmitAudioSample func)
{
	audioCallback.push_back(func);
}

void FFmpegMediaPlayer::CleanAudioCallback()
{
	audioCallback.clear();
}

void FFmpegMediaPlayer::RegisterVideoFormatCallback(SubmitVideoFormat func)
{
	videoFormatCallback.push_back(func);
}

void FFmpegMediaPlayer::CleanVideoFormatCallback()
{
	videoFormatCallback.clear();
}

void FFmpegMediaPlayer::RegisterVideoCallback(SubmitVideoSample func)
{
	videoCallback.push_back(func);
}

void FFmpegMediaPlayer::CleanVideoCallback()
{
	videoCallback.clear();
}

FFmpegMediaPlayer::FFmpegMediaPlayer()
{
	audioCallback = std::list<SubmitAudioSample>();
	audioFormatCallback = std::list<SubmitAudioFormat>();
	api = CreateRenderAPI(UnityGfxRenderer::kUnityGfxRendererD3D11);
}

FFmpegMediaPlayer::~FFmpegMediaPlayer()
{
	audioCallback.clear();
	audioFormatCallback.clear();
	delete api;
}

void FFmpegMediaPlayer::_Update()
{
	switch (state) {
	case LOADING: {
		if (nativeGetDecoderState(id) == 1) {
			_init_media();
			play();
			LOG("Loading successful");
		}
		else if (nativeGetDecoderState(id) == -1) {
			state = UNINITIALIZED;
			LOG_ERROR("Main loop, async loading failed, nativeGetDecoderState == -1");
			LOG_ERROR("Init failed");
		}
	} break;

	case BUFFERING: {
		if (nativeIsVideoBufferFull(id) || nativeIsEOF(id)) {
			global_start_time = globalTime() - hang_time;
			state = DECODING;
		}
	} break;

	case SEEK: {
		if (nativeIsSeekOver(id)) {
			global_start_time = globalTime() - hang_time;
			state = DECODING;
		}
	} break;

	case DECODING: {
		if (paused) {
			return;
		}

		if (video_playback) {
			void* frame_data = nullptr;
			bool frame_ready = false;
			double frameTime = nativeGrabVideoFrame(id, &frame_data, frame_ready, width, height);
			if (frame_ready) {
				api->BeginModifyTexture(texturehandle, width, height, &outRowPitch);
				api->EndModifyTexture(texturehandle, width, height, outRowPitch, frame_data);
				nativeReleaseVideoFrame(id);
			}

			if (clock == -1) {
				video_current_time = globalTime() - global_start_time;
				if (video_current_time < video_length || video_length == -1.0f) {
					nativeSetVideoTime(id, video_current_time);
				}
				else {
					if (!nativeIsVideoBufferEmpty(id)) {
						nativeSetVideoTime(id, video_current_time);
					}
					else {
						state = END_OF_FILE;
					}
				}
			}
		}

		if (nativeIsVideoBufferEmpty(id) && !nativeIsEOF(id) && first_frame_a && first_frame_v) {
			hang_time = globalTime() - global_start_time;
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

void FFmpegMediaPlayer::_FixedUpdate()
{
	int c = 0;
	/*
	if (playback != nullptr && playback.is_valid() && audio_playback) {
		c = playback->get_frames_available();
	}
	else {
		return;
	}
	bool state_check = (state == DECODING || state == BUFFERING) && audioFrame.size() < 1024;
	if (state_check) {
		// TODO: Implement audio.
		unsigned char* raw_audio_data = nullptr;
		int audio_size = 0;
		int channel = 0;
		size_t byte_per_sample = 0;
		/*
		* AV_SAMPLE_FMT_FLT will usually give us byte_per_sample = 4
		*/
		/*
		bool ready = false;
		double frameTime = nativeGetAudioData(id, ready, &raw_audio_data, audio_size, channel, byte_per_sample);
		if (clock == 1) {
			video_current_time = frameTime;
			nativeSetVideoTime(id, video_current_time);
		}
		if (ready) {
			PackedFloat32Array audio_data = PackedFloat32Array();
			audio_data.resize(audio_size * byte_per_sample * channel);
			memcpy(audio_data.ptrw(), raw_audio_data, audio_size * channel * byte_per_sample);
			emit_signal("audio_update", audio_data, audio_size, channel);
			//LOG("Audio info, sample size: %d, channel: %d, byte per sample: %d \n", audio_size, channel, byte_per_sample);
			float s = 0;

			first_frame_a = false;

			for (int i = 0; i < audio_size * channel; i += channel) {
				float* out = new float[channel];
				for (int j = 0; j < channel; j++) { // j have byte per sample padding for each sample
					s = audio_data[i + j];
					out[j] = s;
				}
				float left = out[0];
				float right;
				if (channel <= 1) {
					right = out[0];
				}
				else {
					right = out[1];
				}
				audioFrame.push_back(Vector2(left, right));
				//LOG("Push frame, out: %f, sin: [%f, %f] \n", out, left, right);
				delete[] out;
			}
			nativeFreeAudioData(id);
		}
		else {
			for (int i = 0; i < audio_size; i++) {
				audioFrame.push_back(lastSubmitAudioFrame);
			}
		}
	}
	if (playback.is_valid()) {
		while (c > 0 && audioFrame.size() > 0 && !first_frame_v && state == DECODING) {
			if (audioFrame.size() > 0) {
				Vector2 element = audioFrame.front()->get();
				playback->push_frame(element);
				lastSubmitAudioFrame = element;
				audioFrame.pop_front();
			}
			c -= 1;
		}
		while (c > 0) {
			Vector2 element = Vector2(0, 0);
			playback->push_frame(element);
			lastSubmitAudioFrame = element;
			c -= 1;
		}
	}
	*/
}
