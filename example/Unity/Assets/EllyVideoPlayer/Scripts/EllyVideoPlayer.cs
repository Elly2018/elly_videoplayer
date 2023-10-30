using AOT;
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;
using UnityEngine.Events;
using UnityEngine.UIElements;

namespace Elly
{
    //
    // Big Buck mp4
    // http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4
    // Big Buck 480p
    // https://test-streams.mux.dev/x36xhzz/url_6/193039199_mp4_h264_aac_hq_7.m3u8
    // Big Buck ABR
    // https://test-streams.mux.dev/x36xhzz/x36xhzz.m3u8
    // Elephants Dream
    // http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/ElephantsDream.mp4
    // Tear of steel (audio channel test)
    // https://storage.googleapis.com/gtv-videos-bucket/sample/TearsOfSteel.mp4
    // Bitbop ball for sync test
    // http://devimages.apple.com/iphone/samples/bipbop/bipbopall.m3u8
    //
    [AddComponentMenu("Elly/Video Player")]
    public class EllyVideoPlayer : MonoBehaviour
    {
        [Header("Video")]
        [SerializeField] RenderTexture target;
        [Header("Audio")]
        [SerializeField] AudioSource audioSource;
        [Header("Event")]
        [SerializeField] UnityEvent<int> StateChanged;
        [SerializeField] UnityEvent<Texture2D> ApplyTexture;

        int id;
        AudioClip clip;
        int position = 0;
        int samplerate = 44100;
        int channels = 2;
        Texture2D renderTarget;
        Queue<float> audioFrame;

        public float GetMediaLength => PlayerLowLevelInterface.Player.GetMediaLength(id);
        public float GetMediaPosition => PlayerLowLevelInterface.Player.GetMediaPosition(id);
        public PlayerState GetMediaState => (PlayerState)PlayerLowLevelInterface.Player.GetPlayerState(id);
        public string Path
        {
            set
            {
                PlayerLowLevelInterface.Player.SetPath(id, value);
            }
            get
            {
                return PlayerLowLevelInterface.Player.GetPath(id);
            }
        }
        public string Format
        {
            set
            {
                PlayerLowLevelInterface.Player.SetFormat(id, value);
            }
            get
            {
                return PlayerLowLevelInterface.Player.GetFormat(id);
            }
        }
        
        private void Start()
        {
            audioFrame = new Queue<float>();
            id = PlayerLowLevelInterface.Application.CreatePlayer();
            Debug.Log($"Create video player: ${id}");
            PlayerLowLevelInterface.SubmitAudioSample subSample = new PlayerLowLevelInterface.SubmitAudioSample(SubmitAudioSampleCallback);
            PlayerLowLevelInterface.Application.SetSubmitAudioSampleCallback(id, Marshal.GetFunctionPointerForDelegate(subSample));
            PlayerLowLevelInterface.SubmitAudioFormat subFormat = new PlayerLowLevelInterface.SubmitAudioFormat(SubmitAudioFormatCallback);
            PlayerLowLevelInterface.Application.SetSubmitAudioFormatCallback(id, Marshal.GetFunctionPointerForDelegate(subFormat));

            //PlayerLowLevelInterface.SubmitVideoSample subvSample = new PlayerLowLevelInterface.SubmitVideoSample(SubmitVideoSampleCallback);
            //PlayerLowLevelInterface.Application.SetSubmitVideoSampleCallback(id, Marshal.GetFunctionPointerForDelegate(subvSample));
            PlayerLowLevelInterface.SubmitVideoFormat subvFormat = new PlayerLowLevelInterface.SubmitVideoFormat(SubmitVideoFormatCallback);
            PlayerLowLevelInterface.Application.SetSubmitVideoFormatCallback(id, Marshal.GetFunctionPointerForDelegate(subvFormat));

            PlayerLowLevelInterface.StateChanged subChanged = new PlayerLowLevelInterface.StateChanged((s) =>
            {
                Debug.Log($"State changed: {(PlayerState)s}");
                if(StateChanged != null) StateChanged.Invoke(s);
            });
            PlayerLowLevelInterface.Application.SetStateChangedCallback(id, Marshal.GetFunctionPointerForDelegate(subChanged));

            PlayerLowLevelInterface.GetGlobalTime subgTime = new PlayerLowLevelInterface.GetGlobalTime(() => Time.time);
            PlayerLowLevelInterface.Application.SetGetGlobalTime(id, Marshal.GetFunctionPointerForDelegate(subgTime));
            PlayerLowLevelInterface.SubmitAsyncLoad subasync = new PlayerLowLevelInterface.SubmitAsyncLoad(AsyncLoadCallback);
            PlayerLowLevelInterface.Application.SetSubmitAsyncLoad(id, Marshal.GetFunctionPointerForDelegate(subasync));
            PlayerLowLevelInterface.AudioBufferCount subabuffer = new PlayerLowLevelInterface.AudioBufferCount(() => audioFrame.Count / channels);
            PlayerLowLevelInterface.Application.SetAudioBufferCount(id, Marshal.GetFunctionPointerForDelegate(subabuffer));
            PlayerLowLevelInterface.AudioControl aControl = new PlayerLowLevelInterface.AudioControl(AudioControlCallback);
            PlayerLowLevelInterface.Application.SetAudioControl(id, Marshal.GetFunctionPointerForDelegate(aControl));
        }

