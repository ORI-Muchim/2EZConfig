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



void ChangeMemory(uintptr_t baseaddress, int value, uintptr_t offset)
{
	unsigned long OldProtection;
	VirtualProtect((LPVOID)(baseaddress + offset), sizeof(value), PAGE_EXECUTE_READWRITE, &OldProtection);
    *(BYTE*)(baseaddress + offset) = value;
	VirtualProtect((LPVOID)(baseaddress + offset), sizeof(value), OldProtection, NULL);
}


void zeroMemory(uintptr_t baseaddress, uintptr_t offset)
{
	unsigned long OldProtection;
	VirtualProtect((LPVOID)(baseaddress + offset), 8, PAGE_EXECUTE_READWRITE, &OldProtection);
	*(BYTE*)(baseaddress + offset) = 0x00;
	VirtualProtect((LPVOID)(baseaddress + offset), 8, OldProtection, NULL);

}

void NOPMemory(uintptr_t baseaddress, uintptr_t offset)
{
	unsigned long OldProtection;
	VirtualProtect((LPVOID)(baseaddress + offset), 8, PAGE_EXECUTE_READWRITE, &OldProtection);
	*(BYTE*)(baseaddress + offset) = 0x90;
	VirtualProtect((LPVOID)(baseaddress + offset), 8, OldProtection, NULL);

}

void setDevBinding(uintptr_t baseAddress, uintptr_t startAddress, LPCSTR bindings) {
	DWORD inputOffset = 0x14;
	for (int i = 0; i < IM_ARRAYSIZE(devButtons); i++) {
		//if binding is not this crap, set binding
		int binding = GetPrivateProfileIntA(devButtons[i], "Binding", 0, bindings);
		if (binding != 0) {

			//sets the key that will be checked by the getAsyncKeypress call in the ez2 exe  
			//this is why its kb binding only, theres no point around allowing button bidning as itll just be a joy2key type solution
			//written into the DLL, users should just use the IO binding instead
			ChangeMemory(baseAddress, binding, startAddress + (inputOffset * i));

			//Setting this address to 0 enables the key - but disabled the IO equiv
			//Known values:
			//0 = Enabled  - Disable IO
			//1 = ??
			//2 = ??
			//3 = Disabled - use IO
			ChangeMemory(baseAddress, 0, startAddress + (inputOffset * i) - 4);
		}
	}
}


//experimental save settings patch, crashes lots of game modes :)
//each game felt too different to have a common function for this
void fnNOPResetCode(uintptr_t baseAddress, uintptr_t resetAddresses) {
	for (int z = 0; z < 5; z++) {
		NOPMemory(baseAddress, resetAddresses + z);
	}
}


void FnKeepSettings(uintptr_t baseAddress) {			//Random  //Note    //Auto    /Fade     /Scroll  //Visual	//Panel
	DWORD resetAddresses[7] = {0x30C0F, 0x30C23, 0x30C46, 0x30C0A, 0x30C19, 0x30BEC, 0x30C87};
	DWORD Panel = 0x33D0F3C;
	DWORD Note = 0x33D0EC4;
	DWORD Visual = 0x33D0EF0;
	int defaultNote = GetPrivateProfileIntA("KeepSettings", "DefaultNote", 7, ".\\2EZ.ini");
	int defaultPanel  = GetPrivateProfileIntA("KeepSettings", "DefaultPanel", 0, ".\\2EZ.ini");
	int defaultVisual = GetPrivateProfileIntA("KeepSettings", "DefaultVisual", 0, ".\\2EZ.ini");

	if (GetPrivateProfileIntA("KeepSettings", "Random", 1, ".\\2EZ.ini")) {
		fnNOPResetCode(baseAddress, resetAddresses[0]);
	}

	if (GetPrivateProfileIntA("KeepSettings", "Note", 1, ".\\2EZ.ini")) {
		fnNOPResetCode(baseAddress, resetAddresses[1]);
		ChangeMemory(baseAddress, defaultNote, Note);
	}

	if (GetPrivateProfileIntA("KeepSettings", "Auto", 1, ".\\2EZ.ini")) {
		fnNOPResetCode(baseAddress, resetAddresses[2]);
	}

	if (GetPrivateProfileIntA("KeepSettings", "Fade", 1, ".\\2EZ.ini")) {
		fnNOPResetCode(baseAddress, resetAddresses[3]);
	}

	if (GetPrivateProfileIntA("KeepSettings", "Scroll", 1, ".\\2EZ.ini")) {
		fnNOPResetCode(baseAddress, resetAddresses[4]);
	}

	if (GetPrivateProfileIntA("KeepSettings", "Visual", 1, ".\\2EZ.ini")) {
		fnNOPResetCode(baseAddress, resetAddresses[5]);
		ChangeMemory(baseAddress, defaultVisual, Visual);
	}

	if (GetPrivateProfileIntA("KeepSettings", "Panel", 1, ".\\2EZ.ini")) {
		fnNOPResetCode(baseAddress, resetAddresses[6]);
		ChangeMemory(baseAddress, defaultPanel, Panel);
	}

}

