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

void GDExtension_Initialize(ModuleInitializationLevel p_level)
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

void GDExtension_Terminate(ModuleInitializationLevel p_level)
{
	LOG("FFmpeg extension terminate");
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE)
	{
		return;
	}
}

void GDExtension_Startup()
{
	LOG("FFmpeg extension startup !");
}

void GDExtension_Shutdown()
{
	LOG("FFmpeg extension shutdown !");
}

void GDExtension_Frame()
{
	// LOG("FFmpeg extension frame");
}

extern "C"
{
	GDExtensionBool GDE_EXPORT gdextension_init(
		GDExtensionInterfaceGetProcAddress p_get_proc_address,
		GDExtensionClassLibraryPtr p_library,
		GDExtensionInitialization* p_initialization
	) {
		const GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, p_initialization);

		init_obj.register_initializer(&GDExtension_Initialize);
		init_obj.register_terminator(&GDExtension_Terminate);
		init_obj.register_startup_callback(&GDExtension_Startup);
		init_obj.register_shutdown_callback(&GDExtension_Shutdown);
		init_obj.register_frame_callback(&GDExtension_Frame);
		init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

		return init_obj.init();
	}
}
