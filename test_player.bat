@echo off
cd /d "%~dp0build\Debug"
CloudEngine.exe > stdout_test2.txt 2> stderr_test2.txt
timeout /t 3 /nobreak >nul
type stdout_test2.txt