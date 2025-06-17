#ifndef WIN32_HANDMADE_H_
#define WIN32_HANDMADE_H_
/* ================================================================
   $File: win32_handmade.h
   $Date: 
   $Revision:
   $Creator: Sebastian Bautista
   $Notice: 
   ================================================================*/

struct win32_frame_buffer
{
    BITMAPINFO Info;
    void* Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

struct win32_viewport_dimensions
{
    int Width;
    int Height;
};

struct win32_sound_output
{
    // NOTE(Sebas): Sound test
    int SamplesPerSecond;
    int BytesPerSample;
    uint32 RunningSampleIndex;
    DWORD SecondaryBufferSize;
    DWORD SafetyBytes;
    // TODO(Sebas): Math gets simpler if we add a "bytes per second" field ?
};

struct win32_debug_time_marker
{
    DWORD OutputPlayCursor;
    DWORD OutputWriteCursor;
    DWORD OutputLocation;
    DWORD OutputByteCount;
    DWORD ExpectedFlipPlayCursor;

    DWORD FlipPlayCursor;
    DWORD FlipWriteCursor;
};

struct win32_game_code
{
    HMODULE GameCodeDLL;
    FILETIME DLLLastWriteTime;
    game_update_and_render* UpdateAndRender;
    game_get_sound_samples* GetSoundSamples;
    bool32 IsValid;
};

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH
struct win32_state
{
    uint64 TotalSize;
    void* GameMemoryBlock;
    HANDLE RecordingHandle;
    int InputRecordingIndex;

    HANDLE PlaybackHandle;
    int InputPlaybackIndex;

    wchar_t EXEFileName[WIN32_STATE_FILE_NAME_COUNT];
    wchar_t* OnePastLastEXEFileNameSlash;
};
#endif // WIN32_HANDMADE_H_

