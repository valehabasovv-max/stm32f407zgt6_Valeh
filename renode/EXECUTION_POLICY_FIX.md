# PowerShell Execution Policy - Düzəliş

## Problem
PowerShell-də script işə salarkən xəta:
```
running scripts is disabled on this system
```

## Həll

PowerShell-i **Administrator** kimi açın və aşağıdakı komandanı işə salın:

```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

Və ya daha sadə (yalnız cari istifadəçi üçün):
```powershell
Set-ExecutionPolicy RemoteSigned -Scope CurrentUser
```

## Nədir RemoteSigned?

- **RemoteSigned**: İnternetdən yüklənmiş scriptlər imzalanmalıdır, lokal scriptlər işə sala bilər
- Bu, təhlükəsiz və praktik bir siyasətdir

## Alternativ: Bypass (Yalnız test üçün)

```powershell
Set-ExecutionPolicy Bypass -Scope Process
```

Bu, yalnız cari PowerShell sessiyası üçündür (təhlükəsiz).

## Yoxlama

```powershell
Get-ExecutionPolicy
```

Nəticə: `RemoteSigned` və ya `Bypass` olmalıdır.

## İndi Script İşə Sala Bilərsiniz

```powershell
cd renode
.\run_emulator.ps1
```

