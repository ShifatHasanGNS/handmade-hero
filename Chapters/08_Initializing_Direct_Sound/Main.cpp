#include <stdint.h>
#include <windows.h>
#include <dsound.h>

#define Internal static
#define Global_Var static
#define Local_Persist static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

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

Global_Var Win32_Offscreen_Buffer Global_Back_Buffer;
Global_Var bool Global_Running;
Global_Var int Global_OffsetX;
Global_Var int Global_OffsetY;


Internal int Win32_Rect_Width(RECT* X)  { return X->right - X->left; }
Internal int Win32_Rect_Height(RECT* X) { return X->bottom - X->top; }

Internal Win32_Window_Dimension Win32_Get_Window_Dimension(HWND Window) {
    Win32_Window_Dimension Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return Result;
}


typedef HRESULT WINAPI DirectSoundCreate_Func(LPCGUID lpGUID, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter);

Internal void Win32_Init_DirectSound(HWND Window, int32 Samples_Per_Second, int32 BufferSize) {
    // NOTE: unfortunately Microsoft implemented 'DirectSound' in Object-Oriented manner
    // STEP: - Load the libraries (DSound and DSound3D) dynamically...
    HMODULE DirectSound_Lib   = LoadLibrary("dsound.dll");
    if (!DirectSound_Lib) {
        OutputDebugStringA("Warning: Failed to Load 'dsound.dll'");
        exit(EXIT_FAILURE);
    }
    DirectSoundCreate_Func* DirectSoundCreate = (DirectSoundCreate_Func*)
                                                GetProcAddress(DirectSound_Lib, "DirectSoundCreate");

    // STEP: - Get a DirectSound object; in 'cooperative mode'...
    LPDIRECTSOUND DirectSound;
    if (!DirectSoundCreate || FAILED(DirectSoundCreate(0, &DirectSound, 0))) {
        OutputDebugStringA("Warning: Failed to Create a 'DirectSound' object");
        exit(EXIT_FAILURE);
    }
    if (FAILED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY))) {
        OutputDebugStringA("Warning: Failed to Set the Cooperative-Level");
        exit(EXIT_FAILURE);
    }

    // STEP: - Create a primary buffer...
    // NOT AN ACTUAL BUFFER THOUGH... Just a handle to the Audio-Player...
    LPDIRECTSOUNDBUFFER Primary_Buffer;
    DSBUFFERDESC Primary_Buffer_Desc = {};
    Primary_Buffer_Desc.dwSize = sizeof(DSBUFFERDESC);
    Primary_Buffer_Desc.dwFlags = DSBCAPS_PRIMARYBUFFER;
    Primary_Buffer_Desc.dwBufferBytes = BufferSize;

    if (FAILED(DirectSound->CreateSoundBuffer(&Primary_Buffer_Desc, &Primary_Buffer, 0))) {
        OutputDebugStringA("Warning: Failed to Create a Primary-Sound-Buffer");
        exit(EXIT_FAILURE);
    }

    WAVEFORMATEX WaveFormat = {};
    WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
    WaveFormat.cbSize = 0;
    WaveFormat.nChannels = 2;
    WaveFormat.wBitsPerSample = 16;
    WaveFormat.nSamplesPerSec = Samples_Per_Second;
    WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
    WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;

    if (FAILED(Primary_Buffer->SetFormat(&WaveFormat))) {
        OutputDebugStringA("Warning: Failed to Set Format for the Primary-Sound-Buffer");
        exit(EXIT_FAILURE);
    }

    // STEP: - Create a secondary buffer
    LPDIRECTSOUNDBUFFER Secondary_Buffer;
    DSBUFFERDESC Secondary_Buffer_Desc = {};
    Secondary_Buffer_Desc.dwSize = sizeof(DSBUFFERDESC);
    Secondary_Buffer_Desc.dwFlags = DS3D_IMMEDIATE;
    Secondary_Buffer_Desc.dwBufferBytes = BufferSize;
    Secondary_Buffer_Desc.lpwfxFormat = &WaveFormat;

    if (FAILED(DirectSound->CreateSoundBuffer(&Secondary_Buffer_Desc, &Secondary_Buffer, 0))) {
        OutputDebugStringA("Warning: Failed to Create a Secondary-Sound-Buffer");
        exit(EXIT_FAILURE);
    }

    // STEP: - Start playing the sound
}


