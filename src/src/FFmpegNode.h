#pragma once

#include "ViveMediaDecoder.h"
#include <string>

#include <godot_cpp/classes/audio_stream_playback.hpp>
#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/node.hpp>

using namespace godot;


/**
 * The control node for godot video player
*/
class FFmpegNode : public Node {
	GDCLASS(FFmpegNode, Node);

private:
/**
 * The different stage for this video player,
 * each stage will change under behaviour of decoding processing
*/
	enum State {
		/**
		 * This means the media does not initialize yet.
		 * It will trying to get the information it needs in order to start the decoding process.
		*/
		LOADING,
		UNINITIALIZED,
		INITIALIZED,
		DECODING,
		SEEK,
		BUFFERING,
		END_OF_FILE,
	};

	// TODO: Implement audio.
 	AudioStreamPlayer *player;
 	Ref<AudioStreamPlayback> playback;

	Ref<ImageTexture> texture;
	Ref<Image> image;

	int id = 0;
	int state = UNINITIALIZED;

	bool first_frame = true;
	bool paused = false;
	bool looping = false;

	bool video_playback = false;
	int width = 0;
	int height = 0;
	float video_length = 0.0f;
	double video_current_time = 0.0f;
	int data_size = 0;

	bool audio_playback = false;
	int channels = 0;
	int frequency = 0;
	float audio_length = 0.0f;
	double audio_current_time = 0.0f;

	double global_start_time = 0.0f;
	double hang_time = 0.0f;

	void _init_media();

protected:
	void _notification(int p_what);
	static void _bind_methods();

public:
	bool load_path(String path);
	void load_path_async(String path);

	void stop();
	void play();

	bool is_playing() const;

	void set_paused(bool p_paused);
	bool is_paused() const;

	void set_loop(bool p_enable);
	bool has_loop() const;

	Ref<ImageTexture> get_video_texture();

	float get_length() const;

	float get_playback_position() const;
	void seek(float p_time);

	void _process(float delta);
 	void _physics_process(float delta);

	FFmpegNode();
	~FFmpegNode();
};
