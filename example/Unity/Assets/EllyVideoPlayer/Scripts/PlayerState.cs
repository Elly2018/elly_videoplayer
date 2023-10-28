namespace Elly
{
    public enum PlayerState
    {
        FAILED = -1,
        LOADING,
        UNINITIALIZED,
        INITIALIZED,
        DECODING,
        SEEK,
        BUFFERING,
        END_OF_FILE,
    }
}
