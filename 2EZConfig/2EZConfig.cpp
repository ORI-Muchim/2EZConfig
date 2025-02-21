#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#include "2EZconfig.h"
#include <Windows.h>
#include <iostream>
#include <stdio.h>
#include "injector/injector.h"
#include "helpers.h"
#include "inputManager/device.h"
#include "inputManager/input.h"
#include <dirent.h>
#include <conio.h>
#include <iomanip>
#include <ShlObj.h>
#include <Shlwapi.h>

using namespace EZConfig;

// Function declarations
void showMainMenu();
void showSettingsMenu();
void showButtonsMenu();
void showAnalogsMenu();
void showLightsMenu();
int detectGameVersion();
void clearScreen();
void printHeader();
void waitForKeyPress();

LPSTR ControliniPath;

int EZConfig::RenderUI() {
    while (true) {
        showMainMenu();
        Sleep(100);
    }
    return 0;
}

int EZConfig::sixthBackgroundLoop(char* launcherName) {
    RedirectIOToConsole();
    bool inSixth = false;
    printf("launching 6th loop\n");
    Injector::Inject(launcherName);

    while (true) {
        if (!inSixth) {
            if (Injector::InjectWithName("EZ2DJ6th.exe")) {
                inSixth = true;
                printf("Found 6th\n");
            }
        }
        else {
            if (Injector::InjectWithName("Ez2DJ.exe")) {
                inSixth = false;
                printf("Found 1st\n");
            }
        }
        Sleep(100);
    }
    return 0;
}

void clearScreen() {
    system("cls");
}

void printHeader() {
    std::cout << "====================================\n";
    std::cout << "        2EZ Configuration Tool      \n";
    std::cout << "====================================\n\n";
}

void waitForKeyPress() {
    std::cout << "\nPress any key to continue...";
    _getch();
}

void showMainMenu() {
    clearScreen();
    printHeader();

    char exeName[255];
    GetPrivateProfileStringA("Settings", "EXEName", "EZ2AC.exe", exeName, sizeof(exeName), ConfigIniPath);

    std::cout << "1. Settings\n";
    std::cout << "2. Buttons Configuration\n";
    std::cout << "3. Analogs Configuration\n";
    std::cout << "4. Lights Configuration\n";
    std::cout << "5. Launch Game\n";
    std::cout << "6. Exit\n\n";
    std::cout << "Select an option (1-6): ";

    char choice = _getch();
    switch (choice) {
    case '1':
        showSettingsMenu();
        break;
    case '2':
        showButtonsMenu();
        break;
    case '3':
        showAnalogsMenu();
        break;
    case '4':
        showLightsMenu();
        break;
    case '5':
        if (fileExists(exeName)) {
            int GameVer = GetPrivateProfileIntA("Settings", "GameVer", -1, ConfigIniPath);
            if (strcmp(djGames[GameVer].name, "6th Trax ~Self Evolution~") == 0) {
                sixthBackgroundLoop(exeName);
            }
            else {
                Injector::Inject(exeName);
            }
        }
        else {
            std::cout << "\nError: " << exeName << " not found!\n";
            waitForKeyPress();
        }
        break;
    case '6':
        exit(0);
    }
}

