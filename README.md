# Learning Stuff | "Handmade Hero" series by "Casey Muratori"

---

## Setup Guide

**Install a Package Manager if you don't have one already:**

- For **Windows**: **choco** (_Chocolatey_)
    - Go to: [https://chocolatey.org/install#install-step2](https://chocolatey.org/install#install-step2)
- For **macOS**: **brew** (_Homebrew_)
    - Go to: [https://brew.sh/](https://brew.sh/)

**Install a C/C++ Cross-Compiler: `mingw-w64`:**

- On **Windows**:

    ```sh
    choco install mingw
    ```

- On **macOS**:

    ```sh
    brew install mingw-w64
    ```

- On **Linux**:
    - Debian / Ubuntu (and derivatives):

        ```sh
        sudo apt install mingw-w64
        ```

    - Fedora:

        ```sh
        sudo dnf install mingw64-gcc-c++
        ```

    - Arch Linux:

        ```sh
        sudo pacman -S mingw-w64-gcc
        ```

**Note:**

- Modify the `build.sh` script as per your needs (for macOS and Linux).
- Windows users should create a Batch file (`build.bat`).
- Make sure the build script is working correctly.
- Feel free to use AI if any custom fixes are needed for the build script.

### Linter Configurations (`.clangd`)

- If the configs don't work, adjust paths and settings for your **OS** and system.
- Use AI to locate your **Compiler Binaries** and `windows.h` **Header File** if needed.

---

## Review Videos

- **Handmade Hero Day 005 - Windows Graphics Review**: https://youtu.be/w7ay7QXmo_o?si=4PZ7Pi1ogrl06duc

---

## Microsoft Documentation (MSDN) References

- _Build_ **`Desktop Windows Apps`** _using the_ **`Win32`** _API_: https://learn.microsoft.com/en-us/windows/win32/
- **`WinMain`** _application entry point_: https://learn.microsoft.com/en-us/windows/win32/learnwin32/winmain--the-application-entry-point
- **`WNDCLASSW`** _structure_: https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-wndclassw
- **`WNDPROC`** _callback function_: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nc-winuser-wndproc
- **`System Defined Messages`**: https://learn.microsoft.com/en-us/windows/win32/winmsg/about-messages-and-message-queues#system-defined-messages
- **`WIndows Notifications`**: https://learn.microsoft.com/en-us/windows/win32/winmsg/window-notifications
- **`DefWindowProc`** _function_: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-defwindowproca
- **`RegisterClassW`** _function_: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-registerclassw
- **`CreateWindowExW`** _function_: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createwindowexw
- **`Extended Window Styles`**: https://learn.microsoft.com/en-us/windows/win32/winmsg/extended-window-styles
- **`GetMessage`** _function_: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getmessage
- **`TranslateMessage`** _function_: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-translatemessage
- **`DispatchMessage`** _function_: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-dispatchmessage
- **`BeginPaint`** _function_: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-beginpaint
- **`EndPaint`** _function_: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-endpaint
- **`PAINTSTRUCT`** _structure_: https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-paintstruct
- **`PatBlt`** _function_: https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-patblt
- **`PostQuitMessage`** _function_: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-postquitmessage
- **`Windows GDI`** (_Graphics Device Interface_): https://learn.microsoft.com/en-us/windows/win32/gdi/windows-gdi
- **`GetClientRect`** _function_: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getclientrect
- **`CreateDIBSection`** _function_: https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createdibsection
- **`StretchDIBits`** _function_: https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-stretchdibits
- **`BitBlt`** _function_: https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-bitblt
- **`BITMAPINFO`** _structure_: https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfo
- **`BITMAPINFOHEADER`** _structure_: https://learn.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-bitmapinfoheader
- **`DeleteObject`** _function_: https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-deleteobject
- **`CreateCompatibleDC`** _function_: https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-createcompatibledc
- **`ReleaseDC`** _function_: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-releasedc
- **`VirtualAlloc`** _function_: https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc
- **`VirtualFree`** _function_: https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualfree
- **`Memory Protection Constants`**: https://learn.microsoft.com/en-us/windows/win32/Memory/memory-protection-constants
- **`VirtualProtect`** _function_: https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualprotect
- **`PeekMessageA`** function: https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-peekmessagea
- **`MSG`** structure: https://learn.microsoft.com/en-us/windows/win32/api/winuser/ns-winuser-msg
- **`xinput.h`** _Header_: https://learn.microsoft.com/en-us/windows/win32/api/xinput/
- **`XInput`** _Game Controller APIs_: https://learn.microsoft.com/en-us/windows/win32/xinput/xinput-game-controller-apis-portal
- **`XINPUT_STATE`** structure: https://learn.microsoft.com/en-us/windows/win32/api/xinput/ns-xinput-xinput_state
- **`XInputSetState`** _function_: https://learn.microsoft.com/en-us/windows/win32/api/xinput/nf-xinput-xinputsetstate
- **`XInputGetState`** _function_: https://learn.microsoft.com/en-us/windows/win32/api/xinput/nf-xinput-xinputgetstate
- **`XInputGetKeystroke`** _function_: https://learn.microsoft.com/en-us/windows/win32/api/xinput/nf-xinput-xinputgetkeystroke
- **`LoadLibraryA`**: https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibrarya
- **`Virtual-Key Codes`**: https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
- _Dynamic-link library_ (**`DLL`**) _search order_: https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-search-order
- **`Keyboard and Mouse Input`**: https://learn.microsoft.com/en-us/windows/win32/inputdev/user-input
- **`DirectSound`**: https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee416960(v=vs.85)
- **`DirectSoundCreate`** _function_: https://learn.microsoft.com/en-us/previous-versions/windows/desktop/mt708921(v=vs.85)
- **`IDirectInputDevice8::SetCooperativeLevel`** _Method_: https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee417921(v=vs.85)
- **`IDirectSound8::CreateSoundBuffer`** _Method_: https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee418039(v=vs.85)
- **`WAVEFORMATEX`** _Structure_: https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee419019(v=vs.85)
