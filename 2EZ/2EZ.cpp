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
	UINT8 output = 255;
	int count = 0;
	for (int i = ionRangeStart; i < ionRangeStart + 8; i++) {
		if (buttonBindings[i].bound) {
			if (buttonBindings[i].method) {
				if (input::isJoyButtonPressed(buttonBindings[i].joyID, buttonBindings[i].binding)) {
					output = output & byteArray[count];
				}
			}
			else {
				if (input::isKbButtonPressed(buttonBindings[i].binding)) {
					output = output & byteArray[count];
				}
			}
		}
		count++;
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

LONG WINAPI IOportHandler(PEXCEPTION_POINTERS pExceptionInfo) {
	if (pExceptionInfo->ExceptionRecord->ExceptionCode != EXCEPTION_PRIV_INSTRUCTION) {
		return EXCEPTION_EXECUTE_HANDLER;
	}

	unsigned int eip_val = *(unsigned int*)pExceptionInfo->ContextRecord->Eip;

	if ((eip_val & 0xFF) == 0xEC) {
		switch (pExceptionInfo->ContextRecord->Edx & 0xFFFF) {
		case 0x101:
			pExceptionInfo->ContextRecord->Eax = getButtonInput(0);
			break;
		case 0x102:
			pExceptionInfo->ContextRecord->Eax = getButtonInput(8);
			break;
		case 0x103:
			pExceptionInfo->ContextRecord->Eax = getAnalogInput(0);
			break;
		case 0x104:
			pExceptionInfo->ContextRecord->Eax = getAnalogInput(1);
			break;
		case 0x106:
			pExceptionInfo->ContextRecord->Eax = getButtonInput(16);
			break;
		default:
			pExceptionInfo->ContextRecord->Eax = 0;
			break;
		}

		pExceptionInfo->ContextRecord->Eip++;
		return EXCEPTION_CONTINUE_EXECUTION;
	}

	if ((eip_val & 0xFF) == 0xEE) {
		pExceptionInfo->ContextRecord->Eip++;
		return EXCEPTION_CONTINUE_EXECUTION;
	}

	return EXCEPTION_CONTINUE_EXECUTION;
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

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
	UINT num = 0, size = 0;
	Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;
	Gdiplus::GetImageEncodersSize(&num, &size);
	if (size == 0)
		return -1;
	pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
	if (pImageCodecInfo == NULL)
		return -1;
	Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);
	for (UINT j = 0; j < num; ++j) {
		if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
			*pClsid = pImageCodecInfo[j].Clsid;
			free(pImageCodecInfo);
			return j;
		}
	}
	free(pImageCodecInfo);
	return -1;
}

std::wstring str2wstr(const std::string& str) {
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
	std::wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

void TakeScreenshot() {
	HWND hWnd = GetForegroundWindow();
	if (!hWnd)
		return;

	time_t now = time(nullptr);
	tm ltm;
	localtime_s(&ltm, &now);
	char filename[256];
	sprintf_s(filename, "Screenshot_%04d%02d%02d_%02d%02d%02d.png",
		1900 + ltm.tm_year, 1 + ltm.tm_mon, ltm.tm_mday,
		ltm.tm_hour, ltm.tm_min, ltm.tm_sec);

	CreateDirectoryA("Screenshots", NULL);
	char fullPath[512];
	sprintf_s(fullPath, "Screenshots\\%s", filename);

	HDC hdcWindow = GetDC(hWnd);
	RECT rect;
	GetClientRect(hWnd, &rect);
	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;

	HDC hdcMem = CreateCompatibleDC(hdcWindow);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdcWindow, width, height);
	HBITMAP hOld = (HBITMAP)SelectObject(hdcMem, hBitmap);
	BitBlt(hdcMem, 0, 0, width, height, hdcWindow, 0, 0, SRCCOPY);

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	CLSID pngClsid;
	if (GetEncoderClsid(L"image/png", &pngClsid) >= 0) {
		Gdiplus::Bitmap bitmap(hBitmap, NULL);
		bitmap.Save(str2wstr(fullPath).c_str(), &pngClsid, NULL);
	}

	SelectObject(hdcMem, hOld);
	DeleteObject(hBitmap);
	DeleteDC(hdcMem);
	ReleaseDC(hWnd, hdcWindow);
	Gdiplus::GdiplusShutdown(gdiplusToken);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, ControliniPath);
		PathAppendA(ControliniPath, (char*)"2EZ.ini");
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
