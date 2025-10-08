//========= Copyright 2015-2019, HTC Corporation. All rights reserved. ===========

#pragma once
#include <godot_cpp/variant/utility_functions.hpp>
#include <functional>

using namespace godot;

#define ENABLE_LOG
#define ENABLE_LOG_VERBOSE

#ifdef ENABLE_LOG
#define LOG(...) godot::UtilityFunctions::print( __VA_ARGS__ )
#define LOG_ERROR(...) godot::UtilityFunctions::printerr( __VA_ARGS__ )
#ifdef ENABLE_LOG_VERBOSE
#define LOG_VERBOSE(...) godot::UtilityFunctions::print( __VA_ARGS__ )
#define LOG_ERROR_VERBOSE(...) godot::UtilityFunctions::printerr( __VA_ARGS__ )
#else
#define LOG_VERBOSE(...)
#define LOG_ERROR_VERBOSE(...)
#endif
#else
#define LOG(...)
#define LOG_ERROR(...)
#define LOG_VERBOSE(...)
#define LOG_ERROR_VERBOSE(...)
#endif
