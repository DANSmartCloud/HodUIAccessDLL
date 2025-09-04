#include "UIAccessDLL.h"
#include <tlhelp32.h>
#include <tchar.h>

// 全局变量
static BOOL g_bInitialized = FALSE;
static int g_nLastError = UIACCESS_SUCCESS;

// 内部函数声明
static DWORD DuplicateWinloginToken(DWORD dwSessionId, DWORD dwDesiredAccess, PHANDLE phToken);
static DWORD CreateUIAccessToken(PHANDLE phToken);
static BOOL CheckForUIAccess(DWORD *pdwErr, DWORD *pfUIAccess);
static DWORD PrepareForUIAccess();

// DLL入口点
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        // 进程附加时的初始化
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        // 进程分离时的清理
        g_bInitialized = FALSE;
        break;
    }
    return TRUE;
}

// 初始化函数
__declspec(dllexport) int __stdcall UIAccess_Initialize()
{
    g_bInitialized = TRUE;
    g_nLastError = UIACCESS_SUCCESS;
    return UIACCESS_SUCCESS;
}

// 获取UI Access权限
__declspec(dllexport) int __stdcall UIAccess_GetPermission()
{
    if (!g_bInitialized)
    {
        g_nLastError = UIACCESS_ERROR_NOT_INITIALIZED;
        return UIACCESS_ERROR_NOT_INITIALIZED;
    }

    DWORD dwErr = PrepareForUIAccess();
    
    switch (dwErr)
    {
    case ERROR_SUCCESS:
        g_nLastError = UIACCESS_SUCCESS;
        return UIACCESS_SUCCESS;
    case ERROR_ACCESS_DENIED:
        g_nLastError = UIACCESS_ERROR_ACCESS_DENIED;
        return UIACCESS_ERROR_ACCESS_DENIED;
    default:
        g_nLastError = UIACCESS_ERROR_SYSTEM_ERROR;
        return UIACCESS_ERROR_SYSTEM_ERROR;
    }
}

// 检查UI Access状态
__declspec(dllexport) int __stdcall UIAccess_CheckStatus()
{
    if (!g_bInitialized)
    {
        g_nLastError = UIACCESS_ERROR_NOT_INITIALIZED;
        return -1;
    }

    DWORD dwErr;
    DWORD fUIAccess;
    
    if (CheckForUIAccess(&dwErr, &fUIAccess))
    {
        g_nLastError = UIACCESS_SUCCESS;
        return fUIAccess ? 1 : 0;
    }
    else
    {
        g_nLastError = UIACCESS_ERROR_SYSTEM_ERROR;
        return -1;
    }
}

// 清理函数
__declspec(dllexport) int __stdcall UIAccess_Cleanup()
{
    g_bInitialized = FALSE;
    g_nLastError = UIACCESS_SUCCESS;
    return UIACCESS_SUCCESS;
}

// 获取错误信息
__declspec(dllexport) const char* __stdcall UIAccess_GetErrorMessage(int errorCode)
{
    switch (errorCode)
    {
    case UIACCESS_SUCCESS:
        return "操作成功";
    case UIACCESS_ERROR_ALREADY_ENABLED:
        return "UI Access权限已经启用";
    case UIACCESS_ERROR_ACCESS_DENIED:
        return "访问被拒绝，需要管理员权限";
    case UIACCESS_ERROR_SYSTEM_ERROR:
        return "系统错误";
    case UIACCESS_ERROR_NOT_INITIALIZED:
        return "DLL未初始化";
    default:
        return "未知错误";
    }
}

// 获取最后一个错误码
__declspec(dllexport) int __stdcall UIAccess_GetLastError()
{
    return g_nLastError;
}

