#include <windows.h>
#include "Memory.h"

void MemCopy(void* dest, const void* src, DWORD size)
{
	DWORD i;
	for (i = 0; i < size; i++)
	{
		((LPBYTE)dest)[i] = ((LPBYTE)src)[i];
	}
}

VOID MemZero(PVOID dest, SIZE_T size)
{
	PCHAR Data = NULL;

	if ((Data = (char*)dest) == NULL)
		return;

	for (DWORD i = 0; i < size; i++)
		Data[i] = 0x00;
}

INT MemCmp(CONST VOID *buf1, CONST VOID *buf2, SIZE_T count)
{
    CONST unsigned char *p1;
    CONST unsigned char *p2;
    SIZE_T i;

    if (buf1 == NULL || buf2 == NULL) return -1;
    p1 = (CONST unsigned char *)buf1;
    p2 = (CONST unsigned char *)buf2;

    for (i = 0; i < count; i++)
    {
        if (*p1 < *p2) return -1;
        if (*p1 > *p2) return 1;
        p1++;
        p2++;
    }

    return 0;
}

LPVOID Alloc(SIZE_T Size)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Size);
}

LPVOID ReAlloc(LPVOID Memory, SIZE_T NewSize)
{
    if (!Memory) return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, NewSize);
    return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Memory, NewSize);
}

VOID Free(LPVOID Memory)
{
    HeapFree(GetProcessHeap(), 0, Memory);
}