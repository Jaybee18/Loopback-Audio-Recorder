@REM Build file for Microsoft Visual Studio Compiler. Tested with Visual Studio 2015.
@REM Here: call vcvarsall.bat
@REM g++ /nologo /Zi /MD /I .. /I ..\.. /I "%WindowsSdkDir%Include\um" /I "%WindowsSdkDir%Include\shared" /D UNICODE /D _UNICODE /D _HAS_EXCEPTIONS=0 src\main.cpp /FoDebug/ /FeDebug/mindragndropwin.exe /link "user32.lib" "shell32.lib" "ole32.lib"
@REM g++ main.cpp -I"%WindowsSdkDir%Include\um" -I"%WindowsSdkDir%Include\shared" -luser32 -lshell32 -lole32
g++ main.cpp -lole32 -lshell32 -luuid -lcomctl32
