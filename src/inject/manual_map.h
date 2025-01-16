#ifndef   MANUAL_MAP_HEADER
#define   MANUAL_MAP_HEADER

#include  "common/definitions.h"

typedef FARPROC(__stdcall * pGetProcAddress)(HMODULE, LPCSTR);
typedef INT(__stdcall * dllmain)(HMODULE, DWORD, LPVOID);
typedef HMODULE(__stdcall * pLoadLibraryA)(LPCSTR);

typedef struct _LoaderData {
	LPVOID ImageBase;
	PIMAGE_NT_HEADERS NtHeaders;
	PIMAGE_BASE_RELOCATION BaseReloc;
	PIMAGE_IMPORT_DESCRIPTOR ImportDirectory;
	pLoadLibraryA fnLoadLibraryA;
	pGetProcAddress fnGetProcAddress;
} LoaderData;

enum status_t manual_map(const parsed_args_t*);

#endif // MANUAL_MAP_HEADER
