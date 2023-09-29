//========= Copyright 2015-2019, HTC Corporation. All rights reserved. ===========

#pragma once
#include "godot_cpp/variant/string.hpp"
#include <functional>
#include <stdio.h>
#include <stdarg.h>

#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(...) Logger::instance()->log(__VA_ARGS__)
#else
#define LOG
#endif

using namespace godot;

class Logger {
public:
	static Logger* instance();
	void log(const char* str, ...);
private:
	Logger();
	static Logger* _instance;
};