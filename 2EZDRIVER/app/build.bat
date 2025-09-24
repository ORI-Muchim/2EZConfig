@echo off
REM ========================================
REM  EZ2Controller Build Script
REM  Simple compilation for Windows XP
REM ========================================

echo.
echo ===== Building EZ2Controller.exe =====
echo.

REM Try Visual Studio 2005/2008 (XP compatible)
if exist "C:\Program Files\Microsoft Visual Studio 8\VC\bin\cl.exe" (
    set COMPILER="C:\Program Files\Microsoft Visual Studio 8\VC\bin\cl.exe"
    set VCVARS="C:\Program Files\Microsoft Visual Studio 8\VC\vcvarsall.bat"
    goto :build
)

if exist "C:\Program Files\Microsoft Visual Studio 9.0\VC\bin\cl.exe" (
    set COMPILER="C:\Program Files\Microsoft Visual Studio 9.0\VC\bin\cl.exe"
    set VCVARS="C:\Program Files\Microsoft Visual Studio 9.0\VC\vcvarsall.bat"
    goto :build
)

REM Try MinGW as alternative
if exist "C:\MinGW\bin\g++.exe" (
    echo Using MinGW compiler...
    C:\MinGW\bin\g++.exe -o EZ2Controller.exe EZ2Controller.cpp -lkernel32 -static
    if exist EZ2Controller.exe (
        echo [SUCCESS] Built with MinGW!
        copy EZ2Controller.exe ..
    )
    goto :end
)

echo [ERROR] No compatible compiler found!
echo Please install one of:
echo   - Visual Studio 2005 (recommended for XP)
echo   - Visual Studio 2008
echo   - MinGW
pause
exit /b 1

:build
echo Setting up environment...
call %VCVARS% x86

echo Compiling...
cl.exe /MT /O2 /Fe:EZ2Controller.exe EZ2Controller.cpp kernel32.lib user32.lib

if exist EZ2Controller.exe (
    echo.
    echo [SUCCESS] Build complete!
    copy EZ2Controller.exe ..
    echo Copied to parent directory
) else (
    echo [ERROR] Build failed!
)

:end
echo.
pause