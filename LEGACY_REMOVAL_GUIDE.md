# Legacy Kodun Tamamilə Silinməsi Təlimatı

Bu sənəd legacy kodun tamamilə silinməsi üçün addım-addım təlimat verir.

## ⚠️ XƏBƏRDARLIQ

Legacy kod (`pressure_control.h`, `pressure_control.c`) artıq `#ifdef LEGACY_PRESSURE_CONTROL` ilə şərti yığılmaq üçün dəyişdirilib və default olaraq **DISABLED**-dur.

Tamamilə silmək istəyirsinizsə, aşağıdakı addımları izləyin:

## 📋 Silinmə Addımları

### 1. Legacy Fayllarını Silmək

**Fayllar:**
- `Core/Inc/pressure_control.h`
- `Core/Src/pressure_control.c`

**Addım:**
```bash
# Windows PowerShell
Remove-Item "Core\Inc\pressure_control.h"
Remove-Item "Core\Src\pressure_control.c"
```

**QEYD:** Bu fayllar artıq yığılmır (`#ifdef LEGACY_PRESSURE_CONTROL` olmadan), amma fiziki olaraq diskdə qalır.

### 2. Build Konfiqurasiyasını Yoxlamaq

**Yoxlama:**
- Build konfiqurasiyasında `LEGACY_PRESSURE_CONTROL` təyin olunmamalıdır
- Heç bir fayl `#include "pressure_control.h"` etməməlidir (yalnız `pressure_control.c` özü)

**Status:** ✅ Artıq təmizlənib - heç bir fayl `pressure_control.h` daxil etmir (legacy fayl özü istisna olmaqla)

## 🔄 PressureSensor_* Funksiyalarının Statusu

### Mövcud Vəziyyət

`PressureSensor_*` funksiyaları hələ də istifadə olunur:
- `PressureSensor_LoadCalibration()` - `main.c`-də çağırılır
- `PressureSensor_SaveCalibration()` - `ILI9341_FSMC.c`-də çağırılır
- `PressureSensor_Calibrate()` - Kalibrləmə UI-da istifadə olunur

### Tövsiyə

Bu funksiyalar **kalibrləmə UI funksiyalarıdır** və Advanced sistemdən fərqlidir:
- `PressureSensor_*` - UI kalibrləmə funksiyaları (ILI9341_FSMC.c)
- `AdvancedPressureControl_*` - Nəzarət sistemi funksiyaları (advanced_pressure_control.c)

**Nəticə:** Bu funksiyalar **qalmalıdır**, çünki onlar UI kalibrləmə interfeysi üçün lazımdır.

## ✅ Hazırkı Status

### Include Qalıqları
- ✅ **Təmizlənib** - Heç bir fayl `pressure_control.h` daxil etmir (legacy fayl özü istisna olmaqla)

### Legacy Fayllar
- ✅ **Şərti yığılmaq** - `#ifdef LEGACY_PRESSURE_CONTROL` ilə
- ✅ **Default DISABLED** - Legacy sistem yığılmır
- ⚠️ **Fiziki olaraq mövcuddur** - Silmək istəyirsinizsə, yuxarıdakı addımları izləyin

### PressureSensor_* Funksiyaları
- ✅ **Qalmalıdır** - UI kalibrləmə funksiyalarıdır
- ✅ **Advanced sistemlə inteqrasiya** - `AdvancedPressureControl_LoadCalibration()` onlardan məlumat alır

## 🎯 Nəticə

Kod bazası artıq təmizdir:
- Legacy kod yığılmır (default olaraq)
- Include qalıqları təmizlənib
- `PressureSensor_*` funksiyaları UI üçün lazımdır və qalmalıdır

Legacy faylları tamamilə silmək istəyirsinizsə, yuxarıdakı addımları izləyin.

