#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#define UNICODE

#include <windows.h>
#include <iostream>
#include <winuser.h>

HWND hwnd;
bool isRunning;
WNDCLASSEX windowClass;

#define ID_RECORD_BTN 1

LRESULT CALLBACK wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_CREATE:
	{
		isRunning = true;
	}
	break;
	case WM_DESTROY:
	case WM_CLOSE:
	case WM_QUIT:
		isRunning = false;
		PostQuitMessage(0);
		return 0;
		break;
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_RECORD_BTN) {
            MessageBoxA(hwnd, "Some Text", "Notice", MB_OK);
        }
        break;
	default:
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

bool createWindow(HINSTANCE hInstance, int width, int height, int bpp) {
    windowClass.cbSize          = sizeof(WNDCLASSEX);
	windowClass.style           = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc     = wndProc;
	windowClass.cbClsExtra      = 0;
	windowClass.cbWndExtra      = 0;
	windowClass.hInstance       = hInstance;
	windowClass.hIcon           = LoadIcon(NULL, IDI_APPLICATION);
	windowClass.hCursor         = LoadCursor(NULL, IDC_ARROW);
	windowClass.hbrBackground   = NULL;
	windowClass.lpszMenuName    = NULL;
	windowClass.lpszClassName   = L"DragnDropClass";
	windowClass.hIconSm         = LoadIcon(NULL, IDI_WINLOGO);

    if(!RegisterClassEx(&windowClass))
	{
		std::cout << "Couldn't register window class" << std::endl;
		return false;
	}

	DWORD      dwExStyle;
	DWORD      dwStyle;
	dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	dwStyle = WS_OVERLAPPEDWINDOW;

    RECT windowRect;
	windowRect.left = 0;
	windowRect.right = width;
	windowRect.top = 0;
	windowRect.bottom = height;
	if(AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle) == 0)
	{
		std::cout << "Couldn't set the size of the window rectangle" << std::endl;
		return false;
	}

    hwnd = CreateWindowEx(0,
		windowClass.lpszClassName,
		L"Minimal Drag and Drop Application for Windows",
		dwStyle | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		width,
		height,
		NULL,
		NULL,
		hInstance,
		NULL);

	if(!hwnd)
	{
		std::cout << "Couldn't create window" << std::endl;
		return false;
	}

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

    HWND hwndButton = CreateWindowW( 
        L"BUTTON",  // Predefined class; Unicode assumed 
        L"OK",      // Button text 
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
        10,         // x position 
        10,         // y position 
        100,        // Button width
        100,        // Button height
        hwnd,     // Parent window
        (HMENU)ID_RECORD_BTN,       // No menu.
        (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), 
        NULL);      // Pointer not needed.

    HWND hBtnRecord = CreateWindowEx(
        0,
        L"BUTTON",
        nullptr,
        WS_CHILD | WS_VISIBLE |
        BS_PUSHBUTTON | BS_ICON | BS_CENTER | BS_VCENTER,
        110,
        110,
        100,
        100,
        hwnd,
        (HMENU)2,
        hInstance,
        nullptr
    );
    HICON hIconRecord = (HICON) LoadImage(
        hInstance,
        MAKEINTRESOURCE(101),
        IMAGE_ICON,
        32, 32,
        LR_DEFAULTCOLOR
    );
    SendMessage(hBtnRecord, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIconRecord);

	return true;
}

bool processEvents()
{
	MSG msg;

	while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return isRunning;
}

int WINAPI WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR cmdLine,
    int cmdShow)
{
    AllocConsole();
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTitleA("Minimal Drag and Drop Application for Windows: Output Console");
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);

    if (!createWindow(hInstance, 800, 600, 32)) {
        system("PAUSE");
        printf("Could not create window\n");
        return 1;
    }

    while (processEvents()) {}

    return 0;
}
