#include "VRVideoFilter.h"

using namespace godot;

void VRVideoFilter::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_layout", "layout"), &VRVideoFilter::set_layout);
	ClassDB::bind_method(D_METHOD("get_layout"), &VRVideoFilter::get_layout);
	ClassDB::bind_method(D_METHOD("set_flip", "flip"), &VRVideoFilter::set_flip);
	ClassDB::bind_method(D_METHOD("get_flip"), &VRVideoFilter::get_flip);
	ClassDB::bind_method(D_METHOD("set_texture", "texture", "size"), &VRVideoFilter::set_texture);

	ADD_SIGNAL(MethodInfo("layout_changed", PropertyInfo(Variant::INT, "mode")));
	ADD_SIGNAL(MethodInfo("received_texture", PropertyInfo(Variant::RID, "image", PROPERTY_HINT_RESOURCE_TYPE, "ImageTexture"), PropertyInfo(Variant::VECTOR2I, "size"), PropertyInfo(Variant::INT, "mode")));
}

void VRVideoFilter::add_material(Ref<Material> mat)
{
	Ref<Material> f = mats.find(mat);
	if (f.is_valid()) return;
	mats.push_back(mat);
}

void VRVideoFilter::remove_material(Ref<Material> mat)
{
	Ref<Material> f = mats.find(mat);
	if (f.is_null()) return;
}

void VRVideoFilter::clean_material()
{
	mats.clear();
}

void VRVideoFilter::set_layout(int _layout)
{
	layout = _layout;
}

int VRVideoFilter::get_layout() const
{
	return layout;
}

void VRVideoFilter::set_flip(bool _flip)
{
	flip = _flip;
}

bool VRVideoFilter::get_flip() const
{
	return flip;
}

void VRVideoFilter::set_texture(Ref<ImageTexture> tex, Vector2i size)
{
	texture = tex;
	for (Ref<Material> mat : mats) {
		mat->set_deferred("shader_parameter/tex", tex);
		mat->set_deferred("shader_parameter/flip", flip);
		mat->set_deferred("shader_parameter/mode", layout);
	}
}

VRVideoFilter::VRVideoFilter()
{
	image = Image::create(1, 1, false, Image::FORMAT_RGB8);
	texture = ImageTexture::create_from_image(image);
	mats = List<Ref<Material>>();
}

VRVideoFilter::~VRVideoFilter()
{
	clean_material();
}
