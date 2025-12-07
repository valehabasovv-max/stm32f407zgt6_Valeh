# STM32CubeIDE Simulator - Addım-Addım Təlimat

## 🎯 Məqsəd

STM32CubeIDE-də proyekti simulator mode-da işə salmaq və görüntünü yoxlamaq.

## 📋 Tələblər

- ✅ STM32CubeIDE quraşdırılıb
- ✅ `Valeh_injec_pogram.ioc` faylı mövcuddur
- ✅ Proyekt build edilmişdir

## 🚀 Addım-Addım Təlimat

### Addım 1: Proyekti Import Edin

1. **STM32CubeIDE açın**

2. **File → Import → STM32 Project from an Existing STM32CubeMX Configuration File**
   - Və ya: File → New → STM32 Project
   - "Load an existing .ioc file" seçin

3. **`.ioc` faylını seçin:**
   - `D:\stm32f407zgt6_STM\Valeh_injec_pogram.ioc`
   - "Next" düyməsinə basın

4. **Proyekt parametrləri:**
   - Project Name: `Valeh_injec_pogram` (və ya istədiyiniz ad)
   - Project Location: `D:\stm32f407zgt6_STM` (və ya başqa yerdə)
   - "Finish" düyməsinə basın

5. **STM32CubeMX açılacaq:**
   - Konfiqurasiya yoxlanılacaq
   - "Yes" deyərək kod generate edin

### Addım 2: Mövcud Kodları Kopyalayın

STM32CubeIDE yeni kod generate edəcək. Mövcud kodlarınızı kopyalayın:

1. **Core/Src/main.c** - Mövcud `main.c` kodlarınızı kopyalayın
2. **Core/Src/** - Digər custom faylları kopyalayın:
   - `ILI9341_FSMC.c`
   - `XPT2046.c`
   - `advanced_pressure_control.c`
   - və s.

3. **Core/Inc/** - Header faylları kopyalayın

4. **Drivers/** - HAL driver-ləri artıq olmalıdır

### Addım 3: Proyekti Build Edin

1. **Project → Build All** (və ya Ctrl+B)
2. Build uğurlu olmalıdır
3. Xətalar varsa, düzəldin

### Addım 4: Simulator Mode-da İşə Salın

1. **Debug konfiqurasiyası yaradın:**
   - Run → Debug Configurations
   - Sol tərəfdə "STM32 MCU C/C++ Application" altında yeni konfiqurasiya yaradın
   - Və ya mövcud konfiqurasiyanı redaktə edin

2. **Target seçin:**
   - "Target" tab-ında
   - **"Simulator"** seçin (real hardware deyil!)
   - MCU: STM32F407ZG seçin

3. **Debug düyməsinə basın:**
   - Və ya F11
   - Simulator işə başlayacaq

### Addım 5: Simulator-da İzləyin

1. **Debug perspective açılacaq:**
   - Breakpoint-lər qoya bilərsiniz
   - Dəyişənləri izləyə bilərsiniz
   - Step-by-step işləyə bilərsiniz

2. **Peripherals izləyin:**
   - Window → Show View → SFRs (Special Function Registers)
   - GPIO, UART, ADC və s. registerləri görə bilərsiniz

3. **UART output:**
   - Window → Show View → Console
   - `printf()` mesajları görünəcək

## ⚠️ Məhdudiyyətlər

**STM32CubeIDE Simulator-da:**
- ❌ LCD ekran tam simulyasiya olunmur
- ❌ Touch düymələri simulyasiya olunmur
- ❌ FSMC/LCD periferiyası məhdud dəstəklənir
- ✅ CPU, Memory, UART, GPIO (əsas) işləyir
- ✅ Alqoritmlər test edilə bilər

## 💡 Tövsiyə

1. **Alqoritm Testi:** Simulator-da test edin
2. **Visual Test:** Real hardware-də görüntüyü yoxlayın
3. **Screenshot:** Real hardware-dən foto çəkin

## 🔧 Problemlərin Həlli

### Problem: Simulator seçimi görünmür

**Həll:**
- STM32CubeIDE-nin son versiyasını quraşdırın
- Proyekt düzgün import olunduğundan əmin olun

### Problem: Build xətası

**Həll:**
- Bütün faylları düzgün kopyaladığınızdan əmin olun
- Include path-ləri yoxlayın
- HAL library-ləri düzgün quraşdırıldığından əmin olun

### Problem: Simulator işləmir

**Həll:**
- Target "Simulator" seçildiyindən əmin olun
- Debug konfiqurasiyasını yenidən yaradın

## 📝 Qeyd

Simulator məhduddur, amma alqoritm testi üçün yaxşıdır. Görüntü üçün real hardware lazımdır.

