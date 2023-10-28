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

        private void Awake()
        {
            id = PlayerLowLevelInterface.Application.CreatePlayer();
            PlayerLowLevelInterface.SubmitAudioSample subSample = new PlayerLowLevelInterface.SubmitAudioSample(SubmitAudioSampleCallback);
            PlayerLowLevelInterface.Application.SetSubmitAudioSampleCallback(id, Marshal.GetFunctionPointerForDelegate(subSample));
            PlayerLowLevelInterface.SubmitAudioFormat subFormat = new PlayerLowLevelInterface.SubmitAudioFormat(SubmitAudioFormatCallback);
            PlayerLowLevelInterface.Application.SetSubmitAudioFormatCallback(id, Marshal.GetFunctionPointerForDelegate(subFormat));
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
        public void LoadMediaAsync(string path) => PlayerLowLevelInterface.Player.LoadAsync(id, path);

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

        void SubmitVideoSampleCallback(int width, int height, byte[] data)
        {
            renderTarget = new Texture2D(width, height, TextureFormat.YUY2, false, false);
            renderTarget.LoadRawTextureData(data);
        }
    }
}
