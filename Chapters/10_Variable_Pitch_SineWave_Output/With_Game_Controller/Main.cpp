#include <stdint.h>
#include <windows.h>
#include <xinput.h>
#include <dsound.h>
#include <math.h>

#define PI 3.1415926535897932f

#define Internal      static
#define Global_Var    static
#define Local_Persist static

typedef int32_t  bool32;
typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t   int8;
typedef int16_t  int16;
typedef int32_t  int32;
typedef int64_t  int64;
typedef float    real32;
typedef double   real64;

struct Win32_Offscreen_Buffer {
    void* Memory;
    BITMAPINFO Info;
    int Width;
    int Height;
    int Pitch;
    int Bytes_Per_Pixel;
};

struct Win32_Window_Dimension {
    int Width;
    int Height;
};

struct Win32_Sound_Output {
    int32 Wave_Frequency;        // 432
    int32 Samples_Per_Second;    // 48'000
    int32 Latency_Sample_Count;  // Samples_Per_Second / 15
    int32 Samples_Per_Cycle;     // Samples_Per_Second / Wave_Frequency
    DWORD Bytes_Per_Sample;      // 2 * sizeof(int16)
    int32 Secondary_Buffer_Size; // Samples_Per_Second * Bytes_Per_Sample
    uint32 Running_Sample_Index; // 0
    real32 Wave_Phase;           // 0
    int16 Sound_Volume;          // 2000
};

Global_Var bool Global_Running;
Global_Var int  Global_OffsetX;
Global_Var int  Global_OffsetY;
Global_Var Win32_Offscreen_Buffer Global_Back_Buffer;
Global_Var LPDIRECTSOUNDBUFFER    Global_Secondary_Buffer;
Global_Var Win32_Sound_Output     Global_Sound_Output;

// ----------

// Dynamic function loading pattern: load DLL functions at runtime instead of link-time
// Allows program to run even if the DLL is missing by using stub fallbacks

#define XInputGetState_Macro(_NAME_) DWORD WINAPI _NAME_(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef XInputGetState_Macro(XInputGetState_Func);
XInputGetState_Macro(XInputGetState_Stub) { return ERROR_DEVICE_NOT_CONNECTED; }
Global_Var XInputGetState_Func* XInputGetState_Ptr = XInputGetState_Stub;
#define XInputGetState XInputGetState_Ptr

#define XInputSetState_Macro(_NAME_) DWORD WINAPI _NAME_(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef XInputSetState_Macro(XInputSetState_Func);
XInputSetState_Macro(XInputSetState_Stub) { return ERROR_DEVICE_NOT_CONNECTED; }
Global_Var XInputSetState_Func* XInputSetState_Ptr = XInputSetState_Stub;
#define XInputSetState XInputSetState_Ptr

// NOTE: Here's how it works,
// define a Macro that expands to a full Original function signature
// Create a function pointer type using the macro
// Stub function that returns success but does nothing (fallback if DLL load fails)
// Global function pointer, initially points to the stub function
// Replace Original function's call with the function pointer for dynamic loading
// Done...

// ----------

Internal void Win32_Load_XInput(void) {
    HMODULE XInput_Lib = LoadLibraryA("xinput1_4.dll"); // for: Windows 8+
    if (!XInput_Lib) {
        XInput_Lib = LoadLibraryA("xinput1_3.dll"); // for: Windows 7
    }
    if (!XInput_Lib) {
        XInput_Lib = LoadLibraryA("xinput9_1_0.dll"); // for: older Windows-OS
    }

    if (XInput_Lib) {
        XInputGetState_Ptr = (XInputGetState_Func*)GetProcAddress(XInput_Lib, "XInputGetState");
        XInputSetState_Ptr = (XInputSetState_Func*)GetProcAddress(XInput_Lib, "XInputSetState");
    }
    else {
        // The stub functions will be used instead
        OutputDebugStringA("Warning: XInput library not found. Controller support disabled.\n");
    }
}


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
    // LPDIRECTSOUNDBUFFER Secondary_Buffer; // made it global
    DSBUFFERDESC Secondary_Buffer_Desc = {};
    Secondary_Buffer_Desc.dwSize = sizeof(DSBUFFERDESC);
    Secondary_Buffer_Desc.dwFlags = DS3D_IMMEDIATE;
    Secondary_Buffer_Desc.dwBufferBytes = BufferSize;
    Secondary_Buffer_Desc.lpwfxFormat = &WaveFormat;

    if (FAILED(DirectSound->CreateSoundBuffer(&Secondary_Buffer_Desc, &Global_Secondary_Buffer, 0))) {
        OutputDebugStringA("Warning: Failed to Create a Secondary-Sound-Buffer");
        exit(EXIT_FAILURE);
    }
}


