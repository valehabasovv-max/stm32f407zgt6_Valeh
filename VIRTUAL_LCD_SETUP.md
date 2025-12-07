# Virtual LCD Monitor Quraşdırması

## 🎯 Məqsəd

Board yanınızda olmadıqda, kompüterdə virtual LCD ekran göstərmək.

## 📋 Tələblər

1. ✅ Renode quraşdırılıb
2. ✅ Python 3.8+ quraşdırılıb
3. ✅ Tkinter (Python GUI) - adətən daxildir

## 🚀 Quraşdırma

### Addım 1: Python Yoxlayın

```powershell
python --version
```

Əgər Python yoxdursa: https://www.python.org/downloads/

### Addım 2: Tkinter Yoxlayın

```powershell
python -m tkinter
```

Əgər işləmirsə, quraşdırın (Windows-da adətən daxildir).

### Addım 3: Virtual LCD Monitor İşə Salın

```powershell
cd renode
.\run_with_lcd_monitor.ps1
```

Bu:
1. Renode emulyatorunu işə salacaq
2. Virtual LCD Monitor GUI pəncərəsini açacaq
3. FSMC LCD writes-ı izləyəcək

## 📊 Nə Görəcəksiniz

1. **Renode Konsolu** - Emulyator işləyir
2. **Virtual LCD Monitor Pəncərəsi** - 320x240 ekran
3. **Real-time Display** - LCD yazıları görünəcək

## ⚠️ Məhdudiyyətlər

1. **FSMC Hooking:** FSMC writes-ı hook etmək mürəkkəbdir
2. **ILI9341 Protocol:** LCD driver protokolu kompleksdir
3. **Performance:** Real-time simulyasiya yavaş ola bilər

## 🔧 Problemlərin Həlli

### Problem: LCD Monitor açılmır

**Həll:**
- Python quraşdırıldığından əmin olun
- Tkinter mövcud olduğundan əmin olun
- Manual olaraq işə salın: `python renode/lcd_monitor_gui.py`

### Problem: Ekran boşdur

**Həll:**
- FSMC writes hook düzgün işləmir
- Renode-da FSMC monitoring aktiv deyil
- Test pattern göstərmək üçün: `lcd_monitor_gui.py`-da test pattern var

## 💡 Alternativ: Test Pattern

Virtual LCD-ni test etmək üçün:

```powershell
python renode/lcd_monitor_gui.py
```

Bu test pattern göstərəcək.

## 📝 Qeyd

**Vacib:** FSMC LCD writes-ı tam hook etmək çətindir. Bu, demonstrasiya məqsədi ilədir. 

**Praktik həll:** Real hardware-də test edin, amma development üçün Renode-da alqoritm test edin.

