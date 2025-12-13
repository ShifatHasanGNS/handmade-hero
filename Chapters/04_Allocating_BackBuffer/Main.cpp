#include <windows.h>

// NOTE: using these re-defines, because it's then easier to find -
// NOTE: which static variables are local scoped and which are global...
#define INTERNAL static
#define GLOBAL_VARIABLE static
#define LOCAL_PERSIST static

/*
 * NOTE: 'BLIT' (BLock Image Transfer) is a Fast Graphics Operation
 * NOTE: that copies a block of pixels from one memory location to another,
 * NOTE: often from a source image to a destination buffer that is being displayed
 */

// NOTE: by default 'false'
// NOTE: - that's how global variables get auto-initialized in C/C++
// TODO: these are globals for now
GLOBAL_VARIABLE bool Running;
GLOBAL_VARIABLE BITMAPINFO BitmapInfo;
GLOBAL_VARIABLE void* BitmapMemory;
GLOBAL_VARIABLE HBITMAP BitmapHandle;
GLOBAL_VARIABLE HDC BitmapDeviceContext;

// NOTE: ..DIB.. --> 'DIB' means 'Device Independent BitMap' -
// NOTE: to talk about the things that we can write into buffer as BitMap -
// NOTE: that Windows can Display using GDI...
// NOTE: It (forcefully) initializes/updates DIB section whenever window-size changes...
INTERNAL void Win32ResizeDIBSection(int Width, int Height)
{
    // TODO: free out DIB-Section; Bulletproof this
    if (BitmapHandle) {
        DeleteObject(BitmapHandle);
    }

    if (!BitmapDeviceContext) {
        // TODO: should we recreate these under certain special circumstances
        BitmapDeviceContext = CreateCompatibleDC(0);
    }

    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = Width;
    BitmapInfo.bmiHeader.biHeight = Height;
    BitmapInfo.bmiHeader.biPlanes = 1; // only the BackBuffer
    BitmapInfo.bmiHeader.biBitCount = 32; // per pixel; each color-channel 8 bits
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    BitmapHandle = CreateDIBSection(BitmapDeviceContext, &BitmapInfo,
                                    DIB_RGB_COLORS, &BitmapMemory, 0, 0);
}

INTERNAL void Win32UpdateWindow(HDC DeviceContext, int DestX, int DestY, int Width, int Height)
{
    StretchDIBits(DeviceContext,
                  DestX, DestY, Width, Height,
                  DestX, DestY, Width, Height,
                  BitmapMemory, &BitmapInfo,
                  DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;

    switch (Message) {
        case WM_SIZE: {
            RECT ClientRect;
            GetClientRect(WindowHandle, &ClientRect);
            int Width = ClientRect.right - ClientRect.left;
            int Height = ClientRect.bottom - ClientRect.top;
            Win32ResizeDIBSection(Width, Height);
        } break;

        case WM_CLOSE: {
            // TODO: need to handle this with a Messenger to the User...
            Running = false;
        } break;

        case WM_DESTROY: {
            // TODO: need to handle this with an Error; Recreate Window...
            Running = false;
        } break;

        case WM_ACTIVATEAPP: {
            // TODO: need to implement later...
        } break;

        case WM_PAINT: {
            // using GDI (Graphics Device Interface)
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(WindowHandle, &Paint);

            int OriginX = Paint.rcPaint.left;
            int OriginY = Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

            Win32UpdateWindow(DeviceContext, OriginX, OriginY, Width, Height);

            EndPaint(WindowHandle, &Paint);
        } break;

        default: {
            Result = DefWindowProcA(WindowHandle, Message, WParam, LParam);
        } break;
    }

    return Result;
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
{
    WNDCLASS WindowClass = {};
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    if (RegisterClassA(&WindowClass))
    {
        HWND WindowHandle = CreateWindowExA(0, WindowClass.lpszClassName, "Handmade Hero",
                                            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                            0, 0, Instance, 0);

        MSG Message;
        if (WindowHandle) {
            Running = true;

            while (Running) {
                BOOL MessageResult = GetMessageA(&Message, 0, 0, 0);

                if (MessageResult > 0) {
                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                } else {
                    break;
                }
            }

        } else {
            /* TODO: if WindowHandle is NULL, what then? */
        }

    } else {
        /* TODO: Debug-Logging System */
    }

    return EXIT_SUCCESS;
}
