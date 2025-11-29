# ADC və Təzyiq Problemi - Tez Həll Yolu

## Problem: ADC 632, Təzyiq 0.00

### Addım 1: Serial Monitor-u Açın
Sistem başlayanda avtomatik diaqnostika işə düşür. Serial monitor-da (115200 baud) nəticələri görün.

### Addım 2: ADC Dəyərlərini Yoxlayın
```
--- 2. RAW ADC READINGS (10 consecutive reads) ---
  Read 1: ADC = 632
  Read 2: ADC = 632   ← BU PROBLEMDİR! Dəyər dəyişmir
  Read 3: ADC = 632
  ...
```

**Əgər ADC dəyişirsə (məsələn, 632 → 645 → 698):**
- ADC hardware düzgündür
- Problem kalibrasiyadadır
- Avtomatik recalibration tətbiq olunacaq

**Əgər ADC dəyişmirsə (həmişə 632):**
- ADC hardware problemi
- Sensor qoşulma problemi
- Addım 3-ə keçin

### Addım 3: Hardware Yoxlaması

#### Yoxlama 1: PA3 Pin Gərginliyi
1. Multimetr götürün
2. PA3 pin-ə qoşun (ADC3 girişi)
3. Gərginliyi ölçün:
   - **0.5V və ya aşağı:** Sensor qoşulmayıb və ya xarab
   - **0.5V - 5.0V arası:** Sensor işləyir, ADC problemi
   - **5.0V-dən yuxarı:** Sensor overflow və ya yanlış qoşulma

#### Yoxlama 2: Sensor Qida Gərginliyi
1. Sensor spesifikasiyasına baxın (məsələn, 24V, 12V, 5V)
2. Sensor qida pin-lərini yoxlayın
3. Gərginliyin düzgün olduğunu təsdiq edin

#### Yoxlama 3: Sensor Çıxış Siqnalı
1. Sistemdə təzyiq yaradın (məsələn, 50 bar)
2. Sensor çıxış gərginliyini ölçün
3. **Gözləntilər:**
   - 0 bar → 0.5V
   - 50 bar → ~1.25V
   - 150 bar → ~2.75V
   - 300 bar → 5.0V

**Əgər gərginlik artmırsa:**
- Sensor xarab
- Sensor kabel qırılıb
- Sensor növü düzgün deyil (0-10V və ya 4-20mA sensorlar fərqlidir)

### Addım 4: Kalibrasiya Yoxlaması

Serial monitor-da:
```
--- 3. CALIBRATION DATA ---
Calibrated: YES
ADC Range: 620 - 4095
Pressure Range: 0.00 - 300.00 bar
Slope: 0.086330 bar/ADC
Offset: -53.52 bar
```

**Düzgün dəyərlər:**
- ADC Range: 620 - 4095 ✓
- Pressure Range: 0.0 - 300.0 bar ✓
- Slope: ~0.0864 ✓
- Offset: ~-53.5 ✓

**Əgər dəyərlər fərqlidirsə:**
- Flash-dan yanlış kalibrasiya yüklənib
- Avtomatik recalibration tətbiq olunacaq (sistem başlanğıcında)

### Addım 5: Təzyiq Konversiyası Testi

```
--- 4. PRESSURE CONVERSION TEST ---
  ADC=620 → Raw Pressure=-0.04 bar → CLAMPED to 0.00 bar
  ADC=632 → Raw Pressure=1.04 bar    ← BU DÜZGÜNDÜR
  ADC=700 → Raw Pressure=6.90 bar
  ADC=1000 → Raw Pressure=32.83 bar
```

**ADC=632 üçün gözləntilər:**
- Raw Pressure: ~1.0 bar
- Clamped: 1.0 bar (clamp olunmamalıdır)

**Əgər ADC=632 və Pressure=0.00:**
- Kalibrasiya düzgün deyil
- Avtomatik recalibration çağırın: `ADC_ForceRecalibration()`

## Tez Həll Kodları

### Kod 1: Stuck ADC (Dəyişmir)
**Diaqnoz:** ADC həmişə 632 göstərir
**Səbəb:** 
- Hardware problemi
- Sensor qoşulmayıb
- ADC continuous mode işləmir

**Həll:**
1. Hardware yoxlaması (PA3 gərginliyi)
2. ADC avtomatik restart (500 eyni oxunuşdan sonra)
3. ADC konfigurasiyanı yoxla (`ContinuousConvMode = ENABLE`)

### Kod 2: Yanlış Təzyiq Hesablaması
**Diaqnoz:** ADC dəyişir, amma təzyiq yanlış
**Səbəb:**
- Kalibrasiya yanlış
- Slope/Offset düzgün deyil

**Həll:**
```c
// Serial port-da çağırın
ADC_ForceRecalibration();
```

### Kod 3: ADC 620-640 Arasında (Çox Aşağı)
**Diaqnoz:** ADC davamlı 620-640 diapazonunda
**Səbəb:**
- Təzyiq həqiqətən 0-1 bar arasındadır
- Sensor çıxış gərginliyi 0.5V-ə yaxındır

**Həll:**
1. **Multimetrlə sensor çıxışını ölçün**
2. Əgər gərginlik 0.5V-ə yaxındırsa:
   - Sistemdə təzyiq yoxdur
   - Motor işləmir
   - ZME tam bağlıdır
3. Əgər gərginlik daha yüksəkdirsə (məsələn, 2.0V):
   - ADC3 girişində problem var
   - PA3 pin kontaktı zəifdir

## Avtomatik Düzəlişlər

Sistem aşağıdakı avtomatik düzəlişləri tətbiq edir:

### 1. Başlanğıcda Diaqnostika
```
Sistem başlayır
  ↓
ADC_RunDiagnostic()
  ↓
Problem aşkarlanır?
  ↓ (Bəli)
ADC_ForceRecalibration()
  ↓
Yenidən yoxla
  ↓
Problem həll olundu? → Davam et
```

### 2. Runtime ADC Restart
```
ADC oxunur (10ms-də bir)
  ↓
Eyni dəyər?
  ↓ (Bəli, 500 dəfə)
ADC_Stop() → ADC_Start()
  ↓
Dəyər dəyişdi? → Normal işləməyə qayıt
```

## Log Mesajları Tərcüməsi

| Log Mesajı | Mənası | Nə Etmək |
|------------|--------|----------|
| `ADC value 632 unchanged for 100 reads` | ADC 100 dəfə eyni dəyər oxuyur | Gözlə, 500-də avtomatik restart |
| `CRITICAL: ADC value stuck - RESTARTING ADC!` | ADC stuck, avtomatik restart | Normal, restart olunur |
| `ADC restarted successfully` | ADC restart uğurlu | Yaxşı |
| `ADC value changed from 632 to 645` | ADC düzəldi | Problem həll olundu |
| `⚠ PROBLEMLİ DİAQNOZ TƏSDİQLƏNDİ` | Problem təsdiqləndi | Avtomatik recalibration başlayır |
| `✓ Sistem normal görünür` | Sistem düzgün işləyir | Problem həll olundu |

## Əlaqə və Dəstək

Əgər bu addımlar köməklik etmirsə:
1. Serial monitor log-larını yadda saxlayın
2. Hardware ölçmə nəticələrini (multimetr) qeyd edin
3. Sensor spesifikasiyasını (model, diapazon, çıxış tipi) göndərin

**Debug məlumatı toplama:**
```c
// Serial monitor-da bu əmrləri çağırın
ADC_RunDiagnostic();           // Tam diaqnostika
ADC_TestHardwareDirectly();    // Hardware test
// Log-ları yadda saxlayın və göndərin
```
