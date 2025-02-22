// Ez2 Io Hook built by kasaski - 2022.
#define _CRT_SECURE_NO_WARNINGS

#include "2EZ.h"
#include <ShlObj.h>
#include <Shlwapi.h>
#include "input.h"
#include <Windows.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
#include <ctime>
#include <winnt.h>
#include <winternl.h>

#include <ntstatus.h>
#ifndef STATUS_INVALID_SYSTEM_SERVICE
#define STATUS_INVALID_SYSTEM_SERVICE      ((DWORD)0xC000001CL)
#endif

using namespace std;

ioBinding buttonBindings[NUM_OF_IO];
ioAnalogs analogBindings[NUM_OF_ANALOG];
Joysticks joysticks[NUM_OF_JOYSTICKS];
virtualTT vTT[2];
uintptr_t baseAddress;
uint8_t apKey;
int GameVer;
djGame currGame;
LPCSTR config = ".\\2EZ.ini";
char ControliniPath[MAX_PATH];

UINT8 getButtonInput(int ionRangeStart) {
	UINT8 output = 0xFF;  // 기본값: 모든 버튼이 눌리지 않은 상태

	if (ionRangeStart < 0 || ionRangeStart >= NUM_OF_IO) {
		return output;
	}

	for (int i = 0; i < 8; i++) {
		int buttonIndex = ionRangeStart + i;
		if (buttonIndex >= NUM_OF_IO) break;

		if (buttonBindings[buttonIndex].bound) {
			bool isPressed = false;
			if (buttonBindings[buttonIndex].method == 1) {  // Joypad
				if (buttonBindings[buttonIndex].joyID < NUM_OF_JOYSTICKS) {
					isPressed = input::isJoyButtonPressed(
						buttonBindings[buttonIndex].joyID,
						buttonBindings[buttonIndex].binding
					);
				}
			}
			else if (buttonBindings[buttonIndex].method == 0) {  // Keyboard
				isPressed = input::isKbButtonPressed(
					buttonBindings[buttonIndex].binding
				);
			}

			if (isPressed) {
				output &= ~(1 << (7 - i));
			}
		}
	}

	return output;
}

UINT8 getAnalogInput(int player) {
	if (analogBindings[player].bound) {
		UINT8 ttRawVal = input::JoyAxisPos(analogBindings[player].joyID, analogBindings[player].axis);
		return (analogBindings[player].reverse ? ~ttRawVal : ttRawVal) + vTT[player].pos;
	}
	else {
		return vTT[player].pos;
	}
}

// 메모리 주소 검증 헬퍼 함수
bool IsValidCodePtr(void* ptr) {
	MEMORY_BASIC_INFORMATION mbi;
	if (VirtualQuery(ptr, &mbi, sizeof(mbi)) == 0) {
		return false;
	}

	return (mbi.State == MEM_COMMIT &&
		(mbi.Protect & (PAGE_EXECUTE |
			PAGE_EXECUTE_READ |
			PAGE_EXECUTE_READWRITE |
			PAGE_EXECUTE_WRITECOPY)));
}

