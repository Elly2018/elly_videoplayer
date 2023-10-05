#include "VRVideoFilter.h"

using namespace godot;

void VRVideoFilter::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("set_layout", "layout"), &VRVideoFilter::set_layout);
	ClassDB::bind_method(D_METHOD("get_layout"), &VRVideoFilter::get_layout);
	ClassDB::bind_method(D_METHOD("set_texture", "texture", "size"), &VRVideoFilter::set_texture);

	ADD_SIGNAL(MethodInfo("layout_changed", PropertyInfo(Variant::INT, "mode")));
	ADD_SIGNAL(MethodInfo("received_texture", PropertyInfo(Variant::RID, "image", PROPERTY_HINT_RESOURCE_TYPE, "ImageTexture"), PropertyInfo(Variant::VECTOR2I, "size"), PropertyInfo(Variant::INT, "mode")));
}

void VRVideoFilter::add_material(Ref<Material> mat)
{
	
}

void VRVideoFilter::remove_material(Ref<Material> mat)
{
}

void VRVideoFilter::clean_material()
{
}

void VRVideoFilter::set_layout(int _layout)
{
	layout = _layout;
}

int VRVideoFilter::get_layout() const
{
	return layout;
}

void VRVideoFilter::set_texture(Ref<ImageTexture> tex, Vector2i size)
{
	image = Image::create(tex->get_width(), tex->get_height(), false, Image::FORMAT_RGB8);
}

VRVideoFilter::VRVideoFilter()
{
	image = Image::create(1, 1, false, Image::FORMAT_RGB8);
	texture = ImageTexture::create_from_image(image);
}

VRVideoFilter::~VRVideoFilter()
{
}
