# Sensor 5V Çıxış Gərginliyi Problemi və Həlli

## Problem

Təzyiq sensoru **5V ilə qidalanır** və çıxış gərginliyi **0.5V-5.0V** aralığındadır:
- 0.5V = 0 bar
- 5.0V = 300 bar

STM32F407 ADC isə yalnız **0-3.3V** ölçə bilir:
- 3.3V-dən yuxarı gərginliklər **saturated** (4095) oxunur
- Bu, **~230 bar-dan yuxarı təzyiqləri ölçə bilməyəcəyimiz** deməkdir

## Həll Yolu 1: Voltage Divider (Tövsiyə Edilir) ⭐

### Hardware Dəyişikliyi

Sensor çıxışı ilə STM32 ADC arasında **voltage divider** quraşdırın:

```
Sensor Output (0.5V-5.0V)
    |
    R1 (10kΩ)
    |
    +-----> STM32 PA3 (ADC Input)
    |
    R2 (6.8kΩ)
    |
   GND

Divisor = R2 / (R1 + R2) = 6.8 / (10 + 6.8) = 0.405
Output = Input × 0.405

0.5V → 0.20V  (ADC ≈ 248)
5.0V → 2.02V  (ADC ≈ 2509)
```

### Kod Dəyişikliyi (Voltage Divider üçün)

Kalibrasiya dəyərlərini yeniləyin:

```c
// Voltage divider ilə yeni ADC aralığı
#define ADC_MIN_WITH_DIVIDER 248    // 0.5V × 0.405 = 0.20V
#define ADC_MAX_WITH_DIVIDER 2509   // 5.0V × 0.405 = 2.02V
#define PRESSURE_MIN 0.0f
#define PRESSURE_MAX 300.0f
```

**Üstünlükləri:**
- ✅ Bütün 0-300 bar aralığı ölçülə bilir
- ✅ ADC zədələnmə riski yoxdur
- ✅ Daha dəqiq ölçmə

**Çatışmazlıqları:**
- ⚠️ Hardware dəyişikliyi lazımdır (2 rezistor)

---

## Həll Yolu 2: 3.3V-də Limitləmə (Hal-hazırda Tətbiq Edilib)

### Kod Dəyişikliyi (Hardware Dəyişikliksiz)

Hal-hazırda kod bu şəkildə işləyir:

```c
#define ADC_MIN 620    // 0.5V
#define ADC_MAX 4095   // 3.3V (saturated at 5.0V)
#define PRESSURE_MIN 0.0f
#define PRESSURE_MAX 300.0f  // Amma real max ~230 bar (3.3V-də)
```

**Reallıqda:**
```
0.5V  → ADC 620   → 0 bar    ✓
1.0V  → ADC 1241  → 54 bar   ✓
2.0V  → ADC 2482  → 161 bar  ✓
3.3V  → ADC 4095  → 300 bar  ✓ (amma real ~230 bar)
5.0V  → ADC 4095  → 300 bar  ❌ (saturated, real 300 bar-dır)
```

**Üstünlükləri:**
- ✅ Hardware dəyişikliyi lazım deyil
- ✅ 0-230 bar arası düzgün işləyir

**Çatışmazlıqları:**
- ⚠️ 230 bar-dan yuxarı ölçülə bilmir
- ⚠️ 3.3V-dən yuxarı gərginliklər ADC-ni zədələyə bilər

---

## Həll Yolu 3: Doğru Kalibrasiya (3.3V Limit ilə)

Əgər sisteminizdə **maksimum təzyiq 230 bar-dan aşağıdırsa**, hardware dəyişikliyi lazım deyil. Sadəcə kalibrasiyanu düzəldin:

### Kalibrasiya Proseduru

1. **Minimum Təzyiq (0 bar):**
   - Sistemi boşaldın (təzyiq yoxdur)
   - Sensor çıxışı: **0.50V**
   - ADC oxumalı: **~620**
   - **CAL MIN** düyməsinə basın

