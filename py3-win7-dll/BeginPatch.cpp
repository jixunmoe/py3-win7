#include "Common.h"
#include "PatchUtil.h"
#include "DetourXS/detourxs.h"
#include "DllLoader.h"
#include "inject_dll_create_thread.h"
#include "FindStrInBetween.hpp"

#include <Msi.h>
#include <string>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

tCreateProcessW CreateProcessWOrig;
wchar_t CreateProcessDLL[MAX_PATH] = { 0 };

DetourXS CreateProcessWDetour;


typedef BOOL(WINAPI* tVerifyVersionInfoW)(
    _Inout_ LPOSVERSIONINFOEXW osvi,
    _In_    DWORD dwTypeMask,
    _In_    DWORDLONG dwlConditionMask
);

DetourXS VerifyVersionInfoWDetour;
tVerifyVersionInfoW VerifyVersionInfoWOrig;

BOOL
WINAPI
MyVerifyVersionInfoW(
    _Inout_ LPOSVERSIONINFOEXW osvi,
    _In_    DWORD dwTypeMask,
    _In_    DWORDLONG dwlConditionMask
) {
    // return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WINTHRESHOLD), LOBYTE(_WIN32_WINNT_WINTHRESHOLD), 0);
    // WORD wMajorVersion, WORD wMinorVersion

    DWORDLONG        const dwlConditionMaskGTE = VerSetConditionMask(
        VerSetConditionMask(
            VerSetConditionMask(
                0, VER_MAJORVERSION, VER_GREATER_EQUAL),
            VER_MINORVERSION, VER_GREATER_EQUAL),
        VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL
    );

    if (dwlConditionMask == dwlConditionMaskGTE) {
        // HIBYTE(_WIN32_WINNT_WIN7), LOBYTE(_WIN32_WINNT_WIN7), 1
        if (osvi->dwMajorVersion >= HIBYTE(_WIN32_WINNT_WIN7) && osvi->dwMinorVersion >= LOBYTE(_WIN32_WINNT_WIN7)) {
            osvi->dwMajorVersion = HIBYTE(_WIN32_WINNT_WIN7);
            osvi->dwMinorVersion = LOBYTE(_WIN32_WINNT_WIN7);
        }
    }

    return VerifyVersionInfoWOrig(osvi, dwTypeMask, dwlConditionMask);
};


typedef BOOL(WINAPI* tMsiInstallProductW)(
    _In_ LPCWSTR     szPackagePath,
    _In_opt_ LPCWSTR szCommandLine
    );

DetourXS MsiInstallProductWDetour;
tMsiInstallProductW MsiInstallProductWOrig;

void CopyPythonAPIDLL(std::wstring& pythonInstallDirStr) {
    auto pythonInstallDir = fs::path(pythonInstallDirStr);
    auto pythonInstallPath = pythonInstallDir / L"python.exe";
    if (!fs::exists(pythonInstallPath)) return;

    std::ifstream pythonExeStream(pythonInstallPath, std::ios::binary);
    IMAGE_DOS_HEADER dos_header;
    pythonExeStream.read(reinterpret_cast<char*>(&dos_header), sizeof(dos_header));
    if (pythonExeStream.gcount() != sizeof(dos_header)) return;
    if (dos_header.e_magic != IMAGE_DOS_SIGNATURE) return;
    
    pythonExeStream.seekg(dos_header.e_lfanew, std::ios::beg);

    IMAGE_NT_HEADERS nt_header;
    pythonExeStream.read(reinterpret_cast<char*>(&nt_header), sizeof(nt_header));
    if (pythonExeStream.gcount() != sizeof(nt_header)) return;
    if (nt_header.Signature != IMAGE_NT_SIGNATURE) return;

    const auto magic = nt_header.OptionalHeader.Magic;
    auto dll_copy_dir = fs::path(CreateProcessDLL).parent_path();
    if (magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        dll_copy_dir /= L"x86";
    }
    else if (magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        dll_copy_dir /= L"x64";
    }
    else {
        return;
    }

    fs::copy(dll_copy_dir, pythonInstallDir, fs::copy_options::update_existing | fs::copy_options::recursive);
}

UINT WINAPI MyMsiInstallProductW(
    _In_ LPCWSTR     szPackagePath,
    _In_opt_ LPCWSTR szCommandLine) {
    std::wstring strPackagePath(szPackagePath);

    auto err = MsiInstallProductWOrig(szPackagePath, szCommandLine);
    if (err == ERROR_SUCCESS && szCommandLine != 0) {
        std::wstring strCommandLine(szCommandLine);

        if (!strPackagePath.ends_with(L"\\exe.msi")) {
            return err;
        }

        std::wstring strTargetDir;
        if (!FindStringInBetween(strTargetDir, strCommandLine, L" TARGETDIR=\"", L"\"")) {
            return err;
        }

        CopyPythonAPIDLL(strTargetDir);
    }

    return err;
}

void BeginPatch() {
    GetModuleFileNameW(hInstance, CreateProcessDLL, MAX_PATH);

    // wchar_t exePath[MAX_PATH] = { 0 };
    // GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    // MessageBox(nullptr, exePath, L"Loader Patch Start", MB_ICONINFORMATION);

    CreateProcessWDetour.Create(CreateProcessW, MyCreateProcessW);
    CreateProcessWOrig = (tCreateProcessW)CreateProcessWDetour.GetTrampoline();

    VerifyVersionInfoWDetour.Create(VerifyVersionInfoW, MyVerifyVersionInfoW);
    VerifyVersionInfoWOrig = (tVerifyVersionInfoW)VerifyVersionInfoWDetour.GetTrampoline();

    // 003AEC80  000D2D1F  return to python-3.10.5-amd64.000D2D1F from ???
    // 003AEC84  04F5B060  L"C:\\Users\\Jamie\\AppData\\Local\\Package Cache\\{0FE1250F-6DD6-4948-B211-741B7CDBB335}v3.10.5150.0\\exe.msi"
    // 003AEC88  04F4FC68  L" ARPSYSTEMCOMPONENT=\"1\" MSIFASTINSTALL=\"7\" TARGETDIR=\"C:\\Users\\Jamie\\AppData\\Local\\Programs\\Python\\Python310\" OPTIONALFEATURESREGISTRYKEY=\"Software\\Python\\PythonCore\\3.10\\InstalledFeatures\" ADDLOCAL=\"DefaultFeature,Shortcuts\" REBOOT=ReallySuppress"

    MsiInstallProductWDetour.Create(MsiInstallProductW, MyMsiInstallProductW);
    MsiInstallProductWOrig = (tMsiInstallProductW)MsiInstallProductWDetour.GetTrampoline();
}
