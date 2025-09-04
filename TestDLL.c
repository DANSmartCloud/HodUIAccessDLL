#include <stdio.h>
#include <windows.h>
#include "UIAccessDLL.h"

// Function pointer type definitions
typedef int (__stdcall *pfnUIAccess_Initialize)();
typedef int (__stdcall *pfnUIAccess_GetPermission)();
typedef int (__stdcall *pfnUIAccess_CheckStatus)();
typedef int (__stdcall *pfnUIAccess_Cleanup)();
typedef const char* (__stdcall *pfnUIAccess_GetErrorMessage)(int errorCode);
typedef int (__stdcall *pfnUIAccess_GetLastError)();

int main()
{
    // Use UTF-8 in console to correctly show Chinese messages from DLL
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    HMODULE hDLL;
    pfnUIAccess_Initialize pInitialize;
    pfnUIAccess_GetPermission pGetPermission;
    pfnUIAccess_CheckStatus pCheckStatus;
    pfnUIAccess_Cleanup pCleanup;
    pfnUIAccess_GetErrorMessage pGetErrorMessage;
    pfnUIAccess_GetLastError pGetLastError;
    
    printf("=== UI Access DLL Test Program ===\n\n");
    
    // Load DLL
    hDLL = LoadLibrary(TEXT("UIAccessDLL.dll"));
    if (hDLL == NULL)
    {
        printf("Error: Cannot load UIAccessDLL.dll\n");
        printf("Error code: %d\n", GetLastError());
        return 1;
    }
    
    printf("Successfully loaded UIAccessDLL.dll\n");
    
    // Get function addresses
    pInitialize = (pfnUIAccess_Initialize)GetProcAddress(hDLL, "UIAccess_Initialize");
    pGetPermission = (pfnUIAccess_GetPermission)GetProcAddress(hDLL, "UIAccess_GetPermission");
    pCheckStatus = (pfnUIAccess_CheckStatus)GetProcAddress(hDLL, "UIAccess_CheckStatus");
    pCleanup = (pfnUIAccess_Cleanup)GetProcAddress(hDLL, "UIAccess_Cleanup");
    pGetErrorMessage = (pfnUIAccess_GetErrorMessage)GetProcAddress(hDLL, "UIAccess_GetErrorMessage");
    pGetLastError = (pfnUIAccess_GetLastError)GetProcAddress(hDLL, "UIAccess_GetLastError");
    
    if (!pInitialize || !pGetPermission || !pCheckStatus || !pCleanup || !pGetErrorMessage || !pGetLastError)
    {
        printf("Error: Cannot get DLL function addresses\n");
        FreeLibrary(hDLL);
        return 1;
    }
    
    printf("Successfully got all function addresses\n\n");
    
    // Test initialization
    printf("1. Testing initialization...\n");
    int result = pInitialize();
    printf("   Initialization result: %d (%s)\n", result, pGetErrorMessage(result));
    
    // Test status check
    printf("\n2. Testing UI Access status check...\n");
    int status = pCheckStatus();
    if (status >= 0)
    {
        printf("   Current UI Access status: %s\n", status ? "Enabled" : "Disabled");
    }
    else
    {
        printf("   Status check failed, error code: %d\n", pGetLastError());
        printf("   Error message: %s\n", pGetErrorMessage(pGetLastError()));
    }
    
    // Test permission acquisition
    printf("\n3. Testing UI Access permission acquisition...\n");
    printf("   Note: This operation requires administrator privileges\n");
    result = pGetPermission();
    printf("   Permission acquisition result: %d (%s)\n", result, pGetErrorMessage(result));
    
    if (result == UIACCESS_SUCCESS)
    {
        printf("   Permission acquired successfully!\n");
        
        // Check status again
        printf("\n4. Rechecking UI Access status...\n");
        status = pCheckStatus();
        if (status >= 0)
        {
            printf("   Updated UI Access status: %s\n", status ? "Enabled" : "Disabled");
        }
    }
    else
    {
        printf("   Permission acquisition failed\n");
        if (result == UIACCESS_ERROR_ACCESS_DENIED)
        {
            printf("   Tip: Please run this program as administrator\n");
        }
    }
    
    // Test cleanup
    printf("\n5. Testing cleanup...\n");
    result = pCleanup();
    printf("   Cleanup result: %d (%s)\n", result, pGetErrorMessage(result));
    
    // Free DLL
    FreeLibrary(hDLL);
    printf("\nTest completed, DLL released\n");
    
    printf("\nPress any key to exit...");
    getchar();
    
    return 0;
}