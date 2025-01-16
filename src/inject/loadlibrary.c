#include "inject/loadlibrary.h"

#include "common/logger.h"
#include "common/util.h"

enum status_t load_library_a(const parsed_args_t* args)
{
    HANDLE process_handle = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, args->process_id);

    if (!args->process_id || !process_handle)
    {
        LOG_MSG(args, "Failed to open process", 0);
        return status_process_not_running;
    }

    TCHAR full_dll_path[MAX_PATH] = { 0 };
    GetFullPathName(args->payload_path, MAX_PATH, full_dll_path, NULL);

    if ( !file_exists(args->payload_path) ||
         !file_exists(full_dll_path) )
    {
        LOG_MSG(args, "DLL path is not valid", 0);
        return status_dll_does_not_exist;
    }
    LOG_MSG(args, "DLL path is valid", 0);

    LPVOID pLoadLibraryA = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    if (!pLoadLibraryA) {
        LOG_MSG(args, "Failed to load LoadLibraryA", 0);
        return status_injection_failed;
    }
    LOG_MSG(args, "LoadLibraryA successfully loaded", 0);

    // Allocate space to write the DLL function
    LPVOID dll_parameter_address = VirtualAllocEx(process_handle, 0, strlen(full_dll_path), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!dll_parameter_address) {
        LOG_MSG(args, "Failed to allocate memory to target process", 0);
        CloseHandle(process_handle);
        return status_injection_failed;
    }
    LOG_MSG(args, "Allocated memory to target process", 0);

    BOOL wrote_memory = WriteProcessMemory(process_handle, dll_parameter_address, full_dll_path, strlen(full_dll_path), NULL);
    if (!wrote_memory) {
        LOG_MSG(args, "Failed to write memory to target process", 0);
        CloseHandle(process_handle);
        return status_injection_failed;
    }
    LOG_MSG(args, "Wrote memory to target process", 0);

    HANDLE dll_thread_handle = CreateRemoteThread(process_handle, 0, 0, (LPTHREAD_START_ROUTINE)pLoadLibraryA, dll_parameter_address, 0, 0);
    if (!dll_thread_handle) {
        LOG_MSG(args, "Failed to create remote thread in target process", 0);
        CloseHandle(process_handle);
        return status_injection_failed;
    }
    LOG_MSG(args, "Remote thread created in target process", 0);

    VirtualFreeEx(process_handle, dll_parameter_address, (size_t)wrote_memory, MEM_RELEASE);
    CloseHandle(dll_thread_handle);
    CloseHandle(process_handle);
    
    return 0;
}

enum status_t load_library_w(const parsed_args_t* args)
{
    HANDLE process_handle = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, args->process_id);

    if (!args->process_id || !process_handle)
    {
        LOG_MSG(args, "Failed to open process", 0);
        return status_process_not_running;
    }

    TCHAR full_dll_path[MAX_PATH] = { 0 };
    GetFullPathName(args->payload_path, MAX_PATH, full_dll_path, NULL);

    if ( !file_exists(args->payload_path) ||
         !file_exists(full_dll_path) )
    {
        return status_dll_does_not_exist;
    }

    WCHAR dll_w[MAX_PATH] = { 0 };
    WCHAR full_dll_path_w[MAX_PATH] = { 0 };

    for (size_t i = 0; args->payload_path[i] != 0; ++i)
    {
        dll_w[i] = (WCHAR)args->payload_path[i];
    }
    GetFullPathNameW(dll_w, MAX_PATH, full_dll_path_w, NULL);

    LPVOID pLoadLibraryW = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryW");
    if (!pLoadLibraryW)
    {
        LOG_MSG(args, "Failed to load LoadLibraryW", 0);
        return status_injection_failed;
    }
    LOG_MSG(args, "LoadLibraryW successfully loaded", 0);

    // Allocate space to write the DLL function
    LPVOID dll_parameter_address = VirtualAllocEx(process_handle, 0, wcslen(full_dll_path_w), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!dll_parameter_address)
    {
        LOG_MSG(args, "Failed to allocate memory to target process", 0);
        CloseHandle(process_handle);
        return status_injection_failed;
    }
    LOG_MSG(args, "Allocated memory to target process", 0);

    BOOL wrote_memory = WriteProcessMemory(process_handle, dll_parameter_address, full_dll_path, wcslen(full_dll_path_w), NULL);
    if (!wrote_memory)
    {
        LOG_MSG(args, "Failed to write memory to target process", 0);
        CloseHandle(process_handle);
        return status_injection_failed;
    }
    LOG_MSG(args, "Wrote memory to target process", 0);

    HANDLE dll_thread_handle = CreateRemoteThread(process_handle, 0, 0, (LPTHREAD_START_ROUTINE)pLoadLibraryW, dll_parameter_address, 0, 0);
    if (!dll_thread_handle)
    {
        LOG_MSG(args, "Failed to create remote thread in target process", 0);
        CloseHandle(process_handle);
        return status_injection_failed;
    }
    LOG_MSG(args, "Remote thread created in target process", 0);

    VirtualFreeEx(process_handle, dll_parameter_address, (size_t)wrote_memory, MEM_RELEASE);
    CloseHandle(dll_thread_handle);
    CloseHandle(process_handle);
    
    return 0;
}