void showSettingsMenu() {
    clearScreen();
    printHeader();

    char buff[30];
    char exeName[255];

    int GameVer = GetPrivateProfileIntA("Settings", "GameVer", -1, ConfigIniPath);
    if (GameVer < 0) {
        GameVer = detectGameVersion();
    }

    GetPrivateProfileStringA("Settings", "EXEName", "EZ2AC.exe", exeName, sizeof(exeName), ConfigIniPath);

    bool overrideExe = GetPrivateProfileIntA("Settings", "OverrideExe", 0, ConfigIniPath);
    bool IOHook = GetPrivateProfileIntA("Settings", "EnableIOHook", 1, ConfigIniPath);
    bool DevControl = GetPrivateProfileIntA("Settings", "EnableDevControls", 0, ConfigIniPath);
    bool modeFreeze = GetPrivateProfileIntA("Settings", "ModeSelectTimerFreeze", 0, ConfigIniPath);
    bool songFreeze = GetPrivateProfileIntA("Settings", "SongSelectTimerFreeze", 0, ConfigIniPath);
    bool enableNoteJudgment = GetPrivateProfileIntA("Settings", "JudgmentDelta", 0, ConfigIniPath);
    bool evWin10Fix = GetPrivateProfileIntA("Settings", "EvWin10Fix", 0, ConfigIniPath);

    std::cout << "Settings Menu\n";
    std::cout << "-------------\n\n";

    std::cout << "Current Game Version: " << djGames[GameVer].name << "\n\n";

    std::cout << "1. Change Game Version\n";
    std::cout << "2. Toggle EXE Override [" << (overrideExe ? "ON" : "OFF") << "]\n";
    std::cout << "3. Toggle IO Hook [" << (IOHook ? "ON" : "OFF") << "]\n";
    if (djGames[GameVer].hasDevBindings) {
        std::cout << "4. Toggle Dev Controls [" << (DevControl ? "ON" : "OFF") << "]\n";
    }
    if (djGames[GameVer].hasModeFreeze) {
        std::cout << "5. Toggle Mode Select Timer Freeze [" << (modeFreeze ? "ON" : "OFF") << "]\n";
    }
    if (djGames[GameVer].hasSongFreeze) {
        std::cout << "6. Toggle Song Select Timer Freeze [" << (songFreeze ? "ON" : "OFF") << "]\n";
    }
    if (strcmp(djGames[GameVer].name, "Evolve") == 0) {
        std::cout << "7. Toggle Win10 Fix [" << (evWin10Fix ? "ON" : "OFF") << "]\n";
    }
    std::cout << "0. Back to Main Menu\n\n";

    std::cout << "Select an option: ";

    char choice = _getch();
    switch (choice) {
    case '1': {
        clearScreen();
        printHeader();
        std::cout << "Available Game Versions:\n\n";

        for (int i = 0; i < ARRAY_SIZE(djGames); i++) {
            std::cout << i + 1 << ". " << djGames[i].name << "\n";
        }

        std::cout << "\nSelect game version (1-" << ARRAY_SIZE(djGames) << "): ";
        std::string input;
        std::cin >> input;
        try {
            int newVer = std::stoi(input) - 1;

            if (newVer >= 0 && newVer < ARRAY_SIZE(djGames)) {
                WritePrivateProfileString("Settings", "GameVer", _itoa(newVer, buff, 10), ConfigIniPath);
                std::cout << "\nGame version changed to " << djGames[newVer].name << "\n";
            }
            else {
                std::cout << "\nInvalid selection!\n";
            }
        }
        catch (std::exception&) {
            std::cout << "\nInvalid input!\n";
        }
        waitForKeyPress();
        break;
    }
    case '2':
        overrideExe = !overrideExe;
        WritePrivateProfileString("Settings", "OverrideExe", _itoa(overrideExe, buff, 10), ConfigIniPath);
        if (overrideExe) {
            std::cout << "\nEnter EXE name: ";
            std::cin >> exeName;
            WritePrivateProfileString("Settings", "EXEName", exeName, ConfigIniPath);
        }
        else {
            WritePrivateProfileString("Settings", "EXEName", djGames[GameVer].defaultExeName, ConfigIniPath);
        }
        break;
    case '3':
        IOHook = !IOHook;
        WritePrivateProfileString("Settings", "EnableIOHook", _itoa(IOHook, buff, 10), ConfigIniPath);
        break;
    case '4':
        if (djGames[GameVer].hasDevBindings) {
            DevControl = !DevControl;
            WritePrivateProfileString("Settings", "EnableDevControls", _itoa(DevControl, buff, 10), ConfigIniPath);
        }
        break;
    case '5':
        if (djGames[GameVer].hasModeFreeze) {
            modeFreeze = !modeFreeze;
            WritePrivateProfileString("Settings", "ModeSelectTimerFreeze", _itoa(modeFreeze, buff, 10), ConfigIniPath);
        }
        break;
    case '6':
        if (djGames[GameVer].hasSongFreeze) {
            songFreeze = !songFreeze;
            WritePrivateProfileString("Settings", "SongSelectTimerFreeze", _itoa(songFreeze, buff, 10), ConfigIniPath);
        }
        break;
    case '7':
        if (strcmp(djGames[GameVer].name, "Evolve") == 0) {
            evWin10Fix = !evWin10Fix;
            WritePrivateProfileString("Settings", "EvWin10Fix", _itoa(evWin10Fix, buff, 10), ConfigIniPath);
        }
        break;
    case '0':
        return;
    }
    showSettingsMenu(); // Refresh menu
}

