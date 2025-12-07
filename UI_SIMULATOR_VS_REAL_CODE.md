# UI Simulator vs Real Code - Fərq

## ⚠️ Vacib Qeyd

**UI Simulator-da etdiyiniz dəyişikliklər board-a upload edilmir!**

## 🔍 Fərq

### UI Simulator (Python)
- **Fayl:** `renode/ui_simulator.py`
- **Dil:** Python
- **Məqsəd:** Virtual LCD-də UI-ni göstərmək
- **Board-a upload:** ❌ Yox
- **İstifadə:** Dizayn testi, UI görüntüsü

### Real Proyekt (C Code)
- **Fayllar:** `Core/Src/main.c`, `Core/Src/ILI9341_FSMC.c`, və s.
- **Dil:** C (STM32)
- **Məqsəd:** Real hardware-də işləmək
- **Board-a upload:** ✅ Bəli (ELF faylı)
- **İstifadə:** Real sistem

## 📊 Necə İşləyir

### UI Simulator
```
Python Script (ui_simulator.py)
    ↓
Analiz edir: Core/Src/main.c (yalnız oxuyur)
    ↓
Virtual LCD-də UI göstərir
    ↓
Board-a upload edilmir ❌
```

### Real Proyekt
```
C Kodları (main.c, ILI9341_FSMC.c)
    ↓
Build → ELF faylı
    ↓
STM32CubeProgrammer ilə upload
    ↓
Board-da işləyir ✅
```

## 💡 Praktik İş Prosesi

### 1. UI Dizaynı (UI Simulator-da)
```python
# renode/ui_simulator.py-də dəyişiklik
# Yalnız simulyasiya üçün
```

### 2. Real Kodda Dəyişiklik
```c
// Core/Src/main.c-də dəyişiklik
// Board-a upload ediləcək
```

### 3. Test
- UI Simulator-da görün
- C kodunda dəyişiklik et
- Build et
- Board-a upload et

## 🎯 Nümunə

### UI Simulator-da dəyişiklik:
```python
# ui_simulator.py
self.target_pressure = 150.0  # Dəyişiklik
```

**Nəticə:** Yalnız simulyasiyada görünür, board-da yox ❌

### Real kodda dəyişiklik:
```c
// main.c
float target_pressure = 150.0;  // Dəyişiklik
```

**Nəticə:** Board-a upload edildikdə işləyir ✅

## ✅ Tövsiyə

1. **UI Simulator:** Dizayn planlaşdırmaq üçün
2. **C Kodları:** Real dəyişikliklər üçün
3. **Test:** Hər ikisində test edin

## 📝 Qeyd

UI Simulator proyektin C kodlarını **yalnız oxuyur** (analiz edir), dəyişdirmir. Board-da dəyişiklik etmək üçün C kodlarını dəyişdirmək lazımdır.

