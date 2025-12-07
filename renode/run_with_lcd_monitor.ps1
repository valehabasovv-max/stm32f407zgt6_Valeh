# PowerShell script to run STM32F407 emulator with Virtual LCD Monitor
# Usage: .\run_with_lcd_monitor.ps1

param(
    [string]$BinaryPath = "Debug\Valeh_injec_pogram.elf",
    [switch]$Help
)

if ($Help) {
    Write-Host "STM32F407 Emulator with Virtual LCD Monitor" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Usage:" -ForegroundColor Yellow
    Write-Host "  .\run_with_lcd_monitor.ps1 [-BinaryPath <path>]"
    Write-Host ""
    Write-Host "This will:"
    Write-Host "  1. Start Renode emulator"
    Write-Host "  2. Start Virtual LCD Monitor GUI"
    Write-Host ""
    exit 0
}

# Check if Renode is installed
$renodePath = Get-Command renode -ErrorAction SilentlyContinue
if (-not $renodePath) {
    $defaultPath = "C:\Program Files\Renode\renode.exe"
    if (Test-Path $defaultPath) {
        $renodePath = $defaultPath
    } else {
        Write-Host "ERROR: Renode is not installed" -ForegroundColor Red
        exit 1
    }
}

# Check if binary exists
$parentDir = Split-Path -Parent $PSScriptRoot
$fullBinaryPath = Join-Path $parentDir $BinaryPath
$fullBinaryPath = Resolve-Path $fullBinaryPath -ErrorAction SilentlyContinue

if (-not $fullBinaryPath) {
    Write-Host "ERROR: Binary file not found: $BinaryPath" -ForegroundColor Red
    exit 1
}

if ($fullBinaryPath -is [System.Management.Automation.PathInfo]) {
    $fullBinaryPath = $fullBinaryPath.Path
}

# Check Python
$pythonPath = Get-Command python -ErrorAction SilentlyContinue
if (-not $pythonPath) {
    Write-Host "WARNING: Python not found. LCD Monitor GUI will not work." -ForegroundColor Yellow
    Write-Host "Install Python 3.8+ for LCD visualization" -ForegroundColor Yellow
    $runLCD = $false
} else {
    $runLCD = $true
}

Write-Host "Starting STM32F407 Emulator with Virtual LCD Monitor..." -ForegroundColor Green
Write-Host "Binary: $fullBinaryPath" -ForegroundColor Cyan
Write-Host ""

# Get script directory
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rescFile = Join-Path $scriptDir "virtual_lcd_monitor.resc"
$lcdScript = Join-Path $scriptDir "lcd_monitor_gui.py"

# Start LCD Monitor GUI in background
if ($runLCD) {
    Write-Host "Starting Virtual LCD Monitor GUI..." -ForegroundColor Cyan
    Start-Process python -ArgumentList $lcdScript -WindowStyle Normal
    Start-Sleep -Seconds 2
}

# Escape path for Renode
$escapedPath = $fullBinaryPath -replace '\\', '/'

Write-Host "Starting Renode emulator..." -ForegroundColor Cyan
Write-Host "Press Ctrl+C to stop" -ForegroundColor Yellow
Write-Host ""

# Run Renode
try {
    if ($renodePath -is [string] -and $renodePath -ne "renode") {
        & $renodePath --execute "`$bin='$escapedPath'; include @$rescFile"
    } else {
        renode --execute "`$bin='$escapedPath'; include @$rescFile"
    }
} catch {
    Write-Host "ERROR: Failed to start Renode" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    exit 1
}

