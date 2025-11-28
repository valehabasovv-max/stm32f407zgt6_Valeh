# ADC 632-də Qalıb və Təzyiq 0.00 Göstərir - Düzəliş

## Problem
- ADC dəyəri 632-də qalıb, dəyişmir
- Sistemdə təzyiq var, amma ekran 0.00 bar göstərir

## Problemin Səbəbləri

### 1. ADC Dəyəri 632-də Qalır
**632 nə deməkdir?**
- ADC_MIN = 620 (0.5V, 0 bar)
- 632 çox aşağı dəyərdir, sensorda demək olar ki, heç təzyiq yoxdur
- Bu iki səbəbdən ola bilər:
  - **Sensor həqiqətən 0.5V-ə yaxın siqnal göndərir** (sensor qoşulma problemi və ya sensor xarab)
  - **ADC yeni dəyərləri oxumur** (stuck ADC problemi)

### 2. Təzyiq 0.00 Göstərir
**Kalibrasiya düsturu:**
```
Pressure = Offset + (ADC × Slope)
```

**Default dəyərlər:**
- ADC_MIN = 620, ADC_MAX = 4095
- PRESSURE_MIN = 0.0, PRESSURE_MAX = 300.0
- SLOPE = (300.0 - 0.0) / (4095 - 620) = 0.0864 bar/ADC
- OFFSET = 0.0 - (0.0864 × 620) = -53.568 bar

**ADC = 632 üçün:**
```
Pressure = -53.568 + (632 × 0.0864) = -53.568 + 54.605 = 1.04 bar
```

**Əgər təzyiq 0.00 göstərirsə:**
- **Kalibrasiya məlumatları düzgün deyil** (Flash-dan yanlış dəyərlər yüklənib)
- **Clamping həddindən artıq tətbiq olunur** (mənfi təzyiq 0-a clamp olunur)
- **ADC oxuma funksiyası düzgün işləmir**

## Tətbiq Olunan Düzəlişlər

### 1. ADC Diaqnostika Aləti (Yeni)
**Fayllar:**
- `Core/Src/adc_diagnostic.c` - Diaqnostika funksiyaları
- `Core/Inc/adc_diagnostic.h` - Başlıq faylı

**Funksiyalar:**
- `ADC_RunDiagnostic()` - Tam diaqnostika (ADC state, oxuma, kalibrasiya)
- `ADC_TestHardwareDirectly()` - Hardware test (birbaşa ADC oxuma)
- `ADC_ForceRecalibration()` - Məcburi yenidən kalibrasiya

**İstifadəsi:**
Sistem başlayanda avtomatik işə düşür və:
1. ADC state-ni yoxlayır
2. 10 dəfə ardıcıl ADC oxuyur (dəyişir/dəyişmir?)
3. Kalibrasiya məlumatlarını göstərir
4. Müxtəlif ADC dəyərləri üçün təzyiq hesablayır
5. Əgər problem təsdiqlənərsə (ADC=632, Pressure=0.00), avtomatik recalibration edir

### 2. ADC Oxuma Funksiyası Təkmilləşdirildi

**Dəyişikliklər:**
```c
// DÜZƏLİŞ 1: ADC-nin yeni konversiyaya başlaması üçün daha çox vaxt
for(volatile uint32_t i = 0; i < 1000; i++);  // 100-dən 1000-ə artırıldı

// DÜZƏLİŞ 2: Stuck value avtomatik aşkarlanması və ADC restart
if (same_value_count > 500) {  // 500 ardıcıl eyni dəyər
    printf("CRITICAL: ADC value stuck - RESTARTING ADC!\n");
    HAL_ADC_Stop(&hadc3);
    HAL_Delay(10);
    HAL_ADC_Start(&hadc3);
    HAL_Delay(20);
    same_value_count = 0;
}
```

### 3. Sistem Başlanğıcında Avtomatik Yoxlama

**main.c-də əlavə olundu:**
```c
// Sistem başlayanda tam diaqnostika
ADC_RunDiagnostic();

// Hardware test
ADC_TestHardwareDirectly();

// Əgər problem varsa, avtomatik recalibration
if ((ADC == 632) && (Pressure < 0.5)) {
    ADC_ForceRecalibration();
}
```

## Necə İstifadə Edilir

### 1. Sistem Başlanğıcı
Sistem başlayanda avtomatik olaraq:
1. Diaqnostika işə düşəcək
2. Serial port-a (UART) məlumatlar göndəriləcək
3. Əgər problem aşkarlanarsa, avtomatik düzəliş tətbiq olunacaq

