using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;
using UnityEngine.Events;

namespace Elly
{
    [AddComponentMenu("Elly/Video Player")]
    public class EllyVideoPlayer : MonoBehaviour
    {
        [Header("Video")]
        [SerializeField] RenderTexture target;
        [Header("Audio")]
        [SerializeField] AudioSource audioSource;
        [Header("Event")]
        [SerializeField] UnityEvent StateChanged;

        int id;
        AudioClip clip;
        int position = 0;
        int samplerate = 44100;
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

        private void Awake()
        {
            id = PlayerLowLevelInterface.Application.CreatePlayer();
            PlayerLowLevelInterface.SubmitAudioSample subSample = new PlayerLowLevelInterface.SubmitAudioSample(SubmitAudioSampleCallback);
            PlayerLowLevelInterface.Application.SetSubmitAudioSampleCallback(id, Marshal.GetFunctionPointerForDelegate(subSample));
            PlayerLowLevelInterface.SubmitAudioFormat subFormat = new PlayerLowLevelInterface.SubmitAudioFormat(SubmitAudioFormatCallback);
            PlayerLowLevelInterface.Application.SetSubmitAudioFormatCallback(id, Marshal.GetFunctionPointerForDelegate(subFormat));

            PlayerLowLevelInterface.SubmitVideoSample subvSample = new PlayerLowLevelInterface.SubmitVideoSample(SubmitVideoSampleCallback);
            PlayerLowLevelInterface.Application.SetSubmitVideoSampleCallback(id, Marshal.GetFunctionPointerForDelegate(subvSample));
            PlayerLowLevelInterface.SubmitVideoFormat subvFormat = new PlayerLowLevelInterface.SubmitVideoFormat(SubmitVideoFormatCallback);
            PlayerLowLevelInterface.Application.SetSubmitVideoFormatCallback(id, Marshal.GetFunctionPointerForDelegate(subvFormat));

            PlayerLowLevelInterface.GetGlobalTime subgTime = new PlayerLowLevelInterface.GetGlobalTime(() => Time.time);
            PlayerLowLevelInterface.Application.SetGetGlobalTime(id, Marshal.GetFunctionPointerForDelegate(subgTime));
            PlayerLowLevelInterface.SubmitAsyncLoad subasync = new PlayerLowLevelInterface.SubmitAsyncLoad(AsyncLoadCallback);
            PlayerLowLevelInterface.Application.SetSubmitAsyncLoad(id, Marshal.GetFunctionPointerForDelegate(subasync));
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
            PlayerLowLevelInterface.Application.DestroyPlayer(id);
        }

        public void Play() => PlayerLowLevelInterface.Player.Play(id);
        public void Pause() => PlayerLowLevelInterface.Player.Pause(id);
        public void Stop() => PlayerLowLevelInterface.Player.Stop(id);
        public void Seek(double time) => PlayerLowLevelInterface.Player.Seek(id, time);
        public void LoadMedia(string path) => PlayerLowLevelInterface.Player.Load(id, path);
        public void LoadMedia() => PlayerLowLevelInterface.Player.Load(id);
        public void LoadMediaAsync(string path) => PlayerLowLevelInterface.Player.LoadAsync(id, path);
        public void LoadMediaAsync() => PlayerLowLevelInterface.Player.LoadAsync(id);

        void SubmitAudioFormatCallback(int channel, int sampleRate)
        {
            samplerate = sampleRate;
            audioSource.Stop();
            clip = AudioClip.Create("AudioSample", sampleRate * 2, channel, samplerate, true, OnAudioRead, OnAudioSetPosition);
            audioSource.clip = clip;
            audioSource.Play();
        }

        void SubmitAudioSampleCallback(float[] data)
        {
            int c = data.Length;
            for(int i = 0; i < c; i++)
                audioFrame.Enqueue(data[i]);
        }

        void SubmitVideoFormatCallback(int width, int height)
        {
            renderTarget = new Texture2D(width, height, TextureFormat.YUY2, false, false);
        }

        void SubmitVideoSampleCallback(byte[] bytes)
        {
            renderTarget.LoadRawTextureData(bytes);
            renderTarget.Apply();
        }

        void AsyncLoadCallback(int state)
        {

        }

        void OnAudioRead(float[] data) // fill the data and sumbit to clip
        {
            int count = 0;
            while (count < data.Length)
            {
                data[count] = audioFrame.Dequeue();
                position++;
                count++;
            }
        }

        void OnAudioSetPosition(int newPosition)
        {
            position = newPosition;
        }
    }
}
