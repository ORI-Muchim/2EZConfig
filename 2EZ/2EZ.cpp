// Ez2 Io Hook built by kasaski - 2022.
// Very simple solution to Hook EZ2 IO, surprised how simple it is and why nothing has been released earler 
// by much more capable programmers :).
#define _CRT_SECURE_NO_WARNINGS

#include "2EZ.h"
#include <ShlObj.h>
#include <Shlwapi.h>
#include <iostream>
#include "input.h"
#include <thread>
#include <Windows.h>
#include <queue>
#include <mutex>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
#include <ctime>
using namespace std;

// Serial communication related constants
const DWORD BAUD_RATE = CBR_9600;
const DWORD SERIAL_TIMEOUT = 50;
const size_t MAX_COMMAND_LENGTH = 3;  // "0\n" or "1\n"
const DWORD RECONNECT_DELAY = 1000;   // Reconnection wait time (ms)

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

// We count the consecutive write failures and only reconnect after a certain threshold.
#include <atomic>   // for std::atomic
#include <string>   // for std::string

// Define the maximum number of consecutive errors before reconnect
constexpr int MAX_CONSECUTIVE_ERRORS = 5;
// Atomic counters for consecutive errors
std::atomic_int g_consecutiveErrors(0);
// Flag to request a reconnect if we exceed the threshold
std::atomic_bool g_reconnectRequested(false);

// Class for managing serial communication status
class ArduinoController {
private:
    HANDLE hSerial;
    DCB dcbSerialParams;
    COMMTIMEOUTS timeouts;
    bool isInitialized;
    char lastCommand;
    
    // Windows XP 호환 동기화 객체들
    HANDLE hWorkerThread;
    HANDLE hStopEvent;
    HANDLE hCommandEvent;  // 명령 대기용 이벤트
    CRITICAL_SECTION commandQueueCS;
    
    // std::queue 대신 간단한 배열 기반 큐
    static const int MAX_QUEUE_SIZE = 10;
    char commandQueue[MAX_QUEUE_SIZE];
    int queueHead;
    int queueTail;
    int queueSize;
    
    // std::atomic 대신 volatile 변수와 InterlockedXXX 함수 사용
    volatile LONG isHealthy;
    volatile DWORD lastSuccessTime;
    
    static const DWORD HEALTH_CHECK_INTERVAL = 5000;
    static const DWORD MAX_RESPONSE_TIME = 500;

    bool ConfigureSerialPort() {
        ZeroMemory(&dcbSerialParams, sizeof(dcbSerialParams));
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

        if (!GetCommState(hSerial, &dcbSerialParams)) {
            return false;
        }

        dcbSerialParams.BaudRate = 9600;
        dcbSerialParams.ByteSize = 8;
        dcbSerialParams.StopBits = ONESTOPBIT;
        dcbSerialParams.Parity = NOPARITY;
        dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;
        dcbSerialParams.fRtsControl = RTS_CONTROL_ENABLE;

        if (!SetCommState(hSerial, &dcbSerialParams)) {
            return false;
        }

        ZeroMemory(&timeouts, sizeof(timeouts));
        timeouts.ReadIntervalTimeout = 10;
        timeouts.ReadTotalTimeoutConstant = 100;
        timeouts.ReadTotalTimeoutMultiplier = 1;
        timeouts.WriteTotalTimeoutConstant = 100;
        timeouts.WriteTotalTimeoutMultiplier = 1;

        return SetCommTimeouts(hSerial, &timeouts);
    }

    // 큐 관리 함수들
    void InitQueue() {
        queueHead = 0;
        queueTail = 0;
        queueSize = 0;
        ZeroMemory(commandQueue, sizeof(commandQueue));
    }

    bool EnqueueCommand(char command) {
        EnterCriticalSection(&commandQueueCS);
        
        if (queueSize >= MAX_QUEUE_SIZE) {
            // 큐가 가득 차면 가장 오래된 명령 제거
            queueHead = (queueHead + 1) % MAX_QUEUE_SIZE;
            queueSize--;
        }
        
        commandQueue[queueTail] = command;
        queueTail = (queueTail + 1) % MAX_QUEUE_SIZE;
        queueSize++;
        
        LeaveCriticalSection(&commandQueueCS);
        return true;
    }

    bool DequeueCommand(char* command) {
        bool hasCommand = false;
        
        EnterCriticalSection(&commandQueueCS);
        
        if (queueSize > 0) {
            *command = commandQueue[queueHead];
            queueHead = (queueHead + 1) % MAX_QUEUE_SIZE;
            queueSize--;
            hasCommand = true;
        }
        
        LeaveCriticalSection(&commandQueueCS);
        return hasCommand;
    }

