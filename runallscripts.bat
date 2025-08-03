@echo off
setlocal

set "XOSCRIPT=crossbasic.exe"
set "SCRIPT_DIR=Scripts"

for %%f in ("%SCRIPT_DIR%\*.xs") do (
  powershell -NoProfile -Command ^
    "Write-Host 'Executing %%~nxf' -ForegroundColor Green"
  "%XOSCRIPT%" --s "%%f"
)

pause
