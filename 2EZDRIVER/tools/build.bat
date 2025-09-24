@echo off
echo Building port_test.exe...

REM Try MinGW first (simpler)
if exist "C:\MinGW\bin\g++.exe" (
    C:\MinGW\bin\g++.exe -o port_test.exe port_test.cpp -static
    if exist port_test.exe echo Build complete!
    goto :end
)

REM Try Visual Studio
if exist "C:\Program Files\Microsoft Visual Studio 8\VC\bin\cl.exe" (
    call "C:\Program Files\Microsoft Visual Studio 8\VC\vcvarsall.bat" x86
    cl.exe /MT /O2 /Fe:port_test.exe port_test.cpp kernel32.lib
    if exist port_test.exe echo Build complete!
    goto :end
)

echo No compiler found! Install MinGW or Visual Studio.

:end
pause