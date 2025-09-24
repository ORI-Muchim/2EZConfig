@echo off
REM ========================================
REM  EZ2FastIO Driver Uninstaller
REM ========================================

echo.
echo ===== EZ2FastIO Driver Uninstaller =====
echo.

REM Check for admin rights
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [ERROR] Administrator privileges required!
    echo Right-click and select "Run as Administrator"
    pause
    exit /b 1
)

echo [1] Stopping driver...
sc stop EZ2FastIO >nul 2>&1
timeout /t 2 /nobreak >nul

echo [2] Removing driver service...
sc delete EZ2FastIO >nul 2>&1

echo [3] Deleting driver file...
del /F /Q %WINDIR%\system32\drivers\ez2fastio.sys >nul 2>&1

echo.
echo [SUCCESS] Driver uninstalled!
echo.

pause