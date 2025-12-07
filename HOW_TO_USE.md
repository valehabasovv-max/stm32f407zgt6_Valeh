# VALEH Proyekti - Necə İstifadə Etmək

## 🎯 Hazırlananlar

1. ✅ **Renode Emulyator** - STM32F407 simulyasiyası
2. ✅ **Virtual LCD Monitor** - 320x240 ekran
3. ✅ **UI Simulator** - Proqramın UI-sini göstərir

## 🚀 İstifadə Yolları

### Yol 1: UI Simulator (Ən Asan - Tövsiyə olunur)

Proqramın UI-sini virtual LCD-də görmək:

```powershell
cd renode
python ui_simulator.py
```

**Nə görəcəksiniz:**
- ✅ Tam UI - proqramın bütün elementləri
- ✅ Real-time simulyasiya - pressure dəyərləri dəyişir
- ✅ Düymələr, panellər, göstəricilər
- ✅ Eyni dizayn - proyektin kodlarına əsasən

**Avantajlar:**
- Board lazım deyil
- Hər dəfə qoşmaq lazım deyil
- Dizayn dəyişikliklərini görmək asandır
- Screenshot almaq mümkündür

### Yol 2: Renode Emulyator (Alqoritm Testi)

Alqoritmləri test etmək üçün:

```powershell
cd renode
.\run_emulator.ps1
```

**Nə görəcəksiniz:**
- ✅ UART output - printf() mesajları
- ✅ CPU və Memory state
- ✅ Alqoritm işləməsi (PID, pressure control)

**Məhdudiyyətlər:**
- ⚠️ LCD görüntü yoxdur (yalnız UART)
- ⚠️ Touch düymələri işləmir

### Yol 3: Real Hardware (Tam Funksionallıq)

Görüntü və düymələr üçün:

1. STM32CubeProgrammer ilə upload edin
2. Board-da görüntü görünəcək
3. Touch düymələri işləyəcək

## 💡 Praktik İş Prosesi

### Development (Kod Yazmaq)

1. **UI Dizaynı:**
   ```powershell
   python renode/ui_simulator.py
   ```
   - UI-ni görün
   - Dizayn dəyişikliklərini planlaşdırın
   - Kodda dəyişiklik edin

2. **Alqoritm Testi:**
   ```powershell
   .\run_emulator.ps1
   ```
   - PID, pressure control test edin
   - UART output-da debug

3. **Visual Test:**
   - Real hardware-də upload edin
   - Görüntüyü yoxlayın
   - Screenshot alın

### Screenshot Almaq

**UI Simulator-dan:**
- Pəncərəni screenshot edin (Windows: Win+Shift+S)
- Və ya Print Screen

**Real Hardware-dən:**
- Telefon kamerası ilə
- Və ya serial monitor-dan

## 📊 Nə İşləyir, Nə İşləmir

### UI Simulator-da:
- ✅ Tam UI görüntüsü
- ✅ Real-time simulyasiya
- ✅ Dizayn dəyişiklikləri
- ❌ Real touch (mouse ilə simulyasiya oluna bilər)

### Renode Emulyator-da:
- ✅ Alqoritm testi
- ✅ UART output
- ✅ CPU/Memory state
- ❌ LCD görüntü
- ❌ Touch düymələri

### Real Hardware-də:
- ✅ Tam funksionallıq
- ✅ LCD görüntü
- ✅ Touch düymələri
- ✅ Bütün periferiyalar

## 🎯 Tövsiyə

**Development üçün:**
1. UI Simulator - dizayn üçün
2. Renode Emulyator - alqoritm testi üçün
3. Real Hardware - final test üçün

**Board yanınızda olmadıqda:**
- UI Simulator istifadə edin
- Dizayn dəyişikliklərini görün
- Kodda dəyişiklik edin
- Board gəldikdə upload edin

## 📝 Qeyd

UI Simulator proqramın UI kodlarını analiz edib eyni görüntünü göstərir. Bu, dizayn üçün praktikdir!