void showButtonsMenu() {
    clearScreen();
    printHeader();

    TCHAR ControliniPath[MAX_PATH];
    SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, ControliniPath);
    PathAppendA(ControliniPath, (char*)"2EZ.ini");

    input::InitJoysticks();

    std::cout << "Buttons Configuration\n";
    std::cout << "--------------------\n\n";

    // Display current button bindings
    for (int i = 0; i < ARRAY_SIZE(ioButtons); i++) {
        char method[20];
        GetPrivateProfileStringA(ioButtons[i], "Method", "", method, sizeof(method), ControliniPath);
        int id = GetPrivateProfileIntA(ioButtons[i], "JoyID", NULL, ControliniPath);
        int binding = GetPrivateProfileIntA(ioButtons[i], "Binding", NULL, ControliniPath);

        std::cout << i + 1 << ". " << std::left << std::setw(20) << ioButtons[i];
        if (binding != NULL) {
            if (strcmp(method, "Joy") == 0) {
                if (input::GetJoyState(id).input) {
                    std::cout << "Joy:" << id << " Btn:" << input::getButtonName(binding) << "\n";
                }
                else {
                    std::cout << "Device Removed\n";
                }
            }
            else {
                std::cout << "Key:" << GetKeyName(binding) << "\n";
            }
        }
        else {
            std::cout << "Not bound\n";
        }
    }

    std::cout << "\nB: Bind button | C: Clear binding | 0: Back to Main Menu\n";
    std::cout << "Select an option: ";

    char choice = _getch();
    char buff[10];

    if (choice == 'b' || choice == 'B') {
        std::cout << "\nEnter button number to bind (1-" << ARRAY_SIZE(ioButtons) << "): ";
        std::string input;
        std::cin >> input;
        int buttonIndex = std::stoi(input) - 1;

        if (buttonIndex >= 0 && buttonIndex < ARRAY_SIZE(ioButtons)) {
            std::cout << "\nPress the key or button to bind (ESC to cancel)...\n";

            bool bound = false;
            while (!bound) {
                // Check joystick input
                for (int n = 0; n < joyGetNumDevs(); n++) {
                    if (input::GetJoyState(n).input && input::GetJoyState(n).NumPressed == 1) {
                        WritePrivateProfileString(ioButtons[buttonIndex], "Method", "Joy", ControliniPath);
                        WritePrivateProfileString(ioButtons[buttonIndex], "JoyID", _itoa(n, buff, sizeof(buff)), ControliniPath);
                        WritePrivateProfileString(ioButtons[buttonIndex], "Binding",
                            _itoa(input::GetJoyState(n).buttonsPressed, buff, sizeof(buff)), ControliniPath);
                        bound = true;
                        break;
                    }
                }

                // Check keyboard input
                int key = input::checkKbPressedState();
                if (key > 0) {
                    if (key == VK_ESCAPE) {
                        bound = true;
                    }
                    else {
                        WritePrivateProfileString(ioButtons[buttonIndex], "Method", "Key", ControliniPath);
                        WritePrivateProfileString(ioButtons[buttonIndex], "JoyID", NULL, ControliniPath);
                        WritePrivateProfileString(ioButtons[buttonIndex], "Binding", _itoa(key, buff, sizeof(buff)), ControliniPath);
                        bound = true;
                    }
                }
            }
        }
    }
    else if (choice == 'c' || choice == 'C') {
        std::cout << "\nEnter button number to clear (1-" << ARRAY_SIZE(ioButtons) << "): ";
        std::string input;
        std::cin >> input;
        int buttonIndex = std::stoi(input) - 1;

        if (buttonIndex >= 0 && buttonIndex < ARRAY_SIZE(ioButtons)) {
            WritePrivateProfileString(ioButtons[buttonIndex], NULL, NULL, ControliniPath);
        }
    }
    else if (choice == '0') {
        input::DestroyJoysticks();
        return;
    }

    input::DestroyJoysticks();
    showButtonsMenu(); // Refresh menu
}

