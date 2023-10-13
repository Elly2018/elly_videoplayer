//========= Copyright 2015-2019, HTC Corporation. All rights reserved. ===========

#pragma once
#include <godot_cpp/variant/utility_functions.hpp>
#include <functional>
#include <stdio.h>
#include <stdarg.h>

using namespace godot;

//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(...) godot::UtilityFunctions::print( __VA_ARGS__ )
#define LOG_ERROR(...) godot::UtilityFunctions::printerr( __VA_ARGS__ )
#else
#define LOG
#define LOG_ERROR
#endif