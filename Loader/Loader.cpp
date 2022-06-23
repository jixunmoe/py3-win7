#include <Windows.h>
#include <shlwapi.h>
#include <stdio.h>
#include <string>
#include <cwctype>

#include "py3-win7-dll/PY3_WIN7_DLL_API.h"

static inline void trim_left_w(std::wstring& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](wchar_t ch) {
        return !std::iswspace(ch);
    }));
}


// Returns an empty string if dialog is canceled
INT WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPWSTR lpCmdLine, INT nCmdShow)
{
#if _DEBUG
    InitPy3Win7Lib_Jixun(true);
#else
    InitPy3Win7Lib_Jixun(false);
#endif

    TCHAR fileName[MAX_PATH] = TEXT("");

    std::wstring pythonInstallerPath;
    std::wstring strCmdLine(lpCmdLine);
    trim_left_w(strCmdLine);

    // Pass over command line
    if (!strCmdLine.empty()) {
        if (strCmdLine[0] == L'"') {
            auto offset_end = strCmdLine.find(L'"', 1);
            pythonInstallerPath = strCmdLine.substr(1, offset_end - 1);
            strCmdLine = strCmdLine.erase(0, offset_end + 1);
        }
        else {
            auto offset_end = strCmdLine.find(L' ');
            pythonInstallerPath = strCmdLine.substr(0, offset_end);
            strCmdLine = strCmdLine.erase(0, offset_end);
        }
    }

    // If we don't find anything useful from the command line arg, 
    if (pythonInstallerPath.empty()) {
        pythonInstallerPath.resize(MAX_PATH);

        OPENFILENAME ofn = { 0 };
        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.lpstrFilter = TEXT("Python Installer File (*.exe)\0*.exe\0All Files(*.*)\0*\0");
        ofn.lpstrFile = &pythonInstallerPath[0];
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
        ofn.lpstrDefExt = TEXT("");
        if (!GetOpenFileName(&ofn)) {
            return 0;
        }
    }

    STARTUPINFO si = { 0 };
    si.cb = sizeof(STARTUPINFO);
    PROCESS_INFORMATION pi = { 0 };
    CreateProcessW(&pythonInstallerPath[0], &strCmdLine[0], nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 0;
}
