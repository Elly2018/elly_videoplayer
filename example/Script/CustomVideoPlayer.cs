using Godot;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Threading;

public partial class CustomVideoPlayer : Node
{
    const string uri = "http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4";
    const int w = 1280;
    const int h = 720;
    const int d = 3;

    [Export] bool UseLog;
    [Export] bool UseVideo;
    [Export] GeometryInstance3D RenderTarget;
    [Export] TextureRect viewport;
    [Export] bool UseAudio;
    [Export] AudioStreamPlayer player;
    [Export] AudioStreamWav wav;
    Material material;
    ImageTexture texture;
    Image image;
    Thread thread_video;
    Thread thread_audio;
    Process proc;
    FileStream baseStream;
    double time = 0;

    public override void _Ready()
    {
        base._Ready();
        if (UseVideo)
        {
            image = Image.Create(192, 144, false, Image.Format.Rgb8);
            texture = ImageTexture.CreateFromImage(image);
            material = RenderTarget.MaterialOverride;
            material.Set("shader_parameter/tex", texture);
            viewport.Set("texture", texture);
            thread_video = new Thread(ReadVideo);
            thread_video.Start();
        }

        if (UseAudio)
        {
            player.Stream = wav;
            thread_audio = new Thread(ReadAudio);
            thread_audio.Start();
        }
    }

    public override void _Process(double delta)
    {
        base._Process(delta);
        //ColorTest(delta);
    }

    void ColorTest(double delta)
    {
        time += delta;
        byte r = (byte)(int)(Mathf.Abs(Mathf.Sin((float)time)) * 255f);
        byte g = (byte)(int)(Mathf.Abs(Mathf.Cos((float)time)) * 255f);
        image.SetData(1, 1, false, Image.Format.Rgb8, new byte[3] { r, g, 0 });
        ApplyImage();
    }

    void ReadVideo()
    {
        GD.Print("Start FFmpeg thread");
        proc = new Process();
        proc.StartInfo.FileName = "ffmpeg";
        proc.StartInfo.Arguments = $"-hwaccel cuda -an -hide_banner -re -i \"{uri}\" -c rawvideo -video_size {w}x{h} -pix_fmt rgb24 -f rawvideo pipe:1";
        //proc.StartInfo.Arguments = $"-hide_banner -i \"{uri}\"";
        proc.StartInfo.UseShellExecute = false;
        proc.StartInfo.RedirectStandardInput = true;
        proc.StartInfo.RedirectStandardOutput = true;
        proc.OutputDataReceived += Proc_OutputDataReceived;
        proc.ErrorDataReceived += Proc_OutputDataReceived;
        proc.Start();

        baseStream = proc.StandardOutput.BaseStream as FileStream;
        int lastRead = 0;
        int counter = 0;
        int bytelength = w * h * d;

        List<byte> total = new List<byte>();
        do
        {
            byte[] buffer = new byte[bytelength];
            lastRead = baseStream.Read(buffer, 0, bytelength);
            total.AddRange(buffer.Take(lastRead).ToArray());
            if (total.Count >= bytelength)
            {
                image.CallDeferred("set_data", w, h, false, (int)Image.Format.Rgb8, total.ToArray());
                ApplyImage();
                total.Clear();
                counter++;
            }
            //GD.Print($"length: {bytelength}  {lastRead}");
        } while (lastRead > 0);
        GD.Print($"read counter: {counter}");
    }
    void ReadAudio()
    {
        GD.Print("Start FFmpeg thread");
        proc = new Process();
        proc.StartInfo.FileName = "ffmpeg";
        proc.StartInfo.Arguments = $"-vn -re -i \"{uri}\" -c pcm_f32be -f f32be pipe:1";
        //proc.StartInfo.Arguments = $"-hide_banner -i \"{uri}\"";
        proc.StartInfo.UseShellExecute = false;
        proc.StartInfo.RedirectStandardInput = true;
        proc.StartInfo.RedirectStandardOutput = true;
        proc.OutputDataReceived += Proc_OutputDataReceived;
        proc.ErrorDataReceived += Proc_OutputDataReceived;
        proc.Start();

        baseStream = proc.StandardOutput.BaseStream as FileStream;
        int lastRead = 0;
        int counter = 0;
        int bytelength = 1024;

        List<byte> total = new List<byte>();
        do
        {
            byte[] buffer = new byte[bytelength];
            lastRead = baseStream.Read(buffer, 0, bytelength);
            total.AddRange(buffer.Take(lastRead).ToArray());
            if (total.Count >= bytelength)
            {
                wav.Data = total.ToArray();
                player.Play();
                total.Clear();
                counter++;
            }
            GD.Print($"length: {bytelength}  {lastRead}");
        } while (lastRead > 0);
        GD.Print($"read counter: {counter}");
    }

    void ApplyImage()
    {
        texture.SetDeferred("image", image);
        material.SetDeferred("shader_parameter/tex", texture);
        viewport.SetDeferred("texture", texture);
    }

    public override void _ExitTree()
    {
        base._ExitTree();
        if(thread_video != null && thread_video.ThreadState.HasFlag(System.Threading.ThreadState.Running))
        {
            thread_video.Join(1000);
        }
    }

    private void Proc_OutputDataReceived(object sender, DataReceivedEventArgs e)
    {
        GD.Print($"[FFmpeg] {e.Data}");
    }
}
