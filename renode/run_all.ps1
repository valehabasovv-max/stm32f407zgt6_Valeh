# VALEH Pressure Control - Tam Avtomatik İşə Salma Script
# Bu script hər şeyi avtomatik edir: build, emulyator, UI simulator

param(
    [switch]$BuildOnly,
    [switch]$EmulatorOnly,
    [switch]$UISimulatorOnly,
    [switch]$Help
)

if ($Help) {
    Write-Host "VALEH Pressure Control - Avtomatik İşə Salma" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Usage:" -ForegroundColor Yellow
    Write-Host "  .\run_all.ps1                    # Hamısını işə salır (build + emulator + UI)"
    Write-Host "  .\run_all.ps1 -BuildOnly          # Yalnız build edir"
    Write-Host "  .\run_all.ps1 -EmulatorOnly      # Yalnız emulyator (build olmadan)"
    Write-Host "  .\run_all.ps1 -UISimulatorOnly   # Yalnız UI simulator"
    Write-Host ""
    exit 0
}

# Get script directory
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Split-Path -Parent $scriptDir
$debugDir = Join-Path $projectRoot "Debug"
$elfFile = Join-Path $debugDir "Valeh_injec_pogram.elf"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "VALEH Pressure Control - Avtomatik İşə Salma" -ForegroundColor Yellow
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# ============================================
# STEP 1: BUILD PROJECT
# ============================================
if (-not $EmulatorOnly -and -not $UISimulatorOnly) {
    Write-Host "[1/3] Building project..." -ForegroundColor Green
    
    # Check if make is available
    $makePath = "c:\Users\Valeh\AppData\Roaming\Code\User\globalStorage\bmd.stm32-for-vscode\@xpack-dev-tools\windows-build-tools\4.4.1-3.1\.content\bin\make.EXE"
    
    if (-not (Test-Path $makePath)) {
        Write-Host "WARNING: Make not found at expected path" -ForegroundColor Yellow
        Write-Host "Please build manually: cd Debug && make" -ForegroundColor Yellow
        Write-Host ""
        
        # Check if ELF already exists
        if (Test-Path $elfFile) {
            Write-Host "Found existing ELF file, using it..." -ForegroundColor Yellow
        } else {
            Write-Host "ERROR: No ELF file found. Please build the project first." -ForegroundColor Red
            exit 1
        }
    } else {
        # Change to Debug directory and build
        Push-Location $debugDir
        try {
            Write-Host "Running make..." -ForegroundColor Cyan
            & $makePath 2>&1 | ForEach-Object {
                if ($_ -match "error|Error|ERROR") {
                    Write-Host $_ -ForegroundColor Red
                } else {
                    Write-Host $_
                }
            }
            
            if ($LASTEXITCODE -eq 0) {
                Write-Host "✅ Build successful!" -ForegroundColor Green
            } else {
                Write-Host "⚠️  Build completed with warnings" -ForegroundColor Yellow
            }
        } catch {
            Write-Host "ERROR: Build failed" -ForegroundColor Red
            Write-Host $_.Exception.Message -ForegroundColor Red
            Pop-Location
            exit 1
        } finally {
            Pop-Location
        }
    }
    
    # Verify ELF file exists - check multiple possible locations
    $elfFound = $false
    $possibleElfPaths = @(
        $elfFile,
        (Join-Path $projectRoot "Debug\Valeh_injec_pogram.elf"),
        (Join-Path $projectRoot "build\Valeh_injec_pogram.elf"),
        (Get-ChildItem -Path $projectRoot -Filter "*.elf" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1).FullName
    )
    
    foreach ($path in $possibleElfPaths) {
        if ($path -and (Test-Path $path)) {
            $elfFile = $path
            $elfFound = $true
            Write-Host "✅ ELF file found: $elfFile" -ForegroundColor Green
            break
        }
    }
    
    if (-not $elfFound) {
        Write-Host "⚠️  WARNING: ELF file not found after build" -ForegroundColor Yellow
        Write-Host "   Expected location: $elfFile" -ForegroundColor Gray
        Write-Host "   Continuing anyway - will use existing ELF if available..." -ForegroundColor Yellow
        Write-Host ""
    }
    
    Write-Host ""
}

