#pragma once

#include <godot_cpp/classes/node.hpp>

using namespace godot;

class FFmpegVideoPlayer : public Node
{
	GDCLASS(FFmpegVideoPlayer, Node);
protected:
	static void _bind_methods();

public:
	FFmpegVideoPlayer();
	~FFmpegVideoPlayer();

	void _ready() override;
	void _process(double delta) override;

	void hello_node();
};
