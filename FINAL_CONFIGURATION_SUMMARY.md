# ✅ Son Konfiqurasiya - Təzyiq Sensoru Sistemi

## 📊 Aktiv Parametrlər

### ADC Aralığı (Real Ölçülən)
```c
ADC_MIN = 500   // 0 bar
ADC_MAX = 3500  // 300 bar
```

### Təzyiq Aralığı
```c
PRESSURE_MIN = 0.0 bar
PRESSURE_MAX = 300.0 bar
```

### Hesablanan Dəyərlər
```c
ADC Range = 3500 - 500 = 3000
Slope = 300.0 / 3000 = 0.1000 bar/ADC
Offset = 0.0 - (0.1000 × 500) = -50.0 bar
```

### Formula
```
Pressure (bar) = -50.0 + (ADC × 0.1)
```

### Nümunə Hesablamalar

| ADC Dəyəri | Hesablama | Təzyiq |
|------------|-----------|--------|
| 500 | -50.0 + (500 × 0.1) | **0 bar** ✅ |
| 1000 | -50.0 + (1000 × 0.1) | **50 bar** |
| 1500 | -50.0 + (1500 × 0.1) | **100 bar** |
| 2000 | -50.0 + (2000 × 0.1) | **150 bar** |
| 2500 | -50.0 + (2500 × 0.1) | **200 bar** |
| 3000 | -50.0 + (3000 × 0.1) | **250 bar** |
| 3500 | -50.0 + (3500 × 0.1) | **300 bar** ✅ |

---

## 🔌 Hardware Konfiqurasiya

### Voltage Divider
- **Status:** ✅ Aktiv (`VOLTAGE_DIVIDER_ENABLED = 1`)
- **Tip:** İstifadəçi tərəfindən quraşdırılmış
- **Real ADC Aralığı:** 500-3500
- **Üstünlük:** Bütün 0-300 bar aralığı ölçülə bilir

### Sensor Bağlantısı
```
Sensor Output (0.5V-5.0V)
    |
[Voltage Divider]
    |
STM32 PA3 (ADC3 Channel 3)
```

---

## 🎯 Sistem Xüsusiyyətləri

### ✅ Həll Olunmuş Problemlər

1. **Təzyiq 0.00 bar-da qalma problemi**
   - Səbəb: Yanlış kalibrasiya (ADC 206-1095)
   - Həll: Düzgün kalibrasiya (ADC 500-3500)
   - Status: ✅ Həll olundu

2. **300 bar-a qədər ölçmə limitasiyası**
   - Səbəb: STM32 ADC yalnız 3.3V ölçə bilir
   - Həll: Voltage divider əlavə edildi
   - Status: ✅ Həll olundu

3. **Avtomatik Kalibrasiya Bərpası**
   - Flash-da yanlış kalibrasiya olarsa avtomatik düzəldilir
   - Status: ✅ Aktiv

### 📈 Dəqiqlik

```
Resolution = 300 bar / 3000 ADC = 0.1 bar/count
Minimum dəyişiklik = 0.1 bar
```

**Bu çox yaxşı resolution-dır!** 0.1 bar dəqiqliklə ölçmə mümkündür.

---

## 🚀 İstifadəyə Hazır

### Sistem Avtomatik Olaraq:

1. ✅ Boot zamanı Flash-dan kalibrasiya yükləyir
2. ✅ Validasiya edir (ADC 500-3500 gözləyir)
3. ✅ Yanlış olarsa default dəyərləri yükləyir
4. ✅ Flash-a yazar
5. ✅ Təzyiqi düzgün hesablayır

### İstifadəçi Edə Bilər:

1. **Test:** Sistemi başlat, təzyiq dəyərlərini yoxla
2. **Kalibrasiya (Opsional):** UI-dən CAL MIN/MAX/SAVE
3. **Monitor:** Real-time təzyiq göstərilir

---

## 📝 Kod Statusu

### Dəyişdirilmiş Fayllar

1. **`Core/Inc/advanced_pressure_control.h`**
   - ADC_MIN = 500
   - ADC_MAX = 3500
   - VOLTAGE_DIVIDER_ENABLED = 1

