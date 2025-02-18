#include <stdio.h>
#include <Windows.h>
#include "injector/Injector.h"
#include "helpers.h"
#include "2EZConfig.h"

#define version "v1.05"

static bool debug = false;

int main(int argc, char* argv[])
{
    // 콘솔 설정
    SetConsoleTitle("2EZConfig " version);

    // 디버그 모드 체크
    if (debug) {
        RedirectIOToConsole();
    }

    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-skipConfig") == 0) {
            char exeName[255];
            GetPrivateProfileStringA("Settings", "EXEName", "EZ2AC.exe", exeName, sizeof(exeName), ".\\2EZ.ini");
            int gameVer = GetPrivateProfileIntA("Settings", "GameVer", -1, EZConfig::ConfigIniPath);

            if (strcmp(EZConfig::djGames[gameVer].name, "6th Trax ~Self Evolution~") == 0) {
                return EZConfig::sixthBackgroundLoop(exeName);
            }
            else {
                Injector::Inject(exeName);
                return 0;
            }
        }
    }

    HANDLE hMutex = CreateMutex(NULL, TRUE, "2EZConfig_Instance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(hMutex);
        return 1;
    }

    int result = EZConfig::RenderUI();

    CloseHandle(hMutex);
    return result;
}
