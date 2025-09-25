@echo off
REM ========================================
REM  EZ2FastIO Driver Build Script
REM  For Windows XP DDK/WDK
REM ========================================

echo.
echo =========================================
echo    EZ2FastIO Kernel Driver Build
echo =========================================
echo.

REM Check multiple possible DDK/WDK locations
set DDK_FOUND=0
set DDK_PATH=

REM Check for Windows XP DDK 2600
if exist "C:\WinDDK\2600" (
    set DDK_PATH=C:\WinDDK\2600
    set DDK_VERSION=2600
    set DDK_FOUND=1
    echo Found Windows DDK 2600 (XP)
    goto :build_ddk
)

REM Check for Windows Server 2003 DDK
if exist "C:\WinDDK\3790.1830" (
    set DDK_PATH=C:\WinDDK\3790.1830
    set DDK_VERSION=3790
    set DDK_FOUND=1
    echo Found Windows Server 2003 DDK
    goto :build_ddk
)

REM Check for WDK 7.1 (also supports XP)
if exist "C:\WinDDK\7600.16385.1" (
    set DDK_PATH=C:\WinDDK\7600.16385.1
    set DDK_VERSION=7600
    set DDK_FOUND=1
    echo Found Windows WDK 7.1
    goto :build_ddk
)

if %DDK_FOUND% equ 0 (
    echo [ERROR] No Windows DDK/WDK found!
    echo.
    echo To build the kernel driver, you need one of:
    echo.
    echo 1. Windows XP DDK 2600 (Recommended)
    echo    Download: Microsoft website (legacy)
    echo.
    echo 2. Windows Server 2003 DDK
    echo    Install to: C:\WinDDK\3790.1830
    echo.
    echo 3. Windows WDK 7.1
    echo    Install to: C:\WinDDK\7600.16385.1
    echo.
    echo ========================================
    echo Alternative: Use pre-built driver
    echo.
    echo If you can't install DDK, you can:
    echo 1. Use the pre-built ez2fastio.sys
    echo 2. Or skip driver (use DLL hook mode)
    echo ========================================
    echo.
    pause
    exit /b 1
)

:build_ddk
REM Set DDK environment
set BASEDIR=%DDK_PATH%
set PATH=%BASEDIR%\bin\x86;%PATH%

echo Using DDK at: %BASEDIR%
echo.

echo [1] Setting build environment...
if exist "%BASEDIR%\bin\setenv.bat" (
    call %BASEDIR%\bin\setenv.bat %BASEDIR% fre WXP
) else if exist "%BASEDIR%\bin\x86\SetEnv.bat" (
    call %BASEDIR%\bin\x86\SetEnv.bat %BASEDIR% fre x86 WXP
) else (
    echo [WARNING] Could not find setenv.bat
    echo Trying to build anyway...
)

echo [2] Building driver...
cd /d %~dp0

REM Create SOURCES file for DDK build system
echo Creating SOURCES file...
(
echo TARGETNAME=ez2fastio
echo TARGETTYPE=DRIVER
echo TARGETPATH=obj
echo SOURCES=ez2fastio.c
echo.
echo # Windows XP support
echo _NT_TARGET_VERSION=0x0501
echo NTDDI_VERSION=0x05010000
echo.
echo # Include paths
echo INCLUDES=$(DDK_INC_PATH);$(CRT_INC_PATH)
echo.
echo # Compiler flags
echo C_DEFINES=$(C_DEFINES) -D_WIN32_WINNT=0x0501
echo MSC_WARNING_LEVEL=/W3 /WX
) > SOURCES

REM Create makefile if needed
if not exist "makefile" (
    echo !INCLUDE $(NTMAKEENV)\makefile.def > makefile
)

echo Building driver...
build -cZ

echo.
echo Checking build results...

if exist obj\i386\ez2fastio.sys (
    echo.
    echo ========================================
    echo    BUILD SUCCESSFUL!
    echo ========================================
    echo.
    echo Driver built: obj\i386\ez2fastio.sys

    REM Copy to output directory
    copy /Y obj\i386\ez2fastio.sys ..\ez2fastio.sys >nul
    echo Copied to: ..\ez2fastio.sys
    echo.
    echo File size:
    for %%A in (..\ez2fastio.sys) do echo   %%~zA bytes
    echo.
    echo Next steps:
    echo   1. Run install.bat as Administrator
    echo   2. Run EZ2Controller.exe
) else if exist objfre_wxp_x86\i386\ez2fastio.sys (
    echo.
    echo ========================================
    echo    BUILD SUCCESSFUL!
    echo ========================================
    echo.
    echo Driver built: objfre_wxp_x86\i386\ez2fastio.sys

    copy /Y objfre_wxp_x86\i386\ez2fastio.sys ..\ez2fastio.sys >nul
    echo Copied to: ..\ez2fastio.sys
) else (
    echo.
    echo [ERROR] Build failed!
    echo.
    echo Check these files for errors:
    if exist "build.log" echo   - build.log
    if exist "build.err" echo   - build.err
    if exist "build.wrn" echo   - build.wrn
    echo.
    echo Common issues:
    echo   - Missing DDK headers
    echo   - Syntax errors in code
    echo   - Wrong DDK version
)

echo.
pause