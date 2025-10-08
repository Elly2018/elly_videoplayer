#include "register_types.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
//#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/godot.hpp>

#include "FFmpegMediaPlayer.h"
#include "FFmpegMediaEncoder.h"
#include "Logger.h"
#include "VRVideoFilter.h"

using namespace godot;

//static MySingleton *_my_singleton;

void gdextension_initialize(ModuleInitializationLevel p_level)
{
	LOG("FFmpeg extension initialize");
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	GDREGISTER_CLASS(FFmpegMediaPlayer);
	GDREGISTER_CLASS(FFmpegMediaEncoder);
	GDREGISTER_CLASS(VRVideoFilter);
	//ClassDB::register_class<MySingleton>();
	//_my_singleton = memnew(MySingleton);
	//Engine::get_singleton()->register_singleton("MySingleton", MySingleton::get_singleton());
}

void gdextension_terminate(ModuleInitializationLevel p_level)
{
	LOG("FFmpeg extension terminate");
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
	{
	}
}

void gdextension_startup()
{
	LOG("FFmpeg extension startup !");
}

void gdextension_shutdown()
{
	LOG("FFmpeg extension shutdown !");
}

void gdextension_frame()
{
	//LOG("FFmpeg extension frame");
}

extern "C"
{
	GDExtensionBool GDE_EXPORT gdextension_init(
		GDExtensionInterfaceGetProcAddress p_get_proc_address,
		GDExtensionClassLibraryPtr p_library,
		GDExtensionInitialization* p_initialization
	) {
		const GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, p_initialization);

		init_obj.register_initializer(&gdextension_initialize);
		init_obj.register_terminator(&gdextension_terminate);
		init_obj.register_startup_callback(&gdextension_startup);
		init_obj.register_shutdown_callback(&gdextension_shutdown);
		init_obj.register_frame_callback(&gdextension_frame);
		init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

		return init_obj.init();
	}
}