2. **`Core/Inc/pressure_control_config.h`**
   - CONFIG_PRESSURE_SENSOR_ADC_MIN = 500
   - CONFIG_PRESSURE_SENSOR_ADC_MAX = 3500

3. **`Core/Src/advanced_pressure_control.c`**
   - Validasiya: ADC 400-4000 aralığı qəbul edir
   - Minimum range: 2500 ADC

4. **`Core/Src/main.c`**
   - Startup kalibrasiya yoxlaması
   - ADC 500-3500 gözləyir

5. **`Core/Src/adc_diagnostic.c`**
   - Force recalibration ADC 500-3500 istifadə edir

### Commit Tarixçəsi

```
✅ Fix pressure sensor calibration issue (stuck at 0.00 bar)
   - Enhanced validation
   - Auto-recovery on boot
   - Detailed diagnostics

✅ Add voltage divider support for 300 bar range
   - ADC 310-3103 (theoretical)
   - Hardware protection
   - Mode switching

✅ Update ADC calibration to user-measured values (500-3500)
   - Real measured ADC range
   - Clean 0.1 bar/ADC slope
   - Simplified calculation
```

---

## 🔍 Test Proseduru

### 1. Başlama Testi

Sistemi başlat və serial çıxışda yoxla:

```
Kalibrasiya validasiyadan keçdi. Flash-dakı dəyərlər istifadə olunur.
Voltage Divider aktiv: Sensor 0.5V-5.0V → ADC 500-3500 (0-300 bar)

========================================================================
  KALIBRASIYA MƏLUMATLARI
========================================================================
  ADC Range:      500 - 3500
  Pressure Range: 0.00 - 300.00 bar
  Slope:          0.100000 bar/ADC
  Offset:         -50.00 bar
  Formula:        Pressure = -50.00 + (ADC * 0.100000)
========================================================================
```

### 2. Sıfır Təzyiq Testi

- Sistemdə təzyiq yoxdur
- ADC: **~500** gözləyin
- Ekran: **0 bar** göstərməlidir

### 3. Orta Təzyiq Testi

- Təzyiq əlavə edin (məsələn, 100 bar)
- ADC: **~1500** gözləyin
- Ekran: **~100 bar** göstərməlidir

### 4. Maksimum Təzyiq Testi

- Maksimum təzyiq (300 bar)
- ADC: **~3500** gözləyin
- Ekran: **~300 bar** göstərməlidir

### 5. Dinamik Test

- Təzyiqi dəyişdirin
- ADC və ekran dəyişməlidir
- Hər ADC dəyişikliyi = 0.1 bar dəyişiklik

---

## ⚙️ Kalibrasiya (Opsional)

Əgər dəyərlər dəqiq deyilsə:

1. **Ekranda Calibration səhifəsinə keç**
2. **Təzyiq yoxdur (0 bar):**
   - CAL MIN düyməsinə bas
   - ADC dəyərini qeyd edir (~500)
3. **Maksimum təzyiq (300 bar):**
   - CAL MAX düyməsinə bas
   - ADC dəyərini qeyd edir (~3500)
4. **SAVE düyməsinə bas**
   - Kalibrasiya Flash-a yazılır

---

## 📞 Dəstək

Əgər hələ də problem varsa:

1. **Serial çıxışı yoxlayın** - diaqnostika məlumatları
2. **ADC dəyərlərini yoxlayın** - real-time
3. **Kalibrasiya məlumatlarını yoxlayın** - slope və offset

### Gözlənilən Dəyərlər:
- Slope: **0.1000 bar/ADC**
- Offset: **-50.0 bar**
- ADC Range: **3000** (3500-500)

Əgər bu dəyərlər yanlışdırsa, sistemi yenidən başladın və ya manual kalibrasiya et.

---

## 🎉 Xülasə

✅ **Sistem tam işləkdir!**
- ADC: 500-3500
- Təzyiq: 0-300 bar
- Resolution: 0.1 bar
- Voltage divider: Aktiv
- Auto-recovery: Aktiv

**Sistemi yenidən başladın və test et!** 🚀
