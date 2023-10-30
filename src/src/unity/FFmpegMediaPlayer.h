#pragma once
#include <string>
#include <queue>

#include <Unity/IUnityInterface.h>
#include <unity/UnityInterface.h>
#include <RenderAPI.h>

struct Vector2 {
	float x;
	float y;
};

class FFmpegMediaPlayer{
private:
	/**
	   The different stage for this video player,
	   each stage will change under behaviour of decoding processing
	*/
	enum State {
		/**
		 * This means the media does not initialize yet.
		 * It will trying to get the information it needs in order to start the decoding process.
		*/
		FAILED = -1,
		LOADING,
		UNINITIALIZED,
		INITIALIZED,
		DECODING,
		SEEK,
		BUFFERING,
		END_OF_FILE,
	};

	// TODO: Implement audio.
	//AudioStreamPlayer* player;
	//Ref<AudioStreamGenerator> generator;
	//Ref<AudioStreamGeneratorPlayback> playback;

	//Ref<ImageTexture> texture;
	void* texturehandle;
	int outRowPitch;
	std::string path;
	std::string format;

	RenderAPI* api;
	std::list<StateChanged> stateChangedCallback;
	std::list<SubmitAudioSample> audioCallback;
	std::list<SubmitAudioFormat> audioFormatCallback;
	std::list<SubmitVideoSample> videoCallback;
	std::list<SubmitVideoFormat> videoFormatCallback;
	GetGlobalTime globalTime;
	AsyncLoad asyncLoad;
	AudioControl audioControl;
	AudioBufferCount audioBufferCount;

	bool first_frame_v = true;
	bool first_frame_a = true;
	bool paused = false;
	bool looping = false;
	int clock = -1;


	bool video_playback = false;
	int width = 0;
	int height = 0;
	float framerate = 0.0f;
	float video_length = 0.0f;
	Vector2 lastSubmitAudioFrame; // si
	std::queue<Vector2> audioFrame;
	double video_current_time = 0.0f;
	int data_size = 0;

	bool audio_playback = false;
	int channels = 0;
	int sampleRate = 0;
	float audio_length = 0.0f;
	double audio_current_time = 0.0f;

	double global_start_time = 0.0f;
	double hang_time = 0.0f;

	void _init_media();
	void state_change(State _state);

public:
	int id = -1;
	int state = UNINITIALIZED;

	/*
	  Loading path from local variable
	*/
	void load();
	/*
	  Async loading path from local variable
	  "async_loaded" signal will trigger when finish
	*/
	void load_async();
	/*
	  Loading path from string
	*/
	bool load_path(const char* path);
	/*
	  Async loading path from string
	  "async_loaded" signal will trigger when finish
	*/
	void load_path_async(const char* path);


	/*
	  Stop the player, this will destroy decoder object in the low level
	  And reset every variable
	*/
	void stop();
	/*
	  This method should called after media is loaded
	  Otherwise this will do nothing
	*/
	void play();
	/*
	  Check current state is playing media
	*/
	bool is_playing() const;


	/*
	* Pause decoder
	*/
	void set_paused(bool p_paused);
	/*
	* Check pause state
	*/
	bool is_paused() const;


	/*
	* Set the loop trigger
	* Looping will decide if end of file will causing seek to start
	*/
	void set_loop(bool p_enable);
	/*
	* Check loop state
	*/
	bool has_loop() const;
	/*
	* Get the media length (second)
	*/
	float get_length() const;
	/*
	* Get current play position (second)
	*/
	float get_playback_position() const;
	/*
	* Seeking to particular position within the loaded media
	*/
	void seek(float p_time);


	/*
	* Godot update method
	* This will trying to update the video part
	* And handle some stage of player events
	*/
	void _Update();
	/*
	* Godot fixed update method
	* This will trying to update the audio part
	*/
	void _FixedUpdate();


	/*
	* Set the audio player instance to it
	* This instance will become the primary source of audio player for it
	*/
	//void set_player(AudioStreamPlayer* _player);
	/*
	* Get current use audio player
	*/
	//AudioStreamPlayer* get_player() const;

	void set_path(const char* _path);
	const char* get_path();
	void set_format(const char* _format);
	const char* get_format();

	void RegisterGlobalTimeCallback(GetGlobalTime func);
	void RegisterAsyncLoadCallback(AsyncLoad func);
	void RegisterAudioControlCallback(AudioControl func);
	void RegisterAudioBufferCountCallback(AudioBufferCount func);
	void RegisterStateChangedCallback(StateChanged func);
	void CleanStateChangedCallback();
	void RegisterAudioFormatCallback(SubmitAudioFormat func);
	void CleanAudioFormatCallback();
	void RegisterAudioCallback(SubmitAudioSample func);
	void CleanAudioCallback();
	void RegisterVideoFormatCallback(SubmitVideoFormat func);
	void CleanVideoFormatCallback();
	void RegisterVideoCallback(SubmitVideoSample func);
	void CleanVideoCallback();

	FFmpegMediaPlayer();
	~FFmpegMediaPlayer();
};