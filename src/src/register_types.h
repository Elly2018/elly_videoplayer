#pragma once

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

/**
 * Extension entry point
 */
void GDExtension_Initialize(ModuleInitializationLevel p_level);

/**
 * Extension end point
 */
void GDExtension_Terminate(ModuleInitializationLevel p_level);

/**
 * Extension start point, called after initialize
 */
void GDExtension_Startup();

/**
 * Extension shutdown point, called before terminate
 */
void GDExtension_Shutdown();

/**
 *
 */
void GDExtension_Frame();
