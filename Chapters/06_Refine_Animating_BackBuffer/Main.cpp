#include <stdint.h>
#include <windows.h>

#define INTERNAL static
#define GLOBAL_VARIABLE static
#define LOCAL_PERSIST static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;


struct Win32_Offscreen_Buffer {
    BITMAPINFO Info;
    void* Memory;
    int Width;
    int Height;
    int Pitch;
    int Bytes_Per_Pixel;
};

struct Win32_Window_Dimension {
    int Width;
    int Height;
};


GLOBAL_VARIABLE Win32_Offscreen_Buffer Global_Back_Buffer;
GLOBAL_VARIABLE bool Global_Running;


INTERNAL int Win32_Rect_Width(RECT* X) { return X->right - X->left; }
INTERNAL int Win32_Rect_Height(RECT* X) { return X->bottom - X->top; }

Win32_Window_Dimension Win32_Get_Window_Dimension(HWND Window) {
    Win32_Window_Dimension Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return Result;
}


INTERNAL void Render_Some_Weird_Gradient(Win32_Offscreen_Buffer Buffer, int OffsetX, int OffsetY) {
    uint8* Row = (uint8*)Buffer.Memory;

    for (int Y = 0; Y < Buffer.Height; Y++)
    {
        uint32* Pixel = (uint32*)Row;

        for (int X = 0; X < Buffer.Width; X++)
        {
            uint8 Blue = (uint8)(X + OffsetX);
            uint8 Green = (uint8)(Y + OffsetY);
            uint8 Red = (uint8)((Blue*Blue + Green*Green)/2);

            *Pixel = (Red << 16) | (Green << 8) | Blue; // in hex [0x--RRGGBB] --> in memory [BB GG RR --]

            Pixel++;
        }

        Row += Buffer.Pitch;
    }
}


INTERNAL void Win32_Resize_DIB_Section(Win32_Offscreen_Buffer *Buffer, int Width, int Height)
{
    if (Buffer->Memory) {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;
    Buffer->Bytes_Per_Pixel = 4;

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    int BitmapArea = Buffer->Width * Buffer->Height;
    int BitmapMemorySize = Buffer->Bytes_Per_Pixel * BitmapArea;

    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
    Buffer->Pitch = Buffer->Bytes_Per_Pixel * Buffer->Width;
}


INTERNAL void Win32_Copy_Buffer_To_Window(HDC DeviceContext, int WindowWidth, int WindowHeight,
                                          Win32_Offscreen_Buffer Buffer,
                                          int X, int Y, int Width, int Height)
{
    // FIXME: Aspect Ratio
    StretchDIBits(DeviceContext,
                  0, 0, WindowWidth, WindowHeight,
                  X, Y, Buffer.Width, Buffer.Height,
                  Buffer.Memory, &Buffer.Info,
                  DIB_RGB_COLORS, SRCCOPY);
}


LRESULT CALLBACK Win32_Main_Window_Callback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;

    switch (Message) {
        case WM_SIZE: {} break;

        case WM_CLOSE: {
            Global_Running = false;
        } break;

        case WM_DESTROY: {
            Global_Running = false;
        } break;

        case WM_ACTIVATEAPP: {} break;

        case WM_PAINT: {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);

            int OriginX = Paint.rcPaint.left;
            int OriginY = Paint.rcPaint.top;
            int Width = Win32_Rect_Width(&Paint.rcPaint);
            int Height = Win32_Rect_Height(&Paint.rcPaint);

            Win32_Window_Dimension Dimention = Win32_Get_Window_Dimension(Window);
            Win32_Copy_Buffer_To_Window(DeviceContext, Dimention.Width, Dimention.Height,
                                        Global_Back_Buffer, OriginX, OriginY, Width, Height);

            EndPaint(Window, &Paint);
        } break;

        default: {
            // FIXME: dear Windows! দেকো, যেটা বালো মনে করো...
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }

    return Result;
}


int CALLBACK WinMain(_In_ HINSTANCE Instance,
                     _In_opt_ HINSTANCE PrevInstance,
                     _In_ LPSTR CommandLine,
                     _In_ int ShowCode)
{
    WNDCLASSA WindowClass = {};

    Win32_Resize_DIB_Section(&Global_Back_Buffer, 1280, 720);

    WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    WindowClass.lpfnWndProc = Win32_Main_Window_Callback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "Oh_My_Sweet_WindowClass";

    if (RegisterClassA(&WindowClass))
    {
        HWND Window = CreateWindowExA(0, WindowClass.lpszClassName, "Sweet Gradient Animation",
                                      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                      0, 0, Instance, 0);
        if (Window)
        {
            HDC DeviceContext = GetDC(Window); // One owned Device Context of the Window handle

            int OffsetX = 0;
            int OffsetY = 0;

            Global_Running = true;
            while (Global_Running)
            {
                MSG Message;
                if (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if (Message.message == WM_QUIT) { Global_Running = false; }
                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }

                Render_Some_Weird_Gradient(Global_Back_Buffer, OffsetX, OffsetY);

                // HDC DeviceContext = GetDC(Window);
                Win32_Window_Dimension Dimention = Win32_Get_Window_Dimension(Window);

                Win32_Copy_Buffer_To_Window(DeviceContext, Dimention.Width, Dimention.Height,
                                            Global_Back_Buffer, 0, 0, Dimention.Width, Dimention.Height);
                // ReleaseDC(Window, DeviceContext);

                OffsetX++;
                OffsetY++;
            }
        }
    }

    return 0;
}
