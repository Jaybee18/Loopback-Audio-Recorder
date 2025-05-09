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
#include <chrono>
#include <filesystem>
#include "oleidl.h"
#include "DropSource.h"
#include <codecvt>
#include "windowsx.h"
#include <regex>
#include "Error.h"
#include "Clipboard.h"

HWND hwnd;
bool isRunning;
WNDCLASSEX windowClass;
bool isRecording = false;

ma_result result;
ma_encoder_config encoderConfig;
ma_encoder encoder;
ma_decoder decoder;
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
#define ID_PLAY_BTN 4

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    ma_encoder_write_pcm_frames((ma_encoder*)pDevice->pUserData, pInput, frameCount, NULL);

    (void)pOutput;
}

void play_data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    ma_decoder* pDecoder = (ma_decoder*)pDevice->pUserData;
    if (pDecoder == NULL) {
        return;
    }

    ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount, NULL);

    (void)pInput;
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
		ma_decoder_uninit(&decoder);
		PostQuitMessage(0);
		return 0;
    case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case ID_RECORD_BTN:
		{
			if (!isRecording) {
				// change record button icon to stop record icon
				HWND hwndButton2 = GetDlgItem(hwnd, ID_RECORD_BTN);
				HICON hImg2 = (HICON)LoadImage(NULL, L"resources/square.ico", IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE | LR_LOADFROMFILE);
    			SendMessage(hwndButton2, BM_SETIMAGE, IMAGE_ICON, (LPARAM) hImg2);

				// recording stuff
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
				// change stop record button icon to start record icon
				HWND hwndButton2 = GetDlgItem(hwnd, ID_RECORD_BTN);
				HICON hImg2 = (HICON)LoadImage(NULL, L"resources/record.ico", IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE | LR_LOADFROMFILE);
    			SendMessage(hwndButton2, BM_SETIMAGE, IMAGE_ICON, (LPARAM) hImg2);

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
		case ID_PLAY_BTN:
		{
			result = ma_decoder_init_file(out_file_path.generic_string().c_str(), NULL, &decoder);
			if (result != MA_SUCCESS) {
				printf("Could not load file");
				return -2;
			}

			deviceConfig = ma_device_config_init(ma_device_type_playback);
			deviceConfig.playback.format   = decoder.outputFormat;
			deviceConfig.playback.channels = decoder.outputChannels;
			deviceConfig.sampleRate        = decoder.outputSampleRate;
			deviceConfig.dataCallback      = play_data_callback;
			deviceConfig.pUserData         = &decoder;

			if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
				printf("Failed to open playback device.\n");
				ma_decoder_uninit(&decoder);
				return -3;
			}

			if (ma_device_start(&device) != MA_SUCCESS) {
				printf("Failed to start playback device.\n");
				ma_device_uninit(&device);
				ma_decoder_uninit(&decoder);
				return -4;
			}
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
		L"LARecorder",
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
        L"",      // Button text 
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | BS_ICON,  // Styles 
        10,         // x position 
        10,         // y position 
        30,        // Button width
        30,        // Button height
        hwnd,     // Parent window
        (HMENU)ID_RECORD_BTN,       // No menu.
        (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), 
        NULL);      // Pointer not needed.
	HICON hImg = (HICON)LoadImage(NULL, L"resources/record.ico", IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_LOADFROMFILE | LR_DEFAULTSIZE);
    SendMessage(hwndButton, BM_SETIMAGE, IMAGE_ICON, (LPARAM) hImg);

	HWND hwndButton2 = CreateWindowW( 
		L"BUTTON",
		L"",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | BS_ICON,
		50,
		10,
		30,
		30,
		hwnd,
		(HMENU)ID_CLIPBOARD_BTN,
		(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), 
		NULL
	);
	HICON hImg2 = (HICON)LoadImage(NULL, L"resources/clipboard.ico", IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE | LR_LOADFROMFILE);
    SendMessage(hwndButton2, BM_SETIMAGE, IMAGE_ICON, (LPARAM) hImg2);

	HWND hwndButton3 = CreateWindowW( 
		L"BUTTON",
		L"",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | BS_ICON,
		90,
		10,
		30,
		30,
		hwnd,
		(HMENU)ID_PLAY_BTN,
		(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), 
		NULL
	);
	HICON hImg3 = (HICON)LoadImage(NULL, L"resources/play.ico", IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE | LR_LOADFROMFILE);
    SendMessage(hwndButton3, BM_SETIMAGE, IMAGE_ICON, (LPARAM) hImg3);

	HWND hFrame = CreateWindowExW(
		NULL,
		L"STATIC",
		NULL,
		WS_CHILD | WS_VISIBLE | SS_BLACKFRAME,
		130, 10, 30, 30,
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
    SetConsoleTitleA("L.A.R.");
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);

	// setup miniaudio
    encoderConfig = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, 2, 44100);
	printf(out_file_path.generic_string().c_str());

    if (!createWindow(hInstance, 176, 50 + 29, 32)) {
        system("PAUSE");
        printf("Could not create window\n");
        return 1;
    }

    while (processEvents()) {}

    return 0;
}
