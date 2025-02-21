#pragma  once
#include <Windows.h>
#include <tlhelp32.h>
#include <Psapi.h>
#include <iostream>
#pragma comment( lib, "psapi.lib" )

#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))
#define NUM_OF_IO 24
#define NUM_OF_ANALOG 2
#define NUM_OF_JOYSTICKS 15

DWORD byteArray[8] = { 0b01111111, 0b10111111, 0b11011111,0b11101111,0b11110111,0b11111011,0b11111101, 0b11111110 };

typedef struct IOBinding {
	bool bound = false;
	int method = 3; // 0 = kb 1= joypad
	int joyID = 16;
	DWORD binding = NULL;
	bool pressed = false;
} ioBinding;

typedef struct IOAnalogs {
	bool bound = false;
	int joyID = 16; //must be 15 or below for valid
	int axis = 3;   //0=X  1=Y
	UINT8 pos = 255;
	bool reverse = false;
} ioAnalogs;

typedef struct VirtualTT {
	UINT8 pos = 255;
	UINT8 plus = NULL;
	UINT8 minus = NULL;
} virtualTT;

typedef struct JoySticks {
	bool init = false;
} Joysticks;


//Easily expandable array with settings for common game hacks,
//Special game hacks such as save setting with need a game specific function due to how differently theyre handled in each game
//I do not plan no expanding that anytime soon anyway, gathering the offsets was annoying.

//MAKE SURE THIS ARRAY ALWAYS MATCHES THE 2EZCONFIG djGames ARRAY ORDER & SIZE
#define OFFSET_SIZE 10
static struct djGame {
	const char* name;
	bool baseAddressOverride;
	uintptr_t devOffset;
	uintptr_t modeTimeOffset;
	uintptr_t songTimerOffset;
} djGames[] = {
	{"The 1st Tracks", false, 0x00, 0x00, 0x00}, //no dump
	{"The 1st Tracks Special Edition", false, 0x00, 0x00, 0x00},
	{"2nd Trax ~It rules once again~", false, 0x00, 0x00, 0x00}, //no dump
	{"3rd Trax ~Absolute Pitch~ ", false, 0x00, 0x00, 0x00},
	{"4th Trax ~OVER MIND~ ", false, 0x00, 0x00, 0x00},
	{"5th Platinum", false, 0x00, 0x00, 0x00 },
	{"6th Trax ~Self Evolution~", false, 0x00, 0x00, 0x00},
	{"7th Trax ~Resistance~", false, 0x00, 0x00, 0x00},
	{"Codename: Violet", false, 0x00, 0x00, 0x00},
	{"Bonus Edition", false, 0x00, 0x00, 0x00}, //no dump
	{"Bonus Edition revision A", false, 0x00, 0x00, 0x00},
	{"Azure ExpressioN Integral Composition", false, 0x00, 0x00, 0x00}, //no dump
	{"Endless Circulation", false, 0xEFD4CC, 0x5978A, 0x53289}, //no good dump 
	{"Evolve", false, 0x944C9C, 0x3A0DE, 0x38257},
	{"Night Traveller", true, 0xF0FE14, 0x42A6D, 0x41854},
	{"Time Traveller", true, 0x1309D74, 0x4740A, 0x45F6C},// need updated dump
	{"Final", false, 0x130CFB4, 0x4A693, 0x46C75},
	{"Final:EX", true, 0x130DFFC, 0x4B773, 0x47CBA},
};

const char* devButtons[] = { "dQuit", "dTest", "dService", "dCoin","dP1 Start", "dP2 Start", "dEffector 1", 
                             "dEffector 2", "dEffector 3", "dEffector 4","dP1 1", "dP1 2", "dP1 3", "dP1 4", "dP1 5", 
                             "dP1 TT+", "dP1 TT-","dP1 Pedal", "dP2 1", "dP2 2", "dP2 3", "dP2 4", "dP2 5",
                             "dP2 TT+", "dP2 TT-", "dP2 Pedal" };

const char* ioButtons[] = { "Test", "Service", "Effector 4", "Effector 3", "Effector 2", "Effector 1",
						   "P2 Start", "P1 Start","P1 Pedal","QE", "QE", "P1 5", "P1 4", "P1 3", "P1 2", "P1 1",
						   "P2 Pedal","QE", "QE", "P2 5", "P2 4", "P2 3", "P2 2", "P2 1" };

const char* analogs[] = { "P1 Turntable", "P2 Turntable" };

const char* lights[] =  {"Effector 1", "Effector 2", "Effector 3", "Effector 4", "P1 Start", "P2 Start",
                        "P2 Turntable", "P2 1", "P1 2", "P1 3", "P1 4", "P1 5", "P1 Turntable", "P2 1", "P2 2",
                        "P2 3", "P2 4", "P2 5", "Neons", "Red Lamp L", "Red Lamp R", "Blue Lamp L", "Blue Lamp R" };
