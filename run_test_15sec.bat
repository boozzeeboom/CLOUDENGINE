@echo off
cd /d c:\CLOUDPROJECT\CLOUDENGINE
start /b build_test\Debug\CloudEngine.exe
timeout /t 15 /nobreak >nul
taskkill /f /im CloudEngine.exe >nul 2>&1
echo === Test completed ===
