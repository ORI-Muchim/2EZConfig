@echo off
REM ========================================
REM  Port Test Utility Build Script
REM  Visual Studio 2015 (v140_xp)
REM ========================================

echo.
echo ===== Building port_test.exe =====
echo.

REM Try VS2015 MSBuild first
where msbuild >nul 2>&1
if %errorlevel% equ 0 (
    echo Using MSBuild (VS2015)...
    msbuild port_test.vcxproj /p:Configuration=Release /p:Platform=Win32 /p:PlatformToolset=v140_xp
    if exist Release\port_test.exe (
        echo [SUCCESS] Built with MSBuild!
        copy Release\port_test.exe .
        goto :end
    )
)

REM Try VS2015 compiler directly
if exist "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" (
    echo Found Visual Studio 2015!
    call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86
    goto :build_vs2015
)

if exist "C:\Program Files\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" (
    echo Found Visual Studio 2015!
    call "C:\Program Files\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86
    goto :build_vs2015
)

echo [ERROR] Visual Studio 2015 not found!
echo.
echo Please install Visual Studio 2015 with:
echo   - Visual C++ 2015
echo   - Windows XP support for C++ (v140_xp)
echo.
pause
exit /b 1

:build_vs2015
echo Building with VS2015 (v140_xp toolset)...
cl.exe /D_USING_V140_SDK71_ /D_WIN32_WINNT=0x0501 /MT /O2 /EHa /DWIN32 /D_CRT_SECURE_NO_WARNINGS /Fe:port_test.exe port_test.cpp kernel32.lib

if exist port_test.exe (
    echo [SUCCESS] Build complete with VS2015!
) else (
    echo [ERROR] Build failed!
    echo.
    echo Trying with project file...
    msbuild port_test.vcxproj /p:Configuration=Release /p:Platform=Win32
    if exist Release\port_test.exe (
        copy Release\port_test.exe .
        echo [SUCCESS] Built with project file!
    ) else (
        echo [ERROR] Project build also failed!
    )
)

:end
echo.
pause