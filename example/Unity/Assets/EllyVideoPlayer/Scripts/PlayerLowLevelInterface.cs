using System;
using System.Runtime.InteropServices;

namespace Elly
{
    internal struct PlayerLowLevelInterface
    {
        public delegate void SubmitAudioSample(float[] audioData, int channel, int sampleRate);

        public struct Application
        {
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceCreatePlayer", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern int CreatePlayer();
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceDestroyPlayer", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void DestroyPlayer(int id);

            [DllImport("unity_videoplayer-d.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void SetSubmitAudioSampleCallback(int id, IntPtr aCallback);

            internal static void SetSubmitAudioSampleCallback(int id, object marshall)
            {
                throw new NotImplementedException();
            }
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
            [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceLoadPathAsync", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            public static extern void LoadAsync(int id, string path);
        }
    }

}
