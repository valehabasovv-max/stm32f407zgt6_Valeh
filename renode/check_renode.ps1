# Quick script to check if Renode is installed and accessible
# Usage: .\check_renode.ps1

Write-Host "Checking Renode installation..." -ForegroundColor Cyan
Write-Host ""

# Check PATH
$renodeInPath = Get-Command renode -ErrorAction SilentlyContinue
if ($renodeInPath) {
    Write-Host "✓ Renode found in PATH" -ForegroundColor Green
    Write-Host "  Location: $($renodeInPath.Source)" -ForegroundColor Gray
    Write-Host "  Version:" -NoNewline
    try {
        $version = renode --version 2>&1
        Write-Host " $version" -ForegroundColor Green
    } catch {
        Write-Host " (could not get version)" -ForegroundColor Yellow
    }
    exit 0
}

# Check default installation path
$defaultPath = "C:\Program Files\Renode\renode.exe"
if (Test-Path $defaultPath) {
    Write-Host "✓ Renode found at default location" -ForegroundColor Green
    Write-Host "  Location: $defaultPath" -ForegroundColor Gray
    Write-Host "  Note: Not in PATH, but can be used with full path" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "To add to PATH, run (as Administrator):" -ForegroundColor Cyan
    Write-Host '  [Environment]::SetEnvironmentVariable("Path", $env:Path + ";C:\Program Files\Renode", "Machine")' -ForegroundColor White
    exit 0
}

# Not found
Write-Host "✗ Renode not found" -ForegroundColor Red
Write-Host ""
Write-Host "Installation options:" -ForegroundColor Yellow
Write-Host "  1. Download from: https://renode.io/" -ForegroundColor White
Write-Host "  2. Or: https://github.com/renode/renode/releases" -ForegroundColor White
Write-Host ""
Write-Host "For detailed instructions, see: renode\INSTALL_RENODE.md" -ForegroundColor Cyan
exit 1




