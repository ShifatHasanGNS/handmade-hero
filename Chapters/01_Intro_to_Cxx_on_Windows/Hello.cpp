#include <windows.h>

void greet(void)
{
    // in C++, to use a string as 'char*', it's recommened to cast it...
    // Although implicit cast works perfectly...
    const char* message = (char*)"'Hello World' from C++, using <windows.h>!\n";
    OutputDebugStringA(message);
}

// In C++, WinMain must return an int
int CALLBACK WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine,
                     int nShowCmd)
{
    greet();

    /*
     * All C++ language features work just fine!
     */

    return 0; // must add for C++
}
