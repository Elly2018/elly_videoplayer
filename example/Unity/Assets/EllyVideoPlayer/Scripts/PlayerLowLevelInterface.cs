using System;
using System.Runtime.InteropServices;

namespace Elly
{
    public struct PlayerLowLevelInterface
    {
        [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceCreatePlayer", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern int CreatePlayer();
        [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceDestroyPlayer", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern void DestroyPlayer(int id);
        [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceGetPlayerState", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern int GetPlayerState(int id);
        [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfacePlay", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern void Play(int id);
        [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfacePause", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern void Pause(int id);
        [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfaceStop", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern void Stop(int id);
        [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfacePlay", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern void Load(int id);
        [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfacePlay", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern void SetSampleRate(int id);
        [DllImport("unity_videoplayer-d.dll", EntryPoint = "interfacePlay", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern float GetTimer(int id);

        public delegate void SubmitAudioSample(float[] audioData, int channel, int sampleRate);
        [DllImport("unity_videoplayer-d.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        public static extern void setSubmitAudioSampleCallback(int id, IntPtr aCallback);

        public void callback01(string message)
        {
            System.Console.Write("callback 01 called. Message: " + message + "\n");
        }
    }

}
