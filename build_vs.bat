@echo off

where premake5 >nul 2>&1

if %ERRORLEVEL%==0 (
	premake5 vs2026
) else (
	echo Error: premake5 not found. You need to install premake5 to your PATH.
)

pause
