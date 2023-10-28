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
		nativeSetAudioBufferTime(id, get_buffer_length());
		for (SubmitAudioFormat& submitAudioFormat : audioFormatCallback) {
			submitAudioFormat(channels, sampleRate);
		}
	}

	clock = nativeGetClock(id);
	LOG("Current clock: ", clock);
	state = INITIALIZED;
	LOG("start change to INITIALIZED");
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
}
