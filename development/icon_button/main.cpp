#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <CommCtrl.h>
#include <iostream>
#include <string>

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
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_ICON,  // Styles 
        10,         // x position 
        10,         // y position 
        50,        // Button width
        50,        // Button height
        hwnd,     // Parent window
        (HMENU)102,       // No menu.
        NULL, 
    nullptr);      // Pointer not needed.
    HICON hImg = (HICON)LoadImageW(NULL, L"AProject.ico", IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR | LR_DEFAULTSIZE | LR_LOADFROMFILE);
    SendMessageW(hwndButton, BM_SETIMAGE, IMAGE_ICON, (LPARAM) hImg);

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