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
	// Sync Test
	// https://devstreaming-cdn.apple.com/videos/streaming/examples/img_bipbop_adv_example_fmp4/master.m3u8
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
        int lastposition = 0;
        int samplerate = 44100;
        int channels = 2;
        Texture2D renderTarget;
        Queue<float> audioFrame;

        public float GetMediaLength => Native.Player.GetMediaLength(id);
        public float GetMediaPosition => Native.Player.GetMediaPosition(id);
        public PlayerState GetMediaState => (PlayerState)Native.Player.GetPlayerState(id);
        public string Path
        {
            set
            {
                Native.Player.SetPath(id, value);
            }
            get
            {
                return Native.Player.GetPath(id);
            }
        }
        public string Format
        {
            set
            {
                Native.Player.SetFormat(id, value);
            }
            get
            {
                return Native.Player.GetFormat(id);
            }
        }

        // Store in here prevent GC clean them... NullReferencePointer and crash fixed
        Native.SubmitAudioSample subSample;
        Native.SubmitAudioFormat subFormat;
        Native.SubmitVideoSample subvSample;
        Native.SubmitVideoFormat subvFormat;
        Native.StateChanged subChanged;
        Native.GetGlobalTime subgTime;
        Native.SubmitAsyncLoad subasync;
        Native.AudioBufferCount subabuffer;
        Native.AudioControl aControl;

        private void Awake()
        {
            var config = AudioSettings.GetConfiguration();
            config.dspBufferSize = 2;
            AudioSettings.Reset(config);
            audioSource.loop = true;
            audioFrame = new Queue<float>();
            id = Native.Application.CreatePlayer();
            Debug.Log($"Create video player: ${id}");
            subSample = new Native.SubmitAudioSample(SubmitAudioSampleCallback);
            Native.Application.SetSubmitAudioSampleCallback(id, Marshal.GetFunctionPointerForDelegate(subSample));
            subFormat = new Native.SubmitAudioFormat(SubmitAudioFormatCallback);
            Native.Application.SetSubmitAudioFormatCallback(id, Marshal.GetFunctionPointerForDelegate(subFormat));

            subvSample = new Native.SubmitVideoSample(SubmitVideoSampleCallback);
            Native.Application.SetSubmitVideoSampleCallback(id, Marshal.GetFunctionPointerForDelegate(subvSample));
            subvFormat = new Native.SubmitVideoFormat(SubmitVideoFormatCallback);
            Native.Application.SetSubmitVideoFormatCallback(id, Marshal.GetFunctionPointerForDelegate(subvFormat));

            subChanged = new Native.StateChanged((s) =>
            {
                Debug.Log($"State changed: {(PlayerState)s}");
                if(StateChanged != null) StateChanged.Invoke(s);
            });
            Native.Application.SetStateChangedCallback(id, Marshal.GetFunctionPointerForDelegate(subChanged));

            subgTime = new Native.GetGlobalTime(() => Time.time);
            Native.Application.SetGetGlobalTime(id, Marshal.GetFunctionPointerForDelegate(subgTime));
            subasync = new Native.SubmitAsyncLoad(AsyncLoadCallback);
            Native.Application.SetSubmitAsyncLoad(id, Marshal.GetFunctionPointerForDelegate(subasync));
            subabuffer = new Native.AudioBufferCount(() => audioFrame.Count / channels);
            Native.Application.SetAudioBufferCount(id, Marshal.GetFunctionPointerForDelegate(subabuffer));
            aControl = new Native.AudioControl(AudioControlCallback);
            Native.Application.SetAudioControl(id, Marshal.GetFunctionPointerForDelegate(aControl));
        }

        protected virtual void Update()
        {
            Native.Player.Update(id);
        }

        protected virtual void FixedUpdate()
        {
            Native.Player.FixedUpdate(id);
        }

        private void OnDestroy()
        {
            Native.Application.CleanAudioSampleCallback(id);
            Native.Application.CleanAudioFormatCallback(id);
            Native.Application.CleanVideoSampleCallback(id);
            Native.Application.CleanVideoFormatCallback(id);
            Native.Application.CleanStateChangedCallback(id);
            Debug.Log($"Destroy video player: ${id}");
            Native.Application.DestroyPlayer(id);
            if (clip) Destroy(clip);
        }

        public void Play() => Native.Player.Play(id);
        public void Pause(bool pause) => Native.Player.Pause(id, pause);
        public void Stop() => Native.Player.Stop(id);
        public void Seek(double time) => Native.Player.Seek(id, time);
        public void LoadMedia(string _path) => Native.Player.Load(id, Marshal.StringToHGlobalAnsi(_path));
        public void LoadMedia() => Native.Player.Load(id);
        public void LoadMediaAsync(string _path) => Native.Player.LoadAsync(id, Marshal.StringToHGlobalAnsi(_path));
        public void LoadMediaAsync() => Native.Player.LoadAsync(id);

        void SubmitAudioFormatCallback(int channel, int sampleRate)
        {
            Debug.Log($"Audio format: {channel}, {sampleRate}");
            samplerate = sampleRate;
            channels = channel;
            audioFrame.Clear();
            audioSource.Stop();
            clip = AudioClip.Create("AudioSample", samplerate / 10, channel, samplerate, true, OnAudioRead, OnAudioSetPosition);
            audioSource.loop = true;
            audioSource.clip = clip;
            audioSource.Play();
        }

        void SubmitAudioSampleCallback(int length, IntPtr ptr)
        {
            Debug.Log($"Audio data count: {length}");
            float[] data = new float[length];
            Marshal.Copy(ptr, data, 0, length);
            for (int i = 0; i < length; i++)
            {
                audioFrame.Enqueue(data[i]);
            }
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
            renderTarget.LoadRawTextureData(ptr, length);
            renderTarget.Apply();
            if (ApplyTexture != null) ApplyTexture.Invoke(renderTarget);
        }

        void AudioControlCallback(int control) // 0: play, 1: pause, 2: stop, 3: clean buffer
        {
            Debug.Log($"Audio control: {control}");
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
            if((PlayerState)state == PlayerState.INITIALIZED)
            {
                Play();
            }
        }

        void OnAudioRead(float[] data) // fill the data and sumbit to clip
        {
            Debug.Log($"OnAudioRead {data.Length} {audioFrame.Count}");
            //int data_left = DataLeft(data.Length);
            int data_left = data.Length;
            int count = data_left;
            int writepos = 0;
            while (count > 0 && 
                audioFrame.Count > 0 &&
                GetMediaState == PlayerState.DECODING) // current cycle
            {
                data[writepos] = audioFrame.Dequeue();
                writepos++;
                count--;
            }
            /*
            while(count > 0 &&
                audioFrame.Count > 0 &&
                GetMediaState != PlayerState.DECODING)
            {
                data[writepos] = 0.0f;
                writepos++;
                count--;
            }
            */
            lastposition = position;
        }

        void OnAudioSetPosition(int newPosition)
        {
            Debug.Log($"OnAudioSetPosition: {newPosition}");
            position = newPosition;
        }

        public void MyPlay(string path)
        {
            LoadMedia(path);
            Play();
        }

        int DataLeft(int length)
        {
            int left = lastposition - position;
            int space_left = 0;
            if (left < 0) // if negative, current cycle
            {
                space_left = length + left - 1;
            }
            else if (left > 0) // If positive, another cycle
            {
                space_left = length - (position + (length - lastposition) - 1);
            }
            else if (left == 0)
            {
                space_left = length - 1;
            }
            return length - space_left - 1;
        }
    }
}
