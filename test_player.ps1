$ErrorActionPreference = "Continue"
Set-Location "c:\CLOUDPROJECT\CLOUDENGINE\build\Debug"
$proc = Start-Process -FilePath ".\CloudEngine.exe" -PassThru -NoNewWindow
Start-Sleep -Seconds 5
if (!$proc.HasExited) { Stop-Process -Id $proc.Id -Force }
Get-Content "stdout_test2.txt"