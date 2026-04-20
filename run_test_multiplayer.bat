@echo off
REM Multiplayer Test Script for CLOUDENGINE
REM =====================

echo ======================================
echo CLOUDENGINE Multiplayer Test
echo ======================================
echo.
echo This script will test:
echo   1. Host mode (server on port 12345)
echo   2. Client mode (connects to localhost)
echo.
echo Press Ctrl+C to stop all instances
echo ======================================
echo.

cd /d "%~dp0"

echo [TEST] Starting Host...
start "CloudEngine_Host" build\Release\CloudEngine.exe --host

echo [TEST] Waiting 3 seconds for host to initialize...
timeout /t 3 /nobreak >nul

echo [TEST] Starting Client...
start "CloudEngine_Client" build\Release\CloudEngine.exe --client

echo.
echo ======================================
echo Both instances are running!
echo Check the windows for:
echo   - "Server started on port 12345"
echo   - "Connected to server"
echo   - "Created RemotePlayer entity"
echo.
echo Press any key to stop all instances...
pause >nul

taskkill /IM CloudEngine.exe /F >nul 2>&1

echo.
echo [TEST] Cleanup complete
