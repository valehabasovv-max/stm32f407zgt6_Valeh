# 🎯 STM32F407 Emulyator - Xülasə

## ✅ Nə Hazırlandı

1. **Renode Emulyator Konfiqurasiyası** (`renode/stm32f407_emulator.resc`)
   - STM32F407ZGT6 üçün hazırlanmış script
   - UART2 output dəstəyi
   - ADC simulyasiyası

2. **İşə Salma Scriptləri**
   - `renode/run_emulator.ps1` - PowerShell script (Windows)
   - `renode/run_emulator.bat` - Batch script (Windows)

3. **VS Code Task Konfiqurasiyası** (`.vscode/tasks.json`)
   - "Run Emulator" task-ı
   - "Build and Run Emulator" task-ı

4. **Sənədləşmə**
   - `renode/README_EMULATOR.md` - Ətraflı təlimat
   - `EMULATOR_SETUP.md` - Quraşdırma təlimatı

## 🚀 Tez Başlanğıc

### 1. Renode Quraşdır
```powershell
# https://renode.io/ -dən yükləyin
# PATH-ə əlavə edin: C:\Program Files\Renode
```

### 2. Proyekti Build Et
```bash
cd Debug
make
```

### 3. Emulyatoru İşə Sal
```powershell
cd renode
.\run_emulator.ps1
```

Və ya VS Code-dan:
- `Ctrl+Shift+P` → "Tasks: Run Task" → "Run Emulator"

## 📊 Nə İşləyir

✅ **CPU və Memory** - Tam dəstək  
✅ **UART Output** - printf() mesajları görünür  
✅ **GPIO** - Əsas funksiyalar  
✅ **Timer** - Əsas funksiyalar  
✅ **Alqoritmlər** - PID, pressure control, və s.

⚠️ **Məhdud Dəstək:**
- ADC (test dəyərləri)
- LCD/Touch (tam simulyasiya yoxdur)

## 💡 İstifadə Tövsiyələri

1. **Alqoritm Testi:** PID, pressure control, state machine
2. **Debugging:** Breakpoint-lər, dəyişən izləmə
3. **Real Hardware:** Final test həmişə real hardware-də

## 📝 Qeyd

- Emulyator real hardware-dən daha yavaşdır (normaldır)
- LCD və Touch tam simulyasiya olunmur
- Final test real hardware-də olmalıdır

## 🔗 Faydalı Linklər

- Renode: https://renode.io/
- Ətraflı təlimat: `renode/README_EMULATOR.md`
- Quraşdırma: `EMULATOR_SETUP.md`

---

**Hazır!** İndi proyekti real hardware-ə upload etmədən test edə bilərsiniz! 🎉




