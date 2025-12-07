# 🚀 Emulyatoru Tez İşə Salmaq

## Addım 1: Proyekti Build Et (Əgər lazımdırsa)

ELF faylı artıq mövcuddur: `Debug\Valeh_injec_pogram.elf`

Əgər yenidən build etmək istəyirsinizsə:
```powershell
cd Debug
& "c:\Users\Valeh\AppData\Roaming\Code\User\globalStorage\bmd.stm32-for-vscode\@xpack-dev-tools\windows-build-tools\4.4.1-3.1\.content\bin\make.EXE"
cd ..
```

## Addım 2: Emulyatoru İşə Sal

```powershell
cd renode
.\run_emulator.ps1
```

## Nə Gözləmək Lazımdır

1. **Renode konsolu açılacaq** - qara pəncərə
2. **Platform yüklənəcək** - STM32F4 Discovery
3. **Firmware yüklənəcək** - Valeh_injec_pogram.elf
4. **UART output görünəcək** - printf() mesajları

## Əgər Problem Varsa

### Problem: Platform tapılmır
**Həll:** Script artıq düzəldilib, `stm32f4_discovery.repl` istifadə edir

### Problem: Binary tapılmır
**Həll:** 
```powershell
Test-Path "Debug\Valeh_injec_pogram.elf"
```
Əgər `False` göstərirsə, proyekti build edin.

### Problem: Renode işləmir
**Həll:** 
```powershell
renode --version
```
Əgər xəta verirsə, Renode quraşdırın.

## İpucu

Emulyatoru VS Code-dan da işə sala bilərsiniz:
- `Ctrl+Shift+P` → "Tasks: Run Task" → "Run Emulator"

