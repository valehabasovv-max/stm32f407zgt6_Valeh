# STM32F407 Emulator Quraşdırması

Bu sənəd STM32F407ZGT6 proyekti üçün emulyator quraşdırmasını izah edir.

## 🎯 Məqsəd

Real hardware-ə upload etmədən proyekti test etmək və debug etmək üçün emulyator istifadə edə bilərsiniz.

## 📦 Quraşdırma

### 1. Renode Yüklə və Quraşdır

1. **Renode yüklə:**
   - https://renode.io/ və ya
   - https://github.com/renode/renode/releases
   - Windows üçün `.msi` installer seçin

2. **PATH-ə əlavə et:**
   ```powershell
   # PowerShell-də (admin hüquqları ilə)
   [Environment]::SetEnvironmentVariable("Path", $env:Path + ";C:\Program Files\Renode", "Machine")
   ```

   Və ya manual olaraq:
   - System Properties → Environment Variables
   - Path dəyişəninə əlavə edin: `C:\Program Files\Renode`

3. **Yoxla:**
   ```powershell
   renode --version
   ```

### 2. Proyekti Build Et

```bash
cd Debug
make
```

`Debug/Valeh_injec_pogram.elf` faylı yaradılmalıdır.

## 🚀 İstifadə

### Metod 1: PowerShell Script (Tövsiyə olunur)

```powershell
cd renode
.\run_emulator.ps1
```

### Metod 2: Batch Script

```cmd
cd renode
run_emulator.bat
```

### Metod 3: VS Code Task

1. `Ctrl+Shift+P`
2. "Tasks: Run Task"
3. "Run Emulator" seçin

Və ya `Ctrl+Shift+B` → "Run Emulator"

### Metod 4: Manual (Renode Console)

```bash
renode --console
```

Sonra Renode konsolunda:

```
$bin=@D:\stm32f407zgt6_STM\Debug\Valeh_injec_pogram.elf
include @renode/stm32f407_emulator.resc
```

## 📊 Nə Görəcəksiniz

1. **UART Output** - `printf()` mesajları görünəcək
2. **CPU State** - CPU vəziyyəti
3. **Memory Access** - Yaddaş əməliyyatları
4. **Peripheral Simulation** - Əsas periferiyalar

## ⚠️ Məhdudiyyətlər

### Tam Dəstəklənən:
- ✅ CPU və Memory
- ✅ UART (printf output)
- ✅ GPIO (əsas funksiyalar)
- ✅ Timer (əsas funksiyalar)

### Məhdud Dəstək:
- ⚠️ ADC (test dəyərləri qaytarır)
- ⚠️ SPI (məhdud)
- ⚠️ FSMC/LCD (tam simulyasiya yoxdur)
- ⚠️ Touch Controller (tam simulyasiya yoxdur)

### Dəstəklənməyən:
- ❌ Real LCD ekran
- ❌ Real Touch sensor
- ❌ Real ADC sensor

## 💡 İstifadə Tövsiyələri

### 1. Alqoritm Testi
Emulyator əsas məntiq və alqoritmləri test etmək üçün yaxşıdır:
- PID nəzarəti
- Pressure hesablamaları
- State machine
- Data strukturları

### 2. Debugging
- Breakpoint-lər qoya bilərsiniz
- Dəyişənləri izləyə bilərsiniz
- Memory-ni oxuya/yaza bilərsiniz

### 3. Real Hardware Test
- Final test həmişə real hardware-də olmalıdır
- LCD və Touch funksiyaları real hardware-də test edilməlidir
- Timing-critical kodlar real hardware-də yoxlanılmalıdır

## 🔧 Problemlərin Həlli

### Problem: Renode tapılmır

**Həll:**
```powershell
# PATH-ə əlavə edin
$env:Path += ";C:\Program Files\Renode"
```

Və ya tam path ilə:
```powershell
& "C:\Program Files\Renode\renode.exe" --console
```

### Problem: Binary tapılmır

**Həll:**
1. Proyekti build edin:
   ```bash
   cd Debug
   make
   ```

2. `Debug/Valeh_injec_pogram.elf` faylının mövcud olduğunu yoxlayın

### Problem: UART output görünmür

**Həll:**
1. UART2-nin konfiqurasiya olunduğundan əmin olun
2. `__io_putchar()` funksiyasının düzgün redirect olunduğunu yoxlayın
3. Renode konsolunda UART analyzer-i yoxlayın

### Problem: Emulyator çox yavaş

**Həll:**
Renode script-də CPU sürətini azaltın:
```
sysbus.cpu PerformanceInMips 84  # 168-dən 84-ə
```

## 📝 Qeydlər

1. **Performance:** Emulyator real hardware-dən daha yavaşdır. Bu normaldır.

2. **Peripherals:** Bəzi periferiyalar (LCD, Touch) tam simulyasiya olunmur. Əsas məntiq testi üçün kifayətdir.

3. **Real Hardware:** Final test həmişə real hardware-də olmalıdır.

## 🔗 Faydalı Linklər

- Renode Sənədləri: https://renode.io/
- Renode GitHub: https://github.com/renode/renode
- STM32F407 Platform: Renode-da daxili dəstək var

## 📞 Dəstək

Əgər problem yaşayırsınızsa:
1. `renode/README_EMULATOR.md` faylını oxuyun
2. Renode log fayllarını yoxlayın
3. Build çıxışını yoxlayın




