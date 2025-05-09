WINDRES_PATH = C:\DevTools\MSYS2\mingw64\bin\windres.exe

all:
	$(WINDRES_PATH) resource.rc -O coff -o resource.res
	g++ main.cpp resource.res -o IconApp.exe
