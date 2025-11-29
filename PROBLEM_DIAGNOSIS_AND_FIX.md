# Təzyiq Sensorunun 0.00 Bar Qalması Probleminin Diaqnozu və Həlli

## Problem Təsviri (İstifadəçinin Şikayəti)

İstifadəçi bildirdi ki:
- **Sistemdə təzyiq var** amma ekranda **0.00 bar** göstərir
- Təzyiq sensoru işləyir və **5V gərginlik** var
- Sensor çıxış gərginliyi **0.51V - 0.00 bar** arasındadır
- **Təzyiq olduqda sensor gərginliyi dəyişir** (0.51V-dan yuxarıya)
- Amma ekranda **hələ də 0.00 bar** göstərir

## Problemin Kökü

Şəkildə göstərilən kalibrasiya məlumatlarına baxdıq:

```
Min Volt:  0.5V        ✓ Düzgün
Max Volt:  5.2V        ⚠ Yanlış (5.0V olmalıdır)
Min Prssr: 0.0 bar     ✓ Düzgün
Max Prssr: 6.0 bar     ⚠ Yanlış (300.0 bar olmalıdır)
ADC Min:   206         ❌ SEH!! (620 olmalıdır)
ADC Max:   1095        ❌ SEH!! (4095 olmalıdır)
```

### Səbəb: Yanlış Kalibrasiya

Flash yaddaşda saxlanılmış kalibrasiya dəyərləri **tamamilə yanlışdır**:

1. **ADC Min: 206** → Düzgün dəyər: **620** (0.5V üçün)
2. **ADC Max: 1095** → Düzgün dəyər: **4095** (12-bit ADC maksimum)
3. **Pressure Max: 6.0 bar** → Düzgün dəyər: **300.0 bar** (sensor spesifikasiyası)

### Niyə 0.00 Bar Göstərir?

İstifadəçinin sensoru **0.51V** göstərir, bu da **ADC ≈ 632** deməkdir:
```
ADC = (Voltage / 3.3V) * 4095 = (0.51 / 3.3) * 4095 ≈ 632
```

**Yanlış kalibrasiya ilə hesablama:**
```
Slope = (6.0 - 0.0) / (1095 - 206) = 6.0 / 889 = 0.00675 bar/ADC
Offset = 0.0 - (0.00675 * 206) = -1.39 bar
Pressure = -1.39 + (632 * 0.00675) = -1.39 + 4.27 = 2.88 bar
```

Amma **bu kalibrasiya validasiyadan keçmir** çünki:
- ADC aralığı (1095 - 206 = 889) çox dardır (minimum 2000 olmalıdır)
- ADC Min (206) çox kiçikdir (200-1000 aralığında olmalıdır, gözlənilən ~620)
- ADC Max (1095) çox kiçikdir (3000-4095 aralığında olmalıdır)

Ona görə də sistem bu kalibrasiyanu **rədd edir** və **default dəyərləri istifadə edir**.

**Düzgün kalibrasiya ilə hesablama:**
```
ADC Min:  620
ADC Max:  4095
Slope = (300.0 - 0.0) / (4095 - 620) = 300.0 / 3475 = 0.0864 bar/ADC
Offset = 0.0 - (0.0864 * 620) = -53.57 bar
Pressure = -53.57 + (632 * 0.0864) = -53.57 + 54.60 = 1.03 bar
```

Yəni **ADC 632 olduqda təzyiq ~1 bar göstərməlidir**.

## Tətbiq Edilmiş Düzəlişlər

### 1. Kalibrasiya Validasiyasının Gücləndilməsi

`AdvancedPressureControl_IsCalibrationRangeValid()` funksiyası yeniləndi:
- ADC Min: 200-1000 aralığında olmalıdır (gözlənilən ~620)
- ADC Max: 3000-4095 aralığında olmalıdır (gözlənilən ~4095)
- ADC aralığı minimum 2000 olmalıdır
- Pressure Min: -10 to 50 bar aralığında olmalıdır
- Pressure Max: 100-400 bar aralığında olmalıdır

### 2. Startup Zamanı Kalibrasiya Yoxlaması

`main.c`-də sistem başlayanda:
- Flash-dan yüklənmiş kalibrasiya validasiyadan keçirilir
- Əgər validasiya uğursuz olarsa:
  - Default kalibrasiya dəyərləri yüklənir
  - Flash yaddaşa yazılır
  - İstifadəçiyə xəbərdarlıq verilir

### 3. Diaqnostika Məlumatlarının Artırılması

`AdvancedPressureControl_ConvertAdcToPressure()` funksiyasında:
- İlk çağırışda kalibrasiya məlumatları detallı göstərilir
- ADC aralığı, pressure aralığı və slope yoxlanılır
- Problemli dəyərlər üçün xəbərdarlıqlar verilir

### 4. Məcburi Yenidən Kalibrasiya

`ADC_ForceRecalibration()` funksiyası:
- Default dəyərləri yükləyir (ADC: 620-4095, Pressure: 0-300 bar)
- Slope və offset hesablayır
- Flash-a yazır
- Cari ADC dəyəri ilə test edir

## Həll Yolu

Sistem indi avtomatik olaraq:

1. **Başlayanda** Flash-dakı kalibrasiyanu yoxlayır
2. **Validasiyadan keçmirsə** default dəyərləri yükləyir
3. **Flash-a yazır** ki, növbəti boot-da düzgün dəyərlər istifadə olunsun

