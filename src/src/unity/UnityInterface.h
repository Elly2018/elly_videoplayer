/*
* Basically, the interface is where unity c# script interpolate with.
*/
#pragma once
#include "Unity/IUnityInterface.h"
#include "Unity/IUnityGraphics.h"
#include <Logger.h>

// channel, sample rate
typedef void (*SubmitAudioSample)(int, int);
// raw float data
typedef void (*SubmitAudioFormat)(float*);

extern "C" {
	// Plusin
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API UnityPluginUnload();

	// Application
	UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API interfaceCreatePlayer();
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceDestroyPlayer(int id);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceAudioSampleCallback(int id, SubmitAudioSample func);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceAudioSampleCallback_Clean(int id);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceAudioFormatCallback(int id, SubmitAudioSample func);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceAudioFormatCallback_Clean(int id);

	// Media
	UNITY_INTERFACE_EXPORT double UNITY_INTERFACE_API interfaceMediaLength(int id);
	UNITY_INTERFACE_EXPORT double UNITY_INTERFACE_API interfaceGetCurrentTime(int id);

	// Player
	UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API interfaceGetPlayerState(int id);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfacePlay(int id);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfacePause(int id);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceStop(int id);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceSeek(int id, double time);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceLoadPath(int id, const char* path);
	UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceLoadPathAsync(int id, const char* path);
}