void showAnalogsMenu() {
    clearScreen();
    printHeader();

    TCHAR ControliniPath[MAX_PATH];
    SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, ControliniPath);
    PathAppendA(ControliniPath, (char*)"2EZ.ini");

    input::InitJoysticks();

    std::cout << "Analogs Configuration\n";
    std::cout << "--------------------\n\n";

    for (int i = 0; i < ARRAY_SIZE(analogs); i++) {
        int id;
        char axis[20];
        bool reverse = GetPrivateProfileIntA(analogs[i], "Reverse", 0, ControliniPath);

        id = GetPrivateProfileInt(analogs[i], "JoyID", 16, ControliniPath);
        GetPrivateProfileStringA(analogs[i], "Axis", NULL, axis, sizeof(axis), ControliniPath);

        std::cout << i + 1 << ". " << std::left << std::setw(20) << analogs[i];

        float fraction = 0.0f;
        if (id != 16 && input::GetJoyState(id).input) {
            if (strcmp(axis, "X") == 0) {
                if (reverse) {
                    UINT8 val = ~(input::GetJoyState(id).x);
                    fraction = (float)val / (float)255;
                }
                else {
                    fraction = (float)(input::GetJoyState(id).x) / (float)255;
                }
            }
            else if (strcmp(axis, "Y") == 0) {
                if (reverse) {
                    UINT8 val = ~(input::GetJoyState(id).y);
                    fraction = (float)val / (float)255;
                }
                else {
                    fraction = (float)input::GetJoyState(id).y / (float)255;
                }
            }

            std::cout << "Joy:" << id << " Axis:" << axis << " Value:" << (int)(fraction * 100) << "%\n";
        }
        else {
            std::cout << "Not bound\n";
        }
    }

    std::cout << "\nB: Bind analog | C: Clear binding | 0: Back to Main Menu\n";
    std::cout << "Select an option: ";

    char choice = _getch();
    char buff[10];

    if (choice == 'b' || choice == 'B') {
        std::cout << "\nEnter analog number to bind (1-" << ARRAY_SIZE(analogs) << "): ";
        std::string input;
        std::cin >> input;
        int analogIndex = std::stoi(input) - 1;

        if (analogIndex >= 0 && analogIndex < ARRAY_SIZE(analogs)) {
            std::cout << "\nSelect joystick number (0-15, or -1 to cancel): ";
            int joyNum;
            std::cin >> joyNum;

            if (joyNum >= 0 && joyNum < 16) {
                WritePrivateProfileString(analogs[analogIndex], "JoyID", _itoa(joyNum, buff, 10), ControliniPath);

                std::cout << "Select axis (X/Y): ";
                std::string axisInput;
                std::cin >> axisInput;

                if (axisInput == "X" || axisInput == "x" || axisInput == "Y" || axisInput == "y") {
                    WritePrivateProfileString(analogs[analogIndex], "Axis", axisInput.c_str(), ControliniPath);

                    std::cout << "Invert axis? (Y/N): ";
                    char invertChoice = _getch();
                    bool reverse = (invertChoice == 'Y' || invertChoice == 'y');
                    WritePrivateProfileString(analogs[analogIndex], "Reverse", _itoa(reverse, buff, 10), ControliniPath);
                }
            }
        }
    }
    else if (choice == 'c' || choice == 'C') {
        std::cout << "\nEnter analog number to clear (1-" << ARRAY_SIZE(analogs) << "): ";
        std::string input;
        std::cin >> input;
        int analogIndex = std::stoi(input) - 1;

        if (analogIndex >= 0 && analogIndex < ARRAY_SIZE(analogs)) {
            WritePrivateProfileString(analogs[analogIndex], NULL, NULL, ControliniPath);
        }
    }
    else if (choice == '0') {
        input::DestroyJoysticks();
        return;
    }

    input::DestroyJoysticks();
    showAnalogsMenu(); // Refresh menu
}

