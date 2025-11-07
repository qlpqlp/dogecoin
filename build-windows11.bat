@echo off
REM Build script for Dogecoin Core on Windows 11
REM This script launches WSL and runs the build process

echo ==========================================
echo Building Dogecoin Core for Windows 11
echo ==========================================
echo.
echo This will:
echo   1. Launch WSL (Windows Subsystem for Linux)
echo   2. Build all dependencies (30-60 minutes)
echo   3. Configure the build system
echo   4. Build Dogecoin Core (30-60 minutes)
echo.
echo Total build time: 1-2 hours
echo.
echo Make sure WSL2 and Ubuntu are installed!
echo.
pause

echo.
echo Starting build in WSL...
echo.

REM Get the current directory
set "CURRENT_DIR=%~dp0"
set "CURRENT_DIR=%CURRENT_DIR:~0,-1%"

REM Convert Windows path to WSL path format
REM C:\Users\pvida\Documents\GitHub\dogecoin -> /mnt/c/Users/pvida/Documents/GitHub/dogecoin
set "DRIVE_LETTER=%CURRENT_DIR:~0,1%"
set "WSL_PATH=/mnt/%DRIVE_LETTER:~0,1%/%CURRENT_DIR:~3%"
set "WSL_PATH=%WSL_PATH:\=/%"

echo Windows Path: %CURRENT_DIR%
echo WSL Path: %WSL_PATH%
echo.

REM Run the build script in WSL
wsl bash -c "cd '%WSL_PATH%' && bash build-windows11.sh"

echo.
echo ==========================================
echo Build Process Complete!
echo ==========================================
echo.
echo Check the output above for the build status.
echo.
echo If successful, the executables will be at:
echo   src\qt\dogecoin-qt.exe
echo   src\dogecoind.exe
echo   src\dogecoin-cli.exe
echo   src\dogecoin-tx.exe
echo.
pause

