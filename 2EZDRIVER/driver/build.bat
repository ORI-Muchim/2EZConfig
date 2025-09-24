@echo off
REM ========================================
REM  EZ2FastIO Driver Build Script
REM  For Windows XP DDK 2600
REM ========================================

echo.
echo ===== EZ2FastIO Driver Build Script =====
echo.

REM Check if DDK is installed
if not exist "C:\WinDDK\2600" (
    echo [ERROR] Windows DDK 2600 not found!
    echo Please install Windows DDK for XP from:
    echo https://www.microsoft.com/en-us/download/details.aspx?id=11800
    pause
    exit /b 1
)

REM Set DDK environment
set BASEDIR=C:\WinDDK\2600
set PATH=%BASEDIR%\bin\x86;%PATH%

echo [1] Setting build environment...
call %BASEDIR%\bin\setenv.bat %BASEDIR% fre WXP

echo [2] Building driver...
cd /d %~dp0

REM Create SOURCES file
echo TARGETNAME=ez2fastio > SOURCES
echo TARGETTYPE=DRIVER >> SOURCES
echo TARGETPATH=obj >> SOURCES
echo SOURCES=ez2fastio.c >> SOURCES

REM Build
build -cZ

if exist obj\i386\ez2fastio.sys (
    echo.
    echo [SUCCESS] Driver built successfully!
    echo Output: obj\i386\ez2fastio.sys

    REM Copy to output directory
    copy obj\i386\ez2fastio.sys ..\ez2fastio.sys
    echo Copied to: ..\ez2fastio.sys
) else (
    echo.
    echo [ERROR] Build failed!
    echo Check build.log for details
)

echo.
pause