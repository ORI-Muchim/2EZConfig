#define _WIN32_WINNT_WINXP
#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>  // Keep printf for XP compatibility
#include <shlobj.h>
#include <shlwapi.h>
#include <objbase.h>
#include <string.h>  // For strlen

#pragma comment(lib, "shell32.lib")  // Link ShellExecuteEx
#pragma warning(disable: 4996)  // _CRT_SECURE_NO_WARNINGS

const char DLL_NAME[] = "2EZ.dll";  // DLL name as array (sizeof usable)
const size_t DLL_LEN = strlen(DLL_NAME) + 1;  // 9 bytes including null terminator
const size_t SHELLCODE_SIZE = sizeof(shellcode);  // Shellcode size

// Shellcode (LoadLibraryA call: push dllPath -> mov eax loadLib -> call eax -> cleanup -> ret)
unsigned char shellcode[] = {
    0x9C, 0x60, 0x68, 0x00, 0x00, 0x00, 0x00,  // push dllPathAddr (patch: [3]~[6])
    0xB8, 0x00, 0x00, 0x00, 0x00,              // mov eax, loadLibAddr (patch: [8]~[11])
    0xFF, 0xD0, 0x61, 0x9D, 0xC3
};

// Get OS major version (XP: 5)
int GetOSMajorVersion() {
    OSVERSIONINFO osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);
    return osvi.dwMajorVersion;
}

// Enable debug privilege (essential for XP)
BOOL EnableDebugPrivilege() {
    if (GetOSMajorVersion() >= 6) return TRUE;
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) return FALSE;
    BOOL success = SetPrivilege(hToken, SE_DEBUG_NAME, TRUE);
    CloseHandle(hToken);
    if (success) printf("[+] Debug privilege enabled\n");
    return success;
}

// SetPrivilege (unchanged)
BOOL SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege) {
    LUID luid;
    if (!LookupPrivilegeValue(NULL, lpszPrivilege, &luid)) return FALSE;
    TOKEN_PRIVILEGES tp = {1, {{luid, bEnablePrivilege ? SE_PRIVILEGE_ENABLED : 0}}};
    return AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL) && GetLastError() == ERROR_SUCCESS;
}

// Get process ID (optimized string comparison: _wcsicmp)
DWORD GetProcId(const char* procName) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return 0;

    wchar_t wProcName[256];
    mbstowcs(wProcName, procName, strlen(procName) + 1);

    PROCESSENTRY32 entry = {sizeof(PROCESSENTRY32)};
    DWORD pid = 0;
    if (Process32First(hSnap, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, wProcName) == 0) {
                pid = entry.th32ProcessID;
                break;
            }
        } while (Process32Next(hSnap, &entry));
    }
    CloseHandle(hSnap);
    return pid;
}

// Get LoadLibraryA address
FARPROC GetLoadLibraryAAddress() {
    FARPROC addr = GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
    if (addr) printf("[+] LoadLibraryA: 0x%08X\n", (DWORD)addr);
    else printf("[-] LoadLibraryA not found\n");
    return addr;
}

