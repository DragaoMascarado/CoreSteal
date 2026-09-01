#pragma once

void MemCopy(void* dest, const void* src, DWORD size);
VOID MemZero(PVOID dest, SIZE_T size);
INT MemCmp(CONST VOID *buf1, CONST VOID *buf2, SIZE_T count);
LPVOID Alloc(SIZE_T Size);
LPVOID ReAlloc(LPVOID Memory, SIZE_T NewSize);
VOID Free(LPVOID Memory);