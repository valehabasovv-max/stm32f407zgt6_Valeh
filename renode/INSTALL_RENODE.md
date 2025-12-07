# Renode Quraşdırması - Windows

## Metod 1: Installer ilə (Tövsiyə olunur)

1. **Renode yükləyin:**
   - https://renode.io/ - əsas səhifə
   - Və ya birbaşa: https://github.com/renode/renode/releases
   - Windows üçün `.msi` installer seçin (ən son versiya)

2. **Quraşdırın:**
   - `.msi` faylını işə salın
   - Default yolu qəbul edin: `C:\Program Files\Renode`
   - Quraşdırmanı tamamlayın

3. **PATH-ə əlavə edin:**

   **Metod A: PowerShell (Admin hüquqları ilə)**
   ```powershell
   # PowerShell-i "Run as Administrator" kimi açın
   [Environment]::SetEnvironmentVariable("Path", $env:Path + ";C:\Program Files\Renode", "Machine")
   ```
   
   **Metod B: Manual (System Properties)**
   - `Win + R` → `sysdm.cpl` → Enter
   - "Advanced" tab → "Environment Variables"
   - "System variables" altında "Path" seçin → "Edit"
   - "New" → `C:\Program Files\Renode` əlavə edin
   - OK, OK, OK

4. **Yoxlayın:**
   - Yeni PowerShell/CMD pəncərəsi açın (köhnə pəncərəni bağlayın)
   ```powershell
   renode --version
   ```
   
   Əgər versiya görünsə, quraşdırma uğurludur!

## Metod 2: Portable Versiya (PATH lazım deyil)

1. **Portable versiyanı yükləyin:**
   - https://github.com/renode/renode/releases
   - `renode-*-windows-portable.zip` faylını yükləyin

2. **Açın:**
   - İstədiyiniz yerdə açın (məsələn: `D:\Tools\Renode`)

3. **Script-i dəyişdirin:**
   - `renode/run_emulator.bat` faylını redaktə edin
   - `renode` əvəzinə tam path istifadə edin:
   ```batch
   "D:\Tools\Renode\renode.exe" --console --disable-xwt --execute "$bin='%CD%\%BINARY_PATH%'; include @%RESC_FILE%"
   ```

## Metod 3: Chocolatey ilə (Əgər Chocolatey quraşdırılıbsa)

```powershell
choco install renode
```

## Yoxlama

Yeni terminal açın və test edin:

```powershell
renode --version
```

Əgər işləmirsə:
1. Kompüteri restart edin (PATH dəyişiklikləri üçün)
2. Və ya tam path istifadə edin: `"C:\Program Files\Renode\renode.exe" --version`

## Problemlərin Həlli

### Problem: "renode is not recognized"

**Həll 1:** PATH-ə əlavə edin (yuxarıda göstərildiyi kimi)

**Həll 2:** Tam path istifadə edin:
```powershell
& "C:\Program Files\Renode\renode.exe" --version
```

**Həll 3:** Renode quraşdırıldığını yoxlayın:
```powershell
Test-Path "C:\Program Files\Renode\renode.exe"
```

Əgər `False` göstərirsə, Renode quraşdırılmamışdır.

### Problem: "Access Denied"

PowerShell-i "Run as Administrator" kimi açın.

### Problem: PATH dəyişikliyi işləmir

1. Bütün terminal pəncərələrini bağlayın
2. Kompüteri restart edin
3. Yeni terminal açın və yenidən yoxlayın

## Alternativ: Script-i Düzəltmək

Əgər Renode quraşdırmaq istəmirsinizsə, script-i düzəldə bilərsiniz:

`renode/run_emulator.bat` faylını açın və `renode` əvəzinə tam path yazın:

```batch
"C:\Program Files\Renode\renode.exe" --console --disable-xwt --execute "$bin='%CD%\%BINARY_PATH%'; include @%RESC_FILE%"
```

Və ya portable versiya üçün:

```batch
"D:\Tools\Renode\renode.exe" --console --disable-xwt --execute "$bin='%CD%\%BINARY_PATH%'; include @%RESC_FILE%"
```




