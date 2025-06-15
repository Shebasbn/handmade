/* ================================================================
   $File: $
   $Date: $
   $Revision:
   $Creator: Sebastian Bautista $
   $Notice: $
   ================================================================*/
/*
 * TODO(Sebas): THIS IS NOT A FINAL PLATFORM LAYER!!!
 * - Saved game locations
 * - Getting a handle to our own executable file
 * - Asset loading path
 * - Threading (launch a thread)
 * - Raw Input (support for multiple keyboards)
 * - Sleep/timeBeginPeriod
 * - ClipCursor() (For multimonitor support)
 * - Fullscreen support
 * - WM_SETCURSOR (control cursor visibility)
 * - QueryCancelAutoplay
 * - WM_ACTIVATE (for when we are not the active application)
 * - Bllit speed improvements (BlitBit)
 * - Hardware acceleration (OpenGL or Direct3D or BOTH??)
 * - GetKeyboardLayout (for French keyboards, international WASD support) 
 *
 *  Just a partial list of stuff! 
 */
#include "handmade.h"

#include <windows.h>
#include <xinput.h>
#include <dsound.h>
#include <stdio.h>
#include <malloc.h>

#include "win32_handmade.h" 

// TODO(Sebas):  This is a global for now.
global_variable bool32 GlobalRunning;
global_variable bool32 GlobalPause;
global_variable win32_frame_buffer GlobalFrameBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable int64 GlobalPerfCountFrequency;

// NOTE(Sebas): XInputGetState 
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
} 
global_variable x_input_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// NOTE(Sebas): XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state* XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN  pUnkOuter )
typedef DIRECT_SOUND_CREATE(direct_sound_create);

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
    if (Memory)
    {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
    debug_read_file_result Result = {};
    HANDLE FileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0); 
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize = {};
        if (GetFileSizeEx(FileHandle, &FileSize))
        {
            Result.ContentsSize = SafeTruncateUInt64(FileSize.QuadPart);
            Result.Contents = VirtualAlloc(0, Result.ContentsSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (Result.Contents)
            {
                DWORD BytesRead = 0;
                if (ReadFile(FileHandle, Result.Contents, Result.ContentsSize, &BytesRead, 0) && (Result.ContentsSize == BytesRead))
                {
                    // NOTE(Sebas): File read successfully 
                }
                else
                {
                    // TODO(Sebas): Logging
                    // VirtualFree(Result.Contents, 0, MEM_RELEASE);
                    DEBUGPlatformFreeFileMemory(Result.Contents);
                    Result = {};
                }
            }
            else
            {
                // TODO(Sebas): Logging
            }
        }
        else
        {
            // TODO(Sebas): Logging
        }
        CloseHandle(FileHandle);
    }
    else
    {
       // TODO(Sebas): Logging
    }
    return Result;
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
{
    bool32 Result = false;
    HANDLE FileHandle = CreateFileA(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0); 
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten = 0;
        if (WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
        {
            // NOTE(Sebas): File read successfully 
            Result = BytesWritten == MemorySize;
        }
        else
        {
            // TODO(Sebas): Logging
        }
        CloseHandle(FileHandle);
    }
    else
    {
        // TODO(Sebas): Logging
    }
    return Result;
}

inline FILETIME
Win32GetLastWriteTime(wchar_t* Filename)
{
    FILETIME LastWriteTime = {};

    WIN32_FIND_DATAW FileData;
    HANDLE FindHandle = FindFirstFile(Filename, &FileData);
    if (FindHandle != INVALID_HANDLE_VALUE)
    {
        LastWriteTime = FileData.ftLastWriteTime;
        FindClose(FindHandle);
    }

    return LastWriteTime;
}
internal win32_game_code
Win32LoadGameCode(wchar_t* SourceDLLName, wchar_t* TempDLLName)
{
    win32_game_code Result = {};

    // TODO(Sebas): Need to get the proper path here!
    // TODO(Sebas): Automatic determination of when updates are necessary.
    Result.DLLLastWriteTime = Win32GetLastWriteTime(SourceDLLName);
    CopyFile(SourceDLLName, TempDLLName, FALSE);
    Result.GameCodeDLL = LoadLibrary(TempDLLName);
    if (Result.GameCodeDLL)
    {
        Result.UpdateAndRender = (game_update_and_render*)GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");
        Result.GetSoundSamples = (game_get_sound_samples*)GetProcAddress(Result.GameCodeDLL, "GameGetSoundSamples");

        Result.IsValid = (Result.UpdateAndRender && Result.GetSoundSamples);
    }
    if (!Result.IsValid)
    {
        Result.UpdateAndRender = GameUpdateAndRenderStub;
        Result.GetSoundSamples = GameGetSoundSamplesStub;
    }

    return Result;
}

internal void
Win32UnloadGameCode(win32_game_code* GameCode)
{
    if (GameCode->GameCodeDLL)
    {
        FreeLibrary(GameCode->GameCodeDLL);
        GameCode->GameCodeDLL = 0;
    }
    GameCode->IsValid = false;
    GameCode->UpdateAndRender = GameUpdateAndRenderStub;
    GameCode->GetSoundSamples = GameGetSoundSamplesStub;

}

