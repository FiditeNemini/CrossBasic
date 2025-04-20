@echo off

set "XOSCRIPT=crossbasic.exe"
set "SCRIPT_DIR=Scripts"

for %%f in ("%SCRIPT_DIR%\*.xs") do (
  echo Executing %%f
  "%XOSCRIPT%" --s "%%f"
)

pause