LONG WINAPI IOportHandler(PEXCEPTION_POINTERS pExceptionInfo) {
	__try {
		// 예외 코드 검증
		if (pExceptionInfo->ExceptionRecord->ExceptionCode != EXCEPTION_PRIV_INSTRUCTION &&
			pExceptionInfo->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION) {
			return EXCEPTION_CONTINUE_SEARCH;
		}

		// EIP 검증
		BYTE* ip = (BYTE*)pExceptionInfo->ContextRecord->Eip;
		if (!ip) return EXCEPTION_CONTINUE_SEARCH;

		// 메모리 범위 검증
		if ((uintptr_t)ip < baseAddress ||
			(uintptr_t)ip > baseAddress + 0x400000) {  // 4MB 범위로 제한
			return EXCEPTION_CONTINUE_SEARCH;
		}

		// Windows 98 호환성 모드에서의 메모리 접근 보호
		MEMORY_BASIC_INFORMATION mbi;
		if (VirtualQuery(ip, &mbi, sizeof(mbi)) == 0) {
			return EXCEPTION_CONTINUE_SEARCH;
		}

		// 명령어 패턴 매칭을 위한 안전한 읽기
		BYTE instruction;
		__try {
			instruction = *ip;
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			return EXCEPTION_CONTINUE_SEARCH;
		}

		// IN 명령어 처리
		if (instruction == 0xE4 || instruction == 0xEC) {
			WORD port;
			BYTE value = 0xFF;

			__try {
				if (instruction == 0xE4) {
					port = *(ip + 1);
				}
				else {
					port = (WORD)(pExceptionInfo->ContextRecord->Edx & 0xFFFF);
				}

				switch (port) {
				case 0x101: value = getButtonInput(0); break;
				case 0x102: value = getButtonInput(8); break;
				case 0x103: value = getAnalogInput(0); break;
				case 0x104: value = getAnalogInput(1); break;
				case 0x106: value = getButtonInput(16); break;
				}

				// AL 레지스터 업데이트
				pExceptionInfo->ContextRecord->Eax =
					(pExceptionInfo->ContextRecord->Eax & 0xFFFFFF00) | value;

				// 모든 처리가 성공한 후에 EIP 증가
				pExceptionInfo->ContextRecord->Eip += (instruction == 0xE4) ? 2 : 1;

				return EXCEPTION_CONTINUE_EXECUTION;
			}
			__except (EXCEPTION_EXECUTE_HANDLER) {
				return EXCEPTION_CONTINUE_SEARCH;
			}
		}

		// OUT 명령어 처리
		if (instruction == 0xE6 || instruction == 0xEE) {
			pExceptionInfo->ContextRecord->Eip += (instruction == 0xE6) ? 2 : 1;
			return EXCEPTION_CONTINUE_EXECUTION;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return EXCEPTION_CONTINUE_SEARCH;
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

DWORD WINAPI virtualTTThread(void* data) {
	while (true) {
		if (GetAsyncKeyState(vTT[0].plus) & 0x8000) {
			vTT[0].pos += 1;
		}
		if (GetAsyncKeyState(vTT[0].minus) & 0x8000) {
			vTT[0].pos -= 1;
		}
		if (GetAsyncKeyState(vTT[1].plus) & 0x8000) {
			vTT[1].pos += 1;
		}
		if (GetAsyncKeyState(vTT[1].minus) & 0x8000) {
			vTT[1].pos -= 1;
		}
		Sleep(5);
	}
	return 0;
}

DWORD PatchThread() {
	// 초기 대기 시간
	Sleep(1000);

	// 기본 초기화
	HANDLE ez2Proc = GetCurrentProcess();

	HANDLE hToken;
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		TOKEN_PRIVILEGES tp;
		LUID luid;
		if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) {
			tp.PrivilegeCount = 1;
			tp.Privileges[0].Luid = luid;
			tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
			AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
		}
		CloseHandle(hToken);
	}

	baseAddress = (uintptr_t)GetModuleHandleA(NULL);
	GameVer = GetPrivateProfileIntA("Settings", "GameVer", 0, config);
	currGame = djGames[GameVer];

	// 설정 파일 경로 설정
	SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, ControliniPath);
	PathAppendA(ControliniPath, (char*)"2EZ.ini");

	// 버튼 바인딩 초기화
	memset(buttonBindings, 0, sizeof(buttonBindings));
	for (int i = 0; i < NUM_OF_IO; i++) {
		buttonBindings[i].method = 3;
		buttonBindings[i].joyID = 16;
		buttonBindings[i].binding = NULL;
		buttonBindings[i].pressed = false;
		buttonBindings[i].bound = false;
	}

	// 아날로그 바인딩 초기화
	memset(analogBindings, 0, sizeof(analogBindings));
	for (int i = 0; i < NUM_OF_ANALOG; i++) {
		analogBindings[i].joyID = 16;
		analogBindings[i].axis = 3;
		analogBindings[i].pos = 255;
		analogBindings[i].reverse = false;
		analogBindings[i].bound = false;
	}

	// 가상 턴테이블 초기화
	memset(vTT, 0, sizeof(vTT));
	vTT[0].pos = 255;
	vTT[1].pos = 255;

	// 초기 지연
	Sleep(GetPrivateProfileIntA("Settings", "ShimDelay", 10, config));

	// IO Hook 설정
	if (GetPrivateProfileIntA("Settings", "EnableIOHook", 0, config)) {
		PVOID handler = AddVectoredExceptionHandler(1, IOportHandler);
		if (handler) {
			SetUnhandledExceptionFilter(IOportHandler);
			printf("[+] IO Hook installed successfully\n");
		}
	}

	// 버튼 설정
	for (int b = 0; b < NUM_OF_IO; b++) {
		char buff[20];
		GetPrivateProfileStringA(ioButtons[b], "method", NULL, buff, 20, ControliniPath);
		if (strlen(buff) > 0) {
			if (strcmp(buff, "Key") == 0) {
				buttonBindings[b].bound = true;
				buttonBindings[b].method = 0;
			}
			else {
				buttonBindings[b].bound = true;
				buttonBindings[b].method = 1;
				joysticks[buttonBindings[b].joyID].init = true;
			}
			buttonBindings[b].joyID = GetPrivateProfileIntA(ioButtons[b], "JoyID", 16, ControliniPath);
			buttonBindings[b].binding = GetPrivateProfileIntA(ioButtons[b], "Binding", NULL, ControliniPath);
			printf("[+] Button %d bound: Method=%s, JoyID=%d, Binding=%d\n",
				b, buff, buttonBindings[b].joyID, buttonBindings[b].binding);
		}
	}

	// 아날로그 설정
	for (int a = 0; a < NUM_OF_ANALOG; a++) {
		char buff[20];
		GetPrivateProfileStringA(analogs[a], "Axis", NULL, buff, 20, ControliniPath);
		if (strlen(buff) > 0) {
			analogBindings[a].bound = true;
			joysticks[analogBindings[a].joyID].init = true;
			if (strcmp(buff, "X") == 0) {
				analogBindings[a].axis = 0;
			}
			else {
				analogBindings[a].axis = 1;
			}
			analogBindings[a].joyID = GetPrivateProfileIntA(analogs[a], "JoyID", 16, ControliniPath);
			analogBindings[a].reverse = GetPrivateProfileIntA(analogs[a], "Reverse", 0, ControliniPath);
			printf("[+] Analog %d bound: Axis=%s, JoyID=%d, Reverse=%d\n",
				a, buff, analogBindings[a].joyID, analogBindings[a].reverse);
		}
	}

	// 바인딩 지연
	Sleep(GetPrivateProfileIntA("Settings", "BindDelay", 2000, config));

	// 가상 턴테이블 설정
	vTT[0].plus = GetPrivateProfileIntA("P1 Turntable +", "Binding", NULL, ControliniPath);
	vTT[0].minus = GetPrivateProfileIntA("P1 Turntable -", "Binding", NULL, ControliniPath);
	vTT[1].plus = GetPrivateProfileIntA("P2 Turntable +", "Binding", NULL, ControliniPath);
	vTT[1].minus = GetPrivateProfileIntA("P2 Turntable -", "Binding", NULL, ControliniPath);

	// 가상 턴테이블 스레드 시작
	HANDLE turntableThread = CreateThread(NULL, 0, virtualTTThread, NULL, 0, NULL);
	if (turntableThread) {
		SetThreadPriority(turntableThread, THREAD_PRIORITY_TIME_CRITICAL);
		printf("[+] Virtual turntable thread started with high priority\n");
		CloseHandle(turntableThread);
	}

	// 메모리 보호 설정
	DWORD oldProtect;
	// 더 넓은 범위의 메모리를 보호
	for (uintptr_t addr = baseAddress; addr < baseAddress + 0x80000; addr += 0x8000) {
		VirtualProtect((LPVOID)addr, 0x8000, PAGE_EXECUTE_READWRITE, &oldProtect);
	}

	// 추가 메모리 영역 보호 설정
	VirtualProtect((LPVOID)(baseAddress + 0x7000), 0x1000, PAGE_EXECUTE_READWRITE, &oldProtect);

	printf("[+] PatchThread initialization completed\n");
	return NULL;
}

// DllMain initialization
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		// Initiate ControliniPath
		SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, ControliniPath);
		PathAppendA(ControliniPath, (char*)"2EZ.ini");

		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)PatchThread, 0, 0, 0);
		break;

	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