internal void
Win32LoadXInput()
{
    HMODULE XInputLibrary = LoadLibrary(L"xinput1_4.dll");
    if(!XInputLibrary)
    {
        // TODO(Sebas): Diagnostic
        XInputLibrary = LoadLibrary(L"xinput9_1_0.dll");
    } 
    if(!XInputLibrary)
    {
        // TODO(Sebas): Diagnostic
        XInputLibrary = LoadLibrary(L"xinput1_3.dll");
    } 


    if(XInputLibrary)
    {
        XInputGetState = (x_input_get_state*)GetProcAddress(XInputLibrary, "XInputGetState");
        if(!XInputGetState) {XInputGetState = XInputGetStateStub;}
        XInputSetState = (x_input_set_state*)GetProcAddress(XInputLibrary, "XInputSetState");
        if(!XInputSetState) {XInputSetState = XInputSetStateStub;}
        // TODO(Sebas): Diagnostic
    }
    else
    {
        // TODO(Sebas): Diagnostic
    }

}

internal void
Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
    // NOTE(Sebas): Load the library
    HMODULE DSoundLibrary = LoadLibrary(L"dsound.dll");

    if(DSoundLibrary)
    {
        // NOTE(Sebas): GetDirectSound object!
        direct_sound_create* DirectSoundCreate = (direct_sound_create*)GetProcAddress(DSoundLibrary, "DirectSoundCreate");
        LPDIRECTSOUND DirectSound;
        if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
        {
            WAVEFORMATEX Format = {};
            Format.wFormatTag = WAVE_FORMAT_PCM;
            Format.nChannels = 2;
            Format.nSamplesPerSec = SamplesPerSecond;
            Format.wBitsPerSample = 16;
            Format.nBlockAlign = (Format.nChannels * Format.wBitsPerSample)/8;
            Format.nAvgBytesPerSec = Format.nSamplesPerSec * Format.nBlockAlign;
            if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
            {
                // NOTE(Sebas): "Create" a primary buffer
                // TODO(Sebas): More flags?
                DSBUFFERDESC BufferDesc = {};
                BufferDesc.dwSize = sizeof(DSBUFFERDESC);
                BufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
                BufferDesc.guid3DAlgorithm = DS3DALG_DEFAULT;
                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDesc, &PrimaryBuffer, 0)))
                {
                    if(SUCCEEDED(PrimaryBuffer->SetFormat(&Format)))
                    {
                        // NOTE(Sebas): We have finally set the format!
                        OutputDebugString(L"Primary buffer format was set.\n");
                    }
                    else
                    {
                        // TODO(Sebas): Diagnostic
                    }
                }
                else
                {
                    // TODO(Sebas): Diagnostic
                }
            }
            else
            {
                // TODO(Sebas): Diagnostic
            }
            // NOTE(Sebas): "Create" a secondary buffer
            DSBUFFERDESC BufferDesc = {};
            BufferDesc.dwSize = sizeof(DSBUFFERDESC);
            BufferDesc.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
            BufferDesc.dwBufferBytes = BufferSize;
            BufferDesc.guid3DAlgorithm = DS3DALG_DEFAULT;
            BufferDesc.lpwfxFormat = &Format;
            if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDesc, &GlobalSecondaryBuffer, 0)))
            {
                OutputDebugString(L"Secondary buffer created successfully.\n");
            }
            else
            {
                // TODO(Sebas): Diagnostic
            }
        }
        else
        {
            // TODO(Sebas): Diagnostic
        }
    }
}

internal win32_viewport_dimensions
Win32GetViewportDimensions(HWND Window)
{
    win32_viewport_dimensions Result;
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;
    return Result;
}