// Allocate and write memory (shellcode support: allocate full size)
BOOL AllocateAndWrite(HANDLE hProcess, LPVOID* pAddr, BOOL useShellcode = FALSE) {
    SIZE_T totalSize = useShellcode ? (DLL_LEN + SHELLCODE_SIZE) : DLL_LEN;
    LPVOID baseAddr = VirtualAllocEx(hProcess, NULL, totalSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);  // XP: RWX for shellcode execution
    if (!baseAddr) {
        printf("[-] VirtualAllocEx failed\n");
        return FALSE;
    }
    printf("[+] VirtualAllocEx: 0x%08X (size: %d)\n", (DWORD)baseAddr, totalSize);

    LPVOID dllAddr = baseAddr;
    LPVOID shellAddr = useShellcode ? (LPVOID)((DWORD)baseAddr + DLL_LEN) : NULL;

    BOOL wrote = WriteProcessMemory(hProcess, dllAddr, DLL_NAME, DLL_LEN, NULL);
    if (!wrote) {
        printf("[-] WriteProcessMemory (DLL) failed\n");
        VirtualFreeEx(hProcess, baseAddr, 0, MEM_RELEASE);
        return FALSE;
    }

    if (useShellcode) {
        // Patch shellcode: Insert DLL path address (push target) and LoadLibrary address
        DWORD dllPathAddr = (DWORD)dllAddr;  // DLL start address
        DWORD loadLibAddr = (DWORD)GetLoadLibraryAAddress();
        if (!loadLibAddr) {
            VirtualFreeEx(hProcess, baseAddr, 0, MEM_RELEASE);
            return FALSE;
        }

        // Apply patch (to local shellcode copy)
        unsigned char patchedShell[SHELLCODE_SIZE];
        memcpy(patchedShell, shellcode, SHELLCODE_SIZE);
        memcpy(&patchedShell[3], &dllPathAddr, 4);   // patch push dllPathAddr
        memcpy(&patchedShell[8], &loadLibAddr, 4);   // patch mov eax, loadLibAddr

        wrote = WriteProcessMemory(hProcess, shellAddr, patchedShell, SHELLCODE_SIZE, NULL);
        if (!wrote) {
            printf("[-] WriteProcessMemory (shellcode) failed\n");
            VirtualFreeEx(hProcess, baseAddr, 0, MEM_RELEASE);
            return FALSE;
        }
        *pAddr = shellAddr;  // Thread start: shellcode address
    } else {
        *pAddr = baseAddr;  // Legacy: DLL address
    }

    printf("[+] WriteProcessMemory success\n");
    return TRUE;
}

// Create remote injection (shellcode support)
HANDLE CreateRemoteInjection(HANDLE hProcess, LPVOID addr, BOOL useShellcode = FALSE) {
    LPTHREAD_START_ROUTINE startRoutine = useShellcode ? 
        (LPTHREAD_START_ROUTINE)addr :  // Execute shellcode directly
        (LPTHREAD_START_ROUTINE)GetLoadLibraryAAddress();  // Call LoadLibrary
    LPVOID param = useShellcode ? NULL : addr;  // Shellcode: ignore param, legacy: DLL path

    HANDLE thread = CreateRemoteThread(hProcess, NULL, 0, startRoutine, param, 0, NULL);
    if (!thread) printf("[-] CreateRemoteThread failed\n");
    else printf("[+] Injection success: 0x%08X\n", (DWORD)thread);
    return thread;
}

// Common injection (launchNew: whether to start process)
BOOL PerformInjection(const char* procName, BOOL launchNew = FALSE) {
    if (!EnableDebugPrivilege()) return FALSE;

    DWORD pid = launchNew ? 0 : GetProcId(procName);
    HANDLE hProcess = NULL;
    if (launchNew) {
        SHELLEXECUTEINFOA execInfo = {sizeof(SHELLEXECUTEINFOA), SEE_MASK_NOCLOSEPROCESS, NULL, "open", procName, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, (HANDLE)-1};
        if (!ShellExecuteExA(&execInfo)) {
            printf("[-] ShellExecuteEx failed\n");
            return FALSE;
        }
        hProcess = execInfo.hProcess;
        // XP: Wait for process start (simple loop)
        while (!pid) {
            Sleep(50);  // Minimal delay for stability
            pid = GetProcId(procName);
        }
    } else {
        if (!pid) {
            printf("[-] Process not found\n");
            return FALSE;
        }
        hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    }
    if (!hProcess) {
        printf("[-] OpenProcess failed\n");
        return FALSE;
    }
    printf("[+] OpenProcess success (PID: %d)\n", pid);

    LPVOID addr = NULL;
    if (!AllocateAndWrite(hProcess, &addr, TRUE)) {  // Enable shellcode (TRUE)
        CloseHandle(hProcess);
        return FALSE;
    }

    HANDLE thread = CreateRemoteInjection(hProcess, addr, TRUE);
    CloseHandle(thread ? thread : INVALID_HANDLE_VALUE);
    CloseHandle(hProcess);
    return thread != NULL;
}

// Class functions (assuming namespace Injector)
namespace Injector {
    int Inject(char* exename) { return PerformInjection(exename, TRUE) ? 1 : 0; }
    int InjectWithName(const char* exename) { return PerformInjection(exename, FALSE) ? 1 : 0; }
}
