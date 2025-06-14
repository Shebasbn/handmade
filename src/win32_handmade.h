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

#endif // WIN32_HANDMADE_H_

