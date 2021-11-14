#ifndef _MANUAL_MAP_H
#define _MANUAL_MAP_H

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

typedef FARPROC(__stdcall* pGetProcAddress)(HMODULE, LPCSTR);
typedef INT(__stdcall* dllmain)(HMODULE, DWORD, LPVOID);
typedef HMODULE(__stdcall* pLoadLibraryA)(LPCSTR);

typedef struct loaderdata
{
	LPVOID ImageBase;

	PIMAGE_NT_HEADERS NtHeaders;
	PIMAGE_BASE_RELOCATION BaseReloc;
	PIMAGE_IMPORT_DESCRIPTOR ImportDirectory;

	pLoadLibraryA fnLoadLibraryA;
	pGetProcAddress fnGetProcAddress;

} loaderdata;

DWORD __stdcall LibraryLoader(LPVOID Memory);

DWORD __stdcall stub(void);

int inject_ManualMap(DWORD process_id, const char* dll_path);

#endif /* _MANUAL_MAP_H */
