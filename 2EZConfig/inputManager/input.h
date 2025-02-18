#pragma once

#define IMGUI_DISABLE_SSE2
#define OPENSSL_NO_ASM

#include <string>
namespace input {

    struct JoystickState {
        UINT joyID;
        UINT8 x;
        UINT8 y;
        DWORD buttonsPressed;
        int NumPressed;
        bool input = false;
    };


    bool InitJoystick(int joyID);
    bool InitJoysticks();
    bool DestroyJoysticks();
    //JoystickState GetJoyButtonPressed();
    JoystickState GetJoyState(UINT joyID);
    int checkKbPressedState();

    std::string getButtonName(int button);

}

