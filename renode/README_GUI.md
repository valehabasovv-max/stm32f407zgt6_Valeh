# Emulyatorda Görüntü və Düymələr

## ⚠️ Məhdudiyyət

**Renode emulyatorunda LCD ekran və Touch düymələri tam simulyasiya olunmur.**

Bu, Renode-un məhdudiyyətidir - kompleks periferiyalar (LCD, Touch) tam simulyasiya olunmur.

## 🎯 Həllər

### Seçim 1: Real Hardware (Tövsiyə olunur)

Görüntü və düymələri görmək üçün real hardware-ə upload edin:

```bash
# STM32CubeProgrammer və ya OpenOCD ilə
# Debug/Valeh_injec_pogram.elf faylını upload edin
```

### Seçim 2: UART Debug Output

Emulyatorda UART output-da debug məlumatlarını izləyin:

1. Renode konsolunda UART analyzer açılacaq
2. `printf()` mesajları görünəcək
3. Sistem vəziyyətini izləyə bilərsiniz

### Seçim 3: Renode GUI

Renode GUI-də periferiyaları izləyə bilərsiniz:

1. Renode GUI pəncərəsi açılacaq
2. Periferiyaları (GPIO, UART, ADC) izləyə bilərsiniz
3. Memory və register dəyərlərini görə bilərsiniz

## 📊 Nə Görə Bilərsiniz Emulyatorda

✅ **CPU və Memory** - Tam dəstək  
✅ **UART Output** - printf() mesajları  
✅ **GPIO State** - Pin vəziyyətləri  
✅ **ADC Values** - Simulyasiya olunmuş dəyərlər  
✅ **Timer State** - Timer vəziyyətləri  
✅ **Register Values** - Periferiya registerləri  

❌ **LCD Ekran** - Tam simulyasiya yoxdur  
❌ **Touch Düymələr** - Tam simulyasiya yoxdur  

## 💡 Tövsiyə

1. **Alqoritm Testi:** Emulyatorda PID, pressure control, state machine test edin
2. **Debug:** UART output-da sistem vəziyyətini izləyin
3. **Visual Test:** Real hardware-də LCD və Touch-ı test edin

## 🔧 GUI Mode İşə Salmaq

GUI mode-da işə salmaq üçün:

```powershell
cd renode
.\run_emulator.ps1
```

GUI pəncərəsi avtomatik açılacaq.

## 📝 Qeyd

Emulyator əsas məntiq və alqoritmləri test etmək üçün yaxşıdır, amma visual interface üçün real hardware lazımdır.

