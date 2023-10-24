using System.Runtime.InteropServices;

public struct EllyVideoPlayer
{
    [DllImport("unity_videoplayer-d.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern void Play(int id);
    [DllImport("unity_videoplayer-d.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern void Pause(int id);
    [DllImport("unity_videoplayer-d.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern void stop(int id);
    [DllImport("unity_videoplayer-d.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern void load(int id);
    [DllImport("unity_videoplayer-d.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern void set_sample_rate(int id);
    [DllImport("unity_videoplayer-d.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Unicode)]
    public static extern float get_timer(int id);
}
