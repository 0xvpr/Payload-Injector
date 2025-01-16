#include "common/util.h"

#include "common/definitions.h"
#include "common/version.h"

#include "inject/loadlibrary.h"
#include "inject/manual_map.h"
#include "inject/crt.h"

#include <tlhelp32.h>

#include <stdio.h>

typedef BOOL (WINAPI * IsWow64Process2_t)(HANDLE, USHORT*, USHORT*);

void usage(const char* error_message, parsed_args_t* args)
{
    fprintf(stderr,
        "Error message: %s.\n"
        "\n"
        "Usage:\n"
        "  %s [<optional_arguments>] <target_process> <path/to/payload> \n"
        "\n"
        "Positional arguments (no specific order required):\n"
        "  <target_process>  Specify either the executable name or process id\n"
        "  <path/to/payload> Specify either the full or relative (POSIX compliant)\n"
        "                    path to a payload.\n"
        "\n"
        "Optional arguments:\n"
        "  -i <method>       Specify injection one of the following injection methods:\n"
        "                    LoadLibraryA|lla (default method),\n"
        "                    LoadLibraryW|llw,\n"
        "                    ManualMap|mm,\n"
        "                    CreateRemoteThread|crt.\n"
        "  -d <milliseconds> Add delay to the injection.\n"
        "  -s,               Set stealth level 0-2.\n"
        "  -ss,\n"
        "  --stealth=<N>\n"
        "  -v,               Set verbosity level 0-2.\n"
        "  -vv,\n"
        "  --verbosity=<N>\n"
        "  -o <file>,        Specify output log file (verbose level 2).\n"
        "  --output-file <file>\n"
        "\n"
        "Examples:\n"
        "  Injection using default injection method (LoadLibraryA):\n"
        "    %s calc.exe ./payload.dll\n"
        "  Injection using Manual Map injection method (mm) with verbosity enabled:\n"
        "    %s -i mm ./payload.bin 1234 -v\n"
        "\n"
        "Version:\n"
        "  %d.%d.%d\n"
        "\n"
        "Author:\n"
        "  VPR\n",
        error_message,
        args->program_name,
        args->program_name,
        args->program_name,
        version_major,
        version_minor,
        version_patch
    );

    exit(1);
}

size_t get_process_ids_by_process_name(const char* const process_name, DWORD* processes, size_t processes_size)
{
    size_t n_processes = 0;

    PROCESSENTRY32 process_entry = { 0 };
    process_entry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE processes_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (Process32First(processes_snapshot, &process_entry)) {
        do {
            if (strcmp(process_entry.szExeFile, process_name) == 0) {
                processes[n_processes] = process_entry.th32ProcessID;
                ++n_processes;
            }
        } while ( (n_processes < processes_size) && Process32Next(processes_snapshot, &process_entry) );
    }

    CloseHandle(processes_snapshot);
    return n_processes;
}

int get_process_name_by_process_id(DWORD process_id, char* buffer, size_t buffer_size) {
    PROCESSENTRY32 process_entry = { 0 };
    process_entry.dwSize = sizeof(PROCESSENTRY32);
    HANDLE processes_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (Process32First(processes_snapshot, &process_entry))
    {
        do
        {
            if (process_entry.th32ProcessID == process_id)
            {
                CloseHandle(processes_snapshot);
                size_t exe_file_size = strlen(process_entry.szExeFile);
                strncpy(buffer, process_entry.szExeFile, buffer_size > exe_file_size ? buffer_size-1 : exe_file_size);
                return 1;
            }
        } while (Process32Next(processes_snapshot, &process_entry));
    }

    CloseHandle(processes_snapshot);
    return 0;
}

enum machine_t get_machine_type(const HANDLE process_handle)
{
    unsigned short host_machine = 0;
    unsigned short process_machine = 0;

    IsWow64Process2_t fIsWow64Process2 = (IsWow64Process2_t)GetProcAddress(GetModuleHandleA("kernel32"), "IsWow64Process2");
    if (!fIsWow64Process2)
    {
        fprintf(stderr, "Failed to load IsWow64Process2\n");
        return machine_type_err;
    }
    
    fprintf(stdout, "fIsWow64Process2 found: %p\n", (void *)fIsWow64Process2);

    if (!fIsWow64Process2(process_handle, &process_machine, &host_machine))
    {
        fprintf(stderr, "Failed to detect architecture\n");
        return machine_type_err;
    }

    if ((process_machine) & IMAGE_FILE_32BIT_MACHINE)
    {
        fprintf(stdout, "PE32 detected\n");
        return machine_x86;
    }

    if ((process_machine) & IMAGE_FILE_MACHINE_AMD64)
    {
        fprintf(stdout, "PE32+ detected\n");
        return machine_x64;
    }

    return machine_unknown;
}

int file_exists(const char *filename)
{
    FILE* fp = NULL;
    fopen_s(&fp, filename, "rb");

    if (!fp)
    {
        return 0;
    }
    fclose(fp);

    return 1;
}

enum errcode_t validate_arguments(parsed_args_t* args)
{
    if (!(*args->program_name))
    {
        return err_invalid_arguments;
    }

    return 0;
}
