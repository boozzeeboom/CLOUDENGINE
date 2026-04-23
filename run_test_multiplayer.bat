@echo off
REM Multiplayer test script for CLOUDENGINE
REM Usage: run_test_multiplayer.bat [host|client] [port]

set "EXE=build\Debug\CloudEngine.exe"
set "PORT=12345"

if "%1"=="" goto singleplayer
if "%1"=="host" goto host
if "%1"=="client" goto client

:singleplayer
echo Starting singleplayer mode...
start "" "%EXE%"
goto :eof

:host
echo Starting as host on port %PORT%...
start "" "%EXE%" --host %PORT%
goto :eof

:client
set "HOST=%2"
if "%HOST%"=="" set "HOST=localhost"
echo Starting as client, connecting to %HOST%:%PORT%...
start "" "%EXE%" --client %HOST% %PORT%
goto :eof