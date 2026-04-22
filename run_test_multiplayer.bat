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
echo Logs will be saved to:
echo   - test_multiplayer_host.txt (Host output)
echo   - test_multiplayer_client.txt (Client output)
echo.
echo Press Ctrl+C to stop all instances
echo ======================================
echo.

cd /d "%~dp0"

REM Delete old logs
del /f /q test_multiplayer_host.txt 2>nul
del /f /q test_multiplayer_client.txt 2>nul

echo [TEST] Starting Host (log: test_multiplayer_host.txt)...
start "CloudEngine_Host" cmd /c "build_test\Debug\CloudEngine.exe --host ^> test_multiplayer_host.txt 2^>^&1"

echo [TEST] Waiting 3 seconds for host to initialize...
timeout /t 3 /nobreak >nul

echo [TEST] Starting Client (log: test_multiplayer_client.txt)...
start "CloudEngine_Client" cmd /c "build_test\Debug\CloudEngine.exe --client ^> test_multiplayer_client.txt 2^>^&1"

echo.
echo ======================================
echo Both instances are running!
echo Logs are being written to:
echo   - test_multiplayer_host.txt
echo   - test_multiplayer_client.txt
echo.
echo To view logs in real-time, open new terminals and run:
echo   type test_multiplayer_host.txt
echo   type test_multiplayer_client.txt
echo.
echo Press any key to stop all instances...
pause >nul

taskkill /IM CloudEngine.exe /F >nul 2>&1

echo.
echo [TEST] Logs saved to test_multiplayer_host.txt and test_multiplayer_client.txt
echo.
