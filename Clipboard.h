#include <windows.h>
#include <ShlObj.h>

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
