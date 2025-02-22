#define IMGUI_DISABLE_SSE2
#define OPENSSL_NO_ASM

#define _WIN32_WINNT_WINXP
#include <stdio.h>
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <Windows.h>
#include <Tlhelp32.h>
#include <iostream>
#include <comdef.h> 
#include <shlobj.h>
#include <shlwapi.h>
#include <objbase.h>
#include "Injector.h"
#pragma warning(disable : 4996) //_CRT_SECURE_NO_WARNINGS

BOOL SetPrivilege(HANDLE hToken, LPCTSTR Privilege, BOOL bEnablePrivilege);

// Our shellcode 
unsigned char shellcode[] = {
	0x9C,                           // Push all flags
	0x60,                           // Push all register
	0x68, 0x42, 0x42, 0x42, 0x42,  // Push dllPathAddr
	0xB8, 0x42, 0x42, 0x42, 0x42,  // Mov eax, loadLibAddr
	0xFF, 0xD0,                     // Call eax
	0x61,                           // Pop all register
	0x9D,                           // Pop all flags
	0xC3                            // Ret
};

int version();
FARPROC loadLibraryAddress();
LPVOID virtualAlloc(HANDLE hProcess, char* dll);
BOOL writeProcessMemory(HANDLE hProcess, LPVOID virtualAlloc, char* dll);
VOID createRemoteThreadMethod(HANDLE hProcess, FARPROC loadLibraryAddress, LPVOID virtualAlloc);

DWORD GetProcId(const char* procName) {
	DWORD procId = 0;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	wchar_t wtext[20];
	mbstowcs(wtext, procName, strlen(procName) + 1);//Plus null
	LPWSTR ptr = wtext;

	if (hSnap != INVALID_HANDLE_VALUE) {
		PROCESSENTRY32 procEntry;
		procEntry.dwSize = sizeof(procEntry);

		if (Process32First(hSnap, &procEntry)) {
			do {
				_bstr_t b(procEntry.szExeFile);
				const char* c = b;
				if (!_stricmp(b, procName)) {
					procId = procEntry.th32ProcessID;
					break;
				}
			} while (Process32Next(hSnap, &procEntry));
		}
	}

	CloseHandle(hSnap);
	return procId;
}

