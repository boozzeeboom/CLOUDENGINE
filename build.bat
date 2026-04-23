@echo off
REM =============================================================================
REM CLOUDENGINE Build Script
REM Uses Visual Studio 2022 (18) with MSBuild
REM =============================================================================

REM Set build environment via VS Developer Command Prompt
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -host_arch=amd64 -arch=amd64

REM Configuration
set GENERATOR=Visual Studio 18 2026
set BUILD_DIR=%CD%\build

REM Clean old build (uncomment if needed)
REM if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"

REM Configure with CMake
echo Configuring CLOUDENGINE with %GENERATOR%...
cmake -S . -B build -G "%GENERATOR%" -A x64 -DCMAKE_BUILD_TYPE=Debug

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake configuration failed!
    exit /b 1
)

REM Build
echo Building CLOUDENGINE...
cmake --build build --config Debug

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed!
    exit /b 1
)

echo.
echo [SUCCESS] Build complete!
echo Executable: %BUILD_DIR%\Debug\CloudEngine.exe
echo.