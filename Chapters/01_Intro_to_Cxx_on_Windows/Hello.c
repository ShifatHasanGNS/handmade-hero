#include <windows.h>

void greet(void)
{
    const char *message = "'Hello World' from C, using <windows.h>!\n";
    OutputDebugStringA(message);
}

// In C, WinMain can have a void/int return type
int CALLBACK WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine,
                     int nShowCmd)
{
    greet();

    /*
     * All C language features work just fine!
     */

    // return 0; // optional to add for C (for int return type)
}
