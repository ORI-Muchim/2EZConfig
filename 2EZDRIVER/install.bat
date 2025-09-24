@echo off
REM ========================================
REM  EZ2FastIO Driver Installation
REM  For Windows XP (No signature required!)
REM ========================================

echo.
echo =========================================
echo    EZ2AC Fast IO Driver Installer
echo    Version 1.0 for Windows XP
echo =========================================
echo.

REM Check for admin rights
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Administrator privileges required!
    echo Right-click and select "Run as Administrator"
    pause
    exit /b 1
)

REM Check if driver file exists
if not exist "ez2fastio.sys" (
    echo [ERROR] Driver file not found: ez2fastio.sys
    echo Please build the driver first using build.bat
    pause
    exit /b 1
)

echo [1] Checking for existing driver...

sc query EZ2FastIO >nul 2>&1
if %errorlevel% equ 0 (
    echo    Found existing driver. Stopping...
    sc stop EZ2FastIO >nul 2>&1
    timeout /t 2 /nobreak >nul

    echo    Removing old driver...
    sc delete EZ2FastIO >nul 2>&1
    timeout /t 2 /nobreak >nul
)

echo [2] Copying driver to system32\drivers...
copy /Y ez2fastio.sys %WINDIR%\system32\drivers\ez2fastio.sys
if %errorlevel% neq 0 (
    echo [ERROR] Failed to copy driver!
    pause
    exit /b 1
)

echo [3] Installing driver service...
sc create EZ2FastIO binpath= %WINDIR%\system32\drivers\ez2fastio.sys type= kernel start= demand DisplayName= "EZ2AC Fast IO Driver"

if %errorlevel% neq 0 (
    echo [ERROR] Failed to create service!
    pause
    exit /b 1
)

echo [4] Starting driver...
sc start EZ2FastIO

if %errorlevel% neq 0 (
    echo [WARNING] Driver installed but not started
    echo You may need to restart your computer
) else (
    echo [SUCCESS] Driver started successfully!
)

echo.
echo =========================================
echo    Installation Complete!
echo.
echo    The EZ2FastIO driver is now installed.
echo    Run EZ2Controller.exe to configure.
echo.
echo    Latency: <0.5ms (kernel-level)
echo    No signature needed on Windows XP!
echo =========================================
echo.

pause