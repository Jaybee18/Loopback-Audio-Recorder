@REM Build for Visual Studio compiler. Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
@set INCLUDES=-IC:\DevTools\imgui\ -IC:\DevTools\imgui\backends -IC:\DevTools\SDL2-devel-2.26.4-VC\SDL2-2.26.4\include -IC:\DevTools\SDL\VisualC-WinRT\x64\Debug -IC:\DevTools\implot
@set SOURCES=imgui_starter.cpp C:\DevTools\imgui\backends\imgui_impl_sdl2.cpp C:\DevTools\imgui\backends\imgui_impl_opengl3.cpp C:\DevTools\imgui\imgui*.cpp C:\DevTools\implot\implot.cpp C:\DevTools\implot\implot_items.cpp C:\DevTools\implot\implot_demo.cpp
@set LIBS=-LC:\DevTools\SDL2-devel-2.26.4-VC\SDL2-2.26.4\lib\x64 -lSDL2main -lSDL2 -lopengl32 -lshell32 -luuid -lole32 -lcomctl32
g++ %INCLUDES% %SOURCES% %LIBS%
