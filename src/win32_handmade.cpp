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
#include <math.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

#include "handmade.cpp"

#include <windows.h>
#include <xinput.h>
#include <dsound.h>
#include <stdio.h>
#include <malloc.h>

#include "win32_handmade.h" 

// TODO(Sebas):  This is a global for now.
global_variable bool32 GlobalRunning;
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



internal debug_read_file_result 
DEBUGPlatformReadEntireFile(char* FileName)
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

internal void 
DEBUGPlatformFreeFileMemory(void* Memory)
{
    if (Memory)
    {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}
internal bool32 
DEBUGPlatformWriteEntireFile(char* FileName, void* Memory, uint32 MemorySize)
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
    int BytesPerPixel = 4;
    Buffer->Pitch = Buffer->Width * BytesPerPixel;

    // NOTE(Sebas):
    // Windows knows this is a top-down bitmap
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    int BufferMemorySize = (Buffer->Width * Buffer->Height) * BytesPerPixel;
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

int WINAPI
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
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
    int MonitorRefreshHz = 60;
    int GameUpdateHz = MonitorRefreshHz / 2;
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
            SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;
            Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
            Win32ClearBuffer(&SoundOutput);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            GlobalRunning = true;

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
                uint64 LastCycleCount = __rdtsc();
                while(GlobalRunning)
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
                        if (ControllerIndex == 0)
                        {
                            // TODO(Sebas): Zeroing macro
                            // TODO(Sebas): We can't zero everything because the up/down state will be wrong
                            game_controller_input* OldKeyboardController = GetController(OldInput, ControllerIndex);
                            game_controller_input* NewKeyboardController = GetController(NewInput, ControllerIndex);
                            *NewKeyboardController = {};
                            NewKeyboardController->IsConnected = true;
                            for (int i = 0; 
                                    i < ArrayCount(NewKeyboardController->Buttons);
                                    i++)
                            {
                                NewKeyboardController->Buttons[i].EndedDown = OldKeyboardController->Buttons[i].EndedDown;
                            }

                            Win32ProcessPendingMessages(NewKeyboardController);
                        }
                        else
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
                    DWORD BytesToLock = 0;
                    DWORD BytesToWrite = 0;
                    DWORD TargetCursor;
                    DWORD PlayCursor;
                    DWORD WriteCursor;
                    bool32 SoundIsValid = false;
                    if(SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
                    {
                        BytesToLock = (SoundOutput.RunningSampleIndex*SoundOutput.BytesPerSample)%
                            SoundOutput.SecondaryBufferSize;
                        TargetCursor = (PlayCursor + SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample) % 
                            SoundOutput.SecondaryBufferSize; // 1/15th of a second ahead
                        if(BytesToLock > TargetCursor)
                        {
                            BytesToWrite = SoundOutput.SecondaryBufferSize - BytesToLock + TargetCursor;
                        }
                        else
                        {
                            BytesToWrite = TargetCursor - BytesToLock;
                        }
                        SoundIsValid = true;
                    }

                    // TODO(Sebas): Sound is wrong now, because we haven't updated it to go
                    // with the new frame loop
                    game_sound_output_buffer SoundBuffer = {};
                    SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
                    SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
                    SoundBuffer.Samples = Samples;


                    game_frame_buffer GameBuffer = {};
                    GameBuffer.Memory = GlobalFrameBuffer.Memory;
                    GameBuffer.Width = GlobalFrameBuffer.Width;
                    GameBuffer.Height = GlobalFrameBuffer.Height;
                    GameBuffer.Pitch = GlobalFrameBuffer.Pitch;

                    GameUpdateAndRender(&GameMemory, &GameBuffer, &SoundBuffer, NewInput);

                    if(SoundIsValid)
                    {

                        Win32FillSoundBuffer(&SoundOutput, BytesToLock, BytesToWrite, &SoundBuffer);
                    }
                    else
                    {
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
                                Sleep(SleepMS - 2);
                            }
                        }
                        real32 TestSecondsElapsedPerFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                        Assert(TestSecondsElapsedPerFrame < TargetSecondsPerFrame);
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
                    win32_viewport_dimensions Viewport = Win32GetViewportDimensions(Window);
                    Win32DisplayBufferInWindow(DeviceContext, Viewport.Width, Viewport.Height, &GlobalFrameBuffer);

                    game_input* Temp = NewInput;
                    NewInput = OldInput;
                    OldInput = Temp;

                    LARGE_INTEGER EndCounter = Win32GetWallClock();
                    real32 MSPerFrame = Win32GetSecondsElapsed(LastCounter, EndCounter) * 1000.0f;
                    LastCounter = EndCounter;

                    uint64 EndCycleCount = __rdtsc();
                    uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
                    LastCycleCount = EndCycleCount;

                    real64 FPS = 0.0f;
                    real64 MCPF = (real32)CyclesElapsed / (1000.0f * 1000.0f);

                    char Buffer[256];
                    swprintf_s((LPWSTR)Buffer, 256, L"%0.2fms/f,  %0.2ff/s,  %0.2fmc/f\n", MSPerFrame, FPS, MCPF);
                    OutputDebugStringW((LPWSTR)Buffer);
                    
 
                }
            }
            else
            {
                // TODO(Sebas):  Logging
            }
        }
        else
        {
            // TODO(Sebas):  Logging
        }
    }
    else
    {
        // TODO(Sebas):  Logging 
    }

    return 0;
}
