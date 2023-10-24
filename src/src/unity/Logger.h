//========= Copyright 2015-2019, HTC Corporation. All rights reserved. ===========

#pragma once
#include <functional>
#include <stdio.h>
#include <stdarg.h>

#ifdef UNITY

#endif

#define ENABLE_LOG
//#define ENABLE_LOG_VERBOSE
#ifdef ENABLE_LOG
	#ifdef UNITY
		#define LOG(...)
		#define LOG_ERROR(...)
	#else
		#define LOG
		#define LOG_ERROR
	#endif
#ifdef ENABLE_LOG_VERBOSE
	#ifdef UNITY
		#define LOG_VERBOSE(...)
		#define LOG_ERROR_VERBOSE(...)
	#else
		#define LOG_VERBOSE
		#define LOG_ERROR_VERBOSE
	#endif
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