// 无感获取：若未启用 UIAccess，则以 UIAccess 令牌重启当前进程并退出旧进程
__declspec(dllexport) int __stdcall UIAccess_PrepareAndRelaunchIfNeeded()
{
    if (!g_bInitialized)
    {
        g_nLastError = UIACCESS_ERROR_NOT_INITIALIZED;
        return UIACCESS_ERROR_NOT_INITIALIZED;
    }

    DWORD dwErr = ERROR_SUCCESS;
    DWORD fUIAccess = 0;

    if (CheckForUIAccess(&dwErr, &fUIAccess))
    {
        if (fUIAccess)
        {
            g_nLastError = UIACCESS_SUCCESS;
            return UIACCESS_SUCCESS;
        }
        else
        {
            HANDLE hTokenUIAccess;
            dwErr = CreateUIAccessToken(&hTokenUIAccess);
            if (ERROR_SUCCESS == dwErr)
            {
                STARTUPINFO si;
                PROCESS_INFORMATION pi;
                ZeroMemory(&si, sizeof(si));
                ZeroMemory(&pi, sizeof(pi));
                si.cb = sizeof(si);
                GetStartupInfo(&si);
                if (CreateProcessAsUser(hTokenUIAccess, NULL, GetCommandLine(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
                {
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                    CloseHandle(hTokenUIAccess);
                    ExitProcess(0); // 退出旧进程，新的进程在 UIAccess 令牌下继续运行
                }
                else
                {
                    dwErr = GetLastError();
                }
                CloseHandle(hTokenUIAccess);
            }
        }
    }

    g_nLastError = (dwErr == ERROR_SUCCESS) ? UIACCESS_SUCCESS : UIACCESS_ERROR_SYSTEM_ERROR;
    return g_nLastError == UIACCESS_SUCCESS ? UIACCESS_SUCCESS : UIACCESS_ERROR_SYSTEM_ERROR;
}

// 通过句柄置顶/取消置顶
__declspec(dllexport) int __stdcall UIAccess_SetTopmost(HWND hwnd, int enable)
{
    if (!g_bInitialized)
    {
        g_nLastError = UIACCESS_ERROR_NOT_INITIALIZED;
        return UIACCESS_ERROR_NOT_INITIALIZED;
    }
    BOOL ok = SetWindowPos(hwnd, enable ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0,
                           SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    if (ok)
    {
        g_nLastError = UIACCESS_SUCCESS;
        return UIACCESS_SUCCESS;
    }
    g_nLastError = UIACCESS_ERROR_SYSTEM_ERROR;
    return UIACCESS_ERROR_SYSTEM_ERROR;
}

__declspec(dllexport) const char* __stdcall AboutThisDLL()
{
    return "本DLL由禾云信创-云云探索者编写，加入禾云大社区，和更多爱好者一起研究讨论！禾云大社区QQ群号：961573130\n(C)2025 HoCloudTechnologyStudio";
}

// 内部实现函数（从原始代码移植）
static DWORD DuplicateWinloginToken(DWORD dwSessionId, DWORD dwDesiredAccess, PHANDLE phToken) 
{
    DWORD dwErr;
    PRIVILEGE_SET ps;

    ps.PrivilegeCount = 1;
    ps.Control = PRIVILEGE_SET_ALL_NECESSARY;

    if (LookupPrivilegeValue(NULL, SE_TCB_NAME, &ps.Privilege[0].Luid)) {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (INVALID_HANDLE_VALUE != hSnapshot) {
            BOOL bCont, bFound = FALSE;
            PROCESSENTRY32 pe;

            pe.dwSize = sizeof(pe);
            dwErr = ERROR_NOT_FOUND;

            for (bCont = Process32First(hSnapshot, &pe); bCont; bCont = Process32Next(hSnapshot, &pe)) {
                HANDLE hProcess;

                if (0 != _tcsicmp(pe.szExeFile, TEXT("winlogon.exe"))) {
                    continue;
                }

                hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID);
                if (hProcess) {
                    HANDLE hToken;
                    DWORD dwRetLen, sid;

                    if (OpenProcessToken(hProcess, TOKEN_QUERY | TOKEN_DUPLICATE, &hToken)) {
                        BOOL fTcb;

                        if (PrivilegeCheck(hToken, &ps, &fTcb) && fTcb) {
                            if (GetTokenInformation(hToken, TokenSessionId, &sid, sizeof(sid), &dwRetLen) && sid == dwSessionId) {
                                bFound = TRUE;
                                if (DuplicateTokenEx(hToken, dwDesiredAccess, NULL, SecurityImpersonation, TokenImpersonation, phToken)) {
                                    dwErr = ERROR_SUCCESS;
                                } else {
                                    dwErr = GetLastError();
                                }
                            }
                        }
                        CloseHandle(hToken);
                    }
                    CloseHandle(hProcess);
                }

                if (bFound) break;
            }

            CloseHandle(hSnapshot);
        } else {
            dwErr = GetLastError();
        }
    } else {
        dwErr = GetLastError();
    }

    return dwErr;
}

static DWORD CreateUIAccessToken(PHANDLE phToken) 
{
    DWORD dwErr;
    HANDLE hTokenSelf;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_DUPLICATE, &hTokenSelf)) {
        DWORD dwSessionId, dwRetLen;

        if (GetTokenInformation(hTokenSelf, TokenSessionId, &dwSessionId, sizeof(dwSessionId), &dwRetLen)) {
            HANDLE hTokenSystem;

            dwErr = DuplicateWinloginToken(dwSessionId, TOKEN_IMPERSONATE, &hTokenSystem);
            if (ERROR_SUCCESS == dwErr) {
                if (SetThreadToken(NULL, hTokenSystem)) {
                    if (DuplicateTokenEx(hTokenSelf, TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_ADJUST_DEFAULT, NULL, SecurityAnonymous, TokenPrimary, phToken)) {
                        BOOL bUIAccess = TRUE;

                        if (!SetTokenInformation(*phToken, TokenUIAccess, &bUIAccess, sizeof(bUIAccess))) {
                            dwErr = GetLastError();
                            CloseHandle(*phToken);
                        }
                    } else {
                        dwErr = GetLastError();
                    }
                    RevertToSelf();
                } else {
                    dwErr = GetLastError();
                }
                CloseHandle(hTokenSystem);
            }
        } else {
            dwErr = GetLastError();
        }

        CloseHandle(hTokenSelf);
    } else {
        dwErr = GetLastError();
    }

    return dwErr;
}

static BOOL CheckForUIAccess(DWORD *pdwErr, DWORD *pfUIAccess) 
{
    BOOL result = FALSE;
    HANDLE hToken;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        DWORD dwRetLen;

        if (GetTokenInformation(hToken, TokenUIAccess, pfUIAccess, sizeof(*pfUIAccess), &dwRetLen)) {
            result = TRUE;
        } else {
            *pdwErr = GetLastError();
        }
        CloseHandle(hToken);
    } else {
        *pdwErr = GetLastError();
    }

    return result;
}

static DWORD PrepareForUIAccess() 
{
    DWORD dwErr;
    DWORD fUIAccess;

    if (CheckForUIAccess(&dwErr, &fUIAccess)) {
        if (fUIAccess) {
            dwErr = ERROR_SUCCESS;
        } else {
            HANDLE hTokenUIAccess;
            dwErr = CreateUIAccessToken(&hTokenUIAccess);
            if (ERROR_SUCCESS == dwErr) {
                // 注意：这里不重启进程，只是获取权限
                // 实际的UI Access需要重启进程才能生效
                CloseHandle(hTokenUIAccess);
            }
        }
    }

    return dwErr;
}