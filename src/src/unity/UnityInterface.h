/*
* Basically, the interface is where unity c# script interpolate with.
*/
#pragma once
#include "Unity/IUnityInterface.h"
#include "Unity/IUnityGraphics.h"

typedef void (UNITY_INTERFACE_API* LogFunc)(const char* log);

extern "C" UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API interfaceRegisterDebug(LogFunc logfunc);
extern "C" UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API interfaceCreatePlayer();
extern "C" UNITY_INTERFACE_EXPORT int UNITY_INTERFACE_API interfaceGetPlayerState(int id);
extern "C" UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceDestroyPlayer(int id);
extern "C" UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfacePlay(int id);
extern "C" UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfacePause(int id);
extern "C" UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceStop(int id);
extern "C" UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceSeek(int id, double time);
extern "C" UNITY_INTERFACE_EXPORT double UNITY_INTERFACE_API interfaceGetCurrentTime(int id);
extern "C" UNITY_INTERFACE_EXPORT double UNITY_INTERFACE_API interfaceMediaLength(int id);
extern "C" UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceLoadPath(int id, const char* path);
extern "C" UNITY_INTERFACE_EXPORT void UNITY_INTERFACE_API interfaceLoadPathAsync(int id, const char* path);
