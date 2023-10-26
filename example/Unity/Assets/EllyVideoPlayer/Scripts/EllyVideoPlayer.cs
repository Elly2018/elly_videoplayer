using UnityEngine;

namespace Elly
{
    [AddComponentMenu("Elly/Video Player")]
    public class EllyVideoPlayer : MonoBehaviour
    {
        int id;

        private void Awake()
        {
            id = PlayerLowLevelInterface.CreatePlayer();
        }

        private void OnDestroy()
        {
            PlayerLowLevelInterface.DestroyPlayer(id);
        }
    }
}
