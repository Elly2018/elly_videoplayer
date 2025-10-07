#pragma once

#include <string>

#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/node.hpp>

using namespace godot;


/**
 * The filter node for godot video display
*/
class VRVideoFilter : public Node {
	GDCLASS(VRVideoFilter, Node);

private:
	bool flip = false;
	int layout = 0;
	List<Ref<Material>> mats;

protected:
	static void _bind_methods();

public:
	/*
	* The layout will decide if it's stereo mode or not
	*/
	enum LayoutMode {
		NONE,
		TOP_DOWN,
		SIDE_BY_SIDE
	};

	/*
	* Add additional material to apply the uniform to
	*/
	void add_material(const Ref<Material> &mat);
	/*
	* Remove target material from the apply list.
	*/
	void remove_material(const Ref<Material> &mat);
	/*
	* Clean the apply list
	*/
	void clean_material();

	void set_layout(int _layout);
	int get_layout() const;

	void set_flip(bool flip);
	bool get_flip() const;
 
	void set_texture(Ref<ImageTexture> tex, Vector2i size);

	VRVideoFilter();
	~VRVideoFilter();
};
