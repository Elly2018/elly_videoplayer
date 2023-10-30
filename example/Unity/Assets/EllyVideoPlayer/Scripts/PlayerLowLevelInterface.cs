using System;
using System.Runtime.InteropServices;

namespace Elly
{
    internal struct PlayerLowLevelInterface
    {
        internal const string DLLNAME = "unity_videoplayer-d.dll";

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void SubmitAudioFormat(int channel, int sampleRate);
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void SubmitAudioSample(int length, IntPtr ptr);
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void SubmitVideoFormat(int width, int height);
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void SubmitVideoSample(int length, IntPtr ptr);
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void StateChanged(int state);
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate double GetGlobalTime();
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void SubmitAsyncLoad(int state);
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void AudioControl(int state);
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate int AudioBufferCount();

        public struct Application
        {
            [DllImport(DLLNAME, EntryPoint = "interfaceCreatePlayer", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern int CreatePlayer();
            [DllImport(DLLNAME, EntryPoint = "interfaceDestroyPlayer", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void DestroyPlayer(int id);

            [DllImport(DLLNAME, EntryPoint = "interfaceAudioSampleCallback", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void SetSubmitAudioSampleCallback(int id, IntPtr aCallback);
            [DllImport(DLLNAME, EntryPoint = "interfaceAudioSampleCallback_Clean", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void CleanAudioSampleCallback(int id);
            [DllImport(DLLNAME, EntryPoint = "interfaceAudioFormatCallback", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void SetSubmitAudioFormatCallback(int id, IntPtr aCallback);
            [DllImport(DLLNAME, EntryPoint = "interfaceAudioFormatCallback_Clean", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void CleanAudioFormatCallback(int id);

            [DllImport(DLLNAME, EntryPoint = "interfaceVideoSampleCallback", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void SetSubmitVideoSampleCallback(int id, IntPtr aCallback);
            [DllImport(DLLNAME, EntryPoint = "interfaceVideoSampleCallback_Clean", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void CleanVideoSampleCallback(int id);
            [DllImport(DLLNAME, EntryPoint = "interfaceVideoFormatCallback", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void SetSubmitVideoFormatCallback(int id, IntPtr aCallback);
            [DllImport(DLLNAME, EntryPoint = "interfaceVideoFormatCallback_Clean", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void CleanVideoFormatCallback(int id);

            [DllImport(DLLNAME, EntryPoint = "interfaceStateChangedCallback", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void SetStateChangedCallback(int id, IntPtr aCallback);
            [DllImport(DLLNAME, EntryPoint = "interfaceStateChangedCallback_Clean", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void CleanStateChangedCallback(int id);

            [DllImport(DLLNAME, EntryPoint = "interfaceGlobalTimeCallback", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void SetGetGlobalTime(int id, IntPtr aCallback);
            [DllImport(DLLNAME, EntryPoint = "interfaceAsyncLoadCallback", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void SetSubmitAsyncLoad(int id, IntPtr aCallback);
            [DllImport(DLLNAME, EntryPoint = "interfaceAudioBufferCountCallback", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void SetAudioBufferCount(int id, IntPtr aCallback);
            [DllImport(DLLNAME, EntryPoint = "interfaceAudioControlCallback", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void SetAudioControl(int id, IntPtr aCallback);
        }

        public struct Media
        {
            [DllImport(DLLNAME, EntryPoint = "interfaceSeek", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern double MediaLength(int id);
            [DllImport(DLLNAME, EntryPoint = "interfaceSeek", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern double CurrentTime(int id);
        }

        public struct Player
        {
            [DllImport(DLLNAME, EntryPoint = "interfaceGetPlayerState", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern int GetPlayerState(int id);
            [DllImport(DLLNAME, EntryPoint = "interfacePlay", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void Play(int id);
            [DllImport(DLLNAME, EntryPoint = "interfacePause", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void Pause(int id, bool pause);
            [DllImport(DLLNAME, EntryPoint = "interfaceStop", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void Stop(int id);
            [DllImport(DLLNAME, EntryPoint = "interfaceSeek", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void Seek(int id, double time);
            [DllImport(DLLNAME, EntryPoint = "interfaceLoadPath", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void Load(int id, IntPtr path);
            [DllImport(DLLNAME, EntryPoint = "interfaceLoad", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void Load(int id);
            [DllImport(DLLNAME, EntryPoint = "interfaceLoadPathAsync", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void LoadAsync(int id, IntPtr path);
            [DllImport(DLLNAME, EntryPoint = "interfaceLoadAsync", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void LoadAsync(int id);
            [DllImport(DLLNAME, EntryPoint = "interfaceUpdate", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void Update(int id);
            [DllImport(DLLNAME, EntryPoint = "interfaceFixedUpdate", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void FixedUpdate(int id);
            [DllImport(DLLNAME, EntryPoint = "interfaceLength", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern float GetMediaLength(int id);
            [DllImport(DLLNAME, EntryPoint = "interfacePosition", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern float GetMediaPosition(int id);
            [DllImport(DLLNAME, EntryPoint = "interfaceSetPath", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void SetPath(int id, string path);
            [DllImport(DLLNAME, EntryPoint = "interfaceGetPath", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern string GetPath(int id);
            [DllImport(DLLNAME, EntryPoint = "interfaceSetFormat", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void SetFormat(int id, string path);
            [DllImport(DLLNAME, EntryPoint = "interfaceGetFormat", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern string GetFormat(int id);
        }
    }

}
