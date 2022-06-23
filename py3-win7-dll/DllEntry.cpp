#include "DllLoader.h"
#include "BeginPatch.h"
#include "PY3_WIN7_DLL_API.h"

HINSTANCE hInstance;
HMODULE hDll;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		hInstance = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);
		BeginPatch();
		break;

	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

PY3_WIN7_API void __stdcall InitPy3Win7Lib_Jixun(bool debug)
{
	if (debug) {
		// Re-enable printf functions.
		FILE* stream;
		if (!AttachConsole(-1))
			AllocConsole();
		freopen_s(&stream, "CONOUT$", "w+", stdout);
	}
}
