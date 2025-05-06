#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <CommCtrl.h>
#include <iostream>
#include <string>

//Returns the last Win32 error, in string format. Returns an empty string if there is no error.
std::string GetLastErrorAsString()
{
    //Get the error message ID, if any.
    DWORD errorMessageID = ::GetLastError();
    if(errorMessageID == 0) {
        return std::string(); //No error message has been recorded
    }
    
    LPSTR messageBuffer = nullptr;

    //Ask Win32 to give us the string version of that message ID.
    //The parameters we pass in, tell Win32 to create the buffer that holds the message for us (because we don't yet know how long the message string will be).
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
    
    //Copy the error message into a std::string.
    std::string message(messageBuffer, size);
    
    //Free the Win32's string's buffer.
    LocalFree(messageBuffer);
            
    return message;
}

void Err(const char* msg) {
	std::cout << msg << std::endl << GetLastErrorAsString().c_str() << std::endl;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    // Register the window class.
    const wchar_t CLASS_NAME[]  = L"Sample Window Class";
    
    WNDCLASS wc = { };

    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    // Create the window.

    HWND hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        L"Learn to Program Windows",    // Window text
        WS_OVERLAPPEDWINDOW,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
        );
    
    HWND hwndButton = CreateWindowW(
            L"BUTTON",  // Predefined class; Unicode assumed 
            L"",      // Button text 
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_BITMAP,  // Styles 
            10,         // x position 
            10,         // y position 
            350,        // Button width
            300,        // Button height
            hwnd,     // Parent window
            (HMENU)101,       // No menu.
            NULL, 
    nullptr);      // Pointer not needed.
    auto hImg = LoadImageW(NULL, L"Logo.bmp", IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE | LR_CREATEDIBSECTION | LR_LOADFROMFILE);
    if (hImg == NULL) {
        Err("could not load image");
    }
    SendMessageW(hwndButton, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM) hImg);

    HWND hwndButton2 = CreateWindowW(
            L"BUTTON",  // Predefined class; Unicode assumed 
            L"",      // Button text 
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_ICON,  // Styles 
            380,         // x position 
            10,         // y position 
            350,        // Button width
            300,        // Button height
            hwnd,     // Parent window
            (HMENU)102,       // No menu.
            NULL, 
    nullptr);      // Pointer not needed.
    auto hImg2 = LoadImageW(NULL, L"AProject.ico", IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE | LR_CREATEDIBSECTION | LR_LOADFROMFILE);
    if (hImg2 == NULL) {
        Err("could not load image");
    }
    SendMessageW(hwndButton2, BM_SETIMAGE, IMAGE_ICON, (LPARAM) hImg2);
    
    // HWND staticPicture1 = CreateWindowEx(0, WC_STATIC, L"", WS_CHILD | WS_VISIBLE, 12, 12, 280, 280, hwnd, nullptr, nullptr, nullptr);
    // HBITMAP picture = reinterpret_cast<HBITMAP>(LoadImage(nullptr, L"Logo.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE));
    // SendMessage(hwndButton, BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(picture));

    if (hwnd == NULL)
    {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    // Run the message loop.

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            // All painting occurs here, between BeginPaint and EndPaint.

            FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));

            EndPaint(hwnd, &ps);
        }
        return 0;

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}