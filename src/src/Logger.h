//========= Copyright 2015-2019, HTC Corporation. All rights reserved. ===========

#pragma once
#include <string>
#include <functional>
#include <stdio.h>
#include <stdarg.h>

#define BASE_LOG(X, ...) (std::string("[EllyVideoPlayer_Native]") + std::string(X) + std::string("\n")).c_str() , __VA_ARGS__

#define ENABLE_LOG
//#define ENABLE_LOG_VERBOSE
#ifdef ENABLE_LOG
	#define LOG(X, ...) std::fprintf(stdout, BASE_LOG(X, __VA_ARGS__))
	#define LOG_ERROR(X, ...) std::fprintf(stderr, BASE_LOG(X, __VA_ARGS__))
#ifdef ENABLE_LOG_VERBOSE
	#define LOG_VERBOSE(X, ...) std::fprintf(stdout, BASE_LOG(X, __VA_ARGS__))
	#define LOG_ERROR_VERBOSE(X, ...) std::fprintf(stderr, BASE_LOG(X, __VA_ARGS__))
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
