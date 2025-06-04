/* ================================================================
   $File: $
   $Date: $
   $Revision:
   $Creator: Sebastian Bautista $
   $Notice: $
   ================================================================*/
#include <windows.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

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

// TODO(Sebas):  This is a global for now.
global_variable bool GlobalRunning;
global_variable win32_frame_buffer GlobalFrameBuffer;


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
RenderWeirdGradient(win32_frame_buffer* Buffer, int BlueOffset, int GreenOffset)
{
    uint8* Row = (uint8*)Buffer->Memory;
    for(int y = 0; y < Buffer->Height; y++)
    {

        uint32* Pixel = (uint32*)Row;
        for(int x = 0; x < Buffer->Width; x++)
        {
            uint8 Blue = (uint8)(x + BlueOffset);
            uint8 Green = (uint8)(y + GreenOffset);
            uint8 Red = 0;
            uint8 Alpha = 255;
            *Pixel++ = ((Alpha << 24) | (Red << 16) | (Green << 8) | (Blue)); // BLUE Value
        }
        Row += Buffer->Pitch;
    }
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
    Buffer->Memory = VirtualAlloc(0, BufferMemorySize, MEM_COMMIT, PAGE_READWRITE);

}

internal void
Win32DisplayBufferInWindow(HDC DeviceContext, int ViewportWidth, int ViewportHeight, win32_frame_buffer* Buffer)
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
LRESULT CALLBACK
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

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            // int X = Paint.rcPaint.left;
            // int Y = Paint.rcPaint.top;
            // int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            // int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
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
            GlobalRunning = true;
            int XOffset = 0;
            int YOffset = 0;
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
                RenderWeirdGradient(&GlobalFrameBuffer, XOffset, YOffset);

                
                win32_viewport_dimensions Viewport = Win32GetViewportDimensions(Window);
                Win32DisplayBufferInWindow(DeviceContext, Viewport.Width, Viewport.Height, &GlobalFrameBuffer);

                XOffset++;
                YOffset++;
            }
        }
        else
        {
            DWORD ErrorCode = GetLastError();
            // TODO(Sebas):  Logging
        }
    }
    else
    {
        // TODO(Sebas):  Logging 
    }

    return 0;
}
