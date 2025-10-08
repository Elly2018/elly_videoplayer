#pragma once

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void gdextension_initialize(ModuleInitializationLevel p_level);
void gdextension_terminate(ModuleInitializationLevel p_level);
void gdextension_startup();
void gdextension_shutdown();
void gdextension_frame();
