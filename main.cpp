#define MINIAUDIO_IMPLEMENTATION
#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#define CINTERFACE
#define COBJMACROS
#define UNICODE

#include <windows.h>
#include <iostream>
#include <winuser.h>
#include "uxtheme.h"
#include "miniaudio.h"
#include <ShlObj.h>
#include <chrono>
#include <filesystem>
#include "oleidl.h"
#include "DropSource.h"
#include <codecvt>
#include "windowsx.h"
#include <regex>

HWND hwnd;
bool isRunning;
WNDCLASSEX windowClass;
bool isRecording = false;

ma_result result;
ma_encoder_config encoderConfig;
ma_encoder encoder;
ma_device_config deviceConfig;
ma_device device;
ma_backend backends[] = {
	ma_backend_wasapi /* Loopback mode is currently only supported on WASAPI. */
};
std::filesystem::path file("out.wav");
std::filesystem::path out_file_path = std::filesystem::temp_directory_path() / file;
auto recording_start_timestamp = std::chrono::high_resolution_clock::now();
auto recording_end_timestamp = std::chrono::high_resolution_clock::now();

#define ID_RECORD_BTN 1
#define ID_CLIPBOARD_BTN 2
#define ID_SAMPLE_DRAG_AREA 3

void CopyToClipboard(const char* path) {
    HGLOBAL hGlobal = GlobalAlloc(GHND, sizeof(DROPFILES) + strlen(path) + 2);

    if (!hGlobal)
    return;

    DROPFILES* dropfiles = (DROPFILES*)GlobalLock(hGlobal);

    if (!dropfiles) {
        GlobalFree(hGlobal);
        return;
    }

    dropfiles->pFiles = sizeof(DROPFILES);
    dropfiles->fNC = TRUE;
    dropfiles->fWide = FALSE;

    memcpy(&dropfiles[1], path, strlen(path));

    GlobalUnlock(hGlobal);

    if (!OpenClipboard(NULL)) {
        GlobalFree(hGlobal);
        return;
    }

    if (!EmptyClipboard()) {
        GlobalFree(hGlobal);
        return;
    }

    if (!SetClipboardData(CF_HDROP, hGlobal)) {
        GlobalFree(hGlobal);
        return;
    }

    GlobalFree(hGlobal);

    CloseClipboard();
    return;
}

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    ma_encoder_write_pcm_frames((ma_encoder*)pDevice->pUserData, pInput, frameCount, NULL);

    (void)pOutput;
}

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
    case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case ID_RECORD_BTN:
		{
			if (!isRecording) {
				if (ma_encoder_init_file(out_file_path.generic_string().c_str(), &encoderConfig, &encoder) != MA_SUCCESS) {
					printf("Failed to initialize output file.\n");
					return -1;
				}

				deviceConfig = ma_device_config_init(ma_device_type_loopback);
				deviceConfig.capture.pDeviceID = NULL; /* Use default device for this example. Set this to the ID of a _playback_ device if you want to capture from a specific device. */
				deviceConfig.capture.format    = encoder.config.format;
				deviceConfig.capture.channels  = encoder.config.channels;
				deviceConfig.sampleRate        = encoder.config.sampleRate;
				deviceConfig.dataCallback      = data_callback;
				deviceConfig.pUserData         = &encoder;

				result = ma_device_init_ex(backends, sizeof(backends)/sizeof(backends[0]), NULL, &deviceConfig, &device);
				if (result != MA_SUCCESS) {
					printf("Failed to initialize loopback device.\n");
					return -2;
				}

				result = ma_device_start(&device);
				if (result != MA_SUCCESS) {
					ma_device_uninit(&device);
					printf("Failed to start device.\n");
					return -3;
				}
				recording_start_timestamp = std::chrono::high_resolution_clock::now();
			} else {
				ma_device_stop(&device);
				ma_encoder_uninit(&encoder);
			}
			isRecording = !isRecording;
			HWND hwndButton = GetDlgItem(hwnd, ID_RECORD_BTN);
			SetWindowTextW(hwndButton, isRecording ? L"STOP RECORDING" : L"START RECORDING");
			break;
		}
		case ID_CLIPBOARD_BTN:
		{
			CopyToClipboard(out_file_path.generic_string().c_str());
			break;
		}
		default:
			break;
		}
        break;
	}
	case WM_LBUTTONDOWN:
	{
		POINT ptClick;
		ptClick.x = GET_X_LPARAM(lParam);
		ptClick.y = GET_Y_LPARAM(lParam);
		HWND hwndDragArea = GetDlgItem(hwnd, ID_SAMPLE_DRAG_AREA);
		HWND hHit = ChildWindowFromPoint(hwnd, ptClick);
		if (hHit == hwndDragArea) {
			IDataObject *pObj;
			IDropSource *pSrc;
			std::string p = std::regex_replace(out_file_path.generic_string(), std::regex("\\/"), "\\");
			pObj = (IDataObject*)GetFileUiObject(p.c_str(), IID_IDataObject);
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
		}
		break;
	}
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
	dwStyle = WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX | WS_CAPTION;

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
	
	// make window classic themed
	SetWindowTheme(hwnd, L" ", L" ");

	if(!hwnd)
	{
		std::cout << "Couldn't create window" << std::endl;
		return false;
	}

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

    HWND hwndButton = CreateWindowW( 
        L"BUTTON",  // Predefined class; Unicode assumed 
        L"START RECORDING",      // Button text 
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,  // Styles 
        10,         // x position 
        10,         // y position 
        150,        // Button width
        100,        // Button height
        hwnd,     // Parent window
        (HMENU)ID_RECORD_BTN,       // No menu.
        (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), 
        NULL);      // Pointer not needed.

	HWND hwndButton2 = CreateWindowW( 
		L"BUTTON",
		L"COPY TO CLIPBOARD",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		10,
		120,
		150,
		100,
		hwnd,
		(HMENU)ID_CLIPBOARD_BTN,
		(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), 
		NULL);

	HWND hFrame = CreateWindowExW(
		NULL,
		L"STATIC",
		NULL,
		WS_CHILD | WS_VISIBLE | SS_BLACKFRAME,
		170, 120, 150, 100,
		hwnd,
		(HMENU)ID_SAMPLE_DRAG_AREA,
		(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
		NULL
	);

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
    SetConsoleTitleA("Minimal Drag and Drop Application for Windows: Output Console");
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);

	// setup miniaudio
    encoderConfig = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, 2, 44100);
	printf(out_file_path.generic_string().c_str());

    if (!createWindow(hInstance, 335, 260, 32)) {
        system("PAUSE");
        printf("Could not create window\n");
        return 1;
    }

    while (processEvents()) {}

    return 0;
}
