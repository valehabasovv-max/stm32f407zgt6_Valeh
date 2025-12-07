# STM32CubeProgrammer ilə Upload - Addım-Addım Təlimat

## 📋 Tələblər

1. ✅ STM32CubeProgrammer quraşdırılıb
2. ✅ STM32F407ZGT6 board-una sahibsiniz
3. ✅ ST-Link (və ya USB) kabeli
4. ✅ `Debug\Valeh_injec_pogram.elf` faylı hazırdır

## 🚀 Addım-Addım Təlimat

### Addım 1: Board-u Qoşun

1. STM32 board-unu kompüterə qoşun (ST-Link və ya USB)
2. Board-un power olduğundan əmin olun (LED yanır)
3. Boot mode-u yoxlayın:
   - BOOT0 = 0 (normal işləmə rejimi)
   - BOOT1 = 0

### Addım 2: STM32CubeProgrammer Açın

1. STM32CubeProgrammer proqramını açın
2. Sol tərəfdə "Connect" bölməsini seçin

### Addım 3: Connection Quraşdırın

1. **Connection Type:** ST-LINK seçin
2. **Port:** SWD seçin (və ya USB)
3. **Speed:** 1.8 MHz (və ya daha aşağı)
4. **"Refresh"** düyməsinə basın
5. Board tapıldıqda, **"Connect"** düyməsinə basın

### Addım 4: Firmware Upload

1. Yuxarıda **"Open File"** düyməsinə basın
2. `D:\stm32f407zgt6_STM\Debug\Valeh_injec_pogram.elf` faylını seçin
3. **"Download"** düyməsinə basın (və ya F8)
4. Upload tamamlandıqda mesaj görünəcək

### Addım 5: Reset və İşə Sal

1. Upload tamamlandıqdan sonra board avtomatik reset olacaq
2. Və ya manual olaraq **"Reset"** düyməsinə basın
3. Proqram işə başlayacaq

### Addım 6: Görüntüyü Yoxlayın

1. LCD ekranında görüntü görünməlidir
2. Touch düymələri işləməlidir
3. Sistem normal işləməlidir

## 🔧 Problemlərin Həlli

### Problem: Board tapılmır

**Həll:**
1. ST-Link driver-ləri quraşdırın
2. USB kabeli yoxlayın
3. Board-un power olduğundan əmin olun
4. Başqa USB port yoxlayın

### Problem: Connection uğursuz

**Həll:**
1. ST-Link firmware-ni yeniləyin
2. Connection speed-i azaldın (1.8 MHz)
3. Board-u disconnect/connect edin

### Problem: Upload uğursuz

**Həll:**
1. Boot mode-u yoxlayın (BOOT0 = 0)
2. Flash memory-nin boş olduğundan əmin olun
3. Board-u reset edin və yenidən cəhd edin

### Problem: Proqram işləmir

**Həll:**
1. Upload uğurlu olduğundan əmin olun
2. Board-u reset edin
3. Power cycle edin (açıb-bağlayın)

## 📊 Nə Gözləmək Lazımdır

Upload tamamlandıqdan sonra:

1. ✅ LCD ekranında VALEH interface görünməlidir
2. ✅ Touch düymələri işləməlidir
3. ✅ Pressure control sistemi işləməlidir
4. ✅ UART output (əgər serial monitor varsa)

## 💡 Qeyd

- STM32CubeProgrammer **emulyator deyil**, yalnız upload tool-dur
- Görüntü **real hardware-də** görünəcək (LCD ekranda)
- Emulyatorda görüntü görünmür (Renode məhdudiyyəti)

## 🎯 Alternativ: STM32CubeIDE Simulator

Əgər real hardware yoxdursa, STM32CubeIDE-də məhdud simulator var:

1. STM32CubeIDE yükləyin
2. Proyekti açın
3. Debug mode-da işə salın
4. **Qeyd:** Simulator da LCD/Touch-ı tam dəstəkləmir

## 📝 Sonrakı Addımlar

1. Upload tamamlandıqdan sonra board-da görüntü görünəcək
2. Touch düymələri ilə interface-ə toxuna bilərsiniz
3. Sistem normal işləyəcək

