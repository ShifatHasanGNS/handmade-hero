#include <windows.h>

LRESULT CALLBACK MainWindowCallback(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;

    switch (Message) {
        case WM_SIZE: {
            OutputDebugStringA("WM_SIZE");
        } break;

        case WM_DESTROY: {
            OutputDebugStringA("WM_DESTROY");
        } break;

        case WM_CLOSE: {
            OutputDebugStringA("WM_CLOSE");
        } break;

        case WM_ACTIVATEAPP: {
            OutputDebugStringA("WM_ACTIVATEAPP");
        } break;

        case WM_PAINT: {
            OutputDebugStringA("WM_PAINT");
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(WindowHandle, &Paint);

            int OriginX = Paint.rcPaint.left;
            int OriginY = Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

            static DWORD RasterOpCode = BLACKNESS;
            PatBlt(DeviceContext, OriginX, OriginY, Width, Height, RasterOpCode);
            // Toogle Color for Epilepsy Effect
            RasterOpCode = (RasterOpCode == BLACKNESS) ? WHITENESS : BLACKNESS;

            EndPaint(WindowHandle, &Paint);
        } break;

        default:{
            Result = DefWindowProcA(WindowHandle, Message, WParam, LParam);
        } break;
    }

    return Result;
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
{
    WNDCLASS WindowClass = {};

    // NOTE: CS_..DC --> 'DC' means 'Device Context'
    // NOTE: check if 'CS_OWNDC, CS_HREDRAW, CS_VREDRAW' still matters !!

    // NO_: WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    if (RegisterClassA(&WindowClass))
    {
        HWND WindowHandle = CreateWindowExA(0,
                                            WindowClass.lpszClassName,
                                            "Handmade Hero",
                                            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                            CW_USEDEFAULT,
                                            CW_USEDEFAULT,
                                            CW_USEDEFAULT,
                                            CW_USEDEFAULT,
                                            0,
                                            0,
                                            Instance,
                                            0);

        MSG Message;
        if (WindowHandle) {
            // infinite loop without any constant parameter like '1' or 'true'
            for (;;) {
                BOOL MessageResult = GetMessageA(&Message, 0, 0, 0);

                if (MessageResult > 0) {
                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                } else {
                    break;
                }
            }
        } else { /* TODO: if WindowHandle is NULL, what then? */ }
    } else { /* TODO: Debug-Logging System */ }

    return EXIT_SUCCESS;
}
