#ifndef INJECTOR_HEADER
#define INJECTOR_HEADER

#include "definitions.hpp"

#include <windows.h>
#include <tlhelp32.h>

unsigned inject_ManualMap(DWORD process_id, const char* dll_path);

void inject_payload(types::resource& resource, types::injector& injector);

void __handle_error(int error_code);

#endif // INJECTOR_HEADER
