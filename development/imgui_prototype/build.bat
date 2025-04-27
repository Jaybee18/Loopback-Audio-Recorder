@REM Name of the final output file
@set OUT_EXE=Loopback-Audio-Recorder.exe

@REM Change these paths to your own installation paths
@set IMGUI_DIR=C:\DevTools\imgui
@set SDL2_DIR=C:\DevTools\SDL2-devel-2.26.4-VC\SDL2-2.26.4

@REM Build, link and whatever else
@set INCLUDES=-I%IMGUI_DIR% -I%IMGUI_DIR%\backends -I%SDL2_DIR%\include
@set SOURCES=main.cpp %IMGUI_DIR%\backends\imgui_impl_sdl2.cpp %IMGUI_DIR%\backends\imgui_impl_opengl3.cpp %IMGUI_DIR%\imgui*.cpp
@set LIBS=-L%SDL2_DIR%\lib\x64 -lSDL2main -lSDL2 -lopengl32 -lshell32
g++ %INCLUDES% %SOURCES% %LIBS% -o %OUT_EXE%
