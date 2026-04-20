@echo off
REM ==========================================
REM Network Test Script — CLOUDENGINE
REM ==========================================
REM 
REM Как использовать:
REM 1. Запустите этот скрипт
REM 2. У вас откроются 2 окна:
REM    - Окно 1: HOST (сервер) — смотрите логи подключения
REM    - Окно 2: CLIENT (клиент) — подключится к localhost:12345
REM
REM ==========================================

echo ==========================================
echo CLOUDENGINE Network Test
echo ==========================================
echo.
echo Terminal 1: Starting HOST (server)...
echo Press any key to continue...
pause >nul

start "CLOUDENGINE - HOST" cmd /k "cd /d c:\CLOUDPROJECT\CLOUDENGINE\build\Debug && CloudEngine.exe --host"

echo.
echo Waiting 2 seconds...
timeout /t 2 /nobreak >nul

echo.
echo Terminal 2: Starting CLIENT...
echo Press any key to continue...
pause >nul

start "CLOUDENGINE - CLIENT" cmd /k "cd /d c:\CLOUDPROJECT\CLOUDENGINE\build\Debug && CloudEngine.exe --client localhost"

echo.
echo ==========================================
echo Both instances started!
echo.
echo Watch the logs:
echo - HOST: should show "Player 2 connected"
echo - CLIENT: should show "Assigned player ID=2"
echo ==========================================
echo.
