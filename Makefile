WINDRES_PATH = C:\DevTools\MSYS2\mingw64\bin\windres.exe
WITHOUT_CONSOLE = -mwindows	# uncomment to hide console window
NEW_LOOK = # -DNEW_LOOK			# uncomment to compile with new windows look

all:
	$(WINDRES_PATH) resource.rc -O coff -o resource.o
	g++ $(WITHOUT_CONSOLE) main.cpp resource.o -lUxTheme -luuid -lole32 -lcomctl32 $(NEW_LOOK)

andexec: all
	./a.exe
