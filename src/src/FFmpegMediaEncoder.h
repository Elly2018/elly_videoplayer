#pragma once

#include <string>

#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/camera_texture.hpp>

using namespace godot;

struct EncoderSetting {
	int width;
	int height;
	String codec;
	String format;
	int bitrate;
	int maxrate;
	int bufsize;
};

/**
 * The encoder node for godot
*/
class FFmpegMediaEncoder : public Node {
	GDCLASS(FFmpegMediaEncoder, Node);

private:
	enum State {
		LOADING,
		UNINITIALIZED,
		INITIALIZED,
		ENCODING,
		FINISH
	};

	enum IOType {
		Camera,
		File,
		URL
	};

	bool is_pause;
	State state = UNINITIALIZED;
	Ref<CameraTexture> target;
	IOType in_type;
	IOType out_type;
	EncoderSetting setting;
	String input;
	String output;

protected:
	static void _bind_methods();

public:
	void register_camera(Ref<CameraTexture> tex);

	void start();
	void stop();

	void set_pause(bool pau);
	bool get_pause();

	FFmpegMediaEncoder();
	~FFmpegMediaEncoder();
};
