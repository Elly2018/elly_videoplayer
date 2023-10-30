using System;
using System.Runtime.InteropServices;

namespace Elly
{
    internal struct PlayerLowLevelInterface
    {
        public delegate void SubmitAudioFormat(int channel, int sampleRate);
        public delegate void SubmitAudioSample(float[] data);
        public delegate void SubmitVideoFormat(int width, int height);
        public delegate void SubmitVideoSample(byte[] data);
        public delegate double GetGlobalTime();
        public delegate void SubmitAsyncLoad(int state);

        public struct Application
        {
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceCreatePlayer", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern int CreatePlayer();
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceDestroyPlayer", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void DestroyPlayer(int id);

            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceAudioSampleCallback", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void SetSubmitAudioSampleCallback(int id, IntPtr aCallback);
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceAudioSampleCallback_Clean", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void CleanAudioSampleCallback(int id);
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceAudioFormatCallback", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void SetSubmitAudioFormatCallback(int id, IntPtr aCallback);
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceAudioFormatCallback_Clean", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void CleanAudioFormatCallback(int id);

            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceVideoSampleCallback", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void SetSubmitVideoSampleCallback(int id, IntPtr aCallback);
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceVideoSampleCallback_Clean", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void CleanVideoSampleCallback(int id);
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceVideoFormatCallback", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void SetSubmitVideoFormatCallback(int id, IntPtr aCallback);
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceVideoFormatCallback_Clean", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void CleanVideoFormatCallback(int id);
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceGlobalTimeCallback", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void SetGetGlobalTime(int id, IntPtr aCallback);
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceAsyncLoadCallback", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void SetSubmitAsyncLoad(int id, IntPtr aCallback);
        }

        public struct Media
        {
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceSeek", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern double MediaLength(int id);
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceSeek", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern double CurrentTime(int id);
        }

        public struct Player
        {
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceGetPlayerState", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern int GetPlayerState(int id);
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfacePlay", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void Play(int id);
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfacePause", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void Pause(int id);
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceStop", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void Stop(int id);
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceSeek", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void Seek(int id, double time);
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceLoadPath", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void Load(int id, string path);
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceLoad", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void Load(int id);
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceLoadPathAsync", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void LoadAsync(int id, string path);
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceLoadAsync", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void LoadAsync(int id);
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceUpdate", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void Update(int id);
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceFixedUpdate", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void FixedUpdate(int id);
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceLength", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern float GetMediaLength(int id);
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfacePosition", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern float GetMediaPosition(int id);
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceSetPath", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void SetPath(int id, string path);
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceGetPath", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern string GetPath(int id);
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceSetFormat", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void SetFormat(int id, string path);
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceGetFormat", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern string GetFormat(int id);
        }
    }

}