internal void
Win32ResizeDIBSection(win32_frame_buffer* Buffer, int Width, int Height)
{

    // TODO(Sebas):  Bulletproof this.
    // Maybe don't free first, free after, then free first if that fails
    if (Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;
    Buffer->BytesPerPixel = 4;
    Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;

    // NOTE(Sebas):
    // Windows knows this is a top-down bitmap
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    int BufferMemorySize = (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, BufferMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

}

internal void
Win32DisplayBufferInWindow(HDC DeviceContext, 
                           int ViewportWidth, 
                           int ViewportHeight, 
                           win32_frame_buffer* Buffer)
{
    // TODO(Sebas): Aspect ratio correction  
    // TODO(Sebas): Play with stretch modes 
    StretchDIBits(DeviceContext,
                  0, 0, ViewportWidth, ViewportHeight, 
                  0, 0, Buffer->Width, Buffer->Height,
                  Buffer->Memory,
                  &Buffer->Info,
                  DIB_RGB_COLORS,
                  SRCCOPY);
}

internal void
Win32ClearBuffer(win32_sound_output* SoundOutput)
{
    VOID* Region1;
    DWORD Region1Size;
    VOID* Region2;
    DWORD Region2Size;
    HRESULT Result = GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize, 
                                                 &Region1, &Region1Size, 
                                                 &Region2, &Region2Size, 
                                                 0);
    if(SUCCEEDED(Result))
    {
        uint8* DestSample = (uint8*)Region1;
        for(DWORD ByteIndex = 0; ByteIndex < Region1Size; ByteIndex++)
        {
            *DestSample++ = 0;
        }
        DestSample = (uint8*)Region2;
        for(DWORD ByteIndex = 0; ByteIndex < Region2Size; ByteIndex++)
        {
            *DestSample++ = 0;
        }
        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

internal void
Win32FillSoundBuffer(win32_sound_output* SoundOutput, 
                     DWORD BytesToLock, DWORD BytesToWrite, 
                     game_sound_output_buffer* SourceBuffer)
{
    VOID* Region1;
    DWORD Region1Size;
    VOID* Region2;
    DWORD Region2Size;

    HRESULT Result = GlobalSecondaryBuffer->Lock(BytesToLock, BytesToWrite, 
                    &Region1, &Region1Size, 
                    &Region2, &Region2Size, 
                    0);
    if(SUCCEEDED(Result))
    {
        // TODO(Sebas): Assert
        DWORD Region1SampleCount = Region1Size/SoundOutput->BytesPerSample;
        int16* DestSample = (int16*)Region1;
        int16* SourceSample = SourceBuffer->Samples;
        for(DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; SampleIndex++)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }
        DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSample;
        DestSample = (int16*)Region2;
        for(DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; SampleIndex++)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }
        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
    else
    {
        // TODO(Sebas):  Logging
    }
}

internal void
Win32ProcessKeyboardMessage(game_button_state* NewState, bool32 IsDown)
{
    Assert(NewState->EndedDown != IsDown);
    NewState->EndedDown = IsDown;
    ++NewState->HalfTransitionCount; 
}

internal void
Win32ProcessXInputDigitalButton(game_button_state* OldState, game_button_state* NewState, 
                                 DWORD XInputButtonState, DWORD ButtonBit)
{
    NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
    NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0; 
}

internal real32
Win32ProcessXInputStickValue(SHORT Value, SHORT DeadzoneThreshold)
{
    real32 Result = 0;

    if (Value < -DeadzoneThreshold)
    {
        Result = (real32)((Value + DeadzoneThreshold) / (32768.0f - DeadzoneThreshold));
    }
    else if (Value > DeadzoneThreshold)
    {
        Result = (real32)((Value - DeadzoneThreshold) / (32767.0f - DeadzoneThreshold));
    }
    return Result;
}

// internal void
// Win32ProcessXInputStickValue(game_stick_state* Stick, SHORT X, SHORT Y, SHORT ThumbDeadzone)
// {
//     float Magnitude = (real32)sqrt(X*X + Y*Y);
//     float NormalizedMagnitude = 0;
//
//     float normalizedX = X / Magnitude;
//     float normalizedY = Y / Magnitude;
//
//     if (Magnitude > ThumbDeadzone)
//     {
//         if (Magnitude > 32767) Magnitude = 32767;
//         Magnitude -= ThumbDeadzone;
//         NormalizedMagnitude = Magnitude / (32767 - ThumbDeadzone);
//
//     }
//     else
//     {
//         Magnitude = 0;
//     }
//
//     Stick->AverageX = normalizedX * NormalizedMagnitude;
//     Stick->AverageY = normalizedY * NormalizedMagnitude;
// }

internal void
Win32ProcessPendingMessages(game_controller_input* KeyboardController)
{
    MSG Message;
    while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch(Message.message)
        {
            case WM_QUIT:
            {
                GlobalRunning = false;
            } break;
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                KeyboardController->IsAnalog = false;
                uint32 VKCode = (uint32)Message.wParam;
                #define KeyMessageWasDownBit (1 << 30)
                #define KeyMessageIsDownBit (1 << 31)
                bool32 WasDown = ((Message.lParam & KeyMessageWasDownBit) != 0);
                bool32 IsDown = ((Message.lParam & KeyMessageIsDownBit) == 0);
                if(WasDown != IsDown)
                {
                    if(VKCode == 'W')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown);
                    }
                    if(VKCode == 'A')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
                    }
                    if(VKCode == 'S')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown);
                    }
                    if(VKCode == 'D')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown);
                    }
                    if(VKCode == 'Q')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder, IsDown);
                    }
                    if(VKCode == 'E')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->RightShoulder, IsDown);
                    }
                    if(VKCode == VK_UP)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionUp, IsDown);
                    }
                    if(VKCode == VK_DOWN)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionDown, IsDown);
                    }
                    if(VKCode == VK_LEFT)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionLeft, IsDown);
                    }
                    if(VKCode == VK_RIGHT)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown);
                    }
                    if(VKCode == VK_ESCAPE)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->Back, IsDown);
                    }
                    if(VKCode == VK_SPACE)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->Start, IsDown);
                    }
#if HANDMADE_INTERNAL
                    if(VKCode == 'P')
                    {
                        if (IsDown)
                        {
                            GlobalPause = !GlobalPause;
                        }
                    }
#endif
                }

                bool32 AltKeyWasDown = (Message.lParam & (1 << 29));
                if ((VKCode == VK_F4) && (AltKeyWasDown))
                {
                    GlobalRunning = false;
                }

            } break;
            default:
            {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            } break;
        }
    }
}

internal LRESULT CALLBACK
Win32MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    switch(Message)
    {
        case WM_SIZE:
        {
            OutputDebugString(L"WM_SIZE\n");
        } break;

        case WM_CLOSE:
        {
            // TODO(Sebas):  Handle this with a message to the user?
            GlobalRunning = false;
            OutputDebugString(L"WM_CLOSE\n");
        } break;

        case WM_DESTROY:
        {
            // TODO(Sebas): Handle this as an error - recreate window?  
            GlobalRunning = false;
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugString(L"WM_ACTIVATEAPP:\n");
        } break;
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        {
        } break;
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            Assert(!"Keyboard input came in through a non-dispatch message!");
        
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            win32_viewport_dimensions Viewport = Win32GetViewportDimensions(Window);
            Win32DisplayBufferInWindow(DeviceContext, Viewport.Width, Viewport.Height, &GlobalFrameBuffer);
            EndPaint(Window, &Paint);
        } break;
        default:
        {
            // OutputDebugString(L"default\n");
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }
    return Result;
}

