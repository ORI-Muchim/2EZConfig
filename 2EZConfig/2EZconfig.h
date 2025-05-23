#pragma once

#include <openssl/md5.h>
#include <Windows.h>
#include "imgui_impl_glfw.h"
#include <stdio.h>

namespace EZConfig {
    int RenderUI(GLFWwindow* window);
    int sixthBackgroundLoop(char * launcherName);

    const LPCSTR ConfigIniPath = ".\\2EZ.ini";

    static struct djGame {
        const char* name;
        const char* defaultExeName;
        unsigned char md5[MD5_DIGEST_LENGTH]; //OF LEGIT/UNCRACKED/UNTAMPERED EXE 
        bool hasDevBindings;
        bool hasModeFreeze;
        bool hasSongFreeze;
        bool hasSaveSettings;
        bool hasStageLock;
        bool hasJudgmentDelta;
    } djGames[] = {
        {"The 1st Tracks", "EZ2DJ.exe", "", false, false, false, false, false}, //no dump
        {"The 1st Tracks Special Edition", "EZ2DJ.exe", { '\x57','\x60','\xcf','\xb4','\xf5','\x56','\xd7','\x07','\x11','\xf1','\x8f','\x47','\x34','\x57','\xc5','\x7b'}, false, false, false, false, false},
        {"2nd Trax ~It rules once again~", "EZ2DJ.exe", "", true, true, true, false, false, true}, //no dump
        {"3rd Trax ~Absolute Pitch~ ", "EZ2DJ.exe", {'\xbb','\x44','\x7e','\xe2','\x58','\x1f','\x77','\xd3','\x40','\xd4','\x16','\xd2','\xda','\xf0','\x90','\xab'}, true, true, true, false, false, true},
        {"4th Trax ~OVER MIND~ ", "EZ2DJ.exe", {'\xed','\x02','\x84','\x50','\x0b','\x65','\x01','\x9d','\x21','\x95','\xe1','\xa0','\x22','\xa2','\x95','\xde'}, true, true, true, false, false, true},
        {"5th Platinum", "EZ2DJ.exe", {'\xa3','\xe9','\x90','\x89','\x53','\x6e','\x7e','\xea','\xb5','\xe8','\xeb','\x13','\xf9','\x93','\x09','\xcd'}, true, true, true, false, false, true},
        /*6th requires to be changed from ez2dj.exe for hooking loop to work*/
        {"6th Trax ~Self Evolution~","EZ2DJ-Launcher.exe", {'\xce','\x6d','\x77','\xd8','\x30','\x36','\x82','\x63','\x6a','\x70','\x50','\xa4','\x32','\x15','\xa1','\x40'}, true, true, true, false, false, true},
        {"7th Trax ~Resistance~","EZ2DJ.exe", {'\xc2','\x5f','\xd2','\x44','\xff','\x83','\x3f','\x1c','\x28','\x16','\x41','\xda','\x2f','\xf9','\x62','\x67'}, true, true, true, false, false, true},
        {"Codename: Violet", "EZ2DJ.exe", {'\x80','\x20','\xb2','\xb1','\x1c','\x93','\x1e','\xfc','\x1f','\xa7','\x7e','\x91','\x7d','\xca','\x18','\xb1'}, true, true, true, false, false, true},
        {"Bonus Edition", "EZ2DJBe.exe", "", true, true, true, false, false, true}, //no dump
        {"Bonus Edition revision A", "EZ2DJBe.exe", {'\xeb','\xf1','\xcf','\xd5','\x79','\x8a','\x2d','\xb7','\x63','\xce','\xe4','\x76','\x25','\xa8','\xd1','\xe3'}, true, true, true, false, false, true},
        {"Azure ExpressioN Integral Composition", "EZ2DJ.exe", "", true, true, true, false, false, true}, //no dump
        {"Endless Circulation", "EZ2AC.exe", "", true, true, true, false, false, true}, //no dump
        {"Evolve", "EZ2AC.exe", {'\x09','\x20','\xe5','\x4a','\x25','\x39','\xe3','\xfd','\x5d','\xe5','\x3a','\xa3','\x7d','\x9b','\x33','\x8a'}, true, true, true, false, false, true},
        {"Night Traveller", "EZ2AC.exe", {'\x6c','\x12','\x50','\x9f','\x89','\xb3','\x50','\x4c','\x1a','\xab','\xc2','\x9b','\xa7','\xb7','\x32','\xe8'}, true, true, true, false, true, true},
        {"Time Traveller", "EZ2AC.exe", {'\x03','\x9a','\x5d','\x23','\x3c','\x15','\x12','\x01','\x11','\x2f','\x00','\xfb','\xb6','\x4c','\x21','\xda'}, true, true, true, false, false, true},
        {"Final", "EZ2AC.exe", {'\x33','\x6a','\xb9','\x6c','\xae','\x01','\xe0','\x1e','\x06','\x9f','\xb8','\x05','\x58','\x3d','\x02','\x89'}, true, true, true, true, true, true},
        {"Final:EX", "EZ2AC.exe", {'\xbc','\xe8','\x48','\xf2','\xd7','\x94','\x5c','\x36','\x12','\x0a','\x2a','\xda','\xa5','\x73','\x57','\x48'}, true, true, true, false, true, true},
    };

