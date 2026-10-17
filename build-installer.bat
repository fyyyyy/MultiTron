@echo off
setlocal

cd /d "%~dp0"

echo Building MultiTron2 Release...
cmake --build build --config Release --target MultiTron2
if errorlevel 1 exit /b %errorlevel%

set "ISCC=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if not exist "%ISCC%" set "ISCC=%ProgramFiles%\Inno Setup 6\ISCC.exe"
if not exist "%ISCC%" (
    echo Inno Setup 6 was not found. Install it, then run this script again.
    exit /b 1
)

echo Creating installer...
"%ISCC%" installer\MultiTron2.iss
if errorlevel 1 exit /b %errorlevel%

echo.
echo Installer created in dist\.
