# STM32 Simulator Quraşdırması - Görüntü ilə

## 🎯 Məqsəd

STM32-yə qoşmadan görüntünü simulyatorda görmək və screenshot almaq.

## 📋 Seçimlər

### Seçim 1: STM32CubeIDE Simulator (Tövsiyə olunur)

STM32CubeIDE-də məhdud simulator var ki, bəzi periferiyaları simulyasiya edir.

**Avantajlar:**
- ✅ STMicroelectronics rəsmi tool
- ✅ Bəzi periferiyalar simulyasiya olunur
- ✅ Debugging dəstəyi

**Məhdudiyyətlər:**
- ⚠️ LCD və Touch tam simulyasiya olunmur
- ⚠️ Məhdud periferiya dəstəyi

### Seçim 2: Renode + Virtual LCD (Gəlin Quraşdıraq)

Renode-da custom virtual LCD ekran yarada bilərik.

**Avantajlar:**
- ✅ Açıq mənbə
- ✅ Custom LCD simulator yarada bilərik
- ✅ Screenshot almaq mümkündür

**Məhdudiyyətlər:**
- ⚠️ LCD driver-ləri manual konfiqurasiya lazımdır
- ⚠️ Touch simulation məhduddur

## 🚀 STM32CubeIDE Simulator Quraşdırması

### Addım 1: STM32CubeIDE Yükləyin

1. https://www.st.com/en/development-tools/stm32cubeide.html
2. Windows installer yükləyin
3. Quraşdırın

### Addım 2: Proyekti Import Edin

1. STM32CubeIDE açın
2. File → Import → STM32 Project from an Existing STM32CubeMX Configuration File
3. `Valeh_injec_pogram.ioc` faylını seçin
4. Proyekti import edin

### Addım 3: Simulator Mode-da İşə Salın

1. Debug konfiqurasiyası yaradın
2. Target: "Simulator" seçin
3. Debug düyməsinə basın
4. Simulator işə başlayacaq

**Qeyd:** Simulator məhdud dəstəkləyir, amma bəzi funksiyalar işləyə bilər.

## 🎨 Renode-da Virtual LCD Yaratmaq

Renode-da custom LCD simulator yarada bilərik. Gəlin bunu quraşdıraq!