inline LARGE_INTEGER
Win32GetWallClock()
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return Result;
}

inline real32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    real32 Result = ((real32)(End.QuadPart - Start.QuadPart) /
                             (real32)GlobalPerfCountFrequency);
    return Result;
}

internal void
Win32DebugDrawVertical(win32_frame_buffer* FrameBuffer, int X, int Top, int Bottom, uint32 Color)
{
    if (Top <= 0)
    {
        Top = 0;
    }
    
    if (Bottom > FrameBuffer->Height)
    {
        Bottom = FrameBuffer->Height;
    }
    
    if ((X >= 0) && (X < FrameBuffer->Width))
    {
        uint8* Pixel = ((uint8*)FrameBuffer->Memory + 
                X * FrameBuffer->BytesPerPixel + 
                Top*FrameBuffer->Pitch);
        for (int Y = Top; Y < Bottom; Y++)
        {
            *((uint32*)Pixel) = Color;
            Pixel += FrameBuffer->Pitch;
        }
    }
}

internal void
Win32DrawSoundBufferMarker(win32_frame_buffer* FrameBuffer, win32_sound_output* SoundOutput, 
                           DWORD Cursor, real32 C, int PadX, int Top, int Bottom, uint32 Color)
{
        real32 XReal32 = (C * (real32)Cursor);
        int X = PadX + (int)(XReal32);
        Win32DebugDrawVertical(FrameBuffer, X, Top, Bottom, Color);
}

internal void
Win32DebugSyncDisplay(win32_frame_buffer* FrameBuffer, win32_sound_output* SoundOutput, 
                      win32_debug_time_marker* Markers, int MarkerCount, int CurrentMarkerIndex, 
                      real32 TargetSecondsPerFrame)
{
    int PadX = 16;
    int PadY = 16;

    int LineHeight = 64;

    real32 C = (real32)(FrameBuffer->Width - 2 * PadX) / (real32)SoundOutput->SecondaryBufferSize;
    for (int DebugTimeMarkerIndex = 0;
         DebugTimeMarkerIndex < MarkerCount; 
         DebugTimeMarkerIndex++)
    {
        win32_debug_time_marker* ThisMarker = &Markers[DebugTimeMarkerIndex];

        Assert(ThisMarker->OutputPlayCursor < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputWriteCursor < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputLocation < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputByteCount < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->FlipPlayCursor < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->FlipWriteCursor < SoundOutput->SecondaryBufferSize);

        DWORD PlayColor = 0xFFFFFFFF;
        DWORD WriteColor = 0xFFFF0000;
        DWORD ExpectedFlipColor = 0xFFFFFF00;
        DWORD PlayWindowColor = 0xFFFF00FF;
        
        int Top = PadY;
        int Bottom = PadY + LineHeight;
        
        if (DebugTimeMarkerIndex == CurrentMarkerIndex)
        {
            Top += LineHeight + PadY;
            Bottom += LineHeight + PadY;

             Win32DrawSoundBufferMarker(FrameBuffer, SoundOutput, ThisMarker->OutputPlayCursor, C, PadX, Top, Bottom, PlayColor);
             Win32DrawSoundBufferMarker(FrameBuffer, SoundOutput, ThisMarker->OutputWriteCursor, C, PadX, Top, Bottom, WriteColor);

             Top += LineHeight + PadY;
             Bottom += LineHeight + PadY;

             Win32DrawSoundBufferMarker(FrameBuffer, SoundOutput, ThisMarker->OutputLocation, C, PadX, Top, Bottom, PlayColor);
             Win32DrawSoundBufferMarker(FrameBuffer, SoundOutput, ThisMarker->OutputLocation + ThisMarker->OutputByteCount, C, PadX, Top, Bottom, WriteColor);
             Top += LineHeight + PadY;
             Bottom += LineHeight + PadY;

             Win32DrawSoundBufferMarker(FrameBuffer, SoundOutput, ThisMarker->ExpectedFlipPlayCursor, C, PadX, PadY, Bottom, ExpectedFlipColor);
        }
        Win32DrawSoundBufferMarker(FrameBuffer, SoundOutput, ThisMarker->FlipPlayCursor, C, PadX, Top, Bottom, PlayColor);
        Win32DrawSoundBufferMarker(FrameBuffer, SoundOutput, ThisMarker->FlipPlayCursor + (480 * SoundOutput->BytesPerSample), C, PadX, Top, Bottom, PlayWindowColor);
        Win32DrawSoundBufferMarker(FrameBuffer, SoundOutput, ThisMarker->FlipWriteCursor, C, PadX, Top, Bottom, WriteColor);
    }
}

internal void
CatStrings(wchar_t* SourceA, size_t SourceACount, 
           wchar_t* SourceB, size_t SourceBCount,
           wchar_t* Dest, size_t DestCount)
{
    // TODO(Sebas): Dest bounds checking.
    for (int Index = 0;
         Index < SourceACount;
         Index++)
    {
        *Dest++ = *SourceA++;
    }
    for (int Index = 0;
         Index < SourceBCount;
         Index++)
    {
        *Dest++ = *SourceB++;
    }
    *Dest++ = 0;
}

