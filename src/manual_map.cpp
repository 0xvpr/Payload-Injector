#include "manual_map.hpp"

#include "definitions.hpp"
#include "logger.hpp"
#include "util.hpp"

static
DWORD library_loader(LPVOID memory) {
    LoaderData* LoaderParams = (LoaderData *)memory;
    PIMAGE_BASE_RELOCATION pIBR = LoaderParams->BaseReloc;

    uintptr_t delta = (uintptr_t)((LPBYTE)LoaderParams->ImageBase - LoaderParams->NtHeaders->OptionalHeader.ImageBase); // Calculate the delta

    while (pIBR->VirtualAddress)
    {
        if (pIBR->SizeOfBlock >= sizeof(IMAGE_BASE_RELOCATION))
        {
            size_t count = (pIBR->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
            PWORD list = (PWORD)(pIBR + 1);

            for (size_t i = 0; i < count; i++)
            {
                if (list[i])
                {
                    PDWORD ptr = (PDWORD)((LPBYTE)LoaderParams->ImageBase + (pIBR->VirtualAddress + (list[i] & 0xFFF)));
                    *ptr += (0xFFFFFFFF) & delta;
                }
            }
        }

        pIBR = (PIMAGE_BASE_RELOCATION)((LPBYTE)pIBR + pIBR->SizeOfBlock);
    }

    PIMAGE_IMPORT_DESCRIPTOR pIID = LoaderParams->ImportDirectory;

    // Resolve DLL imports
    while (pIID->Characteristics)
    {
        PIMAGE_THUNK_DATA OrigFirstThunk = (PIMAGE_THUNK_DATA)((LPBYTE)LoaderParams->ImageBase + pIID->OriginalFirstThunk);
        PIMAGE_THUNK_DATA FirstThunk = (PIMAGE_THUNK_DATA)((LPBYTE)LoaderParams->ImageBase + pIID->FirstThunk);

        HMODULE hModule = LoaderParams->fnLoadLibraryA((LPCSTR)LoaderParams->ImageBase + pIID->Name);

        if (!hModule)
        {
            return FALSE;
        }

        while (OrigFirstThunk->u1.AddressOfData)
        {
            if (OrigFirstThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG)
            {
                // Import by ordinal
                uintptr_t Function = (uintptr_t)LoaderParams->fnGetProcAddress(hModule,
                    (LPCSTR)(OrigFirstThunk->u1.Ordinal & 0xFFFF));

                if (!Function)
                {
                    return FALSE;
                }

                FirstThunk->u1.Function = Function;
            }
            else
            {
                // Import by name
                PIMAGE_IMPORT_BY_NAME pIBN = (PIMAGE_IMPORT_BY_NAME)((LPBYTE)LoaderParams->ImageBase + OrigFirstThunk->u1.AddressOfData);
                uintptr_t Function = (uintptr_t)LoaderParams->fnGetProcAddress(hModule, (LPCSTR)pIBN->Name);
                if (!Function)
                {
                    return FALSE;
                }

                FirstThunk->u1.Function = Function;
            }
            OrigFirstThunk++;
            FirstThunk++;
        }
        pIID++;
    }

    if (LoaderParams->NtHeaders->OptionalHeader.AddressOfEntryPoint)
    {
        dllmain EntryPoint = (dllmain)((LPBYTE)LoaderParams->ImageBase + LoaderParams->NtHeaders->OptionalHeader.AddressOfEntryPoint);
        return (DWORD)EntryPoint((HMODULE)LoaderParams->ImageBase, DLL_PROCESS_ATTACH, nullptr);
    }

    return TRUE;
}

static
std::int32_t __declspec(naked) __stdcall stub()
{
}

std::int32_t inject_manual_map(const types::parsed_args_t& args) {
    LoaderData LoaderParams;
    TCHAR abs_payload_path[MAX_PATH];

    GetFullPathName(args.relative_payload_path.data(), MAX_PATH, abs_payload_path, nullptr);

    if ( !file_exists(args.relative_payload_path) || !file_exists(abs_payload_path))
    {
        LOG_MSG(args, "Payload path is invalid", 0);
        return inject::dll_does_not_exist;
    }

    HANDLE hFile = CreateFileA(abs_payload_path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);

    DWORD FileSize = GetFileSize(hFile, nullptr);
    PVOID FileBuffer = VirtualAlloc(nullptr, FileSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    ReadFile(hFile, FileBuffer, FileSize, nullptr, nullptr);

    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)FileBuffer;
    PIMAGE_NT_HEADERS pNtHeaders = (PIMAGE_NT_HEADERS)((LPBYTE)FileBuffer + pDosHeader->e_lfanew);

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, args.process_id);
    
    PVOID ExecutableImage = VirtualAllocEx(hProcess, nullptr, pNtHeaders->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

    // Copy the headers to target process
    WriteProcessMemory(hProcess, ExecutableImage, FileBuffer, pNtHeaders->OptionalHeader.SizeOfHeaders, nullptr);

    // Target dll_full_path's Section Header & copy sections of the dll to the target
    PIMAGE_SECTION_HEADER pSectHeader = (PIMAGE_SECTION_HEADER)(pNtHeaders + 1);
    for (int i = 0; i < pNtHeaders->FileHeader.NumberOfSections; i++)
    {
        WriteProcessMemory(hProcess, (PVOID)((LPBYTE)ExecutableImage + pSectHeader[i].VirtualAddress),
            (PVOID)((LPBYTE)FileBuffer + pSectHeader[i].PointerToRawData), pSectHeader[i].SizeOfRawData, nullptr);
    }

    // Allocating memory for the loader code.
    PVOID LoaderMemory = VirtualAllocEx(hProcess, nullptr, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

    LoaderParams.ImageBase = ExecutableImage;
    LoaderParams.NtHeaders = (PIMAGE_NT_HEADERS)((LPBYTE)ExecutableImage + pDosHeader->e_lfanew);
    LoaderParams.BaseReloc = (PIMAGE_BASE_RELOCATION)((LPBYTE)ExecutableImage
        + pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
    LoaderParams.ImportDirectory = (PIMAGE_IMPORT_DESCRIPTOR)((LPBYTE)ExecutableImage
        + pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
    LoaderParams.fnLoadLibraryA = LoadLibraryA;
    LoaderParams.fnGetProcAddress = GetProcAddress;

    // Write the loader information to target process 
    WriteProcessMemory(hProcess, LoaderMemory, &LoaderParams, sizeof(LoaderData), nullptr);
    WriteProcessMemory(hProcess, (PVOID)((LoaderData*)LoaderMemory + 1), (LPCVOID)library_loader, (uintptr_t)stub - (uintptr_t)library_loader, nullptr);
    
    
    // Create a remote thread to execute the loader code
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)((LoaderData*)LoaderMemory + 1), LoaderMemory, 0, nullptr);

    // Wait for the loader to finish executing
    WaitForSingleObject(hThread, INFINITE);
    VirtualFreeEx(hProcess, LoaderMemory, 0, MEM_RELEASE);

    return FALSE;
}