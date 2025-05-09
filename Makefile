WINDRES_PATH = C:\DevTools\MSYS2\mingw64\bin\windres.exe

all:
	$(WINDRES_PATH) resource.rc -O coff -o resource.o
	g++ main.cpp resource.o -lUxTheme -luuid -lole32 -lcomctl32

andexec: all
	./a.exe
