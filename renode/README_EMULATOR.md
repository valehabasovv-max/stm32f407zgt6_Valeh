# STM32F407 Emulator - İstifadə Təlimatı

Bu qovluq STM32F407ZGT6 üçün Renode əsaslı emulyator konfiqurasiyasını ehtiva edir.

## 📋 Tələblər

1. **Renode** - Açıq mənbə emulyator
   - Yüklə: https://renode.io/ və ya https://github.com/renode/renode/releases
   - Windows üçün installer mövcuddur
   - Quraşdırmadan sonra PATH-ə əlavə edin: `C:\Program Files\Renode`

2. **Build edilmiş .elf faylı**
   - Proyekti build edin: `cd Debug && make`
   - `Debug/Valeh_injec_pogram.elf` faylı yaradılmalıdır

## 🚀 İstifadə

### Windows PowerShell (Tövsiyə olunur)

```powershell
cd renode
.\run_emulator.ps1
```

Və ya fərqli binary path ilə:

```powershell
.\run_emulator.ps1 -BinaryPath "Debug\Valeh_injec_pogram.elf"
```

### Windows Command Prompt

```cmd
cd renode
run_emulator.bat
```

Və ya fərqli binary path ilə:

```cmd
run_emulator.bat Debug\Valeh_injec_pogram.elf
```

### Manual (Renode Console)

```bash
renode --console
```

Sonra Renode konsolunda:

```
$bin=@binaries/Valeh_injec_pogram.elf
include @renode/stm32f407_emulator.resc
```

## 📊 Nə Görəcəksiniz

1. **UART2 Output** - `printf()` çıxışı burada görünəcək
2. **CPU State** - CPU vəziyyəti
3. **Memory Access** - Yaddaş oxuma/yazma əməliyyatları
4. **Peripheral Simulation** - Əsas periferiyalar simulyasiya olunur

## ⚙️ Konfiqurasiya

### UART Output

UART2 istifadə olunur printf üçün. Serial monitor kimi işləyir.

### Memory

- Flash: 0x08000000
- RAM: 0x20000000
- Peripherals: 0x40000000

### Peripherals

Emulyator aşağıdakı periferiyaları dəstəkləyir:
- ✅ UART (printf output)
- ✅ GPIO (əsas funksiyalar)
- ⚠️ ADC (məhdud dəstək)
- ⚠️ SPI (məhdud dəstək)
- ⚠️ FSMC/LCD (məhdud dəstək)
- ⚠️ Touch (məhdud dəstək)

**Qeyd:** Bəzi periferiyalar (LCD, Touch) tam simulyasiya olunmur. Əsas məntiq və alqoritmləri test etmək üçün kifayətdir.

## 🔧 Problemlərin Həlli

### Renode tapılmır

```powershell
# PATH-ə əlavə edin
$env:Path += ";C:\Program Files\Renode"
```

### Binary tapılmır

Proyekti build edin:

```bash
cd Debug
make
```

### UART output görünmür

1. `main.c`-də UART2 konfiqurasiyasını yoxlayın
2. `printf()` funksiyasının redirect olunduğundan əmin olun
3. Renode konsolunda UART analyzer-i yoxlayın

### Emulyator çox yavaş işləyir

Renode script-də CPU sürətini azaltın:

```
sysbus.cpu PerformanceInMips 84  # 168-dən 84-ə azalt
```

## 📝 Qeydlər

1. **Real Hardware vs Emulator:**
   - Emulyator əsas məntiq və alqoritmləri test etmək üçün yaxşıdır
   - Real hardware üçün həmişə final test lazımdır
   - Bəzi periferiyalar (LCD, Touch) tam simulyasiya olunmur

2. **Debugging:**
   - Renode GDB dəstəkləyir
   - VS Code-dan GDB ilə bağlana bilərsiniz
   - Breakpoint-lər qoya bilərsiniz

3. **Performance:**
   - Emulyator real hardware-dən daha yavaşdır
   - Timing-critical kodlar üçün real hardware testi lazımdır

## 🔗 Faydalı Linklər

- Renode Sənədləri: https://renode.io/
- Renode GitHub: https://github.com/renode/renode
- STM32F407 Platform: Renode-da daxili dəstək var

## 💡 İpucu

Emulyatoru VS Code-dan işə salmaq üçün `.vscode/tasks.json` yarada bilərsiniz:

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Run Emulator",
            "type": "shell",
            "command": "powershell",
            "args": [
                "-File",
                "${workspaceFolder}/renode/run_emulator.ps1"
            ],
            "problemMatcher": []
        }
    ]
}
```

Sonra `Ctrl+Shift+P` → "Run Task" → "Run Emulator"




