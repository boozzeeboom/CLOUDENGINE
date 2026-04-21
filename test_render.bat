@echo off
cd /d "%~dp0"
echo ==========================================
echo CLOUDENGINE Render Fix Test
echo ==========================================
echo.
echo This script will test the render fix.
echo.

REM Check if exe exists
if not exist "build\Debug\CloudEngine.exe" (
    echo ERROR: build\Debug\CloudEngine.exe not found!
    echo Please run: cmake --build build --config Debug
    pause
    exit /b 1
)

echo Starting HOST (server + local player)...
start "HOST" "build\Debug\CloudEngine.exe" --host
echo Waiting 3 seconds...
timeout /t 3 /nobreak >nul

echo Starting CLIENT (connecting to host)...
start "CLIENT" "build\Debug\CloudEngine.exe" --client localhost
echo.
echo ==========================================
echo TEST RESULTS
echo ==========================================
echo.
echo In both windows, check:
echo   1. Blue sky gradient is visible (not black screen)
echo   2. White clouds are rendered
echo   3. Player spheres are visible:
echo      - HOST: Green sphere (player id=1)
echo      - CLIENT: Red sphere (player id=2)
echo.
echo If sky is black - the fix did NOT work
echo If clouds appear - the fix WORKS
echo.
pause