int WINAPI
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    // NOTE(Sebas): Never use MAX_PATH in code that is user-facing, because it can be
    // dangerous and lead to bad results.
    wchar_t EXEFileName[MAX_PATH];
    DWORD SizeOfFileName = GetModuleFileNameW(0, EXEFileName, sizeof(EXEFileName));

    wchar_t* OnePastLastSlash = EXEFileName;
    for (wchar_t* Scan = EXEFileName;
        *Scan;
        ++Scan)
    {
        if (*Scan == '\\')
        {
            OnePastLastSlash = Scan + 1;
        }
    }

    wchar_t SourceGameCodeDLLFilename[] = L"handmade.dll";
    wchar_t SourceGameCodeDLLFullPath[MAX_PATH];

   CatStrings(EXEFileName, OnePastLastSlash - EXEFileName,
              SourceGameCodeDLLFilename, ArrayCount(SourceGameCodeDLLFilename) - 1,
              SourceGameCodeDLLFullPath, ArrayCount(SourceGameCodeDLLFullPath));

    wchar_t TempGameCodeDLLFilename[] = L"handmade_temp.dll";
    wchar_t TempGameCodeDLLFullPath[MAX_PATH];

    CatStrings(EXEFileName, OnePastLastSlash - EXEFileName,
              TempGameCodeDLLFilename, ArrayCount(TempGameCodeDLLFilename) - 1,
              TempGameCodeDLLFullPath, ArrayCount(TempGameCodeDLLFullPath));

    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;

    // NOTE(Sebas): Set the Windows scheduler granularity to 1ms
    // so that our Sleep() can be more granular
    UINT DesiredSchedulerMS = 1;
    bool32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

    
    Win32LoadXInput();

    WNDCLASSEXW WindowClass = {};

    Win32ResizeDIBSection(&GlobalFrameBuffer, 1280, 720);

    WindowClass.cbSize = sizeof(WNDCLASSEXW);
    WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    // WindowClass.hIcon;
    WindowClass.lpszClassName = L"HandmadeHeroeWindowClass";

    // TODO(Sebas): How do we reliably query this on Windows?