void showLightsMenu() {
    clearScreen();
    printHeader();

    std::cout << "Arduino Communication Status\n";
    std::cout << "--------------------------\n\n";

    // Read the debug log file to output COM port information
    FILE* fp;
    char line[256];
    bool found = false;

    if (fopen_s(&fp, "debug.log", "r") == 0) {
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "Successfully connected to COM")) {
                char* portInfo = strstr(line, "COM");
                if (portInfo) {
                    found = true;
                    std::cout << "Status: Connected\n";
                    std::cout << "Port: " << std::string(portInfo, 5) << "\n";
                }
                break;
            }
        }
        fclose(fp);
    }

    if (!found) {
        std::cout << "Status: Not Connected\n";
        std::cout << "Port: N/A\n\n";
    }

    std::cout << "\nNote: Arduino LED Controller is used for EZ2DJ cabinet's neon lights.\n";
    std::cout << "Make sure your Arduino is properly connected and configured.\n\n";

    std::cout << "Press any key to return to main menu...";
    _getch();
}

int detectGameVersion() {
    TCHAR fullPath[MAX_PATH];
    TCHAR driveLetter[3];
    TCHAR directory[MAX_PATH];
    TCHAR FinalPath[MAX_PATH];
    DIR* dir;
    struct dirent* ent;
    unsigned char result[MD5_DIGEST_LENGTH];

    GetModuleFileName(NULL, fullPath, MAX_PATH);
    _splitpath(fullPath, driveLetter, directory, NULL, NULL);
    sprintf(FinalPath, "%s%s", driveLetter, directory);

    if ((dir = opendir(FinalPath)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, "2EZConfig.exe") == 0 || !hasEnding(toLower(ent->d_name), ".exe")) {
                printf("Skipping %s\n", ent->d_name);
                continue;
            }

            printf("Comparing %s\n", ent->d_name);

            FILE* inFile = fopen(ent->d_name, "rb");
            MD5_CTX mdContext;
            int bytes;
            unsigned char data[1024];

            if (inFile == NULL) {
                continue;
            }

            MD5_Init(&mdContext);
            while ((bytes = fread(data, 1, 1024, inFile)) != 0) {
                MD5_Update(&mdContext, data, bytes);
            }
            MD5_Final(result, &mdContext);
            fclose(inFile);

            for (int i = 0; i < ARRAY_SIZE(djGames); i++) {
                if (memcmp(result, djGames[i].md5, MD5_DIGEST_LENGTH) == 0) {
                    printf("MD5 Matches: %s\n", djGames[i].name);
                    closedir(dir);
                    return i;
                }
            }
            printf("No Match..\n");
        }
        closedir(dir);
    }

    return ARRAY_SIZE(djGames) - 2;
}
