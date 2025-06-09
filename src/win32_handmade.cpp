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
#define Stmnt(S) do{ S }while(0)

#if !defined(AssertBreak)
    #define AssertBreak() (*(int*)0 = 0)
#endif

#if 1
    #define Assert(c) Stmnt(if(!(c)){ AssertBreak(); })
#else
    #define Assert(c)
#endif


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
            Format.nBlockAlign = (Format.nChannels * Format.wBitsPerSample)/8.0f;
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
        HRESULT R2 = GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
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

    Assert(BytesToLock < SoundOutput->SecondaryBufferSize);
    Assert(BytesToWrite <= SoundOutput->SecondaryBufferSize);
    //Assert(BytesToWrite > 0);
    Assert(BytesToLock % SoundOutput->BytesPerSample == 0);
    Assert(BytesToWrite % SoundOutput->BytesPerSample == 0);
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
        HRESULT R2 = GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
    else
    {
        // TODO(Sebas):  Logging
    }
}

internal void
Win32ProcessXInputDigitalButton(game_button_state* OldState, game_button_state* NewState, 
                                 DWORD XInputButtonState, DWORD ButtonBit)
{
    NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
    NewState->HalfTransitionCount = (OldState->EndedDown != ButtonBit) ? 1 : 0; 
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
            uint32 VKCode = WParam;
            #define KeyMessageWasDownBit (1 << 30)
            #define KeyMessageIsDownBit (1 << 31)
            bool32 WasDown = ((LParam & KeyMessageWasDownBit) != 0);
            bool32 IsDown = ((LParam & KeyMessageIsDownBit) == 0);
            if(WasDown != IsDown)
            {
                if(VKCode == 'W')
                {
                }
                if(VKCode == 'A')
                {
                }
                if(VKCode == 'S')
                {
                }
                if(VKCode == 'D')
                {
                }
                if(VKCode == 'Q')
                {
                }
                if(VKCode == 'E')
                {
                }
                if(VKCode == VK_UP)
                {
                }
                if(VKCode == VK_DOWN)
                {
                }
                if(VKCode == VK_LEFT)
                {
                }
                if(VKCode == VK_RIGHT)
                {
                }
                if(VKCode == VK_ESCAPE)
                {
                    OutputDebugString(L"ESCAPE: ");
                    if(IsDown)
                    {
                        OutputDebugString(L"IsDown");
                    }
                    if(WasDown)
                    {
                        OutputDebugString(L"WasDown");
                    }
                    OutputDebugString(L"\n");
                }
                if(VKCode == VK_SPACE)
                {
                }
            }

            bool32 AltKeyWasDown = (LParam & (1 << 29));
            if ((VKCode == VK_F4) && (AltKeyWasDown))
            {
                GlobalRunning = false;
            }
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

int WINAPI
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    int64 PerfCountFrequency = PerfCountFrequencyResult.QuadPart;

    Win32LoadXInput();
    WNDCLASSEXW WindowClass = {};

    Win32ResizeDIBSection(&GlobalFrameBuffer, 1280, 720);

    WindowClass.cbSize = sizeof(WNDCLASSEXW);
    WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    // WindowClass.hIcon;
    WindowClass.lpszClassName = L"HandmadeHeroeWindowClass";

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

            game_input Input[2] = {};
            game_input* NewInput = &Input[0];
            game_input* OldInput = &Input[1];
            
            LARGE_INTEGER LastCounter;
            QueryPerformanceCounter(&LastCounter);

            uint64 LastCycleCount = __rdtsc();
            while(GlobalRunning)
            {
                MSG Message;
                while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if(Message.message == WM_QUIT)
                    {
                        GlobalRunning = false;
                    }
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }

                int MaxControllerCount = XUSER_MAX_COUNT;
                if(MaxControllerCount > ArrayCount(NewInput->Controllers))
                {
                    MaxControllerCount = ArrayCount(NewInput->Controllers);
                }

                // TODO(Sebas): Should we poll this more frequently
                for(DWORD ControllerIndex = 0; 
                    ControllerIndex < MaxControllerCount; 
                    ControllerIndex++)
                {
                    game_controller_input* OldController = &OldInput->Controllers[ControllerIndex];
                    game_controller_input* NewController = &NewInput->Controllers[ControllerIndex];

                    XINPUT_STATE ControllerState;   
                    if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
                    {
                        // NOTE(Sebas): This controller is plugged in      
                        // TODO(Sebas): See if ControllerState.dwPacketNumber increments to rapidly
                        XINPUT_GAMEPAD* Pad = &ControllerState.Gamepad;

                        NewController->IsConnected = true;
                        NewController->IsAnalog = true;
                        
                        Win32ProcessXInputDigitalButton(&OldController->DPadUp, &NewController->DPadUp, 
                                                        Pad->wButtons, XINPUT_GAMEPAD_DPAD_UP);
                        Win32ProcessXInputDigitalButton(&OldController->DPadDown, &NewController->DPadDown, 
                                                        Pad->wButtons, XINPUT_GAMEPAD_DPAD_DOWN);
                        Win32ProcessXInputDigitalButton(&OldController->DPadLeft, &NewController->DPadLeft, 
                                                        Pad->wButtons, XINPUT_GAMEPAD_DPAD_LEFT);
                        Win32ProcessXInputDigitalButton(&OldController->DPadRight, &NewController->DPadRight, 
                                                        Pad->wButtons, XINPUT_GAMEPAD_DPAD_RIGHT);
                        Win32ProcessXInputDigitalButton(&OldController->Up, &NewController->Up, 
                                                        Pad->wButtons, XINPUT_GAMEPAD_Y);
                        Win32ProcessXInputDigitalButton(&OldController->Down, &NewController->Down, 
                                                        Pad->wButtons, XINPUT_GAMEPAD_A);
                        Win32ProcessXInputDigitalButton(&OldController->Left, &NewController->Left, 
                                                        Pad->wButtons, XINPUT_GAMEPAD_X);
                        Win32ProcessXInputDigitalButton(&OldController->Right, &NewController->Right, 
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
                        real32 LStickX = Pad->sThumbLX;
                        real32 LStickY = Pad->sThumbLY;

                        float Magnitude = sqrt(LStickX*LStickX + LStickY*LStickY);
                        float NormalizedMagnitude = 0;

                        float normalizedX = LStickX / Magnitude;
                        float normalizedY = LStickY / Magnitude;

                        int16 LThumbDeadzone = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
                        if (Magnitude > LThumbDeadzone)
                        {
                            if (Magnitude > 32767) Magnitude = 32767;
                            Magnitude -= LThumbDeadzone;
                            NormalizedMagnitude = Magnitude / (32767 - LThumbDeadzone);

                        }
                        else
                        {
                            Magnitude = 0;
                            NormalizedMagnitude = 0;
                        }

                        NewController->LStartX = OldController->LEndX;
                        NewController->LStartY = OldController->LEndY;
                        NewController->LStick.EndX = normalizedX * NormalizedMagnitude;
                        NewController->LStick.EndY = normalizedY * NormalizedMagnitude;

                        // TODO(Sebas): MIN/MAX macros
                        NewController->LMinX = NewController->LMaxX = NewController->LEndX;
                        NewController->LMinY = NewController->LMaxY = NewController->LEndY;

                        real32 RStickX = Pad->sThumbRX;
                        real32 RStickY = Pad->sThumbRY;

                        Magnitude = sqrt(RStickX*RStickX + RStickY*RStickY);
                        NormalizedMagnitude = 0;

                        normalizedX = RStickX / Magnitude;
                        normalizedY = RStickY / Magnitude;

                        int16 RThumbDeadzone = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
                        if (Magnitude > RThumbDeadzone)
                        {
                            if (Magnitude > 32767) Magnitude = 32767;
                            Magnitude -= RThumbDeadzone;
                            NormalizedMagnitude = Magnitude / (32767 - RThumbDeadzone);

                        }
                        else
                        {
                            Magnitude = 0;
                            NormalizedMagnitude = 0;
                        }
                        NewController->RStartX = OldController->REndX;
                        NewController->RStartY = OldController->REndY;
                        NewController->RStick.EndX = normalizedX * NormalizedMagnitude;
                        NewController->RStick.EndY = normalizedY * NormalizedMagnitude;

                        // TODO(Sebas): MIN/MAX macros
                        NewController->RMinX = NewController->RMaxX = NewController->REndX;
                        NewController->RMinY = NewController->RMaxY = NewController->REndY;

                        // GreenOffset -= (LStickY < -LThumbDeadzone || LStickY > LThumbDeadzone) ? LStickY / 4096: 0;
                        // SoundOutput.ToneHz = 512 + (int)(256.0f*((real32)RStickY / 30000.0f));
                        // SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond/SoundOutput.ToneHz;
                    }
                    else
                    {
                        // NOTE(Sebas): This controller is not available
                    }
                }

                // XINPUT_VIBRATION Vibration;
                // Vibration.wLeftMotorSpeed = 30000;
                // Vibration.wRightMotorSpeed = 30000;
                // XInputSetState(0, &Vibration);
                DWORD BytesToLock;
                DWORD BytesToWrite;
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

                game_sound_output_buffer SoundBuffer = {};
                SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
                SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
                SoundBuffer.Samples = Samples;

                game_frame_buffer GameBuffer = {};
                GameBuffer.Memory = GlobalFrameBuffer.Memory;
                GameBuffer.Width = GlobalFrameBuffer.Width;
                GameBuffer.Height = GlobalFrameBuffer.Height;
                GameBuffer.Pitch = GlobalFrameBuffer.Pitch;

                GameUpdateAndRender(&GameBuffer, &SoundBuffer, NewInput);
                // RenderWeirdGradient(&GlobalFrameBuffer, BlueOffset, GreenOffset);

                // NOTE(Sebas): DirectSound output test
                if(SoundIsValid)
                {
                    
                    Win32FillSoundBuffer(&SoundOutput, BytesToLock, BytesToWrite, &SoundBuffer);
                }
                else
                {
                    // TODO(Sebas):  Logging
                }

                win32_viewport_dimensions Viewport = Win32GetViewportDimensions(Window);
                Win32DisplayBufferInWindow(DeviceContext, Viewport.Width, Viewport.Height, &GlobalFrameBuffer);

                uint64 EndCycleCount = __rdtsc();
                LARGE_INTEGER EndCounter;
                QueryPerformanceCounter(&EndCounter);
                
                uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
                int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
                real32 MSPerFrame = ((1000.0f*(real32)CounterElapsed)/(real32)PerfCountFrequency);
                real32 FPS = ((real32)PerfCountFrequency/(real32)CounterElapsed);
                real32 MCPF = (real32)CyclesElapsed / (1000.0f * 1000.0f);
#if 0
                char Buffer[256];
                swprintf_s((LPWSTR)Buffer, 256, L"%0.2fms/f,  %0.2ff/s,  %0.2fmc/f\n", MSPerFrame, FPS, MCPF);
                OutputDebugStringW((LPWSTR)Buffer);
#endif
                LastCycleCount = EndCycleCount;
                LastCounter = EndCounter;
                
                game_input* Temp = NewInput;
                NewInput = OldInput;
                OldInput = Temp;
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
