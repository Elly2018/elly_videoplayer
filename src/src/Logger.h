//========= Copyright 2015-2019, HTC Corporation. All rights reserved. ===========

#pragma once
#include <functional>
#include <stdio.h>
#include <stdarg.h>

#define ENABLE_LOG
//#define ENABLE_LOG_VERBOSE
#ifdef ENABLE_LOG
	#define LOG(...) std::fprintf(stdout, __VA_ARGS__ )
	#define LOG_ERROR(...) std::fprintf(stderr, __VA_ARGS__ )
#ifdef ENABLE_LOG_VERBOSE
	#define LOG_VERBOSE(...) std::fprintf(stdout, __VA_ARGS__ )
	#define LOG_ERROR_VERBOSE(...) std::fprintf(stderr, __VA_ARGS__ )
#else
	#define LOG_VERBOSE
	#define LOG_ERROR_VERBOSE
#endif
#else
	#define LOG
	#define LOG_ERROR
	#define LOG_VERBOSE
	#define LOG_ERROR_VERBOSE
#endif
