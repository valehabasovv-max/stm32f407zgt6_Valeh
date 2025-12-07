# Real Hardware-ə Upload Etmək

## ⚠️ Vacib Qeyd

**Renode emulyatorunda LCD ekran və Touch düymələri görünmür!**

Görüntü və düymələri görmək üçün proyekti real STM32F407ZGT6 hardware-ə upload etməlisiniz.

## 🚀 Upload Metodları

### Metod 1: STM32CubeProgrammer (Tövsiyə olunur)

1. **STM32CubeProgrammer yükləyin:**
   - https://www.st.com/en/development-tools/stm32cubeprog.html

2. **ELF faylını upload edin:**
   - Open STM32CubeProgrammer
   - Connect your STM32 board (ST-Link, USB, etc.)
   - File → Open File → `Debug\Valeh_injec_pogram.elf` seçin
   - "Download" düyməsinə basın

### Metod 2: OpenOCD (Command Line)

```bash
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program Debug/Valeh_injec_pogram.elf verify reset exit"
```

### Metod 3: VS Code STM32 Extension

1. VS Code-da STM32 extension quraşdırın
2. `Ctrl+Shift+P` → "STM32: Flash"
3. `Debug\Valeh_injec_pogram.elf` seçin

### Metod 4: ST-Link Utility

1. ST-Link Utility açın
   - https://www.st.com/en/development-tools/stsw-link004.html
2. Connect board
3. File → Open File → `Debug\Valeh_injec_pogram.elf`
4. Program & Verify

## 📊 Emulyator vs Real Hardware

### Emulyatorda Görünən:
- ✅ UART output (printf mesajları)
- ✅ CPU və Memory state
- ✅ Periferiya registerləri
- ✅ Alqoritm işləməsi

### Emulyatorda Görünməyən:
- ❌ LCD ekran görüntüsü
- ❌ Touch düymələri
- ❌ Real sensor dəyərləri

### Real Hardware-də Görünən:
- ✅ LCD ekran (tam görüntü)
- ✅ Touch düymələri (işləyir)
- ✅ Real sensor dəyərləri
- ✅ Bütün funksiyalar

## 💡 Tövsiyə

1. **Alqoritm Testi:** Emulyatorda test edin
2. **Visual Test:** Real hardware-də görüntüyü yoxlayın
3. **Final Test:** Real hardware-də tam test edin

## 🔧 Problemlərin Həlli

### Problem: Board tapılmır
- ST-Link driver-ləri quraşdırın
- USB kabeli yoxlayın
- Board-un power olduğundan əmin olun

### Problem: Upload uğursuz
- Boot mode-u yoxlayın (BOOT0 pin)
- ST-Link connection yoxlayın
- Flash memory-nin boş olduğundan əmin olun

## 📝 Qeyd

Emulyator əsas məntiq testi üçün yaxşıdır, amma visual interface üçün real hardware lazımdır.

