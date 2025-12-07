# Tam Simulator Quraşdırması - Görüntü ilə

## 🎯 Məqsəd

STM32-yə qoşmadan görüntünü simulyatorda görmək, screenshot almaq və dəyişiklik etmək.

## 📋 Həllər

### Həll 1: STM32CubeIDE Simulator (Ən Asan)

**Quraşdırma:**
1. STM32CubeIDE yükləyin: https://www.st.com/en/development-tools/stm32cubeide.html
2. Proyekti import edin (`Valeh_injec_pogram.ioc`)
3. Debug → Debug Configurations → Simulator seçin
4. Debug düyməsinə basın

**Avantajlar:**
- ✅ Rəsmi ST tool
- ✅ Debugging dəstəyi
- ✅ Bəzi periferiyalar simulyasiya olunur

**Məhdudiyyətlər:**
- ⚠️ LCD və Touch tam simulyasiya olunmur
- ⚠️ Görüntü məhduddur

### Həll 2: Renode + Python LCD Visualizer (Gəlin Quraşdıraq)

Renode-da virtual LCD ekran yarada bilərik və Python ilə görüntüləyə bilərik.

**Quraşdırma:**

1. **Python quraşdırın:**
   ```powershell
   # Python 3.8+ lazımdır
   python --version
   ```

2. **Tkinter quraşdırın (GUI üçün):**
   ```powershell
   # Windows-da adətən daxildir
   python -m tkinter
   ```

3. **Renode script-i işə salın:**
   ```powershell
   cd renode
   .\run_emulator.ps1
   ```

4. **LCD Visualizer işə salın (ayrı terminal):**
   ```powershell
   python renode/lcd_visualizer.py
   ```

**Avantajlar:**
- ✅ Custom LCD simulator
- ✅ Screenshot almaq mümkündür
- ✅ Açıq mənbə

**Məhdudiyyətlər:**
- ⚠️ FSMC LCD writes-ı hook etmək lazımdır
- ⚠️ Manual konfiqurasiya tələb olunur

### Həll 3: QEMU STM32 (Məhdud Dəstək)

QEMU-da STM32 simulyasiyası var, amma məhduddur.

## 🎨 Ən Praktik Həll: Hybrid Yanaşma

### 1. Alqoritm Testi: Renode-da
- PID, pressure control, state machine
- UART output-da debug

### 2. Visual Test: Real Hardware-də
- LCD ekran
- Touch düymələri
- Screenshot almaq

### 3. Screenshot Almaq

**Real Hardware-dən:**
- Telefon kamerası ilə
- Və ya serial monitor-dan screenshot

**Simulator-dan:**
- STM32CubeIDE: Debug → Screenshot
- Renode: Python visualizer-dan screenshot

## 💡 Tövsiyə

1. **Development:** Renode-da alqoritm test edin
2. **Visual Design:** Real hardware-də görüntüyü yoxlayın
3. **Screenshot:** Real hardware-dən foto çəkin
4. **Final Test:** Real hardware-də tam test edin

## 🔧 Problemlərin Həlli

### Problem: Simulator-da görüntü görünmür

**Həll:** 
- Simulator məhduddur, real hardware lazımdır
- Və ya custom visualizer quraşdırın

### Problem: Screenshot almaq

**Həll:**
- Real hardware-dən foto çəkin
- Və ya simulator GUI-dən screenshot alın

## 📝 Qeyd

**Vacib:** Heç bir simulator LCD və Touch-ı tam simulyasiya etmir. Görüntü üçün real hardware lazımdır.

**Praktik yanaşma:**
- Development: Emulyator (alqoritm testi)
- Visual: Real hardware (görüntü və screenshot)

