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
    int SecondaryBufferSize;
    int LatencySampleCount;
};

struct win32_debug_time_marker
{
    DWORD PlayCursor;
    DWORD WriteCursor;
};

#endif // WIN32_HANDMADE_H_

