#include <stdint.h>
#include <windows.h>
#include <xinput.h>

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

void Check_XInput_DLLs()
{
    const char* dlls[] = {
        "xinput1_4.dll",
        "xinput1_3.dll",
        "xinput9_1_0.dll"
    };

    for (int i = 0; i < 3; i++)
    {
        HMODULE lib = LoadLibraryA(dlls[i]);
        if (lib)
        {
            char buffer[256];
            wsprintfA(buffer, "Found: %s\n", dlls[i]);
            OutputDebugStringA(buffer);
            FreeLibrary(lib);
        }
        else
        {
            char buffer[256];
            wsprintfA(buffer, "Missing: %s\n", dlls[i]);
            OutputDebugStringA(buffer);
        }
    }
}

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


Internal void Win32_Copy_Buffer_To_Window(Win32_Offscreen_Buffer* Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight)
{
    // FIXME: Aspect Ratio
    StretchDIBits(DeviceContext,
                  0, 0, WindowWidth, WindowHeight,
                  0, 0, WindowWidth, WindowHeight,
                  Buffer->Memory, &Buffer->Info,
                  DIB_RGB_COLORS, SRCCOPY);

    // Screen blitting (bit block transfer) using the Windows
    // GDI (Graphics Device Interface) within the Win32 API is
    // the process of rapidly moving a block of pixels from
    // one location to another. It is a fundamental operation
    // used for tasks like moving sprites, drawing backgrounds,
    // and updating the screen efficiently.
    // The core function used for blitting in Win32 is BitBlt,
    // or the more powerful StretchBlt for scaling.
}


Internal LRESULT CALLBACK Win32_Main_Window_Callback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
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
            if (Alt_Key_Was_Down && (VKCode == VK_F4)) { // Alt + F4
                Global_Running = false;
            }
        } break;

        case WM_PAINT: {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            Win32_Window_Dimension Dimention = Win32_Get_Window_Dimension(Window);
            Win32_Copy_Buffer_To_Window(&Global_Back_Buffer, DeviceContext, Dimention.Width, Dimention.Height);
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
    Check_XInput_DLLs();
    Win32_Load_XInput();

    WNDCLASSA WindowClass = {};

    Win32_Resize_DIB_Section(&Global_Back_Buffer, 1280, 720);

    WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    WindowClass.lpfnWndProc = Win32_Main_Window_Callback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "MySweetWindowClass";

    if (RegisterClassA(&WindowClass))
    {
        HWND Window = CreateWindowExA(0, WindowClass.lpszClassName, "Sweet Gradient Animation",
                                      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                      CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                      0, 0, Instance, 0);
        if (Window)
        {
            HDC DeviceContext = GetDC(Window);

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

                for (int Controller_Index = 0; Controller_Index < XUSER_MAX_COUNT; Controller_Index++)
                {
                    XINPUT_STATE Controller_State;

                    if (XInputGetState(Controller_Index, &Controller_State) == ERROR_SUCCESS) // FIXME: not working
                    {
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

                        OffsetX += (StickX >> 12);
                        OffsetY += (StickY >> 12);
                    }
                    else {
                        // NOTE: controller is not available
                        OutputDebugStringA("Game-Controller is NOT Available!");

                        OffsetX = Global_OffsetX;
                        OffsetY = Global_OffsetY;
                    }
                }

                XINPUT_VIBRATION Vibration;
                Vibration.wLeftMotorSpeed  = 60000;
                Vibration.wRightMotorSpeed = 60000;
                XInputSetState(0, &Vibration);

                Render_Some_Weird_Gradient(&Global_Back_Buffer, OffsetX, OffsetY);
                Win32_Window_Dimension Dimention = Win32_Get_Window_Dimension(Window);
                Win32_Copy_Buffer_To_Window(&Global_Back_Buffer, DeviceContext, Dimention.Width, Dimention.Height);
            }
        }
    }

    return 0;
}