    static DWORD WINAPI WorkerThread(LPVOID param) {
        ArduinoController* controller = static_cast<ArduinoController*>(param);
        return controller->WorkerThreadProc();
    }

    DWORD WorkerThreadProc() {
        DWORD lastHealthCheck = GetTickCount();
        HANDLE waitEvents[2] = { hStopEvent, hCommandEvent };
        
        while (true) {
            DWORD waitResult = WaitForMultipleObjects(2, waitEvents, FALSE, 100);
            
            if (waitResult == WAIT_OBJECT_0) {
                // 종료 이벤트
                break;
            }
            
            DWORD currentTime = GetTickCount();
            
            // 명령 큐 처리
            ProcessCommandQueue();
            
            // 정기적인 헬스체크
            if (currentTime - lastHealthCheck > HEALTH_CHECK_INTERVAL) {
                PerformHealthCheck();
                lastHealthCheck = currentTime;
            }
            
            // 응답성 체크
            if (currentTime - InterlockedExchange(&lastSuccessTime, lastSuccessTime) > MAX_RESPONSE_TIME * 10) {
                InterlockedExchange(&isHealthy, 0);
            }
        }
        return 0;
    }

    void ProcessCommandQueue() {
        char command;
        
        while (DequeueCommand(&command) && isInitialized) {
            if (SendCommandInternal(command)) {
                InterlockedExchange(&lastSuccessTime, GetTickCount());
                InterlockedExchange(&isHealthy, 1);
            } else {
                InterlockedExchange(&isHealthy, 0);
                break;
            }
        }
    }

    bool SendCommandInternal(char command) {
        if (!isInitialized || hSerial == INVALID_HANDLE_VALUE) {
            return false;
        }

        if (command == lastCommand) {
            return true;
        }

        PurgeComm(hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);

        char cmdBuffer[3];
        wsprintfA(cmdBuffer, "%c\n", command);  // sprintf_s 대신 wsprintf 사용

        DWORD bytesWritten = 0;
        BOOL result = WriteFile(hSerial, cmdBuffer, lstrlenA(cmdBuffer), &bytesWritten, NULL);

        if (result && bytesWritten == (DWORD)lstrlenA(cmdBuffer)) {
            lastCommand = command;
            FlushFileBuffers(hSerial);
            return true;
        }
        
        return false;
    }

    void PerformHealthCheck() {
        if (!isInitialized) return;
        
        char ping = 'P';
        DWORD bytesWritten = 0;
        BOOL result = WriteFile(hSerial, &ping, 1, &bytesWritten, NULL);
        
        if (!result || bytesWritten != 1) {
            InterlockedExchange(&isHealthy, 0);
            ClosePort();
            Initialize();
        }
    }

public:
    ArduinoController() : 
        hSerial(INVALID_HANDLE_VALUE), 
        isInitialized(false), 
        lastCommand(0),
        hWorkerThread(NULL),
        hStopEvent(NULL),
        hCommandEvent(NULL),
        queueHead(0),
        queueTail(0),
        queueSize(0),
        isHealthy(1),
        lastSuccessTime(0) {
        
        InitializeCriticalSection(&commandQueueCS);
        InitQueue();
        
        hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        hCommandEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        
        InterlockedExchange(&lastSuccessTime, GetTickCount());
    }

    ~ArduinoController() {
        ClosePort();
        
        if (hStopEvent) {
            SetEvent(hStopEvent);
            if (hWorkerThread) {
                WaitForSingleObject(hWorkerThread, 2000);
                CloseHandle(hWorkerThread);
            }
            CloseHandle(hStopEvent);
        }
        
        if (hCommandEvent) {
            CloseHandle(hCommandEvent);
        }
        
        DeleteCriticalSection(&commandQueueCS);
    }

    void ClosePort() {
        if (hSerial != INVALID_HANDLE_VALUE) {
            QueueCommand('0');
            Sleep(100);
            
            CloseHandle(hSerial);
            hSerial = INVALID_HANDLE_VALUE;
        }
        isInitialized = false;
        lastCommand = 0;
    }

    bool SendCommand(char command) {
        if (!isInitialized) {
            return false;
        }
        
        QueueCommand(command);
        return InterlockedExchange(&isHealthy, isHealthy) != 0;
    }

