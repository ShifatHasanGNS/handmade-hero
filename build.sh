#!/bin/bash

OUTPUT="./Build/Handmade_Hero.exe"

# Check the 'README.md' if you don't know how to install mingw-w64
# on different platforms [Windows, macOS, Linux]...

#
# Compiler configuration:
# - macOS: Use x86_64-w64-mingw32-g++ (install via Homebrew: brew install mingw-w64)
# - Linux: Usually x86_64-w64-mingw32-g++ (install via apt/yum/pacman)
# - Windows (MSYS2/MinGW): Use g++ directly (mingw already provides native compiler)
C_CPP_COMPILER="x86_64-w64-mingw32-g++" # Change to 'g++' if on Windows
#
# If you have 'zig' installed, then:
Zig_C_COMPILER="zig c++ -target x86_64-windows-gnu" # Works from any OS
#

#
# Windows libraries to link
# Note: With Zig, you may not need to explicitly link these as Zig handles them automatically
LIBS_TO_LINK="-lgdi32 -lwinmm"
# Dropped: -lkernel32 -luser32 (automatically linked by both compilers)
# Dropped: -lxinput (loaded dynamically at runtime)
#
# [ Here are some notes that I have collected from the internet ]
#
# NOTE: kernel32
# kernel32.dll (Kernel 32-bit) is a fundamental component of the Microsoft Windows OS,
# acting as a Dynamic Link Library (DLL) that manages essential functions like -
# memory allocation, input/output (I/O) operations, process/thread creation,
# and system interrupts, providing the core Windows API for applications
# to interact with the OS. It's loaded into protected memory during startup...
#
# NOTE: gdi32
# GDI32 (gdi32.dll) is a core Windows system file providing the Graphics Device Interface,
# enabling applications to draw 2D graphics, manage fonts, display text,
# and handle printer output by communicating with graphics hardware drivers,
# essential for almost all graphical functions on a Windows PC.
# It contains crucial functions for rendering and managing visual elements...
#
# NOTE: user32
# User32 refers to User32.dll, a fundamental Windows system file containing core functions
# for managing the graphical user interface (GUI), like creating and controlling
# windows, menus, and dialog boxes; it's essential for almost all interactive applications...
#
# NOTE: winmm
# WinMM (Windows Multimedia) refers to the legacy Windows Multimedia API, implemented by winmm.dll,
# providing basic audio (wave, MIDI, CD audio) and joystick functions for Windows apps,
# acting as a bridge to hardware drivers, though newer APIs like DirectSound/DirectMusic
# now handle more advanced audio. Developers use it for simple tasks, but it's also crucial
# for backward compatibility, ensuring older games and apps still work on modern Windows,
# with recent updates even adding multi-client support...
#
# NOTE: xinput
# XInput is Microsoft's modern, cross-platform API for handling game controller input,
# especially Xbox controllers, making it easy for Windows apps and games to read
# controller states (buttons, sticks, triggers) and control vibration, offering simpler integration
# than older methods like DirectInput, supporting up to four controllers with standard Xbox features,
# and providing automatic recognition for many modern gamepads.
#

usage() {
    printf "\n"
    printf "Usage:\n"
    printf "\n"
    printf "  To Remove Build:\n"
    printf "      ./build.sh clean\n"
    printf "\n"
    printf "  To Build [debug / release] with [cc / zig-cc]:\n"
    printf "      ./build.sh debug cc file01.cpp file02.cpp file03.cpp ...\n"
    printf "      ./build.sh debug zig-cc file01.cpp file02.cpp file03.cpp ...\n"
    printf "      ./build.sh release cc file01.cpp file02.cpp file03.cpp ...\n"
    printf "      ./build.sh release zig-cc file01.cpp file02.cpp file03.cpp ...\n"
    printf "\n"
}

case $1 in
    clean)
        printf "\nCleaning build directory..."
        rm -rf Build
        printf "\nDone!\n"
        ;;
    debug)
        if [[ $2 == 'cc' ]]; then
            mkdir -p Build
            printf "\nBuilding in DEBUG mode with MinGW...\n"
            time ${C_CPP_COMPILER} -std=c++26 -g -O0 "${@:3}" -o ${OUTPUT} ${LIBS_TO_LINK}
        elif [[ $2 == 'zig-cc' ]]; then
            mkdir -p Build
            printf "\nBuilding in DEBUG mode with Zig...\n"
            time ${Zig_C_COMPILER} -std=c++26 -g "${@:3}" -o ${OUTPUT} ${LIBS_TO_LINK}
        else
            printf "\nSorry! Invalid Arguments...\n"
            usage
            exit 1
        fi

        if (( $? == 0 )); then
            printf "\nBuild successful: ${OUTPUT}\n"
        else
            printf "\nBuild failed!\n"
            exit 1
        fi
        ;;
    release)
        if [[ $2 == 'cc' ]]; then
            mkdir -p Build
            printf "\nBuilding in RELEASE mode with MinGW...\n"
            time ${C_CPP_COMPILER} -std=c++20 -O2 "${@:3}" -o ${OUTPUT} ${LIBS_TO_LINK}
        elif [[ $2 == 'zig-cc' ]]; then
            mkdir -p Build
            printf "\nBuilding in RELEASE mode with Zig...\n"
            time ${Zig_C_COMPILER} -O2 "${@:3}" -o ${OUTPUT} ${LIBS_TO_LINK}
        else
            printf "\nSorry! Invalid Arguments...\n"
            usage
            exit 1
        fi

        if (( $? == 0 )); then
            printf "\nBuild successful: ${OUTPUT}\n"
        else
            printf "\nBuild failed!\n"
            exit 1
        fi
        ;;
    help)
        usage
        ;;
    *)
        usage
        exit 1
        ;;
esac
