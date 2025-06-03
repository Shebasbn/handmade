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



// TODO(Sebas):  This is a global for now.
global_variable bool Running;

global_variable BITMAPINFO BitmapInfo;
global_variable void* BitmapMemory;
global_variable int BitmapWidth;
global_variable int BitmapHeight;
global_variable int BytesPerPixel = 4;

internal void
RenderWeirdGradient(int XOffset, int YOffset)
{
    int Width = BitmapWidth;
    int Height = BitmapHeight;
    int Pitch = Width * BytesPerPixel;
    uint8* Row = (uint8*)BitmapMemory;
    for(int y = 0; y < Height; y++)
    {

        uint32* Pixel = (uint32*)Row;
        for(int x = 0; x < Width; x++)
        {
            uint8 Blue = (uint8)(x + XOffset);
            uint8 Green = (uint8)(y + YOffset);
            uint8 Red = 0;
            uint8 Alpha = 255;
            *Pixel++ = ((Alpha << 24) | (Red << 16) | (Green << 8) | (Blue)); // BLUE Value
        }
        Row += Pitch;
    }
}

internal void
Win32ResizeDIBSection(int Width, int Height)
{

    // TODO(Sebas):  Bulletproof this.
    // Maybe don't free first, free after, then free first if that fails
    if (BitmapMemory)
    {
        VirtualFree(BitmapMemory, 0, MEM_RELEASE);
    }

    BitmapWidth = Width;
    BitmapHeight = Height;

    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = BitmapWidth;
    BitmapInfo.bmiHeader.biHeight = -BitmapHeight;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = (BitmapWidth * BitmapHeight) * BytesPerPixel;
    BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

}

internal void
Win32UpdateWindow(HDC DeviceContext, RECT* ViewportRect, int X, int Y, int Width, int Height)
{
    int ViewportWidth = ViewportRect->right - ViewportRect->left;
    int ViewportHeight = ViewportRect->bottom - ViewportRect->top;
    StretchDIBits(DeviceContext,
                  // X, Y, Width, Height,
                  // X, Y, Width, Height,
                  0, 0, BitmapWidth, BitmapHeight, 
                  0, 0, ViewportWidth, ViewportHeight, 
                  BitmapMemory,
                  &BitmapInfo,
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
            RECT ViewportRect;
            GetClientRect(Window, &ViewportRect);
            int Width = ViewportRect.right - ViewportRect.left;
            int Height = ViewportRect.bottom - ViewportRect.top;
            Win32ResizeDIBSection(Width, Height);
            OutputDebugString(L"WM_SIZE\n");
        } break;

        case WM_CLOSE:
        {
            // TODO(Sebas):  Handle this with a message to the user?
            Running = false;
            OutputDebugString(L"WM_CLOSE\n");
        } break;

        case WM_DESTROY:
        {
            // TODO(Sebas): Handle this as an error - recreate window?  
            Running = false;
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugString(L"WM_ACTIVATEAPP:\n");
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
            RECT ViewportRect;
            GetClientRect(Window, &ViewportRect);
            Win32UpdateWindow(DeviceContext, &ViewportRect, X, Y, Width, Height);
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
    // TODO(Sebas): Check if HREDRAW/VREDRAW/OWNDC still matter
    WNDCLASSEXW WindowClass = {};
    WindowClass.cbSize = sizeof(WNDCLASSEXW);
    WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
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
            Running = true;
            int XOffset = 0;
            int YOffset = 0;
            while(Running)
            {
                MSG Message;
                while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if(Message.message == WM_QUIT)
                    {
                        Running = false;
                    }
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }
                RenderWeirdGradient(XOffset, YOffset);
                XOffset++;
                RECT ViewportRect;
                GetClientRect(Window, &ViewportRect);
                HDC DeviceContext = GetDC(Window);
                Win32UpdateWindow(DeviceContext, &ViewportRect, 0, 0, BitmapWidth, BitmapHeight);
                ReleaseDC(Window, DeviceContext);
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
