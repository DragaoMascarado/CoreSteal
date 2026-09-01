#pragma once

#include <Windows.h>
#include "Libs\sqlite3.h"
#include "Memory\Memory.h"
#include "Extractor.h"

typedef struct {
    const unsigned char* data;
    sqlite3_int64 size;
} MemBuf;

typedef struct {
    MemBuf mainFile;
    MemBuf walFile;
    MemBuf shmFile;
} MemCtx;

void MemInit(MemCtx* ctx);
BOOL MemLoadFile(MemCtx* ctx, const char* srcPath);
void MemCleanup(MemCtx* ctx);
int MemRegisterVfs(MemCtx* ctx, const char* vfsName);
int MemUnregisterVfs(const char* vfsName);

BYTE* ReadFileMem(const char* filePath, DWORD* outSize);

DbHandle OpenDbMem(const char* srcPath);
void CloseDb(DbHandle* handle);
