@echo off
echo ==========================================
echo Testing CLOUDENGINE Render Fix
echo ==========================================
echo.
echo Opening HOST window (press ESC to exit)...
start "HOST" /min "build\Debug\CloudEngine.exe" --host
timeout /t 2 /nobreak >nul
echo.
echo Opening CLIENT window...
start "CLIENT" /min "build\Debug\CloudEngine.exe" --client localhost
echo.
echo Both windows should now be running.
echo - HOST: displays player sphere and clouds
echo - CLIENT: displays player sphere and clouds
echo.
echo Check for:
echo - Blue sky gradient (not black)
echo - White clouds with Ghibli style
echo - Green player sphere (HOST) and red player sphere (CLIENT)
echo.
pause
