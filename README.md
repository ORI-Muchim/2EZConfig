# 2EZProject (WIP)

## [Original Repo](https://github.com/ben-rnd/2EZConfig)

## Changes
- Add Light(NEON) OUT -> Working confirmed on FNEX(EZ2AC Memory -> Arduino)

## To-Do
Fixed[✅] / Working In Progress[🚧] / Canceled, Deprecated[❌]

 - [✅] NEON OUT -> Working confirmed on FNEX, NT(EZ2AC game memory -> Arduino)
 - [✅] Number of issues with Mode Select Timer Freeze and Song Select Timer Freeze not working properly - edited ```static struct djGame```
 - [✅] Screenshot sometimes not working
 - [🚧] 6th "Remember 1st?" injection issue fix
 - [🚧] Enable note judgment range changes

## About Autoplay
If the autoplay feature is not enabled on the start screen, it will not work properly in-game (input is delayed).

## Thanks to DJKero and Kasaski for their help with this project!

disclaimer: This project was migrated from dev.s-ul.eu gitlab ni 2023

This is a tool designed to enable USB controllers to work with all versions of EZ2DJ/AC, aswell as offering various patches and hacks to make the home experience more enjoyable. The motivation behind this tool is for the preservation of the most important gamemode of all time: EZ2Catch.

The tool was initally a small personal project that was hacked together in a few nights after frustration when playing EC with joy2key, so i apologise for the general lack of readability.

## Current Compatibility and Features

All EZ2DJ/AC games support this method of IO input emulation.

### Patches 
All Patches are applied in memory after launching the game, and make no modification to original files.
#### Endless Circulation -> FNEX
- Enable and bind deveveloper inputs.
- Timer freeze patches
#### FN
- "Keep Settings" patch. An experimental patch that persists your game settings set between credits - Only stable when playing 5k and 7k Standard.
	
## Get Involved!
Very open to contributions or suggestions, so please get involved to improve the tool :). 
### Current Priority
 - HID Integration for greater compatibilty with input devices such as midi (and also hid lights out), dropping the need for the shit windows JoyAPI.

### WIN XP COMPATIBILTY IS A MUST.
All contributions will be tested on a EZ2AC winxp install with hardware matching that of an EZ2 PC.

## Building
All external library files should be included and pre linked so building should be straight forward. 
Requires the Visual studio 2015 - Windows XP(v140_XP) Platform toolset.

## Roadmap
- Headless mode button mapping (no GUI for legacy systems using TNT2 GPU)
- HID Input, support for various input device (eg.MIDI)
- HID Lighting out
- EZ2Dancer support - IOhooking done, just need to RE the button mappings on ports and create config interface for it (kind of done, check out the ez2dancer branch)
- Increase library of game patches

## Help! The game looks ugly!
EZ2 was built to run on win98 (and later windows xp), so you can expect to come across some pretty ugly graphic issues.

Here are some software that can make the game look much nicer on windows 10
- DDrawCompat (https://github.com/narzoul/DDrawCompat/releases)
- dgVoodoo (http://dege.freeweb.hu/dgVoodoo2/dgVoodoo2/)
  
If i find anything else that improves compatibilty ill update this list

## Contact

Please feel free to contact me on Discord (ori_muchim)
