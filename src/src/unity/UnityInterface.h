/*
* Basically, the interface is where unity c# script interpolate with.
*/
#pragma once
#include "Unity/IUnityInterface.h"
#include "Unity/IUnityGraphics.h"
#include <Logger.h>

// channel, sample rate
typedef void (UNITY_INTERFACE_API *SubmitAudioFormat)(int, int);
// raw float data
typedef void (UNITY_INTERFACE_API *SubmitAudioSample)(int, float*);
// width, height
typedef void (UNITY_INTERFACE_API *SubmitVideoFormat)(int, int);
// raw byte data
typedef void (UNITY_INTERFACE_API *SubmitVideoSample)(int, char*);
// Get app time in second
typedef double (UNITY_INTERFACE_API *GetGlobalTime)();
// The callback for async load
typedef void (UNITY_INTERFACE_API *AsyncLoad)(int);
// Control audio source
typedef void (UNITY_INTERFACE_API *AudioControl)(int);
typedef int (UNITY_INTERFACE_API *AudioBufferCount)();
typedef void (UNITY_INTERFACE_API *StateChanged)(int);

extern "C" {
	//
	//
	// Plugin
	//
	//
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API UnityPluginUnload();
	//
	//
	// Application
	//
	//
	UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API interfaceCreatePlayer();
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceDestroyPlayer(int id);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceAudioSampleCallback(int id, SubmitAudioSample func);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceAudioSampleCallback_Clean(int id);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceAudioFormatCallback(int id, SubmitAudioFormat func);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceAudioFormatCallback_Clean(int id);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceVideoSampleCallback(int id, SubmitVideoSample func);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceVideoSampleCallback_Clean(int id);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceVideoFormatCallback(int id, SubmitVideoFormat func);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceVideoFormatCallback_Clean(int id);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceStateChangedCallback(int id, StateChanged func);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceStateChangedCallback_Clean(int id);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceGlobalTimeCallback(int id, GetGlobalTime func);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceAsyncLoadCallback(int id, AsyncLoad func);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceAudioBufferCountCallback(int id, AudioBufferCount func);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceAudioControlCallback(int id, AudioControl func);
	//
	//
	// Media
	//
	//
	UNITY_INTERFACE_EXPORT double UNITY_INTERFACE_API interfaceMediaLength(int id);
	UNITY_INTERFACE_EXPORT double UNITY_INTERFACE_API interfaceGetCurrentTime(int id);
	//
	//
	// Player
	//
	//
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceUpdate(int id);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceFixedUpdate(int id);
	UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API interfaceGetPlayerState(int id);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfacePlay(int id);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfacePause(int id, bool pause);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceStop(int id);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceSeek(int id, double time);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceLoad(int id);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceLoadAsync(int id);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceLoadPath(int id, const char* path);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceLoadPathAsync(int id, const char* path);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceSetPath(int id, const char* path);
	UNITY_INTERFACE_EXPORT const char* UNITY_INTERFACE_API interfaceGetPath(int id);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceSetFormat(int id, const char* format);
	UNITY_INTERFACE_EXPORT const char* UNITY_INTERFACE_API interfaceGetFormat(int id);
}
