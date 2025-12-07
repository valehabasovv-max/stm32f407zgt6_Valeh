@echo off
REM Batch script to run STM32F407 emulator with Renode
REM Usage: run_emulator.bat [binary_path]

setlocal

set BINARY_PATH=%~1
if "%BINARY_PATH%"=="" set BINARY_PATH=Debug\Valeh_injec_pogram.elf

REM Check if Renode is in PATH
where renode >nul 2>&1
if errorlevel 1 (
    echo ERROR: Renode is not installed or not in PATH
    echo.
    echo ========================================
    echo QUICK FIX OPTIONS:
    echo ========================================
    echo.
    echo Option 1: Install Renode (Recommended)
    echo   1. Download from: https://renode.io/
    echo   2. Install to: C:\Program Files\Renode
    echo   3. Add to PATH: C:\Program Files\Renode
    echo   4. Restart terminal/computer
    echo.
    echo Option 2: Use full path (if already installed)
    echo   Edit this script and replace 'renode' with:
    echo   "C:\Program Files\Renode\renode.exe"
    echo.
    echo Option 3: See detailed instructions
    echo   Read: renode\INSTALL_RENODE.md
    echo.
    echo ========================================
    echo.
    echo For detailed installation guide, see: renode\INSTALL_RENODE.md
    exit /b 1
)

REM Check if binary exists
if not exist "%BINARY_PATH%" (
    echo ERROR: Binary file not found: %BINARY_PATH%
    echo.
    echo Please build the project first:
    echo   cd Debug
    echo   make
    exit /b 1
)

echo Starting STM32F407 Emulator...
echo Binary: %BINARY_PATH%
echo.

REM Get script directory
set SCRIPT_DIR=%~dp0
set RESC_FILE=%SCRIPT_DIR%stm32f407_emulator.resc

REM Run Renode
renode --console --disable-xwt --execute "$bin='%CD%\%BINARY_PATH%'; include @%RESC_FILE%"

endlocal

