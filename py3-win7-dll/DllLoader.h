#pragma once

#include "Common.h"
#include "PatchUtil.h"

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);

extern HINSTANCE hInstance;

void BeginPatch();