    void QueueCommand(char command) {
        EnqueueCommand(command);
        SetEvent(hCommandEvent);  // 워커 스레드에 명령 도착 알림
    }

    bool Initialize() {
        char portName[12];
        
        for (int i = 2; i <= 10; i++) {
            wsprintfA(portName, "COM%d", i);
            
            hSerial = CreateFileA(portName,
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL);

            if (hSerial != INVALID_HANDLE_VALUE && ConfigureSerialPort()) {
                PurgeComm(hSerial, PURGE_TXCLEAR | PURGE_RXCLEAR);
                Sleep(1000);
                
                isInitialized = true;
                InterlockedExchange(&lastSuccessTime, GetTickCount());
                InterlockedExchange(&isHealthy, 1);
                
                if (hWorkerThread) {
                    CloseHandle(hWorkerThread);
                }
                hWorkerThread = CreateThread(NULL, 0, WorkerThread, this, 0, NULL);
                
                QueueCommand('0');
                return true;
            }
            
            if (hSerial != INVALID_HANDLE_VALUE) {
                CloseHandle(hSerial);
                hSerial = INVALID_HANDLE_VALUE;
            }
        }
        
        return false;
    }

    bool IsInitialized() const {
        return isInitialized;
    }
    
    bool IsHealthy() const {
        return InterlockedExchange(const_cast<volatile LONG*>(&isHealthy), isHealthy) != 0;
    }
};

// Global Arduino controller instance
ArduinoController arduinoController;

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

// Neon status handling function
void HandleNeonOutput(UINT8 lightPattern) {
    static bool lastNeonState = false;
    static DWORD lastStateChangeTime = 0;
    bool currentNeonState = !(lightPattern & 0x10);
    DWORD currentTime = GetTickCount();

    if (currentNeonState != lastNeonState) {
        arduinoController.SendCommand(currentNeonState ? '1' : '0');  // If you want the neon to be reversed, change the position of the '1' and '0'. 

        lastStateChangeTime = currentTime;
        lastNeonState = currentNeonState;
    }
    else if (currentTime - lastStateChangeTime > 200) {  // If the same state persists for more than 0.2 seconds
        arduinoController.SendCommand('0');  // Relay off
        lastStateChangeTime = currentTime;
    }
}

