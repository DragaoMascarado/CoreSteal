#pragma once

#include <Windows.h>
#include <wincrypt.h>
#include <bcrypt.h>
#include "Extractor.h"

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")

BOOL AesGcmDecrypt(const BYTE* key, DWORD keyLen, const BYTE* encrypted, DWORD encryptedLen, BYTE** plaintext, DWORD* plaintextLen, BOOL skipCookieHeader);
BOOL GetEncryptedKey(BYTE** key, DWORD* keyLen);