        protected virtual void Update()
        {
            PlayerLowLevelInterface.Player.Update(id);
        }

        protected virtual void FixedUpdate()
        {
            PlayerLowLevelInterface.Player.FixedUpdate(id);
        }

        private void OnDestroy()
        {
            Debug.Log($"Destroy video player: ${id}");
            PlayerLowLevelInterface.Application.DestroyPlayer(id);
        }

        public void Play() => PlayerLowLevelInterface.Player.Play(id);
        public void Pause(bool pause) => PlayerLowLevelInterface.Player.Pause(id, pause);
        public void Stop() => PlayerLowLevelInterface.Player.Stop(id);
        public void Seek(double time) => PlayerLowLevelInterface.Player.Seek(id, time);
        public void LoadMedia(string _path) => PlayerLowLevelInterface.Player.Load(id, Marshal.StringToHGlobalAnsi(_path));
        public void LoadMedia() => PlayerLowLevelInterface.Player.Load(id);
        public void LoadMediaAsync(string _path) => PlayerLowLevelInterface.Player.LoadAsync(id, Marshal.StringToHGlobalAnsi(_path));
        public void LoadMediaAsync() => PlayerLowLevelInterface.Player.LoadAsync(id);

        void SubmitAudioFormatCallback(int channel, int sampleRate)
        {
            Debug.Log($"Audio format: {channel}, {sampleRate}");
            samplerate = sampleRate;
            channels = channel;
            audioSource.Stop();
            clip = AudioClip.Create("AudioSample", samplerate * 2, channel, samplerate, true, OnAudioRead, OnAudioSetPosition);
            audioSource.clip = clip;
            audioSource.Play();
        }

        void SubmitAudioSampleCallback(int length, IntPtr ptr)
        {
            Debug.Log($"Audio data count: {length}");
            float[] data = new float[length];
            Marshal.Copy(ptr, data, 0, length);
            for (int i = 0; i < length; i++)
                audioFrame.Enqueue(data[i]);
        }

        void SubmitVideoFormatCallback(int width, int height)
        {
            Debug.Log($"Video format: {width}x{height}");
            renderTarget = new Texture2D(width, height, TextureFormat.RGB24, false, false);
            if (ApplyTexture != null) ApplyTexture.Invoke(renderTarget);
        }

        void SubmitVideoSampleCallback(int length, IntPtr ptr)
        {
            Debug.Log($"Video data count: {length}");
            byte[] bytes = new byte[length];
            Marshal.Copy(ptr, bytes, 0, length);
            renderTarget.LoadRawTextureData(bytes);
            renderTarget.Apply();
            if (ApplyTexture != null) ApplyTexture.Invoke(renderTarget);
        }

        void AudioControlCallback(int control) // 0: play, 1: pause, 2: stop, 3: clean buffer
        {
            switch (control)
            {
                case 0:
                    audioSource.Play();
                    break;
                case 1:
                    audioSource.Pause();
                    break;
                case 2:
                    audioFrame.Clear();
                    audioSource.Stop();
                    break;
                case 3:
                    audioFrame.Clear();
                    break;
            }
        }

        void AsyncLoadCallback(int state)
        {
            Play();
        }

        void OnAudioRead(float[] data) // fill the data and sumbit to clip
        {
            int count = 0;
            int channelCount = 0;
            while (count < data.Length && audioFrame.Count > 0)
            {
                data[count] = audioFrame.Dequeue();
                channelCount++;
                if (channelCount == channels)
                {
                    position++;
                    channelCount = 0;
                }
                count++;
            }
            while (count < data.Length)
            {
                data[count] = 0.0f;
                channelCount++;
                if (channelCount == channels)
                {
                    position++;
                    channelCount = 0;
                }
                count++;
            }
            //Debug.Log($"Current position: {position}");
        }

        void OnAudioSetPosition(int newPosition)
        {
            position = newPosition;
        }
    }
}