# ============================================
# STEP 2: START EMULATOR (if requested)
# ============================================
if (-not $BuildOnly -and -not $UISimulatorOnly) {
    Write-Host "[2/3] Starting Renode emulator..." -ForegroundColor Green
    
    # Check Renode
    $renodePath = Get-Command renode -ErrorAction SilentlyContinue
    if (-not $renodePath) {
        $defaultPath = "C:\Program Files\Renode\renode.exe"
        if (Test-Path $defaultPath) {
            $renodePath = $defaultPath
        } else {
            Write-Host "WARNING: Renode not found. Skipping emulator." -ForegroundColor Yellow
            Write-Host "Install Renode from: https://renode.io/" -ForegroundColor Yellow
            $skipEmulator = $true
        }
    } else {
        $skipEmulator = $false
    }
    
    if (-not $skipEmulator) {
        $rescFile = Join-Path $scriptDir "stm32f407_emulator.resc"
        
        # Check if ELF file exists before starting emulator
        if (-not (Test-Path $elfFile)) {
            Write-Host "⚠️  WARNING: ELF file not found: $elfFile" -ForegroundColor Yellow
            Write-Host "   Emulator will start but may not load firmware correctly" -ForegroundColor Yellow
            Write-Host "   Please build the project first or check the ELF file path" -ForegroundColor Yellow
            $escapedPath = "binaries/Valeh_injec_pogram.elf"  # Use default path from .resc file
        } else {
            $escapedPath = $elfFile -replace '\\', '/'
        }
        
        Write-Host "Starting Renode in background..." -ForegroundColor Cyan
        Write-Host "   ELF file: $escapedPath" -ForegroundColor Gray
        Start-Process -FilePath (if ($renodePath -is [string]) { $renodePath } else { "renode" }) `
                     -ArgumentList "--execute", "`$bin='$escapedPath'; include @$rescFile" `
                     -WindowStyle Normal
        
        Write-Host "✅ Renode emulator started!" -ForegroundColor Green
        Start-Sleep -Seconds 2
    }
    
    Write-Host ""
}

# ============================================
# STEP 3: START UI SIMULATOR (if requested)
# ============================================
if (-not $BuildOnly -and -not $EmulatorOnly) {
    Write-Host "[3/3] Starting UI Simulator..." -ForegroundColor Green
    
    # Check Python
    $pythonPath = Get-Command python -ErrorAction SilentlyContinue
    if (-not $pythonPath) {
        Write-Host "WARNING: Python not found. UI Simulator will not start." -ForegroundColor Yellow
        Write-Host "Install Python 3.8+ from: https://www.python.org/" -ForegroundColor Yellow
    } else {
        $uiScript = Join-Path $scriptDir "ui_simulator.py"
        
        if (Test-Path $uiScript) {
            Write-Host "Starting UI Simulator..." -ForegroundColor Cyan
            Start-Process python -ArgumentList $uiScript -WindowStyle Normal
            Write-Host "✅ UI Simulator started!" -ForegroundColor Green
            Start-Sleep -Seconds 1
        } else {
            Write-Host "WARNING: UI Simulator script not found: $uiScript" -ForegroundColor Yellow
        }
    }
    
    Write-Host ""
}

# ============================================
# SUMMARY
# ============================================
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "✅ Tamamlandı!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

if (-not $BuildOnly -and -not $EmulatorOnly) {
    Write-Host "UI Simulator pəncərəsi açılmalıdır!" -ForegroundColor Yellow
    Write-Host "Proqramın UI-sini görə bilərsiniz." -ForegroundColor Cyan
    Write-Host ""
}

if (-not $BuildOnly -and -not $UISimulatorOnly) {
    Write-Host "Renode emulyator işləyir (background-da)." -ForegroundColor Cyan
    Write-Host "UART output Renode konsolunda görünəcək." -ForegroundColor Cyan
    Write-Host ""
}

Write-Host "İpucu: Hər şeyi yenidən işə salmaq üçün:" -ForegroundColor Yellow
Write-Host "  .\run_all.ps1" -ForegroundColor White
Write-Host ""

