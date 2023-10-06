#pragma once

#include <string>

#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/node.hpp>

using namespace godot;


/**
 * The control node for godot video player
*/
class VRVideoFilter : public Node {
	GDCLASS(VRVideoFilter, Node);

private:
	bool flip = false;
	Ref<Image> image;
	Ref<ImageTexture> texture;
	List<Ref<Material>> mats;
	int layout = 0;

protected:
	static void _bind_methods();

public:

	enum LayoutMode {
		NONE,
		TOP_DOWN,
		SIDE_BY_SIDE
	};

	void add_material(Ref<Material> mat);
	void remove_material(Ref<Material> mat);
	void clean_material();

	void set_layout(int _layout);
	int get_layout() const;

	void set_flip(bool flip);
	bool get_flip();

	void set_texture(Ref<ImageTexture> tex, Vector2i size);

	VRVideoFilter();
	~VRVideoFilter();
};
