#define CINTERFACE
#define COBJMACROS

#include <iostream>
#include "oleidl.h"
#include "ShlObj.h"

typedef struct __DSV_TDropSource {
    IDropSource     This;
    IDropSourceVtbl Func;
    ULONG           RefCnt;
} __DSV_TDropSource;

HRESULT WINAPI __DSV_QueryInterface(IDropSource *This, REFIID riid, void **ppvObject)
{
    IUnknown *punk = NULL;

    if (riid == IID_IUnknown)
    {
        punk = (IUnknown*)This;
    }
    else if (riid == IID_IDropSource)
    {
        punk = (IUnknown*)This;
    }

    *ppvObject = punk;

    if (punk)
    {
        IUnknown_AddRef(punk);
        return S_OK;
    }
    else {
        return E_NOINTERFACE;
    }
}

ULONG WINAPI __DSV_AddRef(IDropSource *This)
{
    __DSV_TDropSource *pThis = (__DSV_TDropSource*)This;
    return pThis->RefCnt++;
}

ULONG WINAPI __DSV_Release(IDropSource *This)
{
    __DSV_TDropSource *pThis = (__DSV_TDropSource*)This;

    LONG iRes = (LONG)pThis->RefCnt - 1;
    if (iRes < 1) { iRes = 0; }
    pThis->RefCnt = iRes;

    if (iRes == 0) { free(pThis); }
    return iRes;
}

HRESULT WINAPI __DSV_QueryContinueDrag(IDropSource *This, BOOL fEscapePressed, DWORD grfKeyState)
{
    if (fEscapePressed) { return DRAGDROP_S_CANCEL; }

    if (!(grfKeyState & (MK_LBUTTON | MK_RBUTTON))) { return DRAGDROP_S_DROP; }

    return S_OK;
}

HRESULT WINAPI __DSV_GiveFeedback(IDropSource *This, DWORD dwEffect)
{
    return DRAGDROP_S_USEDEFAULTCURSORS;
}

IDropSource* CreateDropSource()
{
    __DSV_TDropSource *pResu = (__DSV_TDropSource*)malloc(sizeof(__DSV_TDropSource));
    if (!pResu) { return 0; }

    pResu->This.lpVtbl = &(pResu->Func);
    pResu->Func.QueryInterface = __DSV_QueryInterface;
    pResu->Func.AddRef = __DSV_AddRef;
    pResu->Func.Release = __DSV_Release;
    pResu->Func.QueryContinueDrag = __DSV_QueryContinueDrag;
    pResu->Func.GiveFeedback = __DSV_GiveFeedback;

    pResu->RefCnt = 1;
    return (IDropSource*)pResu;
}


void** GetFileUiObject(TCHAR *ptFile, REFIID riid)
{

    void** pInterfaceResu = 0;
    IShellFolder *pFolder;
    PIDLIST_RELATIVE pFile;
    PIDLIST_ABSOLUTE pITEMDLIST_File;
    HRESULT iResu;

    pITEMDLIST_File = ILCreateFromPath(ptFile);
    if (!pITEMDLIST_File)
        return 0;

    iResu = SHBindToParent(pITEMDLIST_File, IID_IShellFolder, (void**)&pFolder, (PCUITEMID_CHILD*)&pFile);
    if (iResu != S_OK)
        return 0;

    const ITEMIDLIST* pArray[1] = { pFile };
    iResu = IShellFolder_GetUIObjectOf(pFolder, NULL, 1, pArray, riid, NULL, (void**)&pInterfaceResu);
    if (iResu != S_OK)
        return 0;

    IShellFolder_Release(pFolder);

    return pInterfaceResu;
}