using namespace Injector;
int Injector::Inject(char* exename) {
	char* dll = (char*)"2EZ.dll";
	const char* procName = exename;

	// 디버그 권한 획득
	HANDLE hToken;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		SetPrivilege(hToken, SE_DEBUG_NAME, TRUE);
		printf("[+] Debug privilege enabled\n");
	}
	CloseHandle(hToken);

	// 프로세스 실행
	SHELLEXECUTEINFOW ShExecInfo = { 0 };
	ShExecInfo.cbSize = sizeof(ShExecInfo);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.nShow = SW_SHOW;

	wchar_t procName2[255];
	mbstowcs(procName2, procName, strlen(procName) + 1);
	ShExecInfo.lpFile = procName2;

	if (!ShellExecuteExW(&ShExecInfo)) {
		printf("[-] ShellExecuteEx failed\n");
		return 0;
	}

	// 프로세스 초기화 대기
	Sleep(1500);

	DWORD pid = GetProcessId(ShExecInfo.hProcess);
	int attempts = 0;

	while (!pid && attempts < 15) {
		pid = GetProcId(procName);
		if (!pid) {
			Sleep(200);
			attempts++;
		}
	}

	if (!pid) {
		printf("[-] Failed to get process ID\n");
		return 0;
	}

	printf("[+] Process ID: %d\n", pid);

	// 프로세스 핸들 획득
	HANDLE hProcess = NULL;
	for (int i = 0; i < 3 && !hProcess; i++) {
		hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
		if (!hProcess) {
			Sleep(200);
		}
	}

	if (!hProcess) {
		printf("[-] OpenProcess failed\n");
		return 0;
	}

	printf("[+] Process handle acquired\n");

	// LoadLibrary 주소 획득
	FARPROC llAddr = GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "LoadLibraryA");
	if (!llAddr) {
		printf("[-] Failed to get LoadLibraryA address\n");
		CloseHandle(hProcess);
		return 0;
	}

	printf("[+] LoadLibraryA address: 0x%p\n", llAddr);

	// 메모리 할당
	SIZE_T dllPathLen = strlen(dll) + 1;
	LPVOID dllAddr = VirtualAllocEx(hProcess, NULL, dllPathLen,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_READWRITE);

	if (!dllAddr) {
		printf("[-] VirtualAllocEx failed\n");
		CloseHandle(hProcess);
		return 0;
	}

	printf("[+] Memory allocated at: 0x%p\n", dllAddr);

	// 메모리 보호 설정 변경
	DWORD oldProtect;
	if (!VirtualProtectEx(hProcess, dllAddr, dllPathLen,
		PAGE_EXECUTE_READWRITE, &oldProtect)) {
		printf("[-] VirtualProtectEx failed\n");
		VirtualFreeEx(hProcess, dllAddr, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return 0;
	}

	// DLL 경로 쓰기
	SIZE_T bytesWritten;
	if (!WriteProcessMemory(hProcess, dllAddr, dll, dllPathLen, &bytesWritten)
		|| bytesWritten != dllPathLen) {
		printf("[-] WriteProcessMemory failed\n");
		VirtualFreeEx(hProcess, dllAddr, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return 0;
	}

	printf("[+] DLL path written successfully\n");

	// 쓰기 완료 대기
	Sleep(100);

	// 스레드 생성 시도
	HANDLE hThread = NULL;

	// 먼저 CreateRemoteThread 시도
	hThread = CreateRemoteThread(hProcess, NULL, 0,
		(LPTHREAD_START_ROUTINE)llAddr,
		dllAddr, 0, NULL);

	// CreateRemoteThread 실패시 APC 사용
	if (!hThread) {
		printf("[*] CreateRemoteThread failed, trying APC method\n");
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
		if (hSnapshot != INVALID_HANDLE_VALUE) {
			THREADENTRY32 te;
			te.dwSize = sizeof(te);

			BOOL apcSuccess = FALSE;
			for (BOOL success = Thread32First(hSnapshot, &te);
				success && !apcSuccess;
				success = Thread32Next(hSnapshot, &te)) {
				if (te.th32OwnerProcessID == pid) {
					HANDLE threadHandle = OpenThread(THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME,
						FALSE, te.th32ThreadID);
					if (threadHandle) {
						if (QueueUserAPC((PAPCFUNC)llAddr, threadHandle, (ULONG_PTR)dllAddr)) {
							apcSuccess = TRUE;
							printf("[+] APC queued successfully\n");
						}
						CloseHandle(threadHandle);
					}
				}
			}
			CloseHandle(hSnapshot);
			if (apcSuccess) {
				hThread = (HANDLE)1;  // APC 성공 표시
			}
		}
	}

	if (!hThread) {
		printf("[-] Failed to create thread or queue APC\n");
		VirtualFreeEx(hProcess, dllAddr, 0, MEM_RELEASE);
		CloseHandle(hProcess);
		return 0;
	}

	printf("[+] Injection successful\n");

	// 리소스 정리
	if (hThread != (HANDLE)1) {
		CloseHandle(hThread);
	}
	CloseHandle(hProcess);
	return 1;
}

int Injector::InjectWithName(const char* exename) {
	Sleep(10);
	char* dll = (char*)"2EZ.dll";
	int pid = 0;

	if (!pid) {
		while (!pid) {
			pid = GetProcId(exename);
		}
	}

	if (version() < 6) {
		HANDLE hProcess = GetCurrentProcess();
		HANDLE hToken;
		if (OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken)) {
			BOOL bPriv = SetPrivilege(hToken, SE_DEBUG_NAME, TRUE);
			CloseHandle(hToken);
			printf("[+] Debug privilege\n");
		}
	}

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (hProcess == NULL) {
		printf("[-] OpenProcess failed\n");
		exit(0);
	}
	else
		printf("[+] OpenProcess success\n");

	FARPROC llAddr = loadLibraryAddress();
	LPVOID dllAddr = virtualAlloc(hProcess, dll);
	writeProcessMemory(hProcess, dllAddr, dll);

	createRemoteThreadMethod(hProcess, llAddr, dllAddr);
	CloseHandle(hProcess);
	return 1;
}

int version() {
	OSVERSIONINFO osvi;
	ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osvi);
	return osvi.dwMajorVersion;
}

BOOL SetPrivilege(HANDLE hToken, LPCTSTR Privilege, BOOL bEnablePrivilege) {
	LUID luid;
	BOOL bRet = FALSE;

	if (LookupPrivilegeValue(NULL, Privilege, &luid)) {
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = luid;
		tp.Privileges[0].Attributes = (bEnablePrivilege) ? SE_PRIVILEGE_ENABLED : 0;

		if (AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
			bRet = (GetLastError() == ERROR_SUCCESS);
		}
	}
	return bRet;
}

FARPROC loadLibraryAddress() {
	FARPROC LLA = GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "LoadLibraryA");
	if (LLA == NULL) {
		printf("[-] LoadLibraryA address not found");
		exit(0);
	}
	else
		printf("[+] LoadLibraryA address found 0x%08x\n", LLA);
	return LLA;
}

LPVOID virtualAlloc(HANDLE hProcess, char* dll) {
	LPVOID VAE = (LPVOID)VirtualAllocEx(hProcess, NULL, strlen(dll), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (VAE == NULL) {
		printf("[-] VirtualAllocEx failed");
		exit(0);
	}
	else
		printf("[+] VirtualAllocEx  0x%08x\n", VAE);
	return VAE;
}

BOOL writeProcessMemory(HANDLE hProcess, LPVOID dllAddr, char* dll) {
	BOOL WPM = WriteProcessMemory(hProcess, dllAddr, dll, strlen(dll), NULL);
	if (!WPM) {
		printf("[-] WriteProcessMemory failed");
		exit(0);
	}
	else
		printf("[+] WriteProcessMemory success\n");
	return WPM;
}

VOID createRemoteThreadMethod(HANDLE hProcess, FARPROC loadLibraryAddress, LPVOID virtualAlloc) {
	HANDLE CRT = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddress, virtualAlloc, 0, NULL);
	if (CRT == NULL) {
		printf("[-] CreateRemoteThread failed\n");
		exit(0);
	}
	else {
		printf("Injection successful process pointer: %x \n", CRT);
		printf("[+] CreateRemoteThread success\n");
	}
}
