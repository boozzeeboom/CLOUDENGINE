@echo off
REM CLOUDENGINE Ship Physics Test Script
REM Test results will be saved for analysis
REM Дата: 2026-04-23

echo ================================================
echo CLOUDENGINE Ship Physics Test
echo ================================================
echo.

REM Check if logs directory exists
if not exist "logs" mkdir logs

REM Backup previous logs if they exist
if exist "logs\cloudengine.log" (
    echo Backing up previous logs...
    powershell -Command "Get-Date -Format 'yyyy-MM-dd_HH-mm-ss' | Out-File -FilePath 'logs\test_timestamp.txt' -Encoding ASCII"
    set /p TIMESTAMP=<logs\test_timestamp.txt
    copy "logs\cloudengine.log" "logs\cloudengine_%TIMESTAMP%.log" >nul 2>&1
    copy "logs\spdlog-debug.log" "logs\spdlog-debug_%TIMESTAMP%.log" >nul 2>&1
    copy "logs\spdlog-errors.log" "logs\spdlog-errors_%TIMESTAMP%.log" >nul 2>&1
)

REM Clear current logs for clean test
echo Clearing previous logs...
if exist "logs\cloudengine.log" del "logs\cloudengine.log" >nul 2>&1
if exist "logs\spdlog-debug.log" del "logs\spdlog-debug.log" >nul 2>&1
if exist "logs\spdlog-errors.log" del "logs\spdlog-errors.log" >nul 2>&1

echo.
echo ================================================
echo TEST INSTRUCTIONS
echo ================================================
echo 1. CloudEngine.exe will launch
echo 2. Press RMB to capture cursor
echo 3. Test ship controls:
echo    - W = forward thrust
echo    - A/D = turn left/right
echo    - Q/E = up/down
echo    - Shift = boost
echo 4. Check if ship falls (gravity) when idle
echo 5. Press ESC to exit
echo ================================================
echo.
echo Press any key to launch CloudEngine.exe...
pause >nul

REM Launch the engine
cd build_test\Debug
start "CLOUDENGINE" CloudEngine.exe

echo.
echo ================================================
echo TEST IN PROGRESS
echo ================================================
echo After closing CloudEngine:
echo - Logs will be saved to logs\ folder
echo - Check for:
echo   * "CURSOR CAPTURED" = input captured
echo   * "ShipController" = system running
echo   * "applying" logs = forces being applied
echo ================================================
echo.
echo Close CloudEngine when done testing
pause