//Debugging anything in here is a bitch as attaching a debugger 
//often crashes the game when using the io shim.
LONG WINAPI IOportHandler(PEXCEPTION_POINTERS pExceptionInfo) {
    //ty batteryshark for the great writeup :)

    if (pExceptionInfo->ExceptionRecord->ExceptionCode != EXCEPTION_PRIV_INSTRUCTION) {
        if (pExceptionInfo->ExceptionRecord->ExceptionCode != DBG_PRINTEXCEPTION_C) {
        }
        return EXCEPTION_EXECUTE_HANDLER;
    }

    unsigned int eip_val = *(unsigned int*)pExceptionInfo->ContextRecord->Eip;
    //
    // EZ2DJ IO INPUT.
    // 

    // Each port has 8 buttons assigned to it. 
    // each port reports a 8bit value, 0 = pressed, 1 = not pressed

    //EZ2DJ Io Input - Missing coin input, no clue what QE are?
    // HANDLE IN AL, DX
    if ((eip_val & 0xFF) == 0xEC) {
        //handle io calls while controller still being setup
        DWORD VAL = pExceptionInfo->ContextRecord->Edx & 0xFFFF;
        switch (pExceptionInfo->ContextRecord->Edx & 0xFFFF) {
        case 0x101: //TEST SVC E4 E3 E2 E1 P2 P1
            pExceptionInfo->ContextRecord->Eax = getButtonInput(0);
            break;
        case 0x102: //P1 Buttons - PEDAL QE QE B5 B4 B3 B2 B1
            pExceptionInfo->ContextRecord->Eax = getButtonInput(8);
            break;
        case 0x103://P1 TT 0-255
            pExceptionInfo->ContextRecord->Eax = getAnalogInput(0);
            break;
        case 0x104://P2 TT 0-255
            pExceptionInfo->ContextRecord->Eax = getAnalogInput(1);
            break;
        case 0x106: //P2 Buttons - PEDAL QE QE B10 B9 B8 B7 B6
            pExceptionInfo->ContextRecord->Eax = getButtonInput(16);
            break;
        default:
            pExceptionInfo->ContextRecord->Eax = 0;
            break;
        }

        pExceptionInfo->ContextRecord->Eip++;
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    //IO OUTPUT
    //For Lighting
    //HANDLE IN  DX, AL
    // LED output processing
    if ((eip_val & 0xFF) == 0xEE) {
        DWORD port = pExceptionInfo->ContextRecord->Edx & 0xFFFF;
        UINT8 lightPattern = (UINT8)(pExceptionInfo->ContextRecord->Eax & 0xFF);

        // Process only the neon bit of port 0x100
        if (port == 0x100) {
            HandleNeonOutput(lightPattern);
        }

        pExceptionInfo->ContextRecord->Eip++;
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    //
    // EZ2Dancer IO Ports
    // 

    // Game input
    // -- Handle IN AX,DX --

    //testing indicates this is somewhat the right range just need to verify further
    //its a PIA so low prio
    if ((eip_val & 0xFFFF) == 0xED66) {
        DWORD VAL = pExceptionInfo->ContextRecord->Edx & 0xFFFF;
        switch (pExceptionInfo->ContextRecord->Edx & 0xFFFF) {
        case 0x300://No clue what this maps to
            /*pExceptionInfo->ContextRecord->Eax = do something
            break;*/
        case 0x302://No clue what this maps to
            /*pExceptionInfo->ContextRecord->Eax = do something
            break;*/
        case 0x304://No clue what this maps to
            /*pExceptionInfo->ContextRecord->Eax = do something
            break;*/
        case 0x306://No clue what this maps to
            /*pExceptionInfo->ContextRecord->Eax = do something
            break;*/
        case 0x308://No clue what this maps to
            /*pExceptionInfo->ContextRecord->Eax = do something
            break;*/
        case 0x310://No clue what this maps to
            /*pExceptionInfo->ContextRecord->Eax = do something
            break;*/
        case 0x312://No clue what this maps to
            /*pExceptionInfo->ContextRecord->Eax = do something
            break;*/
        case 0x314://No clue what this maps to
            /*pExceptionInfo->ContextRecord->Eax = do something
            break;*/
        default:
            pExceptionInfo->ContextRecord->Eax = 0;
            break;
        }
        // Skip over the instruction that caused the exception:
        pExceptionInfo->ContextRecord->Eip += 2;
    }

    // Game output (lights)
    //--Handle IN DX, AX--
    if ((eip_val & 0xFFFF) == 0xEF66) {
        DWORD VAL = pExceptionInfo->ContextRecord->Edx & 0xFFFF;
        switch (pExceptionInfo->ContextRecord->Edx & 0xFFFF) {
        default:
            pExceptionInfo->ContextRecord->Eax = 0;
            break;
        }
        // Skip over the instruction that caused the exception:
        pExceptionInfo->ContextRecord->Eip += 2;
    }

    /*
    If another handler should be called in sequence,
    use return EXCEPTION_EXECUTE_HANDLER;
    Otherwise - let the thread continue.
    */
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
        Sleep(0);
    }

    return 0;
}

// Add these helper functions at the beginning of the file
// GDI+ Encoder CLSID Get Function
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) {
    UINT num = 0;
    UINT size = 0;

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

// String to wstring conversion
std::wstring str2wstr(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// Screenshot function to be called when the hotkey is pressed
void TakeScreenshot() {
    Beep(800, 100);

    // Get game executable name
    char exeName[255];
    GetPrivateProfileStringA("Settings", "EXEName", "EZ2AC.exe", exeName, sizeof(exeName), ControliniPath);

    // Remove extension (keep only filename)
    char* dot = strrchr(exeName, '.');
    if (dot) *dot = '\0';

    // Find game window
    HWND gameWindow = FindWindowA(NULL, exeName);
    if (!gameWindow) {
        // Backup - try other common window names
        const char* backupNames[] = { "EZ2AC", "EZ2DJ" };
        for (const char* name : backupNames) {
            gameWindow = FindWindowA(NULL, name);
            if (gameWindow) break;
        }
    }

    if (gameWindow) {
        // Generate filename with current timestamp
        time_t now = time(0);
        tm* ltm = localtime(&now);
        char filename[256];
        sprintf_s(filename, "Screenshot_%d%02d%02d_%02d%02d%02d.png",
            1900 + ltm->tm_year, 1 + ltm->tm_mon, ltm->tm_mday,
            ltm->tm_hour, ltm->tm_min, ltm->tm_sec);

        // Create Screenshots directory
        CreateDirectoryA("Screenshots", NULL);

        // Create full file path
        char fullPath[512];
        sprintf_s(fullPath, "Screenshots\\%s", filename);

        // Start screenshot capture
        HDC hdcWindow = GetDC(gameWindow);
        HDC hdcMemDC = CreateCompatibleDC(hdcWindow);

        RECT windowRect;
        GetClientRect(gameWindow, &windowRect);
        int width = windowRect.right - windowRect.left;
        int height = windowRect.bottom - windowRect.top;

        HBITMAP hbmScreen = CreateCompatibleBitmap(hdcWindow, width, height);
        HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMemDC, hbmScreen);
        BitBlt(hdcMemDC, 0, 0, width, height, hdcWindow, 0, 0, SRCCOPY);

        // Initialize GDI+
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        ULONG_PTR gdiplusToken;
        Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

        // Get PNG encoder CLSID
        CLSID pngClsid;
        GetEncoderClsid(L"image/png", &pngClsid);

        // Save as PNG
        Gdiplus::Bitmap* bitmap = new Gdiplus::Bitmap(hbmScreen, NULL);
        bitmap->Save(str2wstr(fullPath).c_str(), &pngClsid);

        // Cleanup resources
        delete bitmap;
        Gdiplus::GdiplusShutdown(gdiplusToken);
        SelectObject(hdcMemDC, hbmOld);
        DeleteObject(hbmScreen);
        DeleteDC(hdcMemDC);
        ReleaseDC(gameWindow, hdcWindow);
    }
}

DWORD WINAPI alternateInputThread(void* data) {
    // Get screenshot key binding
    int screenshotKey = GetPrivateProfileIntA("Screenshot", "Binding", NULL, ControliniPath);

    bool autoPlayButtonState = false;
    uintptr_t apOffset = 0;

    // Set autoplay offset based on game
    if (strcmp(currGame.name, "Final:EX") == 0) {
        apOffset = 0x175F2E0;
    }
    else if (strcmp(currGame.name, "Final") == 0) {
        apOffset = 0x175E290;
    }
    else if (strcmp(currGame.name, "Night Traveller") == 0) {
        apOffset = 0x1360EA4;
    }
    else if (strcmp(currGame.name, "Endless Circulation") == 0) {
        apOffset = 0xEF606C;
    }

    while (true) {
        // Autoplay handling
        if (apOffset != 0 && (GetAsyncKeyState(apKey) & 0x8000)) {
            if (!autoPlayButtonState) {
                autoPlayButtonState = true;
                ChangeMemory(baseAddress, 1 + (0 - (*reinterpret_cast<int*>(baseAddress + apOffset))), apOffset);
            }
        }
        else if (autoPlayButtonState) {
            autoPlayButtonState = false;
        }

        // Screenshot handling for all games
        if (screenshotKey && (GetAsyncKeyState(screenshotKey) & 0x8000)) {
            TakeScreenshot();
        }
    }
    Sleep(1);
    return 0;
}

// ==============================================================================
// ReconnectThread: checks g_consecutiveErrors and g_reconnectRequested
//                  to reconnect only when necessary
// ==============================================================================
DWORD WINAPI ReconnectThread(LPVOID data) {
    ArduinoController* pController = reinterpret_cast<ArduinoController*>(data);

    while (true) {
        // Sleep 1 second between checks
        Sleep(1000);

        int errorCount = g_consecutiveErrors.load();
        if (g_reconnectRequested.load() || errorCount >= MAX_CONSECUTIVE_ERRORS) {
            // If the port is initialized, close it before re-initializing
            if (pController->IsInitialized()) {
                pController->ClosePort();
            }

            bool success = pController->Initialize();
            if (success) {
                // Reset the counters and flags on success
                g_consecutiveErrors.store(0);
                g_reconnectRequested.store(false);
            }
            else {
                // If it fails again, we simply wait for the next iteration
            }
        }
    }
    return 0;
}

DWORD PatchThread() {
    // Get Game config file
    HANDLE ez2Proc = GetCurrentProcess();
    baseAddress = (uintptr_t)GetModuleHandleA(NULL);
    GameVer = GetPrivateProfileIntA("Settings", "GameVer", 0, config);
    currGame = djGames[GameVer];
    SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, ControliniPath);
    PathAppendA(ControliniPath, (char*)"2EZ.ini");

    // Short sleep to fix crash when using legitimate data with dongle, can be overiden in ini if causing issues.
    Sleep(GetPrivateProfileIntA("Settings", "ShimDelay", 10, config));

    // re-enable keyboard if playing FNEX - do this before any delays
    if (strcmp(currGame.name, "Final:EX") == 0) {
        zeroMemory(baseAddress, 0x15D51);
    }

    // Hook IO 
    if (GetPrivateProfileIntA("Settings", "EnableIOHook", 0, config)) {
        SetUnhandledExceptionFilter(IOportHandler);
    }

    // Setup Buttons
    for (int b = 0; b < NUM_OF_IO; b++) {
        char buff[20];
        GetPrivateProfileStringA(ioButtons[b], "method", NULL, buff, 20, ControliniPath);
        if (buff != NULL) {
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
        }
    }

    // Setup Analogs
    for (int a = 0; a < NUM_OF_ANALOG; a++) {
        char buff[20];
        GetPrivateProfileStringA(analogs[a], "Axis", NULL, buff, 20, ControliniPath);
        if (buff != NULL) {
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
        }
    }

    // auto play key
    apKey = GetPrivateProfileIntA("Autoplay", "Binding", VK_F11, ControliniPath);

    // Some of the games (ie final) take a while to initialise and will crash or clear out the bindings unless delayed
    // Doesnt cause any issues so i just set this globally on all games, can be overidden in .ini if needed.
    // since we're already hooking IO theres no problem doing this.
    Sleep(GetPrivateProfileIntA("Settings", "BindDelay", 1000, config));

    // Set version text in test menu
    char pattern[] = "Version %d.%02d";
    DWORD versionText = FindPattern(pattern);
    char newText[] = "Version 2.00";
    unsigned long OldProtection;
    VirtualProtect((LPVOID)(versionText), sizeof(newText), PAGE_EXECUTE_READWRITE, &OldProtection);
    CopyMemory((void*)(versionText), &newText, sizeof(newText));
    VirtualProtect((LPVOID)(versionText), sizeof(newText), OldProtection, NULL);

    if (strcmp(currGame.name, "Evolve") == 0 && GetPrivateProfileIntA("Settings", "EvWin10Fix", 0, config)) {
        NOPMemory(baseAddress, 0x11757);
        NOPMemory(baseAddress, 0x11758);
        NOPMemory(baseAddress, 0x11759);
        NOPMemory(baseAddress, 0x11770);
        NOPMemory(baseAddress, 0x11771);
    }

    if (currGame.devOffset != 0x00 && GetPrivateProfileIntA("Settings", "EnableDevControls", 0, config)) {
        setDevBinding(baseAddress, currGame.devOffset, ControliniPath);
    }

    if (currGame.modeTimeOffset != 0x00 && GetPrivateProfileIntA("Settings", "ModeSelectTimerFreeze", 0, config)) {
        zeroMemory(baseAddress, currGame.modeTimeOffset);
    }

    if (currGame.songTimerOffset != 0x00 && GetPrivateProfileIntA("Settings", "SongSelectTimerFreeze", 0, config)) {
        zeroMemory(baseAddress, currGame.songTimerOffset);
    }

    // FINAL save settings patch
    if (strcmp(currGame.name, "Final") == 0 && GetPrivateProfileIntA("KeepSettings", "Enabled", 0, config)) {
        FnKeepSettings(baseAddress);
    }

    if (GetPrivateProfileIntA("StageLock", "Enabled", 0, config)) {
        if (strcmp(currGame.name, "Final") == 0) {
            FNStageLock(baseAddress);
        }

        if (strcmp(currGame.name, "Night Traveller") == 0) {
            NTStageLock(baseAddress);
        }

        if (strcmp(currGame.name, "Final:EX") == 0) {
            FNEXStageLock(baseAddress);
        }
    }

    vTT[0].plus = GetPrivateProfileIntA("P1 Turntable +", "Binding", NULL, ControliniPath);
    vTT[0].minus = GetPrivateProfileIntA("P1 Turntable -", "Binding", NULL, ControliniPath);
    vTT[1].plus = GetPrivateProfileIntA("P2 Turntable +", "Binding", NULL, ControliniPath);
    vTT[1].minus = GetPrivateProfileIntA("P2 Turntable -", "Binding", NULL, ControliniPath);

    HANDLE turntableThread = CreateThread(NULL, 0, virtualTTThread, NULL, 0, NULL);
    HANDLE inputThread = CreateThread(NULL, 0, alternateInputThread, NULL, 0, NULL);

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

        if (!arduinoController.Initialize()) {
            // Handle initialization failure (if needed)
        }

        // Create a separate thread to handle reconnection logic
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ReconnectThread, &arduinoController, 0, 0);

        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)PatchThread, 0, 0, 0);
        break;

    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
