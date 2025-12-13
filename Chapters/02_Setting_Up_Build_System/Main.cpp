#include <windows.h>

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, int nShowCmd) {

  MessageBox(NULL, "Hello, World!", "My First Windows App",
             MB_OK | MB_ICONINFORMATION);

  return 0;
}