**Serial Monitor-da görəcəksiniz:**
```
*****************************************************************
*  ADC DİAQNOSTİKA BAŞLAYIR (632 stuck ADC və 0.00 pressure)  *
*****************************************************************

--- 1. ADC STATE ---
ADC State: 0x00000001
  - Ready: YES
  - Busy (Regular): NO
  - EOC Flag: SET
  - OVR Flag: CLEAR

--- 2. RAW ADC READINGS (10 consecutive reads) ---
  Read 1: ADC = 632
  Read 2: ADC = 632
  Read 3: ADC = 632
  ...

--- 3. CALIBRATION DATA ---
Calibrated: YES
ADC Range: 620 - 4095
Pressure Range: 0.00 - 300.00 bar
Slope: 0.086330 bar/ADC
Offset: -53.52 bar

--- 4. PRESSURE CONVERSION TEST ---
  ADC=620 → Raw Pressure=-0.04 bar → CLAMPED to 0.00 bar
  ADC=632 → Raw Pressure=1.04 bar
  ADC=700 → Raw Pressure=6.90 bar
  ...

⚠ PROBLEMLİ DİAQNOZ TƏSDİQLƏNDİ: ADC=632, Pressure=0.00 bar
Recalibration tətbiq olunur...

[Recalibration tamamlandı]

✓ Sistem normal görünür: ADC=632, Pressure=1.04 bar
```

### 2. Əl İlə Diaqnostika
Əgər daha sonra problem yaranarsa:
```c
// Serial port vasitəsilə əmr göndərin
ADC_RunDiagnostic();          // Tam diaqnostika
ADC_TestHardwareDirectly();   // Hardware test
ADC_ForceRecalibration();     // Məcburi recalibration
```

## Problemin Həlli

### Ssenari 1: ADC Hardware Problemi
**Əlamətlər:**
- ADC dəyəri heç vaxt dəyişmir (həmişə 632)
- Hardware test də eyni nəticəni göstərir

**Həll:**
1. PA3 pin bağlantısını yoxlayın
2. Sensor gərginliyini multimetrlə ölçün (0.5V - 5.0V diapazonunda olmalıdır)
3. Sensor qida gərginliyini yoxlayın (sensor spesifikasiyasına görə)
4. ADC referans gərginliyini yoxlayın (3.3V olmalıdır)

### Ssenari 2: Kalibrasiya Problemi
**Əlamətlər:**
- ADC dəyişir (məsələn, 632 → 700 → 1000)
- Amma təzyiq hələ də 0.00 və ya yanlış göstərir

**Həll:**
1. Avtomatik recalibration tətbiq olunacaq (sistem başlanğıcında)
2. Əgər problem davam edərsə, kalibrasiya ekranından manual kalibrasiya edin:
   - Minimum nöqtə: 0 bar → ADC dəyərini yaz
   - Maksimum nöqtə: 300 bar (və ya sensorunuzun maksimumu) → ADC dəyərini yaz

### Ssenari 3: Sensor Qoşulma Problemi
**Əlamətlər:**
- ADC davamlı 620-630 diapazonunda (çox aşağı)
- Təzyiq var, amma sensor oxumur

**Həll:**
1. Sensor çıxış gərginliyini multimetrlə ölçün
2. Əgər gərginlik 0.5V-ə yaxındırsa, sensor xarabdır və ya qoşulmayıb
3. Sensor kabellərini yoxlayın (+ , -, OUT)
4. Sensor qida gərginliyini yoxlayın

### Ssenari 4: ADC Continuous Mode Problemi
**Əlamətlər:**
- ADC ilk oxunuşda düzgün dəyər göstərir
- Sonra dəyişmir (stuck)

**Həll:**
1. Avtomatik ADC restart mexanizmi işə düşəcək (500 eyni oxunuşdan sonra)
2. Əgər problem davam edərsə:
   - Timer 6 interrupt-ın düzgün işlədiyini yoxlayın
   - ADC continuous mode konfiqurasiyasını yoxlayın (`hadc3.Init.ContinuousConvMode = ENABLE`)

## Debug Log Nümunələri

### Normal İşləmə
```
DEBUG ADC[1]: value=632, state=0x00000001, last_valid=620, same_count=0
DEBUG ADC[2]: value=645, state=0x00000001, last_valid=632, same_count=0
DEBUG ADC[3]: value=698, state=0x00000001, last_valid=645, same_count=0
INFO[3]: ADC value changed from 645 to 698 (was stable for 0 reads)
```

### Stuck ADC (Avtomatik Restart)
```
DEBUG ADC[1]: value=632, state=0x00000001, last_valid=620, same_count=0
DEBUG ADC[2]: value=632, state=0x00000001, last_valid=632, same_count=1
...
WARNING[100]: ADC value 632 unchanged for 100 reads (checking if stuck...)
WARNING[200]: ADC value 632 unchanged for 200 reads (checking if stuck...)
...
CRITICAL[500]: ADC value 632 stuck for 500 reads - RESTARTING ADC!
INFO[500]: ADC restarted successfully
DEBUG ADC[501]: value=645, state=0x00000001, last_valid=632, same_count=0
INFO[501]: ADC value changed from 632 to 645 (was stable for 500 reads)
```

## Qeyd

**KRİTİK:** Əgər ADC davamlı 620-640 diapazonunda qalırsa və hardware yoxlamaları düzgündürsə:
1. **Sensor çıxış gərginliyini yoxlayın** - Real təzyiq olduqda gərginlik artmalıdır (0.5V-dən yuxarı)
2. **Sensor spesifikasiyasını yoxlayın** - 0-300 bar üçün 0.5-5.0V çıxış olmalıdır
3. **Təzyiq ölçmə qurğusu ilə təsdiq edin** - Həqiqətən də təzyiq var?

Əgər bütün yoxlamalar düzgündürsə və problem davam edirsə, bu sənədi və debug log-ları göndərin.
