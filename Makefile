WINDRES_PATH = C:\DevTools\MSYS2\mingw64\bin\windres.exe
WITHOUT_CONSOLE = # -mwindows	# uncomment to hide console window

all:
	$(WINDRES_PATH) resource.rc -O coff -o resource.o
	g++ $(WITHOUT_CONSOLE) main.cpp resource.o -lUxTheme -luuid -lole32 -lcomctl32

andexec: all
	./a.exe
