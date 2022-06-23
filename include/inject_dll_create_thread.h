#pragma once
#include <Windows.h>
#include "PatchUtil.h"

typedef BOOL(WINAPI* tCreateProcessW)(
    _In_opt_ LPCWSTR lpApplicationName,
    _Inout_opt_ LPWSTR lpCommandLine,
    _In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes,
    _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
    _In_ BOOL bInheritHandles,
    _In_ DWORD dwCreationFlags,
    _In_opt_ LPVOID lpEnvironment,
    _In_opt_ LPCWSTR lpCurrentDirectory,
    _In_ LPSTARTUPINFOW lpStartupInfo,
    _Out_ LPPROCESS_INFORMATION lpProcessInformation
    );
extern tCreateProcessW CreateProcessWOrig;
extern wchar_t CreateProcessDLL[MAX_PATH];

BOOL WINAPI MyCreateProcessW(
    _In_opt_ LPCWSTR lpApplicationName,
    _Inout_opt_ LPWSTR lpCommandLine,
    _In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes,
    _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
    _In_ BOOL bInheritHandles,
    _In_ DWORD dwCreationFlags,
    _In_opt_ LPVOID lpEnvironment,
    _In_opt_ LPCWSTR lpCurrentDirectory,
    _In_ LPSTARTUPINFOW lpStartupInfo,
    _Out_ LPPROCESS_INFORMATION lpProcessInformation
) {
    auto create_ok = CreateProcessWOrig(lpApplicationName,
        lpCommandLine,
        lpProcessAttributes,
        lpThreadAttributes,
        bInheritHandles,
        dwCreationFlags | CREATE_SUSPENDED,
        lpEnvironment,
        lpCurrentDirectory,
        lpStartupInfo,
        lpProcessInformation);

    if (!create_ok) {
        return FALSE;
    }

    // push 0x12345678

    /*
        push LIB_PATH *
        call LoadLibraryW
        mov eax, dword[esp+4]
        inc byte[eax]
        ret

        LIB_PATH:
            db "version.dll"
        */
    SIZE_T unused;
    unsigned char loaderCode[sizeof(CreateProcessDLL) + 5 * 2 + 8] = { 0 };
    auto pWorkDone = VirtualAllocEx(lpProcessInformation->hProcess, nullptr, 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    auto pCodeLoadLibrary = VirtualAllocEx(lpProcessInformation->hProcess, nullptr, sizeof(loaderCode), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (pCodeLoadLibrary == nullptr || pWorkDone == nullptr) {
        TerminateProcess(lpProcessInformation->hProcess, 0);
        return FALSE;
    }

    // { 0xE8, 0xFB, 0x00, 0x00, 0x00, 
    //   [MAX_PATH]
    //   0xE8, 0x7A, 0x56, 0x34, 0x12,
    //   ...
    // }

    // *push Payload
    loaderCode[0] = 0xE8;
    *reinterpret_cast<DWORD*>(&loaderCode[1]) = DWORD(sizeof(CreateProcessDLL));

    memcpy(&loaderCode[5], CreateProcessDLL, sizeof(CreateProcessDLL));

    // Call LoadLibraryW
    loaderCode[0 + 5 + sizeof(CreateProcessDLL) + 0] = 0xE8;
    WriteRelativeAddressAtPayload(loaderCode, 0 + 5 + sizeof(CreateProcessDLL) + 1, uintptr_t(pCodeLoadLibrary), uintptr_t(LoadLibraryW));
    WriteMemRaw<uint32_t>(uintptr_t(&loaderCode[0 + 5 + sizeof(CreateProcessDLL) + 5 + 0]), 0x0424448B);
    WriteMemRaw<uint32_t>(uintptr_t(&loaderCode[0 + 5 + sizeof(CreateProcessDLL) + 5 + 4]), 0x00C300FF);

    WriteProcessMemory(lpProcessInformation->hProcess, pCodeLoadLibrary, loaderCode, sizeof(loaderCode), &unused);
    DWORD unused_dword;
    VirtualProtectEx(lpProcessInformation->hProcess, pCodeLoadLibrary, sizeof(pCodeLoadLibrary), PAGE_EXECUTE_READ, &unused_dword);

    CreateRemoteThread(lpProcessInformation->hProcess, nullptr, 8, (LPTHREAD_START_ROUTINE)(pCodeLoadLibrary), pWorkDone, 0, &unused_dword);

    // Wait until the DLL is loaded.
    uint8_t buf_work = 0;
    while (ReadProcessMemory(lpProcessInformation->hProcess, pWorkDone, &buf_work, sizeof(buf_work), &unused)) {
        if (buf_work == 1) {
            VirtualFreeEx(lpProcessInformation->hProcess, pWorkDone, 0, MEM_RELEASE);
            VirtualFreeEx(lpProcessInformation->hProcess, pCodeLoadLibrary, 0, MEM_RELEASE);
            ResumeThread(lpProcessInformation->hThread);
            return TRUE;
        }
        Sleep(500);
    }

    TerminateProcess(lpProcessInformation->hProcess, 0);
    return FALSE;
}
