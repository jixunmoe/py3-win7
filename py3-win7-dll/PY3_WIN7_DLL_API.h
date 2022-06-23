#pragma once

#ifdef PY3_WIN7_DLL_EXPORTS
#define PY3_WIN7_API __declspec(dllexport)
#else
#define PY3_WIN7_API __declspec(dllimport)
#endif

PY3_WIN7_API void __stdcall InitPy3Win7Lib_Jixun(bool debug);
