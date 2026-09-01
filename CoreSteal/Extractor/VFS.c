#include <Windows.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "VFS.h"
#include "Memory\Memory.h"
#include "Libs\sqlite3.h"

typedef struct {
    sqlite3_file base;
    const unsigned char *data;
    sqlite3_int64 size;
} MemoryFile;

static MemCtx* g_currentCtx = NULL;

static int MemClose(sqlite3_file *file) {
    MemoryFile *mem = (MemoryFile *)file;
    mem->data = NULL;
    mem->size = 0;
    return SQLITE_OK;
}

static int MemRead(sqlite3_file *file, void *buffer, int amount, sqlite3_int64 offset) {
    MemoryFile *mem = (MemoryFile *)file;

    if (offset < 0) return SQLITE_IOERR_READ;

    if (offset >= mem->size) {
        ZeroMemory(buffer, amount);
        return SQLITE_IOERR_SHORT_READ;
    }

    sqlite3_int64 available = mem->size - offset;

    if (available < amount) {
        CopyMemory(buffer, mem->data + offset, (SIZE_T)available);
        ZeroMemory((unsigned char *)buffer + available, (SIZE_T)(amount - available));
        return SQLITE_IOERR_SHORT_READ;
    }

    CopyMemory(buffer, mem->data + offset, amount);
    return SQLITE_OK;
}

static int MemWrite(sqlite3_file *file, const void *buffer, int amount, sqlite3_int64 offset) {
    return SQLITE_READONLY;
}

static int MemTruncate(sqlite3_file *file, sqlite3_int64 size) {
    return SQLITE_READONLY;
}

static int MemSync(sqlite3_file *file, int flags) {
    return SQLITE_OK;
}

static int MemFileSize(sqlite3_file *file, sqlite3_int64 *size) {
    MemoryFile *mem = (MemoryFile *)file;
    *size = mem->size;
    return SQLITE_OK;
}

static int MemLock(sqlite3_file *file, int lock) {
    return SQLITE_OK;
}

static int MemUnlock(sqlite3_file *file, int lock) {
    return SQLITE_OK;
}

static int MemCheckReservedLock(sqlite3_file *file, int *result) {
    *result = 0;
    return SQLITE_OK;
}

static int MemFileControl(sqlite3_file *file, int op, void *arg) {
    return SQLITE_NOTFOUND;
}

static int MemSectorSize(sqlite3_file *file) {
    return 4096;
}

static int MemDeviceCharacteristics(sqlite3_file *file) {
    return SQLITE_IOCAP_IMMUTABLE;
}

static const sqlite3_io_methods g_ioMethods = {
    1,
    MemClose,
    MemRead,
    MemWrite,
    MemTruncate,
    MemSync,
    MemFileSize,
    MemLock,
    MemUnlock,
    MemCheckReservedLock,
    MemFileControl,
    MemSectorSize,
    MemDeviceCharacteristics
};

static const MemBuf* FindBufferByName(MemCtx* ctx, const char* name) {
    if (!ctx || !name) return NULL;
    size_t len = strlen(name);

    if (len >= 4 && strcmp(name + len - 4, "-wal") == 0) {
        return &ctx->walFile;
    }
    if (len >= 4 && strcmp(name + len - 4, "-shm") == 0) {
        return &ctx->shmFile;
    }
    return &ctx->mainFile;
}

static int MemOpen(sqlite3_vfs *vfs, const char *name, sqlite3_file *file, int flags, int *outFlags) {
    MemoryFile *mem = (MemoryFile *)file;
    ZeroMemory(mem, sizeof(*mem));

    const MemBuf* buf = FindBufferByName(g_currentCtx, name);
    if (buf && buf->data) {
        mem->data = buf->data;
        mem->size = buf->size;
    } else {
        mem->data = NULL;
        mem->size = 0;
    }
    mem->base.pMethods = &g_ioMethods;

    if (outFlags) *outFlags = SQLITE_OPEN_READONLY;
    return SQLITE_OK;
}

static int MemDelete(sqlite3_vfs *vfs, const char *name, int syncDir) {
    return SQLITE_READONLY;
}

static int MemAccess(sqlite3_vfs *vfs, const char *name, int flags, int *result) {
    const MemBuf* buf = FindBufferByName(g_currentCtx, name);
    *result = (buf && buf->data != NULL);
    return SQLITE_OK;
}

static int MemFullPathname(sqlite3_vfs *vfs, const char *name, int outputSize, char *output) {
    if (!name) name = "database";
    sqlite3_snprintf(outputSize, output, "%s", name);
    return SQLITE_OK;
}

static void *MemDlOpen(sqlite3_vfs *vfs, const char *filename) {
    return NULL;
}

static void MemDlError(sqlite3_vfs *vfs, int size, char *message) {
    sqlite3_snprintf(size, message, "unsupported");
}

static void (*MemDlSym(sqlite3_vfs *vfs, void *handle, const char *symbol))(void) {
    return NULL;
}

static void MemDlClose(sqlite3_vfs *vfs, void *handle) {
}

static int MemRandomness(sqlite3_vfs *vfs, int size, char *buffer) {
    sqlite3_vfs *original = sqlite3_vfs_find(NULL);
    if (!original || !original->xRandomness) return 0;
    return original->xRandomness(original, size, buffer);
}

static int MemSleep(sqlite3_vfs *vfs, int microseconds) {
    sqlite3_vfs *original = sqlite3_vfs_find(NULL);
    if (!original || !original->xSleep) return microseconds;
    return original->xSleep(original, microseconds);
}

