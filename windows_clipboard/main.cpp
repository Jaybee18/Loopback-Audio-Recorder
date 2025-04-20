#include <string.h>
#include <windows.h>
#include <shellapi.h>
#include <ShlObj.h>
#pragma comment(lib, "shell32.lib")
#include <Windows.h>

int main() {
    const char* path = "C:\\Users\\miste\\Desktop\\miniaudio-loopback\\music.wav";

    HGLOBAL hGlobal = GlobalAlloc(GHND, sizeof(DROPFILES) + strlen(path) + 2);

    if (!hGlobal)
    return 1;

    DROPFILES* dropfiles = (DROPFILES*)GlobalLock(hGlobal);

    if (!dropfiles) {
    GlobalFree(hGlobal);
    return 1;
    }

    dropfiles->pFiles = sizeof(DROPFILES);
    dropfiles->fNC = TRUE;
    dropfiles->fWide = FALSE;

    memcpy(&dropfiles[1], path, strlen(path));

    GlobalUnlock(hGlobal);

    if (!OpenClipboard(NULL)) {
    GlobalFree(hGlobal);
    return 1;
    }

    if (!EmptyClipboard()) {
    GlobalFree(hGlobal);
    return 1;
    }

    if (!SetClipboardData(CF_HDROP, hGlobal)) {
    GlobalFree(hGlobal);
    return 1;
    }

    GlobalFree(hGlobal);

    CloseClipboard();
    return 0;
}