Internal void Render_Some_Weird_Gradient(Win32_Offscreen_Buffer *Buffer, int OffsetX, int OffsetY) {
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

    /*
        NOTE:
        Screen blitting (bit block transfer) using the Windows
        GDI (Graphics Device Interface) within the Win32 API is
        the process of rapidly moving a block of pixels from
        one location to another. It is a fundamental operation
        used for tasks like moving sprites, drawing backgrounds,
        and updating the screen efficiently.
        The core function used for blitting in Win32 is BitBlt,
        or the more powerful StretchBlt for scaling.
    */
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
                } else if (VKCode == VK_UP) {
                    Global_Sound_Output.Sound_Volume += 100;
                } else if (VKCode == VK_DOWN) {
                    Global_Sound_Output.Sound_Volume -= 100;
                } else if (VKCode == VK_RIGHT) {
                    Global_Sound_Output.Wave_Frequency += 128;
                    Global_Sound_Output.Samples_Per_Cycle = Global_Sound_Output.Samples_Per_Second
                                                          / Global_Sound_Output.Wave_Frequency;
                } else if (VKCode == VK_LEFT) {
                    Global_Sound_Output.Wave_Frequency -= 128;
                    Global_Sound_Output.Samples_Per_Cycle = Global_Sound_Output.Samples_Per_Second
                                                          / Global_Sound_Output.Wave_Frequency;
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


Internal void Win32_Fill_Sound_Buffer(Win32_Sound_Output* Sound_Output, DWORD Bytes_To_Lock, DWORD Bytes_To_Write) {
    DWORD Region1_Size, Region2_Size;
    void *Region1, *Region2;

    if (FAILED(Global_Secondary_Buffer->Lock(Bytes_To_Lock, Bytes_To_Write,
                                             &Region1, &Region1_Size,
                                             &Region2, &Region2_Size, 0)))
    {
        OutputDebugStringA("Warning: Failed to Lock the Secondary Buffer");
        exit(EXIT_FAILURE);
    }

    // NOTE: Assert that Region#_Size and Region# are valid
    DWORD Region1_Sample_Count = Region1_Size / Sound_Output->Bytes_Per_Sample;
    int16 *Sample_Out = (int16*)Region1;

    real32 Sine_Value = 0;
    int16 SampleValue = 0;

    for (DWORD Sample_Index = 0; Sample_Index < Region1_Sample_Count; ++Sample_Index)
    {
        Sine_Value = sinf(Sound_Output->Wave_Phase);
        SampleValue = (int16)(Sound_Output->Sound_Volume * Sine_Value);

        *Sample_Out++ = SampleValue;
        *Sample_Out++ = SampleValue;

        Sound_Output->Wave_Phase += (2.0f * PI) * (1.0f / Sound_Output->Samples_Per_Cycle);
        if (Sound_Output->Wave_Phase > (2.0f * PI)) {
            Sound_Output->Wave_Phase -= (2.0f * PI);
        }
        ++Sound_Output->Running_Sample_Index;
    }

    DWORD Region2_Sample_Count = Region2_Size / Sound_Output->Bytes_Per_Sample;
    Sample_Out = (int16*)Region2;

    for (DWORD Sample_Index = 0; Sample_Index < Region2_Sample_Count; ++Sample_Index)
    {
        Sine_Value = sinf(Sound_Output->Wave_Phase);
        SampleValue = (int16)(Sound_Output->Sound_Volume * Sine_Value);

        *Sample_Out++ = SampleValue;
        *Sample_Out++ = SampleValue;

        Sound_Output->Wave_Phase += (2.0f * PI) * (1.0f / Sound_Output->Samples_Per_Cycle);
        if (Sound_Output->Wave_Phase > (2.0f * PI)) {
            Sound_Output->Wave_Phase -= (2.0f * PI);
        }
        ++Sound_Output->Running_Sample_Index;
    }

    Global_Secondary_Buffer->Unlock(&Region1, Region1_Size, &Region2, Region2_Size);
}


int CALLBACK WinMain(_In_ HINSTANCE Instance,
                     _In_opt_ HINSTANCE PrevInstance,
                     _In_ LPSTR CommandLine,
                     _In_ int ShowCode)
{
    Win32_Load_XInput(); // If you have a Game Controller...

    WNDCLASSA WindowClass = {};

    Win32_Resize_DIB_Section(&Global_Back_Buffer, 1280, 720);

    WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    WindowClass.lpfnWndProc = Win32_Main_Window_Callback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "MySweetWindowClass";

    if (!RegisterClassA(&WindowClass)) {
        OutputDebugStringA("Warning: Failed to Register Window-Class");
        exit(EXIT_FAILURE);
    }

    HWND Window = CreateWindowExA(0, WindowClass.lpszClassName,
                                    "Sweet Gradient Animation",
                                    WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                    CW_USEDEFAULT, CW_USEDEFAULT,
                                    CW_USEDEFAULT, CW_USEDEFAULT,
                                    0, 0, Instance, 0);
    if (!Window) {
        OutputDebugStringA("Warning: Failed to Create Window");
        exit(EXIT_FAILURE);
    }

    HDC DeviceContext = GetDC(Window);

    // NOTE: DirectSound Output Test
    // FIXME and TEST: [START]
    Global_Sound_Output.Wave_Frequency        = 432;
    Global_Sound_Output.Samples_Per_Second    = 48'000;
    Global_Sound_Output.Latency_Sample_Count  = Global_Sound_Output.Samples_Per_Second / 15;
    Global_Sound_Output.Samples_Per_Cycle     = Global_Sound_Output.Samples_Per_Second / Global_Sound_Output.Wave_Frequency;
    Global_Sound_Output.Bytes_Per_Sample      = 2 * sizeof(int16);
    Global_Sound_Output.Secondary_Buffer_Size = Global_Sound_Output.Samples_Per_Second * Global_Sound_Output.Bytes_Per_Sample;
    Global_Sound_Output.Running_Sample_Index  = 0;
    Global_Sound_Output.Wave_Phase            = 0;
    Global_Sound_Output.Sound_Volume          = 2'000;

    Win32_Init_DirectSound(Window, Global_Sound_Output.Samples_Per_Second,
                           Global_Sound_Output.Secondary_Buffer_Size);

    Win32_Fill_Sound_Buffer(&Global_Sound_Output, 0,
                            Global_Sound_Output.Latency_Sample_Count * Global_Sound_Output.Bytes_Per_Sample);

    // Play Sound
    if (FAILED(Global_Secondary_Buffer->Play(0, 0, DSBPLAY_LOOPING)))
    {
        OutputDebugStringA("Warning: Failed to Play Sound");
        exit(EXIT_FAILURE);
    }

    DWORD Play_Cursor    = 0;
    DWORD Target_Cursor  = 0;
    DWORD Write_Cursor   = 0;
    DWORD Bytes_To_Lock  = 0;
    DWORD Bytes_To_Write = 0;
    // FIXME and TEST: [END]

    Global_Running = true;
    MSG Message;
    int OffsetX = 0;
    int OffsetY = 0;

    while (Global_Running)
    {
        if (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
        {
            if (Message.message == WM_QUIT) { Global_Running = false; }
            TranslateMessage(&Message);
            DispatchMessageA(&Message);
        }

        for (int Controller_Index = 0; Controller_Index < XUSER_MAX_COUNT; Controller_Index++)
        {
            XINPUT_STATE Controller_State;

            // FIXME: not working
            if (FAILED(XInputGetState(Controller_Index, &Controller_State)))
            {
                // NOTE: No Need to Panic; Just use Keyboard
                OutputDebugStringA("Game-Controller is NOT Available!");
                OffsetX = Global_OffsetX;
                OffsetY = Global_OffsetY;
            }

            // NOTE: controller is plugged in
            // TODO: check if 'Controller_State.dwPacketNumber' increments too rapidly

            XINPUT_GAMEPAD* Pad = &Controller_State.Gamepad;

            WORD Button_Pressed = Pad->wButtons;

            bool Is_Up             = (Button_Pressed & XINPUT_GAMEPAD_DPAD_UP);
            bool Is_Down           = (Button_Pressed & XINPUT_GAMEPAD_DPAD_DOWN);
            bool Is_Left           = (Button_Pressed & XINPUT_GAMEPAD_DPAD_LEFT);
            bool Is_Right          = (Button_Pressed & XINPUT_GAMEPAD_DPAD_RIGHT);
            bool Is_Start          = (Button_Pressed & XINPUT_GAMEPAD_START);
            bool Is_Back           = (Button_Pressed & XINPUT_GAMEPAD_BACK);
            bool Is_Left_Shoulder  = (Button_Pressed & XINPUT_GAMEPAD_LEFT_SHOULDER);
            bool Is_Right_Shoulder = (Button_Pressed & XINPUT_GAMEPAD_RIGHT_SHOULDER);
            bool Is_A              = (Button_Pressed & XINPUT_GAMEPAD_A);
            bool Is_B              = (Button_Pressed & XINPUT_GAMEPAD_B);
            bool Is_X              = (Button_Pressed & XINPUT_GAMEPAD_X);
            bool Is_Y              = (Button_Pressed & XINPUT_GAMEPAD_Y);

            int16 StickX = Pad->sThumbLX;
            int16 StickY = Pad->sThumbLY;

            if (Is_Right) OffsetX += (StickX >> 12);
            else if (Is_Left) OffsetX -= (StickX >> 12);
            else if (Is_Right) OffsetY += (StickY >> 12);
            else if (Is_Left) OffsetY -= (StickY >> 12);
        }

        XINPUT_VIBRATION Vibration;
        Vibration.wLeftMotorSpeed  = 60000;
        Vibration.wRightMotorSpeed = 60000;
        XInputSetState(0, &Vibration);

        Render_Some_Weird_Gradient(&Global_Back_Buffer, Global_OffsetX, Global_OffsetY);

        // FIXME and TEST: [START]
        // NOTE: DirectSound Output Test
        if (FAILED(Global_Secondary_Buffer->GetCurrentPosition(&Play_Cursor, &Write_Cursor))) {
            OutputDebugStringA("Warning: Failed to Get the Current Position of Sound-play");
            exit(EXIT_FAILURE);
        }

        Bytes_To_Lock = (Global_Sound_Output.Running_Sample_Index * Global_Sound_Output.Bytes_Per_Sample)
                        % Global_Sound_Output.Secondary_Buffer_Size;

        Target_Cursor = (Play_Cursor
                      + (Global_Sound_Output.Latency_Sample_Count * Global_Sound_Output.Bytes_Per_Sample))
                      % Global_Sound_Output.Secondary_Buffer_Size;

        // TODO: change this using a lower latency offset from the 'Target_Cursor'
        // TODO: when we're actually start having sound effects...
        Bytes_To_Write = 0;
        if (Bytes_To_Lock < Target_Cursor) {
            Bytes_To_Write = Target_Cursor - Bytes_To_Lock;
        } else if (Bytes_To_Lock > Target_Cursor) {
            Bytes_To_Write = (Global_Sound_Output.Secondary_Buffer_Size - Bytes_To_Lock) + Target_Cursor;
        }

        Win32_Fill_Sound_Buffer(&Global_Sound_Output, Bytes_To_Lock, Bytes_To_Write);
        // FIXME and TEST: [END]

        Win32_Window_Dimension Dimention = Win32_Get_Window_Dimension(Window);
        Win32_Copy_Buffer_To_Window(&Global_Back_Buffer, DeviceContext,
                                    Dimention.Width, Dimention.Height);
    }

    return EXIT_SUCCESS;
}
