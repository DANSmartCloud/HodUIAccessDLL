#pragma once

#ifndef UIACCESSDLL_H
#define UIACCESSDLL_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

// 错误码定义
#define UIACCESS_SUCCESS                0
#define UIACCESS_ERROR_ALREADY_ENABLED  1
#define UIACCESS_ERROR_ACCESS_DENIED    2
#define UIACCESS_ERROR_SYSTEM_ERROR     3
#define UIACCESS_ERROR_NOT_INITIALIZED  4

// DLL导出函数声明 - 使用__stdcall调用约定以兼容易语言
__declspec(dllexport) int __stdcall UIAccess_Initialize();
__declspec(dllexport) int __stdcall UIAccess_GetPermission();
__declspec(dllexport) int __stdcall UIAccess_CheckStatus();
__declspec(dllexport) int __stdcall UIAccess_Cleanup();
__declspec(dllexport) const char* __stdcall UIAccess_GetErrorMessage(int errorCode);
__declspec(dllexport) int __stdcall UIAccess_GetLastError();
__declspec(dllexport) int __stdcall UIAccess_PrepareAndRelaunchIfNeeded();
__declspec(dllexport) int __stdcall UIAccess_SetTopmost(HWND hwnd, int enable);
__declspec(dllexport) const char* __stdcall AboutThisDLL();

#ifdef __cplusplus
}
#endif

#endif // UIACCESSDLL_H