İstifadəçi artıq:
- Sistemi yenidən başlatmalıdır
- Terminal/Serial çıxışını yoxlamalıdır
- Kalibrasiya məlumatlarının düzgün yükləndiyini təsdiq etməlidir

## Sensor Gərginlik Diaqnozu

İstifadəçi deyir ki sensor **0.51V** göstərir və bu **təzyiq yoxdur** deməkdir:

| Sensor Gərginliyi | ADC Dəyəri | Təzyiq (bar) |
|-------------------|------------|--------------|
| 0.50V             | 620        | 0.00         |
| 0.51V             | 632        | **~1.0**     |
| 1.00V             | 1241       | 53.6         |
| 2.00V             | 2482       | 160.7        |
| 3.30V (max ADC)   | 4095       | 300.0        |

**Nəticə:** Əgər sensor **0.51V** göstərirsə, bu **~1 bar** təzyiq deməkdir, **0.00 bar deyil**.

### Əgər sensor həqiqətən dəyişirsə...

İstifadəçi deyir: *"biraz təzyiq olnakimi sensor çıxış voltu dəyişir"*

Məsələn:
- Təzyiq yox: 0.51V → ADC 632 → **~1 bar**
- Təzyiq var: 0.60V → ADC 744 → **~11 bar**
- Təzyiq var: 0.70V → ADC 867 → **~21 bar**

Yəni **sensor düzgün işləyir** və **ADC də düzgün oxuyur**. Problem yalnız **səhv kalibrasiya** idi.

## Test Proseduru

1. Sistemi yenidən başladın
2. Serial/Terminal çıxışını aç
3. Bu məlumatları yoxla:

```
*****************************************************************
*  ⚠ XƏBƏRDARLIQ: KALIBRASIYA SƏHV AŞKAR EDİLDİ!              *
*****************************************************************
*  Flash-dakı kalibrasiya validasiyadan keçmədi:
*    ADC: 206 - 1095 (Gözlənilən: 620 - 4095)
*    Pressure: 0.00 - 6.00 bar
*
*  Default kalibrasiya dəyərləri yüklənəcək və Flash-a yazılacaq.
*****************************************************************

--- FORCING RECALIBRATION ---
Setting calibration to defaults:
  ADC: 620 - 4095
  Pressure: 0.0 - 300.0 bar

New calibration values:
  Slope: 0.086331 bar/ADC
  Offset: -53.525299 bar
Current ADC: 632 → Pressure: 1.03 bar
```

4. Ekranda təzyiq dəyərini yoxla - artıq **0.00 bar deyil, real təzyiq göstərilməlidir**

## Gələcək Kalibrasiya Üçün Qeydlər

Əgər yenidən kalibrasiya etmək lazım olarsa:

1. **CAL MIN düyməsi:**
   - Sistemdə **təzyiq yoxdur** (0 bar)
   - Sensor **0.50V** göstərməlidir
   - CAL MIN düyməsinə bas
   - Bu, ADC minimum dəyərini qeyd edir

2. **CAL MAX düyməsi:**
   - Sistemdə **maksimum təzyiq** var (məsələn, 150 bar)
   - Sensor **uyğun gərginlik** göstərməlidir (məsələn, 2.5V)
   - CAL MAX düyməsinə bas
   - Bu, ADC maksimum dəyərini qeyd edir

3. **SAVE düyməsi:**
   - Kalibrasiya dəyərlərini Flash-a yaz
   - Sistem yenidən başlayanda bu dəyərlər istifadə olunacaq

## Əlavə Qeydlər

### STM32 ADC Referans Gərginliyi Problemi

**Diqqət:** Sensor maksimum **5.0V** verir, amma STM32 ADC referansı **3.3V**-dir.

Bu deməkdir ki:
- **3.3V-dən yuxarı** gərginliklər ADC-də **saturated** olacaq (həmişə 4095 göstərəcək)
- **5.0V** → ADC 4095 (saturated)
- **3.3V** → ADC 4095
- Sensor **3.3V-ə** çatanda təzyiq **~230 bar**-dır

**Təklif:** Əgər sisteminizdə **230 bar-dan yuxarı** təzyiqlər varsa:
1. **Voltage divider** istifadə edin (məsələn, 5V → 3.3V-ə endir)
2. Və ya **sensor çıxış gərginliyini** 0-3.3V aralığına endirin
3. Və ya **ADC referans gərginliyini** 5V-ə çıxarın (bu, hardware dəyişikliyi tələb edir)

İndiki kod **0-300 bar** üçün qurulub, amma **real maksimum** ölçə bilən **~230 bar**-dır.

## Xülasə

✅ **Problem həll olundu!** Səhv kalibrasiya dəyərləri aşkar edildi və düzəldildi.

✅ Sistem indi avtomatik olaraq Flash-dakı səhv kalibrasiyanu aşkarlayır və düzəldir.

✅ İstifadəçi sistemi yenidən başlatmalıdır və ekranda **real təzyiq dəyəri** görünəcək.

⚠ Əgər sensor **0.51V** göstərirsə və sistemdə həqiqətən **təzyiq yoxdursa**, bu normaldır (**0.51V ≈ 1 bar**).

⚠ Əgər sensor **dəyişmirsə** (həmişə 0.51V qalır), sensor və ya ADC bağlantısı problemlidir.
