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
		LOG("\tStream Count: %d", count);
		LOG("\tCurrent Index: %d", current);
		LOG("\tWidth: %d", width);
		LOG("\tHeight: %d", height);
		LOG("\tFramerate: %d", framerate);
		for (SubmitVideoFormat& submitVideoFormat : videoFormatCallback) {
			submitVideoFormat(width, height);
		}
	}

	audio_playback = nativeIsAudioEnabled(id);
	if (audio_playback) {
		first_frame_a = true;
		nativeGetAudioFormat(id, channels, sampleRate, audio_length);
		nativeGetOtherStreamIndex(id, 1, li, count, current);
		LOG("Audio info:");
		LOG("\tStream Count: %d", count);
		LOG("\tCurrent Index: %d", current);
		LOG("\tChannel: %d", channels);
		LOG("\tSamplerate: %d", sampleRate);
		LOG("\tFLength: %d", audio_length);
		LOG("Audio info. channel: %d, samplerate: %d, audio_length: %d", channels, sampleRate, audio_length);
		nativeSetAudioBufferTime(id, sampleRate * 2);
		for (SubmitAudioFormat& submitAudioFormat : audioFormatCallback) {
			submitAudioFormat(channels, sampleRate);
		}
		audioControl(0);
	}

	clock = nativeGetClock(id);
	LOG("Current clock: %d", clock);
	state_change(INITIALIZED);
	LOG("start change to INITIALIZED");
}

void FFmpegMediaPlayer::state_change(State _state)
{
	state = _state;
	for (StateChanged& stateChanged : stateChangedCallback) {
		stateChanged(_state);
	}
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
	set_path(p);
	LOG("start load path: ", path.c_str());
	if (audioControl == nullptr) {
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
		state_change(UNINITIALIZED);
	}

	return is_loaded;
}

void FFmpegMediaPlayer::load_path_async(const char* p) {
	set_path(p);
	LOG("start load path: ", path.c_str());
	int d_state = nativeGetDecoderState(id);
	if (d_state > 1) {
		LOG_ERROR("Decoder state: ", d_state);
		return;
	}

	LOG("State change to LOADING");
	state_change(LOADING);

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
		//nativeStartDecoding(id);
		audioControl(0);
	}

	global_start_time = globalTime();

	LOG("start change to Decoding");
	state_change(DECODING);
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
	audioControl(2);

	for (SubmitVideoFormat& submitVideoFormat : videoFormatCallback) {
		submitVideoFormat(1, 1);
	}

	char* b = new char[24];
	for (int i = 0; i < 16; i++)
	{
		b = 0;
	}

	for (SubmitVideoSample& submitVideoSample : videoCallback) {
		submitVideoSample(24, b);
	}

	LOG("start change to INITIALIZED");
	state_change(INITIALIZED);
}

bool FFmpegMediaPlayer::is_playing() const {
	return !paused && state == DECODING;
}

void FFmpegMediaPlayer::set_paused(bool p_paused) {
	paused = p_paused;

	if (paused) {
		hang_time = globalTime() - global_start_time;
	}
	else {
		global_start_time = globalTime() - hang_time;
	}
	audioControl(1);
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

	audioControl(3);

	state_change(SEEK);
}


void FFmpegMediaPlayer::set_path(const char* _path)
{
	LOG("Path set to: ", _path);
	path = _path;
}

const char* FFmpegMediaPlayer::get_path()
{
	return path.c_str();
}

void FFmpegMediaPlayer::set_format(const char* _format)
{
	LOG("Format set to: ", _format);
	format = _format;
}

const char* FFmpegMediaPlayer::get_format()
{
	return format.c_str();
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

void FFmpegMediaPlayer::RegisterAudioControlCallback(AudioControl func)
{
	audioControl = func;
}

void FFmpegMediaPlayer::RegisterAudioBufferCountCallback(AudioBufferCount func)
{
	audioBufferCount = func;
}

void FFmpegMediaPlayer::RegisterStateChangedCallback(StateChanged func)
{
	stateChangedCallback.push_back(func);
}

void FFmpegMediaPlayer::CleanStateChangedCallback()
{
	stateChangedCallback.clear();
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
	stateChangedCallback = std::list<StateChanged>();
	audioCallback = std::list<SubmitAudioSample>();
	audioFormatCallback = std::list<SubmitAudioFormat>();
	videoCallback = std::list<SubmitVideoSample>();
	videoFormatCallback = std::list<SubmitVideoFormat>();
	//api = CreateRenderAPI(UnityGfxRenderer::kUnityGfxRendererD3D11);
}

FFmpegMediaPlayer::~FFmpegMediaPlayer()
{
	stateChangedCallback.clear();
	audioCallback.clear();
	audioFormatCallback.clear();
	videoCallback.clear();
	videoFormatCallback.clear();
	nativeDestroyDecoder(id);
	//delete api;
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
			state_change(UNINITIALIZED);
			LOG_ERROR("Main loop, async loading failed, nativeGetDecoderState == -1");
			LOG_ERROR("Init failed");
		}
	} break;

	case BUFFERING: {
		if (nativeIsVideoBufferFull(id) || nativeIsEOF(id)) {
			global_start_time = globalTime() - hang_time;
			state_change(DECODING);
		}
	} break;

	case SEEK: {
		if (nativeIsSeekOver(id)) {
			global_start_time = globalTime() - hang_time;
			state_change(DECODING);
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
				LOG("Video frame: %d %d", width, height);
				for (SubmitVideoSample& submitVideoSample : videoCallback) {
					submitVideoSample(width * height * 3, (char*)frame_data);
				}
				//api->BeginModifyTexture(texturehandle, width, height, &outRowPitch);
				//api->EndModifyTexture(texturehandle, width, height, outRowPitch, frame_data);
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
						state_change(END_OF_FILE);
					}
				}
			}
		}

		if (nativeIsVideoBufferEmpty(id) && !nativeIsEOF(id) && first_frame_a && first_frame_v) {
			hang_time = globalTime() - global_start_time;
			state_change(BUFFERING);
		}
	} break;

	case END_OF_FILE: {
		if (looping) {
			state_change(DECODING);
			seek(0.0f);
		}
	} break;
	}
}

void FFmpegMediaPlayer::_FixedUpdate()
{
	bool state_check = (state == DECODING || state == BUFFERING) && audioBufferCount() < 2048;
	if (state_check) {
		// TODO: Implement audio.
		unsigned char* raw_audio_data = nullptr;
		int audio_size = 0;
		int channel = 0;
		size_t byte_per_sample = 0;
		/*
		* AV_SAMPLE_FMT_FLT will usually give us byte_per_sample = 4
		*/
		bool ready = false;
		double frameTime = nativeGetAudioData(id, ready, &raw_audio_data, audio_size, channel, byte_per_sample);
		if (clock == 1) {
			video_current_time = frameTime;
			nativeSetVideoTime(id, video_current_time);
		}
		if (ready) {
			LOG("Audio frame: %d %d", audio_size, channel);
			float* audio_data = (float*)malloc(audio_size * byte_per_sample * channel);
			memcpy(audio_data, raw_audio_data, audio_size * channel * byte_per_sample);
			for (SubmitAudioSample& submitAudioSample : audioCallback) {
				submitAudioSample(audio_size * channel, (float*)audio_data);
			}
			first_frame_a = false;
			nativeFreeAudioData(id);
			free(audio_data);
		}
	}
}
