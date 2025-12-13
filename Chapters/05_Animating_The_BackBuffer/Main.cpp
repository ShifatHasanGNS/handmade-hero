#include <stdint.h>
#include <windows.h>

// NOTE: to make it work both for C and C++
#define bool BOOL
#define true 1
#define false 0

#define INTERNAL static
#define GLOBAL_VARIABLE static
#define LOCAL_PERSIST static

typedef uint8_t uint8;   // unsigned char
typedef uint16_t uint16; // unsigned short
typedef uint32_t uint32; // unsigned int
typedef uint64_t uint64; // unsigned long long

typedef int8_t int8;   // signed char
typedef int16_t int16; // signed short
typedef int32_t int32; // signed int
typedef int64_t int64; // signed long long

GLOBAL_VARIABLE bool Running;
GLOBAL_VARIABLE BITMAPINFO BitmapInfo;
GLOBAL_VARIABLE void* BitmapMemory;
GLOBAL_VARIABLE int BitmapWidth;
GLOBAL_VARIABLE int BitmapHeight;
GLOBAL_VARIABLE int BytesPerPixel = 4; // 'RGB_' to match 2^x sized aligned block of memory

INTERNAL int RectWidth(RECT* X) { return X->right - X->left; }
INTERNAL int RectHeight(RECT* X) { return X->bottom - X->top; }

INTERNAL void RenderSomeWeirdGradient(int OffsetX, int OffsetY) {
    int Pitch = BytesPerPixel * BitmapWidth;
    uint8* Row = (uint8*)BitmapMemory;

    for (int Y = 0; Y < BitmapHeight; Y++)
    {
        // NOTE: Here are some important info about Pixel in Memory:
        // A (packed) Pixel in Memory: BB GG RR --
        // It's --> Little Endian Memory Model
        // So, [0x--RRGGBB] --> in memory --> [BB | GG | RR | --]
        // ...least significant byte goes to the lowest memory-address of the data
        //
        // NOTE: first byte's address for the Row and Pixel are the same
        uint32* Pixel = (uint32*)Row;

        for (int X = 0; X < BitmapWidth; X++)
        {
            uint8 Blue = (uint8)(X + OffsetX);
            uint8 Green = (uint8)(Y + OffsetY);
            uint8 Red = (uint8)((Blue*Blue + Green*Green)/2);

            *Pixel = (Red << 16) | (Green << 8) | Blue;

            Pixel++; // point to the next pixel (4 bytes ahead)
        }

        Row += Pitch; // NOTE: point to the next row
    }
}

INTERNAL void Win32ResizeDIBSection(int Width, int Height)
{
    if (BitmapMemory) { // free it, if already exists
        VirtualFree(BitmapMemory, 0, MEM_RELEASE);
    }

    BitmapWidth = Width;
    BitmapHeight = Height;

    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = BitmapWidth;
    BitmapInfo.bmiHeader.biHeight = -BitmapHeight; // Top-Down Co-ordinate System; Origin is at the Top-Left
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    int BitmapArea = BitmapWidth * BitmapHeight;
    int BitmapMemorySize = BytesPerPixel * BitmapArea;

    BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    // TODO: inititally clear this to BLACK...
}

INTERNAL void Win32UpdateWindow(HDC DeviceContext, RECT ClientRect, int DestX, int DestY, int Width, int Height)
{
    int WindowWidth = RectWidth(&ClientRect);
    int WindowHeight = RectHeight(&ClientRect);

    DestX = 0; // TODO: may need to change later
    DestY = 0; // TODO: may need to change later
    StretchDIBits(DeviceContext,
                  0, 0, BitmapWidth, BitmapHeight,
                  0, 0, BitmapWidth, BitmapHeight,
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
            int Width = RectWidth(&ClientRect);
            int Height = RectHeight(&ClientRect);
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
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(WindowHandle, &Paint);

            int OriginX = Paint.rcPaint.left;
            int OriginY = Paint.rcPaint.top;
            int Width = RectWidth(&Paint.rcPaint);
            int Height = RectHeight(&Paint.rcPaint);

            RECT ClientRect;
            GetClientRect(WindowHandle, &ClientRect);
            Win32UpdateWindow(DeviceContext, ClientRect, OriginX, OriginY, Width, Height);

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
    WindowClass.style = CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    if (RegisterClassA(&WindowClass))
    {
        HWND WindowHandle = CreateWindowExA(0, WindowClass.lpszClassName, "Made with C, by Shifat... :)",
                                            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                            0, 0, Instance, 0);

        if (WindowHandle)
        {
            int OffsetX = 0;
            int OffsetY = 0;

            Running = true;
            while (Running)
            {
                MSG Message;
                if (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if (Message.message == WM_QUIT) {
                        Running = false;
                    }
                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }

                RenderSomeWeirdGradient(OffsetX, OffsetY);

                HDC DeviceContext = GetDC(WindowHandle);
                RECT ClientRect;
                GetClientRect(WindowHandle, &ClientRect);
                int WindowWidth = RectWidth(&ClientRect);
                int WindowHeight = RectHeight(&ClientRect);

                Win32UpdateWindow(DeviceContext, ClientRect, 0, 0, WindowWidth, WindowHeight);

                ReleaseDC(WindowHandle, DeviceContext);

                OffsetX++;
                OffsetY++;
            }

        } else {
            /* TODO: if WindowHandle is NULL, what then? */
        }

    } else {
        /* TODO: Debug-Logging System */
    }

    return EXIT_SUCCESS;
}
