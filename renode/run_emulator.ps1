# PowerShell script to run STM32F407 emulator with Renode
# Usage: .\run_emulator.ps1

param(
    [string]$BinaryPath = "Debug\Valeh_injec_pogram.elf",
    [switch]$Help
)

if ($Help) {
    Write-Host "STM32F407 Emulator Runner" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Usage:" -ForegroundColor Yellow
    Write-Host "  .\run_emulator.ps1 [-BinaryPath <path>]"
    Write-Host ""
    Write-Host "Options:"
    Write-Host "  -BinaryPath    Path to .elf file (default: Debug\Valeh_injec_pogram.elf)"
    Write-Host "  -Help          Show this help message"
    Write-Host ""
    Write-Host "Examples:"
    Write-Host "  .\run_emulator.ps1"
    Write-Host "  .\run_emulator.ps1 -BinaryPath Debug\Valeh_injec_pogram.elf"
    exit 0
}

# Check if Renode is installed
$renodePath = Get-Command renode -ErrorAction SilentlyContinue

# Try default installation path if not in PATH
if (-not $renodePath) {
    $defaultPath = "C:\Program Files\Renode\renode.exe"
    if (Test-Path $defaultPath) {
        Write-Host "Renode found at default location, using: $defaultPath" -ForegroundColor Yellow
        $renodePath = $defaultPath
    } else {
        Write-Host "ERROR: Renode is not installed or not in PATH" -ForegroundColor Red
        Write-Host ""
        Write-Host "========================================" -ForegroundColor Cyan
        Write-Host "QUICK FIX OPTIONS:" -ForegroundColor Cyan
        Write-Host "========================================" -ForegroundColor Cyan
        Write-Host ""
        Write-Host "Option 1: Install Renode (Recommended)" -ForegroundColor Yellow
        Write-Host "  1. Download from: https://renode.io/" -ForegroundColor White
        Write-Host "  2. Install to: C:\Program Files\Renode" -ForegroundColor White
        Write-Host "  3. Add to PATH: C:\Program Files\Renode" -ForegroundColor White
        Write-Host "  4. Restart terminal/computer" -ForegroundColor White
        Write-Host ""
        Write-Host "Option 2: Use portable version" -ForegroundColor Yellow
        Write-Host "  Download portable ZIP and extract anywhere" -ForegroundColor White
        Write-Host "  Edit this script to use full path" -ForegroundColor White
        Write-Host ""
        Write-Host "Option 3: See detailed instructions" -ForegroundColor Yellow
        Write-Host "  Read: renode\INSTALL_RENODE.md" -ForegroundColor White
        Write-Host ""
        Write-Host "========================================" -ForegroundColor Cyan
        Write-Host ""
        Write-Host "For detailed installation guide, see: renode\INSTALL_RENODE.md" -ForegroundColor Cyan
        exit 1
    }
}

# Check if binary exists
# Get parent directory (project root)
$parentDir = Split-Path -Parent $PSScriptRoot
# Build full path to binary
$fullBinaryPath = Join-Path $parentDir $BinaryPath
# Resolve to absolute path
$fullBinaryPath = Resolve-Path $fullBinaryPath -ErrorAction SilentlyContinue

if (-not $fullBinaryPath) {
    Write-Host "ERROR: Binary file not found: $BinaryPath" -ForegroundColor Red
    Write-Host "Searched at: $(Join-Path $parentDir $BinaryPath)" -ForegroundColor Gray
    Write-Host ""
    Write-Host "Please build the project first:" -ForegroundColor Yellow
    Write-Host "  cd Debug" -ForegroundColor Cyan
    Write-Host "  make" -ForegroundColor Cyan
    exit 1
}

# Convert to string if it's a PathInfo object
if ($fullBinaryPath -is [System.Management.Automation.PathInfo]) {
    $fullBinaryPath = $fullBinaryPath.Path
}

Write-Host "Starting STM32F407 Emulator..." -ForegroundColor Green
Write-Host "Binary: $fullBinaryPath" -ForegroundColor Cyan
Write-Host ""
Write-Host "IMPORTANT: LCD and Touch are NOT fully simulated in Renode" -ForegroundColor Yellow
Write-Host "For visual interface, upload to real hardware" -ForegroundColor Yellow
Write-Host "UART debug output will be available in Renode console" -ForegroundColor Cyan
Write-Host ""

# Get the script directory
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$rescFile = Join-Path $scriptDir "stm32f407_emulator.resc"

# Create temporary script with binary path
$tempScript = Join-Path $env:TEMP "renode_temp_$(Get-Date -Format 'yyyyMMddHHmmss').resc"
$scriptContent = Get-Content $rescFile -Raw
$scriptContent = $scriptContent -replace '@binaries/Valeh_injec_pogram.elf', $fullBinaryPath
$scriptContent | Out-File -FilePath $tempScript -Encoding UTF8

Write-Host "Running Renode with script: $rescFile" -ForegroundColor Cyan
Write-Host "Press Ctrl+C to stop the emulator" -ForegroundColor Yellow
Write-Host ""

# Run Renode
try {
    # Escape path for Renode script (use forward slashes or escape backslashes)
    $escapedPath = $fullBinaryPath -replace '\\', '/'
    
    Write-Host "Starting Renode with GUI..." -ForegroundColor Cyan
    Write-Host "Note: GUI window will open showing the emulator" -ForegroundColor Yellow
    Write-Host ""
    
    if ($renodePath -is [string] -and $renodePath -ne "renode") {
        # Use full path - WITH GUI (remove --disable-xwt)
        & $renodePath --execute "`$bin='$escapedPath'; include @$rescFile"
    } else {
        # Use command from PATH - WITH GUI (remove --disable-xwt)
        renode --execute "`$bin='$escapedPath'; include @$rescFile"
    }
} catch {
    Write-Host "ERROR: Failed to start Renode" -ForegroundColor Red
    Write-Host $_.Exception.Message -ForegroundColor Red
    exit 1
} finally {
    # Cleanup temp file
    if (Test-Path $tempScript) {
        Remove-Item $tempScript -Force
    }
}