#define MonitorRefreshHz 60
#define GameUpdateHz (int32)((real32)MonitorRefreshHz / 2.0f)
    real32 TargetSecondsPerFrame = 1.0f / (real32)GameUpdateHz;

    if (RegisterClassExW(&WindowClass))
    {
        HWND Window =
            CreateWindowExW(
                0,
                WindowClass.lpszClassName,
               L"Handmade Hero",
                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                0,
                0,
                Instance,
                0);
        if (Window)
        {
            // NOTE(Sebas): Since we specified CS_OWNDC, we can just get one 
            // device context and use it forever because we are not sharing
            // it with anyone
            HDC DeviceContext = GetDC(Window);

            // NOTE(Sebas): Graphics test

            win32_sound_output SoundOutput = {};
            SoundOutput.SamplesPerSecond = 48000;
            SoundOutput.BytesPerSample  = sizeof(int16) * 2; 
            SoundOutput.RunningSampleIndex = 0;
            SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
#define SafetyBytesDivisor 2
            SoundOutput.SafetyBytes = ((SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample) / GameUpdateHz) / SafetyBytesDivisor;
            Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
            Win32ClearBuffer(&SoundOutput);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            GlobalRunning = true;
#if 0
            // NOTE(Sebas): This tests the play/write cursor update frequency on my machine
            while(GlobalRunning)
            {
                DWORD PlayCursor;
                DWORD WriteCursor;
                GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);

                char TextBuffer[256];
                swprintf_s((LPWSTR)TextBuffer, 256, L"PC:%u, WC:%u\n", PlayCursor, WriteCursor);
                OutputDebugStringW((LPWSTR)TextBuffer);
            }
#endif

            int16* Samples = (int16*)VirtualAlloc(0, 
                                                  SoundOutput.SecondaryBufferSize, 
                                                  MEM_RESERVE | MEM_COMMIT, 
                                                  PAGE_READWRITE);
#if HANDMADE_INTERNAL
            LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
            LPVOID BaseAddress = 0;
#endif
            game_memory GameMemory = {};
            GameMemory.PermanentStorageSize = Megabytes(64);
            GameMemory.TransientStorageSize = Gigabytes(4);
            GameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
            GameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
            GameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;

            uint64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;

            GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, (size_t)TotalSize,
                                                       MEM_RESERVE | MEM_COMMIT,
                                                       PAGE_READWRITE);
            GameMemory.TransientStorage = ((uint8*)GameMemory.PermanentStorage + 
                                           GameMemory.PermanentStorageSize);
            
            if(Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage)
            {
                game_input Input[2] = {};
                game_input* NewInput = &Input[0];
                game_input* OldInput = &Input[1];

                LARGE_INTEGER LastCounter = Win32GetWallClock();
                LARGE_INTEGER FlipWallClock = Win32GetWallClock();

                int DebugTimeMarkerIndex = 0;
                win32_debug_time_marker DebugTimeMarkers[GameUpdateHz / 2] = {};

                DWORD LastWriteCursor = 0;
                bool32 SoundIsValid = false;

                win32_game_code Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath);
                uint32 LoadCounter = 0;

                uint64 LastCycleCount = __rdtsc();
                while(GlobalRunning)
                {
                    FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
                    if (CompareFileTime(&Game.DLLLastWriteTime, &NewDLLWriteTime) != 0)
                    {
                        OutputDebugString(L"Reloading Game Code DLL!\n");
                        Win32UnloadGameCode(&Game);
                        Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath);
                    }

                    // TODO(Sebas): Zeroing macro
                    // TODO(Sebas): We can't zero everything because the up/down state will be wrong
                    game_controller_input* OldKeyboardController = GetController(OldInput, 0);
                    game_controller_input* NewKeyboardController = GetController(NewInput, 0);
                    *NewKeyboardController = {};
                    NewKeyboardController->IsConnected = true;
                    for (int i = 0; 
                            i < ArrayCount(NewKeyboardController->Buttons);
                            i++)
                    {
                        NewKeyboardController->Buttons[i].EndedDown = OldKeyboardController->Buttons[i].EndedDown;
                    }

                    Win32ProcessPendingMessages(NewKeyboardController);

                    if (!GlobalPause)
                    {
                        DWORD MaxControllerCount = XUSER_MAX_COUNT + 1;
                        if(MaxControllerCount > ArrayCount(NewInput->Controllers))
                        {
                            MaxControllerCount = ArrayCount(NewInput->Controllers);
                        }

                        for(DWORD ControllerIndex = 0; 
                                ControllerIndex < MaxControllerCount; 
                                ControllerIndex++)
                        {
                            if (ControllerIndex > 0)
                            {
                                // TODO(Sebas): Should we poll this more frequently
                                game_controller_input* OldController = GetController(OldInput, ControllerIndex);
                                game_controller_input* NewController = GetController(NewInput, ControllerIndex);

                                XINPUT_STATE ControllerState;   
                                if(XInputGetState(ControllerIndex-1, &ControllerState) == ERROR_SUCCESS)
                                {
                                    NewController->IsConnected = true;
                                    // NOTE(Sebas): This controller is plugged in      
                                    // TODO(Sebas): See if ControllerState.dwPacketNumber increments to rapidly
                                    XINPUT_GAMEPAD* Pad = &ControllerState.Gamepad;

                                    NewController->LAverageX = Win32ProcessXInputStickValue(Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                                    NewController->LAverageY = Win32ProcessXInputStickValue(Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                                    NewController->RAverageX = Win32ProcessXInputStickValue(Pad->sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
                                    NewController->RAverageY = Win32ProcessXInputStickValue(Pad->sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
                                    if ((NewController->LAverageX != 0)  || (NewController->LAverageY != 0)) 
                                    {
                                        NewController->IsAnalog = true;
                                    }
                                    real32 Threshold = 0.5f;
                                    Win32ProcessXInputDigitalButton(&OldController->MoveLeft, &NewController->MoveLeft, 
                                            ((NewController->LAverageX < -Threshold) ? 1 : 0), 1);
                                    Win32ProcessXInputDigitalButton(&OldController->MoveRight, &NewController->MoveRight, 
                                            ((NewController->LAverageX > Threshold) ? 1 : 0), 1);
                                    Win32ProcessXInputDigitalButton(&OldController->MoveDown, &NewController->MoveDown, 
                                            ((NewController->LAverageY < -Threshold) ? 1 : 0), 1);
                                    Win32ProcessXInputDigitalButton(&OldController->MoveUp, &NewController->MoveUp, 
                                            ((NewController->LAverageY > Threshold) ? 1 : 0), 1);

                                    if ((Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP) ||
                                            (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) ||
                                            (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) ||
                                            (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT))
                                    {
                                        // NewController->LAverageY = 1;
                                        NewController->IsAnalog = false;
                                    }

                                    if (!NewController->IsAnalog)
                                    {
                                        Win32ProcessXInputDigitalButton(&OldController->MoveUp, &NewController->MoveUp, 
                                                Pad->wButtons, XINPUT_GAMEPAD_DPAD_UP);
                                        Win32ProcessXInputDigitalButton(&OldController->MoveDown, &NewController->MoveDown, 
                                                Pad->wButtons, XINPUT_GAMEPAD_DPAD_DOWN);
                                        Win32ProcessXInputDigitalButton(&OldController->MoveLeft, &NewController->MoveLeft, 
                                                Pad->wButtons, XINPUT_GAMEPAD_DPAD_LEFT);
                                        Win32ProcessXInputDigitalButton(&OldController->MoveRight, &NewController->MoveRight, 
                                                Pad->wButtons, XINPUT_GAMEPAD_DPAD_RIGHT);
                                    }
                                    Win32ProcessXInputDigitalButton(&OldController->ActionUp, &NewController->ActionUp, 
                                            Pad->wButtons, XINPUT_GAMEPAD_Y);
                                    Win32ProcessXInputDigitalButton(&OldController->ActionDown, &NewController->ActionDown, 
                                            Pad->wButtons, XINPUT_GAMEPAD_A);
                                    Win32ProcessXInputDigitalButton(&OldController->ActionLeft, &NewController->ActionLeft, 
                                            Pad->wButtons, XINPUT_GAMEPAD_X);
                                    Win32ProcessXInputDigitalButton(&OldController->ActionRight, &NewController->ActionRight, 
                                            Pad->wButtons, XINPUT_GAMEPAD_B);
                                    Win32ProcessXInputDigitalButton(&OldController->LeftShoulder, &NewController->LeftShoulder, 
                                            Pad->wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER);
                                    Win32ProcessXInputDigitalButton(&OldController->RightShoulder, &NewController->RightShoulder, 
                                            Pad->wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER);
                                    Win32ProcessXInputDigitalButton(&OldController->Start, &NewController->Start, 
                                            Pad->wButtons, XINPUT_GAMEPAD_START);
                                    Win32ProcessXInputDigitalButton(&OldController->Back, &NewController->Back, 
                                            Pad->wButtons, XINPUT_GAMEPAD_BACK);

                                    // bool32 LThumbBtn = Pad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB;
                                    // bool32 RThumbBtn = Pad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB;
                                    // uint8 LeftTrigger = Pad->bLeftTrigger;
                                    // uint8 RightTrigger = Pad->bRightTrigger;
                                    // Input.Controllers[ControllerLStickX = Pad->sThumbLX;

                                    // TODO(Sebas): MIN/MAX macros
                                }
                                else
                                {
                                    NewController->IsConnected = false;
                                    // NOTE(Sebas): This controller is not available
                                }
                            }
                        }

                        // XINPUT_VIBRATION Vibration;
                        // Vibration.wLeftMotorSpeed = 30000;
                        // Vibration.wRightMotorSpeed = 30000;
                        // XInputSetState(0, &Vibration);
                        // NOTE(Sebas): Compute how much sound to write and where
                        game_frame_buffer GameBuffer = {};
                        GameBuffer.Memory = GlobalFrameBuffer.Memory;
                        GameBuffer.Width = GlobalFrameBuffer.Width;
                        GameBuffer.Height = GlobalFrameBuffer.Height;
                        GameBuffer.Pitch = GlobalFrameBuffer.Pitch;

                        Game.UpdateAndRender(&GameMemory, &GameBuffer, NewInput);

                        LARGE_INTEGER AudioWallClock = Win32GetWallClock();
                        real32 FromBeginToAudioSeconds = Win32GetSecondsElapsed(FlipWallClock, AudioWallClock);

                        DWORD PlayCursor;
                        DWORD WriteCursor;
                        if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
                        {
                            /* NOTE(Sebas):

                               Here is how sound output computation works.

                               We define a safety value that is the number 
                               of samples we think our game update loop 
                               may vary by (let's say up to 2ms). 

                               When we wake up to write audio, we will look
                               and see what the play cursor position is and we
                               will forecast ahead where we think the
                               play cursor will be on the next frame boundary.

                               We will then look to see if the write cursor is
                               before that by at least our safety value. If it is, the
                               target fill position is that frame boundary
                               plus one frame. This gives us perfect audio
                               sync in the case of a card that has low enough
                               latency.

                               If the write cursor is _after_ that safety 
                               margin, then we assume we can never sync the
                               audio perfectly, so we will write one frame's
                               worth of audio plus the safety margin's worth
                               of guard samples.
                               */
                            if (!SoundIsValid)
                            {
                                SoundOutput.RunningSampleIndex = WriteCursor / SoundOutput.BytesPerSample;
                                SoundIsValid = true;
                            }
                            DWORD BytesToLock = (SoundOutput.RunningSampleIndex*SoundOutput.BytesPerSample)%
                                SoundOutput.SecondaryBufferSize;

                            DWORD ExpectedSoundBytesPerFrame = (SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample) / GameUpdateHz;
                            
                            real32 SecondsLeftUntilFlip = TargetSecondsPerFrame - FromBeginToAudioSeconds;
                            DWORD ExpectedBytesUntilFlip = (DWORD)((SecondsLeftUntilFlip/TargetSecondsPerFrame) * (real32)ExpectedSoundBytesPerFrame);

                            DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedBytesUntilFlip;
                            DWORD SafeWriteCursor = WriteCursor;
                            if (SafeWriteCursor < PlayCursor)
                            {
                                SafeWriteCursor += SoundOutput.SecondaryBufferSize;
                            }
                            Assert(SafeWriteCursor >= PlayCursor);
                            SafeWriteCursor += SoundOutput.SafetyBytes;
                            bool32 AudioCardIsLowLatency = SafeWriteCursor < ExpectedFrameBoundaryByte;

                            DWORD TargetCursor = 0;
                            if (AudioCardIsLowLatency)
                            {
                                TargetCursor = ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame;
                            }
                            else
                            {
                                TargetCursor = WriteCursor + ExpectedSoundBytesPerFrame + SoundOutput.SafetyBytes;
                            }

                            TargetCursor = TargetCursor % SoundOutput.SecondaryBufferSize;

                            DWORD BytesToWrite = 0;
                            if(BytesToLock > TargetCursor)
                            {
                                BytesToWrite = (SoundOutput.SecondaryBufferSize - BytesToLock) + TargetCursor;
                            }
                            else
                            {
                                BytesToWrite = TargetCursor - BytesToLock;
                            }

                            game_sound_output_buffer SoundBuffer = {};
                            SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
                            SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
                            SoundBuffer.Samples = Samples;

                            Game.GetSoundSamples(&GameMemory, &SoundBuffer);
                            Win32FillSoundBuffer(&SoundOutput, BytesToLock, BytesToWrite, &SoundBuffer);
#if HANDMADE_INTERNAL
                            win32_debug_time_marker* Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
                            Marker->OutputPlayCursor = PlayCursor;
                            Marker->OutputWriteCursor = WriteCursor;
                            Marker->OutputLocation = BytesToLock;
                            Marker->OutputByteCount = BytesToWrite;
                            Marker->ExpectedFlipPlayCursor = ExpectedFrameBoundaryByte;

                            DWORD UnwrappedWriteCursor = WriteCursor;
                            if (UnwrappedWriteCursor < PlayCursor)
                            {
                                UnwrappedWriteCursor += SoundOutput.SecondaryBufferSize;
                            }
                            DWORD AudioLatencyBytes = UnwrappedWriteCursor - PlayCursor;
                            real32 AudioLatencySeconds = (((real32)AudioLatencyBytes / (real32)SoundOutput.BytesPerSample) / (real32)SoundOutput.SamplesPerSecond);
#if 0
                            char TextBuffer[256];
                            swprintf_s((LPWSTR)TextBuffer, 256, L"BTL:%u TC:%u BTW:%u - PC:%u WC:%u DELTA:%u (%.3fs)\n", 
                                    BytesToLock, TargetCursor, BytesToWrite, PlayCursor, WriteCursor, AudioLatencyBytes, AudioLatencySeconds);
                            OutputDebugStringW((LPWSTR)TextBuffer);
#endif
#endif
                        }
                        else
                        {
                            SoundIsValid = false;
                            // TODO(Sebas):  Logging
                        }


                        LARGE_INTEGER WorkCounter = Win32GetWallClock();
                        real32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);

                        // TODO(Sebas): Not tested yet! Probably buggy!!!
                        real32 SecondsElapsedPerFrame = WorkSecondsElapsed;
                        if (SecondsElapsedPerFrame < TargetSecondsPerFrame)
                        {
                            DWORD SleepMS = (DWORD)((TargetSecondsPerFrame - SecondsElapsedPerFrame) * 1000.0f);
                            if (SleepMS > 0)
                            {   
                                if (SleepIsGranular)
                                {
                                    Sleep(SleepMS);
                                }
                            }
                            SecondsElapsedPerFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                            if (SecondsElapsedPerFrame > TargetSecondsPerFrame)
                            {
                                // TODO(Sebas):  Logging
                            }
                            while(SecondsElapsedPerFrame < TargetSecondsPerFrame)
                            {
                                SecondsElapsedPerFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                            }
                        }
                        else
                        {
                            // TODO(Sebas): MISSED FRAME RATE!
                            // TODO(Sebas):  Logging
                        }
                        LARGE_INTEGER EndCounter = Win32GetWallClock();
                        real32 MSPerFrame = Win32GetSecondsElapsed(LastCounter, EndCounter) * 1000.0f;
                        LastCounter = EndCounter;


                        win32_viewport_dimensions Viewport = Win32GetViewportDimensions(Window);
#if HANDMADE_INTERNAL
                        // TODO(Sebas): Current is wrong on the 0th index
                        Win32DebugSyncDisplay(&GlobalFrameBuffer, &SoundOutput, 
                                DebugTimeMarkers, ArrayCount(DebugTimeMarkers), DebugTimeMarkerIndex - 1, 
                                TargetSecondsPerFrame);
#endif
                        Win32DisplayBufferInWindow(DeviceContext, Viewport.Width, Viewport.Height, &GlobalFrameBuffer);
                        FlipWallClock = Win32GetWallClock();

#if HANDMADE_INTERNAL
                        // NOTE(Sebas): This is DEBUG code
                        {
                            DWORD FlipPlayCursor;
                            DWORD FlipWriteCursor;
                            if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&FlipPlayCursor, &FlipWriteCursor)))
                            {
                                Assert(DebugTimeMarkerIndex < ArrayCount(DebugTimeMarkers));
                                win32_debug_time_marker* Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
                                Marker->FlipPlayCursor = FlipPlayCursor;
                                Marker->FlipWriteCursor = FlipWriteCursor;
                            }
                        }
#endif
                        game_input* Temp = NewInput;
                        NewInput = OldInput;
                        OldInput = Temp;

                        uint64 EndCycleCount = __rdtsc();
                        uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
                        LastCycleCount = EndCycleCount;

                        real64 FPS = 0.0f;
                        real64 MCPF = (real32)CyclesElapsed / (1000.0f * 1000.0f);
#if 1
                        wchar_t Buffer[256];
                        swprintf_s((LPWSTR)Buffer, 256, L"%0.2fms/f,  %0.2ff/s,  %0.2fmc/f\n", MSPerFrame, FPS, MCPF);
                        OutputDebugStringW((LPWSTR)Buffer);
#endif
#if HANDMADE_INTERNAL
                        ++DebugTimeMarkerIndex;
                        if (DebugTimeMarkerIndex == ArrayCount(DebugTimeMarkers))
                        {
                            DebugTimeMarkerIndex = 0;
                        }
#endif
                    } // if (!GlobalPause) 
                } // while (global running)
            } // if (samples && game memory)
            else
            {
                // TODO(Sebas):  Logging
            }
        } // if (window)
        else
        {
            // TODO(Sebas):  Logging
        }
    } // If (RegisterClass)
    else
    {
        // TODO(Sebas):  Logging 
    }

    return 0;
} // WinMain
