/*
	Minimal Drag and Drop Application for Windows
	
	===
	
	This application creates an application window and a console window.
	Whenever you drag a file or a set of files from the Windows explorer
	to the application window, the absolute paths of the dropped files
	are printed in the console window.
*/

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN

#define CINTERFACE
#define COBJMACROS

#include <windows.h>
#include "DropSource.h"
#include <string>
#include "oleidl.h"
#include "ShObjIdl.h"
#include "ShlObj.h"
#include <WinUser.h>

HWND hwnd;
bool isRunning;
WNDCLASSEX windowClass;
// std::wstring fName = L"C:\\Users\\miste\\Desktop\\miniaudio-loopback\\Makefile";
std::wstring fName = L"C:\\Users\\miste\\Desktop\\miniaudio-loopback\\development\\mindragndropwin copy\\main.cpp";

size_t to_narrow(const wchar_t *src, char *dest, size_t dest_len)
{
	size_t i;
	wchar_t code;

	i = 0;

	while (src[i] != '\0' && i < (dest_len - 1))
	{
		code = src[i];
		if (code < 128)
			dest[i] = char(code);
		else
		{
			dest[i] = '?';
			if (code >= 0xD800 && code <= 0xDBFF)
				// lead surrogate, skip the next code unit, which is the trail
				i++;
		}
		i++;
	}

	dest[i] = '\0';

	return i - 1;
}

LRESULT CALLBACK wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_NOTIFY:
	{
		printf("wm_notify event\n");
		auto pdi = (NMLVDISPINFO*) lParam;
		auto nmlv = (NMLISTVIEW*) lParam;
		switch (pdi->hdr.code)
		{
		case LVN_BEGINDRAG:
			printf("begin drag\n");
			IDataObject *pObj;
			IDropSource *pSrc;
			char* dest = new char[fName.length()];
			to_narrow(LPWSTR(fName.c_str()), dest, fName.length());
			pObj = (IDataObject*)GetFileUiObject(dest, IID_IDataObject);
			if (!pObj)
				break;

			pSrc = CreateDropSource();
			if (!pSrc)
			{
				IDataObject_Release(pObj);
				break;
			}

			DWORD dwEffect;
			DoDragDrop(pObj, pSrc, DROPEFFECT_COPY | DROPEFFECT_LINK, &dwEffect);

			IDropSource_Release(pSrc);
			IDataObject_Release(pObj);
			printf("end drag event\n");
			break;
		}
	}
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
	case WM_LBUTTONDOWN:
	{
		printf("button click\n");
		IDataObject *pObj;
		IDropSource *pSrc;
		char* dest = new char[fName.length() + 1];
		to_narrow(LPWSTR(fName.c_str()), dest, fName.length() + 1);
		pObj = (IDataObject*)GetFileUiObject(dest, IID_IDataObject);
		if (!pObj) {
			printf(dest);
			printf("no object\n");
			break;
		}

		pSrc = CreateDropSource();
		if (!pSrc)
		{
			printf("already released\n");
			IDataObject_Release(pObj);
			break;
		}

		DWORD dwEffect;
		DoDragDrop(pObj, pSrc, DROPEFFECT_COPY | DROPEFFECT_LINK, &dwEffect);

		IDropSource_Release(pSrc);
		IDataObject_Release(pObj);
		printf("button event end\n");
		break;
	}
	default:
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

bool createWindow(HINSTANCE hInstance, int width, int height, int bpp)
{
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
	windowClass.lpszClassName   = "DragnDropClass";
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
		"Minimal Drag and Drop Application for Windows",
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

	// Button
	HWND hwndButton = CreateWindow( 
		"BUTTON",  // Predefined class; Unicode assumed 
		"OK",      // Button text 
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
		10,         // x position 
		10,         // y position 
		100,        // Button width
		100,        // Button height
		hwnd,     // Parent window
		NULL,       // No menu.
		(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), 
		NULL);      // Pointer not needed.
	ShowWindow(hwndButton, SW_SHOW);
	UpdateWindow(hwndButton);

	HRESULT oleResult = OleInitialize(NULL);
	if(oleResult != S_OK)
	{
		std::cout << "Couldn't initialize Ole" << std::endl;
		return false;
	}
	InitCommonControls();

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
	SetConsoleTitle("Minimal Drag and Drop Application for Windows: Output Console");
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);

    if (!createWindow(hInstance, 800, 600, 32))
    {
		system("PAUSE");
        return 1;
    }

    while(processEvents())
    {
    }

    return 0;
}