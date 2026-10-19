@echo off
taskkill /F /IM MultiTron2.exe >nul 2>nul
call cmake -S . -B build
call npm run build:ui
if errorlevel 1 exit /b %errorlevel%
cmake --build build --config Release
if errorlevel 1 exit /b %errorlevel%
set "MOUSE_VJOY_ROUTER_UI_PATH=%~dp0src\UI"
.\build\Release\MultiTron2.exe
