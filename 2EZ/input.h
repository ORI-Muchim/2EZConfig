// Keep SSE2 disabled for ImGui
#define IMGUI_DISABLE_SSE2

// Enable MMX and SSE, disable SSE2
#define __MMX__      // Enable MMX support
#define __SSE__      // Enable SSE support
#define HAVE_MMX     // Enable MMX intrinsics
#define HAVE_SSE     // Enable SSE intrinsics
#define OPENSSL_NO_ASM  // Disable assembly optimizations

#pragma once
#include <string>
#include <Windows.h>
namespace input {

   typedef struct JoystickState {
        UINT joyID = 16;
        UINT8 x = 0;
        UINT8 y = 0;
        DWORD buttonsPressed = 0;
        int NumPressed = 0;
        bool input = false;
    };

    bool InitJoystick(int joyID);
    bool isJoyButtonPressed(int joyID, DWORD Button);
    UINT8 JoyAxisPos(int joyID, int axis);
    bool isKbButtonPressed(DWORD key);

}
