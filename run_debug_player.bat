@echo off
cd /d "%~dp0build\Debug"
CloudEngine.exe > stdout_run.txt 2> stderr_run.txt