Internal void Render_Some_Weird_Gradient(Win32_Offscreen_Buffer *Buffer,
                                         int OffsetX, int OffsetY)
{
    uint8* Row = (uint8*)Buffer->Memory;

    for (int Y = 0; Y < Buffer->Height; Y++)
    {
        uint32* Pixel = (uint32*)Row;

        for (int X = 0; X < Buffer->Width; X++)
        {
            uint8 Blue = (uint8)(X + OffsetX);
            uint8 Green = (uint8)(Y + OffsetY);
            uint8 Red = (uint8)((Blue*Blue + Green*Green)/2);

            // in hex [0x--RRGGBB] --> in memory [BB GG RR --]
            *Pixel = (Red << 16) | (Green << 8) | Blue;

            Pixel++;
        }

        Row += Buffer->Pitch;
    }
}


Internal void Win32_Resize_DIB_Section(Win32_Offscreen_Buffer* Buffer, int Width, int Height)
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


Internal void Win32_Copy_Buffer_To_Window(Win32_Offscreen_Buffer* Buffer, HDC DeviceContext,
                                          int WindowWidth, int WindowHeight)
{
    // FIXME: Aspect Ratio
    StretchDIBits(DeviceContext,
                  0, 0, WindowWidth, WindowHeight,
                  0, 0, WindowWidth, WindowHeight,
                  Buffer->Memory, &Buffer->Info,
                  DIB_RGB_COLORS, SRCCOPY);
}


Internal LRESULT CALLBACK Win32_Main_Window_Callback(HWND Window, UINT Message,
                                                     WPARAM WParam, LPARAM LParam)
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

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP: {
            uint32 VKCode = WParam;

            bool32 Was_Down = LParam & (1 << 30);
            bool32 Is_Down  = LParam & (1 << 31);

            if (Was_Down != Is_Down) {
                if (VKCode == 'W') {
                    Global_OffsetY += 16;
                } else if (VKCode == 'S') {
                    Global_OffsetY -= 16;
                } else if (VKCode == 'A') {
                    Global_OffsetX += 16;
                } else if (VKCode == 'D') {
                    Global_OffsetX -= 16;
                }
            }

            bool32 Alt_Key_Was_Down = LParam & (1 << 29);
            // on Mac (via WINE), the 'command' key maps to Windows's 'Alt' key.
            // And, I don't know, why...
            // (mac) CMD+F4 == (windows) ALT+F4
            if ((VKCode == VK_F4) && Alt_Key_Was_Down) {
                Global_Running = false; // to close the Window
            }
        } break;

        case WM_PAINT: {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            Win32_Window_Dimension Dimention = Win32_Get_Window_Dimension(Window);
            Win32_Copy_Buffer_To_Window(&Global_Back_Buffer, DeviceContext,
                                        Dimention.Width, Dimention.Height);
            EndPaint(Window, &Paint);
        } break;

        default: {
            // let windows handle the remaining cases for us
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
    WindowClass.lpszClassName = "MySweetWindowClass";

    if (RegisterClassA(&WindowClass))
    {
        HWND Window = CreateWindowExA(0, WindowClass.lpszClassName,
                                      "Sweet Gradient Animation",
                                      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                      CW_USEDEFAULT, CW_USEDEFAULT,
                                      CW_USEDEFAULT, CW_USEDEFAULT,
                                      0, 0, Instance, 0);
        if (Window)
        {
            HDC DeviceContext = GetDC(Window);

            Win32_Init_DirectSound(Window, 48'000, 48'000 * sizeof(int32) * 2);

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
                Render_Some_Weird_Gradient(&Global_Back_Buffer, Global_OffsetX, Global_OffsetY);
                Win32_Window_Dimension Dimention = Win32_Get_Window_Dimension(Window);
                Win32_Copy_Buffer_To_Window(&Global_Back_Buffer, DeviceContext,
                                            Dimention.Width, Dimention.Height);
            }
        }
    }

    return EXIT_SUCCESS;
}
