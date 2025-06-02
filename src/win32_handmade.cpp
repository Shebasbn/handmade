/* ================================================================
   $File: $
   $Date: $
   $Revision:
   $Creator: Sebastian Bautista $
   $Notice: $
   ================================================================*/
#include <windows.h>

#define internal static
#define local_persist static
#define global_variable static
// TODO(Sebas):  This is a global for now.
global_variable bool Running;

global_variable BITMAPINFO BitmapInfo;
global_variable void* BitmapMemory;
global_variable HBITMAP BitmapHandle;

internal void
Win32ResizeDIBSection(int Width, int Height)
{
    // TODO(Sebas):  Bulletproof this.
    // Maybe don't free first, free after, then free first if that fails
    if(BitmapHandle)
    {
        DeleteObject(BitmapHandle);
    }
    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = Width;
    BitmapInfo.bmiHeader.biHeight = Height;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    HDC DeviceContext = CreateCompatibleDC(0);

    BitmapHandle = CreateDIBSection(DeviceContext, &BitmapInfo, DIB_RGB_COLORS, &BitmapMemory, 0, 0);

    ReleaseDC(0, DeviceContext);
}

internal void
Win32UpdateWindow(HDC DeviceContext, int X, int Y, int Width, int Height)
{
    StretchDIBits(DeviceContext,
                  X, Y, Width, Height,
                  X, Y, Width, Height,
                  &BitmapMemory,
                  &BitmapInfo,
                  DIB_RGB_COLORS,
                  SRCCOPY);
}
LRESULT CALLBACK
Win32MainWindowCallback(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    switch(Message)
    {
        case WM_SIZE:            
        {
            RECT ViewportRect;
            GetClientRect(WindowHandle, &ViewportRect);
            int Width = ViewportRect.right - ViewportRect.left;
            int Height = ViewportRect.bottom - ViewportRect.top;
            Win32ResizeDIBSection(Width, Height);
            OutputDebugStringA("WM_SIZE\n");
        } break;

        case WM_CLOSE:
        {
            // TODO(Sebas):  Handle this with a message to the user?
            Running = false;
            OutputDebugStringA("WM_CLOSE\n");
        } break;

        case WM_DESTROY:
        {
            // TODO(Sebas): Handle this as an error - recreate window?  
            Running = false;
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP:\n");
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(WindowHandle, &Paint);
            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
            Win32UpdateWindow(DeviceContext, X, Y, Width, Height);
            EndPaint(WindowHandle, &Paint);
        } break;
        default:
        {
            // OutputDebugStringA("default\n");
            Result = DefWindowProc(WindowHandle, Message, WParam, LParam);
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
        HWND WindowHandle =
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
        if (WindowHandle)
        {
            Running = true;
            while(Running)
            {
                MSG Message;
                BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
                if (MessageResult > 0)
                {
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }
                else
                {
                    break;
                }
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
