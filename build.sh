# Name of the final output file
export OUT_EXE=out

# Change these paths to your own installation paths
export IMGUI_DIR=/Users/jbes/DevTools/imgui
export SDL2_DIR=/Users/jbes/DevTools/SDL2-2.32.4

# Build, link and whatever else
export INCLUDES=-I$IMGUI_DIR -I$IMGUI_DIR/backends -I$SDL2_DIR/include
export SOURCES=main.cpp $IMGUI_DIR/backends/imgui_impl_sdl2.cpp $IMGUI_DIR/backends/imgui_impl_opengl3.cpp $IMGUI_DIR/imgui*.cpp
export LIBS=-L$SDL2_DIR/lib/x64 -lSDL2main -lSDL2
g++ $INCLUDES $SOURCES $LIBS -o $OUT_EXE
