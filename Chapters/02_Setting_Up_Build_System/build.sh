#!/bin/bash

FILES="./main.cpp"
EXE="./build/app.exe"

case $1 in
    clean)
        rm -rf build
        ;;
    debug)
        mkdir -p build
        time x86_64-w64-mingw32-g++ -std=c++20 -luser32 -lgdi32 -lkernel32 -g -O0 -Wall -Wextra ${FILES} -o ${EXE}
        ;;
    *)
        mkdir -p build
        time x86_64-w64-mingw32-g++ -std=c++20 -luser32 -lgdi32 -lkernel32 ${FILES} -o ${EXE}
        ;;
esac
