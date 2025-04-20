# Loopback-Audio-Recorder
Tool for recording desktop audio

Uses loopback to record the output audio from the current audio output device. Saved recordings
can directly be copied to the clipboard to paste into a DAW or the filesystem.

![image](https://github.com/user-attachments/assets/1a1ca87b-caac-416e-872c-2be718b44cc2)

Currently only works on windows.

## Build
To build you just need
- [ImGui](https://github.com/ocornut/imgui) and
- [SDL2](https://github.com/libsdl-org/SDL) and
- a compiler of your choice. I'm using g++.

First download ImGui:
```sh
git clone https://github.com/ocornut/imgui
```

Then download SDL2 **in the development version** from the release page in GitHub.
Not using the development version may lead to errors with files that are not found.

![image](https://github.com/user-attachments/assets/9afde9c0-9183-4e51-98f0-d988c8c99125)

Download the `SDL2-devel-2.26.4-VC.zip` and extract it afterwards.

Finally write both installation paths in the `build.bat` file:
```bat
@REM Name of the final output file
@set OUT_EXE=Loopback-Audio-Recorder.exe

@REM Change these paths to your own installation paths
@set IMGUI_DIR=C:\DevTools\imgui <-- This path
@set SDL2_DIR=C:\DevTools\SDL2-devel-2.26.4-VC\SDL2-2.26.4 <-- And this path

@REM Build, link and whatever else
@set INCLUDES=-I%IMGUI_DIR% -I%IMGUI_DIR%\backends -I%SDL2_DIR%\include
@set SOURCES=main.cpp %IMGUI_DIR%\backends\imgui_impl_sdl2.cpp %IMGUI_DIR%\backends\imgui_impl_opengl3.cpp %IMGUI_DIR%\imgui*.cpp
@set LIBS=-L%SDL2_DIR%\lib\x64 -lSDL2main -lSDL2 -lopengl32 -lshell32
g++ %INCLUDES% %SOURCES% %LIBS% -o %OUT_EXE%

```
