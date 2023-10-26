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

        private void Awake()
        {
            id = PlayerLowLevelInterface.Application.CreatePlayer();
            PlayerLowLevelInterface.SubmitAudioSample sub = new PlayerLowLevelInterface.SubmitAudioSample(SubmitAudioSampleCallback);
            PlayerLowLevelInterface.Application.SetSubmitAudioSampleCallback(id, Marshal.GetFunctionPointerForDelegate(sub));
        }

        private void OnDestroy()
        {
            PlayerLowLevelInterface.Application.DestroyPlayer(id);
        }

        public void Play() => PlayerLowLevelInterface.Player.Play(id);
        public void Pause() => PlayerLowLevelInterface.Player.Pause(id);
        public void Stop() => PlayerLowLevelInterface.Player.Stop(id);
        public void Seek(double time) => PlayerLowLevelInterface.Player.Seek(id, time);

        void SubmitAudioSampleCallback(float[] audioData, int channel, int sampleRate)
        {

        }
    }
}