static int MemCurrentTime(sqlite3_vfs *vfs, double *time) {
    sqlite3_vfs *original = sqlite3_vfs_find(NULL);
    if (!original || !original->xCurrentTime) return SQLITE_ERROR;
    return original->xCurrentTime(original, time);
}

static sqlite3_vfs g_memoryVfs = {
    1,
    sizeof(MemoryFile),
    MAX_PATH,
    NULL,
    "memdb_vfs",
    NULL,
    MemOpen,
    MemDelete,
    MemAccess,
    MemFullPathname,
    MemDlOpen,
    MemDlError,
    MemDlSym,
    MemDlClose,
    MemRandomness,
    MemSleep,
    MemCurrentTime,
    NULL
};

void MemInit(MemCtx* ctx) {
    if (!ctx) return;
    ZeroMemory(ctx, sizeof(*ctx));
}

BOOL MemLoadFile(MemCtx* ctx, const char* srcPath) {
    if (!ctx || !srcPath) return FALSE;

    char walPath[MAX_PATH];
    char shmPath[MAX_PATH];
    DWORD mainSize = 0, walSize = 0, shmSize = 0;

    ctx->mainFile.data = (const unsigned char*)ReadFileMem(srcPath, &mainSize);
    if (!ctx->mainFile.data) return FALSE;
    ctx->mainFile.size = mainSize;

    snprintf(walPath, sizeof(walPath), "%s-wal", srcPath);
    ctx->walFile.data = (const unsigned char*)ReadFileMem(walPath, &walSize);
    ctx->walFile.size = walSize;

    snprintf(shmPath, sizeof(shmPath), "%s-shm", srcPath);
    ctx->shmFile.data = (const unsigned char*)ReadFileMem(shmPath, &shmSize);
    ctx->shmFile.size = shmSize;

    return TRUE;
}

void MemCleanup(MemCtx* ctx) {
    if (!ctx) return;
    if (ctx->mainFile.data) { Free((LPVOID)ctx->mainFile.data); }
    if (ctx->walFile.data) { Free((LPVOID)ctx->walFile.data); }
    if (ctx->shmFile.data) { Free((LPVOID)ctx->shmFile.data); }
    ZeroMemory(ctx, sizeof(*ctx));
}

int MemRegisterVfs(MemCtx* ctx, const char* vfsName) {
    if (!ctx) return SQLITE_ERROR;
    g_currentCtx = ctx;
    if (vfsName) {
        g_memoryVfs.zName = vfsName;
    } else {
        g_memoryVfs.zName = "memdb_vfs";
    }
    return sqlite3_vfs_register(&g_memoryVfs, 0);
}

int MemUnregisterVfs(const char* vfsName) {
    g_currentCtx = NULL;
    return sqlite3_vfs_unregister(&g_memoryVfs);
}

BYTE* ReadFileMem(const char* filePath, DWORD* outSize) {
    HANDLE hFile = CreateFileA(
        filePath,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        0
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        return 0;
    }

    DWORD fileSize = GetFileSize(hFile, 0);
    if (fileSize == INVALID_FILE_SIZE || fileSize == 0) {
        CloseHandle(hFile);
        return 0;
    }

    BYTE* buffer = (BYTE*)Alloc(fileSize);
    if (!buffer) {
        CloseHandle(hFile);
        return 0;
    }

    DWORD bytesRead;
    BOOL success = ReadFile(hFile, buffer, fileSize, &bytesRead, 0) && bytesRead == fileSize;
    CloseHandle(hFile);

    if (!success) {
        Free(buffer);
        return 0;
    }

    *outSize = fileSize;
    return buffer;
}

DbHandle OpenDbMem(const char* srcPath) {
    DbHandle handle = {0};
    sqlite3* dbDisk = 0;
    sqlite3* dbMem = 0;
    sqlite3_backup* backup = 0;
    int rc;
    MemCtx ctx;

    MemInit(&ctx);
    if (!MemLoadFile(&ctx, srcPath)) {
        MemCleanup(&ctx);
        return handle;
    }

    rc = MemRegisterVfs(&ctx, "memdb_vfs");
    if (rc != SQLITE_OK) {
        MemCleanup(&ctx);
        return handle;
    }

    rc = sqlite3_open_v2("database", &dbDisk, SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX, "memdb_vfs");
    if (rc != SQLITE_OK) {
        if (dbDisk) sqlite3_close(dbDisk);
        MemUnregisterVfs(NULL);
        MemCleanup(&ctx);
        return handle;
    }

    sqlite3_exec(dbDisk, "PRAGMA wal_checkpoint(TRUNCATE);", 0, 0, 0);

    rc = sqlite3_open(":memory:", &dbMem);
    if (rc != SQLITE_OK) {
        sqlite3_close(dbDisk);
        MemUnregisterVfs(NULL);
        MemCleanup(&ctx);
        return handle;
    }

    backup = sqlite3_backup_init(dbMem, "main", dbDisk, "main");
    if (backup) {
        sqlite3_backup_step(backup, -1);
        sqlite3_backup_finish(backup);
    }

    sqlite3_close(dbDisk);
    MemUnregisterVfs(NULL);
    MemCleanup(&ctx);

    handle.db = dbMem;
    return handle;
}

void CloseDb(DbHandle* handle)
{
    if (!handle) return;
    if (handle->db) sqlite3_close(handle->db);
}