    static struct dancerGame {
        const char* name;
        unsigned char md5[MD5_DIGEST_LENGTH];
    } dancerGames[] = {
        {"1st Move", ""}, //no dump
        {"2nd Move", ""}, 
        {"UK Move", ""}, //no dump
        {"UK Move SE", ""},
        {"Super China", ""},
    };
    //EZ2 INPUT/OUTPUT NAMES
    //i cant remember why i have 2 of these?
    static const char* devButtons[] = { "dQuit", "dTest", "dService", "dCoin","dP1 Start", "dP2 Start", "dEffector 1",
                             "dEffector 2", "dEffector 3", "dEffector 4","dP1 1", "dP1 2", "dP1 3", "dP1 4", "dP1 5",
                             "dP1 TT+", "dP1 TT-","dP1 Pedal", "dP2 1", "dP2 2", "dP2 3", "dP2 4", "dP2 5",
                             "dP2 TT+", "dP2 TT-", "dP2 Pedal" };
    //why?
    static const char* devButtonsDisplay[] = { "Quit", "Test", "Service", "Coin","P1 Start", "P2 Start", "Effector 1",
                                 "Effector 2", "Effector 3", "Effector 4","P1 1", "P1 2", "P1 3", "P1 4", "P1 5",
                                 "P1 Turntable +", "P1 Turntable -","P1 Pedal", "P2 1", "P2 2", "P2 3", "P2 4", "P2 5",
                                 "P2 Turntable +", "P2 Turntable -", "P2 Pedal" };

    static const char* ioButtons[] = {  "Test", 
                                        "Service", 
                                        "Effector 1", 
                                        "Effector 2", 
                                        "Effector 3", 
                                        "Effector 4",                 
                                        "P1 Start", 
                                        "P2 Start", 
                                        "P1 1", 
                                        "P1 2", 
                                        "P1 3", 
                                        "P1 4", 
                                        "P1 5", 
                                        "P1 Turntable +",
                                        "P1 Turntable -",
                                        "P1 Pedal",
                                        "P2 1", 
                                        "P2 2", 
                                        "P2 3", 
                                        "P2 4", 
                                        "P2 5", 
                                        "P2 Turntable +",
                                        "P2 Turntable -",
                                        "P2 Pedal",
                                        };

    static const char* analogs[] = { "P1 Turntable", 
                                     "P2 Turntable" };

    static const char* lights[] = { "Effector 1", 
                                    "Effector 2", 
                                    "Effector 3",
                                    "Effector 4", 
                                    "P1 Start", 
                                    "P2 Start",       
                                    "P1 Turntable", 
                                    "P1 1", 
                                    "P1 2", 
                                    "P1 3", 
                                    "P1 4", 
                                    "P1 5", 
                                    "P2 Turntable", 
                                    "P2 1", 
                                    "P2 2",                 
                                    "P2 3", 
                                    "P2 4", 
                                    "P2 5", 
                                    "Neons", 
                                    "Red Lamp L", 
                                    "Red Lamp R", 
                                    "Blue Lamp L", 
                                    "Blue Lamp R" };
    
    
    //FINAL SHIT
    static const char* noteSkins[] = { "Default", "2nd", "1st SE", "Simple", "Steel","3S","3S RB","Circle", "Disc", "Star", "Turtle", "Gem" };

    static const char* panelSkins[] = { "FN", "TT Blue", "TT Green", "NT", "EV","EC","AEIC", "3S", "CV", "7th", "6th", "5th", "4th",
                                "3rd - Silver", "3rd - Gold", "3rd - Green", "2nd - Silver", "2nd - Green", "2nd - Black", "1st SE", "1st" };

    static const char* VisualSettings[] = { "Default", "Black Panel", "BGA Off" };
}