//Expermental pfree like hack
void FNStageLock(uintptr_t baseAddress) {								 


	if (GetPrivateProfileIntA("StageLock", "noFail", 0, ".\\2EZ.ini")) { // will freese at stage 1 - You cant fail out of a song during this

		ChangeMemory(baseAddress, 0x3B, 0x16D15);//5kOnly & Radio Mix
		ChangeMemory(baseAddress, 0x3B, 0x16835);//RubyMix
		ChangeMemory(baseAddress, 0x3B, 0x16AFF);//5 key Standard (Street Mix)
		ChangeMemory(baseAddress, 0x3B, 0x177EC);//10 key (Club Mix)
		ChangeMemory(baseAddress, 0x3B, 0x17979);//14k
		ChangeMemory(baseAddress, 0x00, 0x16250);//ez2Catch
		ChangeMemory(baseAddress, 0x3B, 0x17E25);//Turntable

	} else { //After 1st stage completed, will freese at stage 2, allowing you to fail out of a song
			
		ChangeMemory(baseAddress, 0x8B, 0x16D15);//5kOnly & Radio Mix
		ChangeMemory(baseAddress, 0x8B, 0x16835);//RubyMix
		ChangeMemory(baseAddress, 0x8B, 0x16AFF);//5 key Standard (Street Mix)
		ChangeMemory(baseAddress, 0x8B, 0x177EC);//10 key (Club Mix)
		ChangeMemory(baseAddress, 0x8B, 0x17977);//14k
		ChangeMemory(baseAddress, 0x6A, 0x16253);//ez2Catch
		ChangeMemory(baseAddress, 0x01, 0x16254);
		ChangeMemory(baseAddress, 0x5E, 0x16255);
		ChangeMemory(baseAddress, 0x8B, 0x17E25);//Turntable
	}

	if (GetPrivateProfileIntA("StageLock", "noGameOver", 0, ".\\2EZ.ini")) {
		
		ChangeMemory(baseAddress, 0xCF, 0x16C3B);//5 Key Only and Radio Mix
		ChangeMemory(baseAddress, 0xC6, 0x16A27);//5 key Standard (Street Mix)
		ChangeMemory(baseAddress, 0xCE, 0x17707);//10 key (Club Mix)
		ChangeMemory(baseAddress, 0xFF, 0x17927);//14Key (Space Mix)
		ChangeMemory(baseAddress, 0xFF, 0x161C7);//ez2Catch
		ChangeMemory(baseAddress, 0xCE, 0x17D3B);//Turntable
	}
}

//Expermental pfree like hack
void NTStageLock(uintptr_t baseAddress) {

	// will freese at stage 1 - You cant fail out of a song during this
	if (GetPrivateProfileIntA("StageLock", "noFail", 0, ".\\2EZ.ini")) {

		ChangeMemory(baseAddress, 0x00, 0x16D64);//5kOnly

		ChangeMemory(baseAddress, 0x00, 0x16070);//5kStandard

	}
	//After 1st stage completed, will freese at stage 2, allowing you to fail out of a song
	else { 
		//5kOnly
		ChangeMemory(baseAddress, 0x6A, 0x16D62);
		ChangeMemory(baseAddress, 0x01, 0x16D63);
		ChangeMemory(baseAddress, 0x5B, 0x16D64);
		//ruby
		ChangeMemory(baseAddress, 0x6A, 0x15dA2);
		ChangeMemory(baseAddress, 0x01, 0x15dA3);
		ChangeMemory(baseAddress, 0x5B, 0x15dA4);
		//5kStandard
		ChangeMemory(baseAddress, 0x6A, 0x1606E);
		ChangeMemory(baseAddress, 0x01, 0x1606F);
		ChangeMemory(baseAddress, 0x5B, 0x16070);
		//7kStandard
		ChangeMemory(baseAddress, 0x6A, 0x1630A);
		ChangeMemory(baseAddress, 0x01, 0x1630B);
		ChangeMemory(baseAddress, 0x5F, 0x1630C);
		//10kManiac
		ChangeMemory(baseAddress, 0x6A, 0x17012);
		ChangeMemory(baseAddress, 0x01, 0x17013);
		ChangeMemory(baseAddress, 0x5B, 0x17014);
		//14kManiac
		ChangeMemory(baseAddress, 0x6A, 0x172F9);
		ChangeMemory(baseAddress, 0x01, 0x172FA);
		ChangeMemory(baseAddress, 0x5B, 0x172FB);

		//Ez2Catch
		ChangeMemory(baseAddress, 0x6A, 0x1580D);
		ChangeMemory(baseAddress, 0x01, 0x1580E);
		ChangeMemory(baseAddress, 0x5B, 0x1580F);

		//Turntable
		ChangeMemory(baseAddress, 0x6A, 0x17897);
		ChangeMemory(baseAddress, 0x01, 0x17898);
		ChangeMemory(baseAddress, 0x5B, 0x17899);
	}

	if (GetPrivateProfileIntA("StageLock", "noGameOver", 0, ".\\2EZ.ini")) {
		//5kOnly
		ChangeMemory(baseAddress, 0x15, 0x16CDA);
		//Ruby
		ChangeMemory(baseAddress, 0x15, 0x15D08);
		//5kStandard
		ChangeMemory(baseAddress, 0x15, 0x15FE8);
		//7kStandard
		ChangeMemory(baseAddress, 0x15, 0x1625A);
		ChangeMemory(baseAddress, 0x15, 0x162B1);
		//10kManiac
		ChangeMemory(baseAddress, 0x15, 0x16F8A);
		//14Maniac
		ChangeMemory(baseAddress, 0x15, 0x1725A);
		//Ez2Catch
		ChangeMemory(baseAddress, 0x15, 0x15777);
		//Turntable
		ChangeMemory(baseAddress, 0x15, 0x177E7);

	}
}