2. **Maksimum Təzyiq (real max):**
   - Sistemi maksimum təzyiqə qaldırın (məsələn, 150 bar)
   - Sensor çıxışını ölçün (məsələn, 2.74V)
   - ADC oxumalı: **~3400**
   - **CAL MAX** düyməsinə basın

3. **Yadda Saxla:**
   - **SAVE** düyməsinə basın
   - Kalibrasiya Flash-a yazılacaq

### Nümunə Hesablama

Əgər sizin maksimum təzyiq **150 bar** olarsa:

```
Sensor 0.5V-5.0V, amma 150 bar-da:
Voltage = 0.5 + (150/300) × (5.0 - 0.5) = 0.5 + 0.5 × 4.5 = 2.75V

ADC = (2.75 / 3.3) × 4095 = 3410

Kalibrasiya:
ADC_MIN = 620  (0 bar, 0.5V)
ADC_MAX = 3410 (150 bar, 2.75V)
```

---

## Kodda Avtomatik Düzəliş (Təklif)

Əgər voltage divider **istifadə edilmirsə** və **3.3V-də saturasiya** problemi varsa, kodu aşağıdakı kimi düzəldə bilərəm:

### Variant A: Pressure Max-ı Real Dəyərə Düzəlt

```c
// Real maksimum təzyiq 3.3V-də
#define ADC_MIN 620    // 0.5V
#define ADC_MAX 4095   // 3.3V
#define PRESSURE_MIN 0.0f
#define PRESSURE_MAX_REAL 230.0f  // 3.3V = 230 bar (real)
```

### Variant B: İstifadəçi Seçimli Mod

Kalibrasiya ekranına düymə əlavə et:
- **"5V Mode"**: Voltage divider istifadə olunur
- **"3.3V Mode"**: Hardware dəyişiklik yoxdur, 230 bar limit

---

## Tövsiyə

### Qısa Müddət (Hal-hazırda):
**Həll Yolu 2** istifadə edin - kod artıq düzgündür, amma unutmayın ki:
- ✅ 0-230 bar arası mükəmməl işləyir
- ⚠️ 230 bar-dan yuxarı ölçülə bilmir

### Uzun Müddət (İdeal):
**Həll Yolu 1** tətbiq edin - **Voltage Divider** əlavə edin:
- 2 ədəd rezistor (10kΩ + 6.8kΩ və ya 10kΩ + 8.2kΩ)
- Sensor çıxışı ilə PA3 arasına qoşun
- Kodu yenilə (ADC_MIN və ADC_MAX dəyərlərini)

---

## Hazırda Nə Etməlisiniz?

1. **Sisteminizdə maksimum təzyiq nə qədərdir?**
   - Əgər **< 230 bar**: Heç bir dəyişiklik lazım deyil ✓
   - Əgər **> 230 bar**: Voltage divider lazımdır ⚠️

2. **Sistemi test edin:**
   - Təzyiqi artırın
   - Sensor gərginliyini ölçün (multimetr ilə)
   - Əgər 3.3V-dən yuxarı olarsa, ekranda təzyiq artmayacaq (4095 ADC-də qalacaq)

3. **Əgər voltage divider əlavə etmək istəyirsinizsə:**
   - Mənə deyin, kodu yeniləyim
   - Yeni ADC_MIN və ADC_MAX hesablayım
   - Kalibrasiya prosedurunu izah edim

## Suallar

1. **Sisteminizdə maksimum təzyiq nə qədərdir?** (50 bar? 100 bar? 200 bar? 300 bar?)
2. **Hal-hazırda hardware dəyişikliyi etmək mümkündürmü?** (voltage divider əlavə etmək)
3. **Sensor çıxış gərginliyini ölçmüsünüzmü?** (təzyiq artanda nə qədər volt olur?)

Bu suallara cavab verəndən sonra ən uyğun həlli tətbiq edə bilərəm.