void FNEXStageLock(uintptr_t baseAddress) {

	if (GetPrivateProfileIntA("StageLock", "noFail", 0, ".\\2EZ.ini")) { // will freese at stage 1 - You cant fail out of a song during this

		ChangeMemory(baseAddress, 0x3B, 0x16CC5);//5kOnly & Radio Mix
		ChangeMemory(baseAddress, 0x3B, 0x167E5);//RubyMix
		ChangeMemory(baseAddress, 0x3B, 0x16AAF);//5 key Standard (Street Mix)
		ChangeMemory(baseAddress, 0x3B, 0x1779C);//10 key (Club Mix)
		ChangeMemory(baseAddress, 0x3B, 0x17927);//14k
		ChangeMemory(baseAddress, 0x00, 0x16205);//ez2Catch
		ChangeMemory(baseAddress, 0x3B, 0x17DD5);//Turntable

	}
	else { //After 1st stage completed, will freese at stage 2, allowing you to fail out of a song

		ChangeMemory(baseAddress, 0x8B, 0x16CC5);//5kOnly & Radio Mix
		ChangeMemory(baseAddress, 0x8B, 0x167E5);//RubyMix
		ChangeMemory(baseAddress, 0x8B, 0x16AAF);//5 key Standard (Street Mix)
		ChangeMemory(baseAddress, 0x8B, 0x1779C);//10 key (Club Mix)
		ChangeMemory(baseAddress, 0x8B, 0x17927);//14k
		ChangeMemory(baseAddress, 0x6A, 0x16203);//ez2Catch
		ChangeMemory(baseAddress, 0x01, 0x16204);
		ChangeMemory(baseAddress, 0x5E, 0x16205);
		ChangeMemory(baseAddress, 0x8B, 0x17DD5);//Turntable
	}

	if (GetPrivateProfileIntA("StageLock", "noGameOver", 0, ".\\2EZ.ini")) {

		ChangeMemory(baseAddress, 0xC2, 0x16BEB);//5 Key Only and Radio Mix
		//ChangeMemory(baseAddress, 0xC6, 0x16A27);//5 key Standard (Street Mix)
		//ChangeMemory(baseAddress, 0xCE, 0x17707);//10 key (Club Mix)
		//ChangeMemory(baseAddress, 0xFF, 0x17927);//14Key (Space Mix)
		//ChangeMemory(baseAddress, 0xFF, 0x161C7);//ez2Catch
		//ChangeMemory(baseAddress, 0xCE, 0x17D3B);//Turntable
	}
}

DWORD FindPattern(char* pattern)
{

	HMODULE hModule = GetModuleHandle(NULL);

	MODULEINFO mInfo; GetModuleInformation(GetCurrentProcess(), hModule, &mInfo, sizeof(MODULEINFO));
	DWORD base = (DWORD)mInfo.lpBaseOfDll;
	DWORD size = (DWORD)mInfo.SizeOfImage;
	DWORD patternLength = (DWORD)strlen(pattern);
	for (DWORD i = 0; i < size - patternLength; i++)
	{
		bool found = true;
		for (DWORD j = 0; j < patternLength; j++)
		{
			found &= pattern[j] == '?' || pattern[j] == *(char*)(base + i + j);
		}
		if (found)
		{
			return base + i;
		}
	}
	return NULL;

}
