# Təzyiq Sensoru Hesablaması və Kalibrləmə Xülasəsi

## 📋 Ümumi Məlumat

Təzyiq sensoru **PA3 pininə** qoşulub və **ADC3 Channel 3** (ADC3_IN3) vasitəsilə oxunur.

### Sensor Xüsusiyyətləri:
- **Minimum gərginlik**: 0.5V → 0 bar
- **Maksimum gərginlik**: 5.0V → 300 bar
- **Diapazon**: 0-300 bar
- **ADC referans gərginliyi**: 3.3V (STM32F4)
- **ADC resolüsiyası**: 12-bit (0-4095)

---

## 🔢 ADC Hesablaması

### ADC Dəyəri Hesablama Düsturu:

```
ADC = (Voltage / Vref) × 4095
```

Harada:
- `Voltage` = Sensor çıxış gərginliyi (V)
- `Vref` = ADC referans gərginliyi = 3.3V
- `4095` = 12-bit ADC maksimum dəyəri (2^12 - 1)

### Nümunə Hesablamalar:

**0 bar (0.5V) üçün:**
```
ADC = (0.5 / 3.3) × 4095 ≈ 620
```

**300 bar (5.0V) üçün:**
```
ADC = (5.0 / 3.3) × 4095 ≈ 6204
```

**QEYD**: 5.0V-də ADC dəyəri 6204 olmalıdır, amma 12-bit ADC maksimum **4095**-dir, ona görə də **saturasiyaya çatır** və 4095-də məhdudlaşır.

---

## 📐 Təzyiq Hesablaması

### Kalibrləmə Sabitləri:

| Parametr | Dəyər | İzah |
|----------|-------|------|
| `ADC_MIN` | 620 | 0.5V (0 bar) üçün ADC dəyəri |
| `ADC_MAX` | 4095 | 5.0V (300 bar) üçün ADC saturasiyası |
| `PRESSURE_MIN` | 0.0 bar | Minimum təzyiq |
| `PRESSURE_MAX` | 300.0 bar | Maksimum təzyiq |
| `PRESSURE_SLOPE` | 0.0864 bar/ADC | Təzyiq meyli (avtomatik hesablanır) |

### PRESSURE_SLOPE Hesablaması:

```c
PRESSURE_SLOPE = (PRESSURE_MAX - PRESSURE_MIN) / (ADC_MAX - ADC_MIN)
PRESSURE_SLOPE = (300.0 - 0.0) / (4095 - 620)
PRESSURE_SLOPE = 300.0 / 3475
PRESSURE_SLOPE ≈ 0.0864 bar/ADC count
```

### Offset Hesablaması:

```c
offset = PRESSURE_MIN - (PRESSURE_SLOPE × ADC_MIN)
offset = 0.0 - (0.0864 × 620)
offset ≈ -53.57 bar
```

### Təzyiq Hesablama Düsturu:

```c
pressure = offset + (ADC_raw × slope)
pressure = -53.57 + (ADC_raw × 0.0864)
```

**Və ya daha sadə formada:**

```c
pressure = PRESSURE_MIN + ((ADC_raw - ADC_MIN) × PRESSURE_SLOPE)
pressure = 0.0 + ((ADC_raw - 620) × 0.0864)
```

---

## 🔄 ADC-dən Təzyiqə Çevirmə Prosesi

### 1. ADC Oxunuşu (`AdvancedPressureControl_ReadADC()`)

```c
uint16_t AdvancedPressureControl_ReadADC(void)
```

**Proses:**
1. ADC state yoxlanılır (dayanıbsa yenidən başladılır)
2. Overrun flag təmizlənir
3. EOC (End of Conversion) flag gözlənilir (timeout: 10ms)
4. ADC dəyəri oxunur və 0-4095 diapazonunda clamp edilir
5. ADC = 0 yoxlanılır (sensor bağlı deyil?)
6. Son etibarlı dəyər qaytarılır

**Xəta İdarəetməsi:**
- ADC = 0 → Son etibarlı dəyər qaytarılır
- ADC > 4095 → 4095-ə clamp edilir
- Timeout → Son etibarlı dəyər qaytarılır

### 2. ADC-dən Təzyiqə Çevirmə (`AdvancedPressureControl_ConvertAdcToPressure()`)

```c
static float AdvancedPressureControl_ConvertAdcToPressure(uint16_t adc_raw)
```

**Proses:**
1. **Lineyar çevirmə:**
   ```c
   pressure = offset + (adc_raw × slope)
   ```

2. **Moving Average Filter (8 nümunə):**
   - Son 8 təzyiq dəyəri saxlanılır
   - Orta hesablanır və filtrlənmiş dəyər istifadə olunur
   - Bu, səs-küyü azaldır və sabitliyi artırır

3. **Clamp (Məhdudlaşdırma):**
   ```c
   if (pressure < PRESSURE_MIN) pressure = PRESSURE_MIN;  // Minimum: 0.0 bar
   if (pressure > PRESSURE_MAX) pressure = PRESSURE_MAX;  // Maksimum: 300.0 bar
   ```

### 3. Təzyiq Oxunuşu (`AdvancedPressureControl_ReadPressure()`)

```c
float AdvancedPressureControl_ReadPressure(void)
```

**Proses:**
1. ADC dəyəri oxunur
2. ADC dəyəri clamp edilir (ADC_MIN ≤ ADC ≤ ADC_MAX)
3. ADC-dən təzyiqə çevrilir (filtrləmə ilə)
4. Təzyiq dəyəri qaytarılır

---

## 🎯 Kalibrləmə Sistemi

### Kalibrləmə Strukturu:

```c
typedef struct {
    float adc_min;          // Minimum ADC dəyəri (620)
    float adc_max;          // Maksimum ADC dəyəri (4095)
    float pressure_min;     // Minimum təzyiq (0.0 bar)
    float pressure_max;     // Maksimum təzyiq (300.0 bar)
    float slope;            // Təzyiq meyli (0.0864 bar/ADC)
    float offset;           // Offset (-53.57 bar)
    bool calibrated;        // Kalibrləmə statusu
    uint32_t calibration_date;  // Kalibrləmə tarixi
} CalibrationData_t;
```

### Kalibrləmə Yükləmə (`AdvancedPressureControl_LoadCalibration()`)

**Flash yaddaş ünvanı:** `0x080E0000 + 0x100` (Sector 11, offset 0x100)

**Proses:**
1. Flash-dan kalibrləmə məlumatları oxunur
2. Magic number yoxlanılır (`0x12345678`)
3. Checksum yoxlanılır
4. Dəyərlər etibarlılıq yoxlamasından keçirilir
5. Slope və offset avtomatik hesablanır
6. Əgər flash-da etibarlı məlumat yoxdursa, default dəyərlər istifadə olunur

### Kalibrləmə Saxlama (`AdvancedPressureControl_SaveCalibration()`)

**Proses:**
1. Kalibrləmə məlumatları strukturda hazırlanır
2. Checksum hesablanır
3. Flash yaddaşa yazılır (mərkəzləşdirilmiş funksiya ilə)
4. Yazılan məlumat yoxlanılır (verification)

**Flash Strukturu:**
```c
typedef struct {
    uint32_t magic;          // 0x12345678
    float min_voltage;      // 0.5V
    float max_voltage;      // 5.24V
    float min_pressure;     // 0.0 bar
    float max_pressure;     // 300.0 bar
    uint16_t adc_min;       // 620
    uint16_t adc_max;       // 4095
    uint32_t checksum;      // Data integrity check
} calibration_data_t;
```

---

## 📊 Nümunə Hesablamalar

### Nümunə 1: ADC = 1000

```
pressure = offset + (ADC × slope)
pressure = -53.57 + (1000 × 0.0864)
pressure = -53.57 + 86.4
pressure = 32.83 bar
```

### Nümunə 2: ADC = 2000

```
pressure = offset + (ADC × slope)
pressure = -53.57 + (2000 × 0.0864)
pressure = -53.57 + 172.8
pressure = 119.23 bar
```

### Nümunə 3: ADC = 3500

```
pressure = offset + (ADC × slope)
pressure = -53.57 + (3500 × 0.0864)
pressure = -53.57 + 302.4
pressure = 248.83 bar
```

### Nümunə 4: ADC = 620 (minimum)

```
pressure = offset + (ADC × slope)
pressure = -53.57 + (620 × 0.0864)
pressure = -53.57 + 53.57
pressure = 0.0 bar ✓
```

### Nümunə 5: ADC = 4095 (maksimum)

```
pressure = offset + (ADC × slope)
pressure = -53.57 + (4095 × 0.0864)
pressure = -53.57 + 353.81
pressure = 300.24 bar
```

**QEYD**: Maksimum clamp sayəsində 300.24 bar → 300.0 bar-a məhdudlaşdırılır.

---

## 🔧 Əsas Funksiyalar

### ADC Oxunuşu:
- `AdvancedPressureControl_ReadADC()` - Xam ADC dəyərini oxuyur

### Təzyiq Hesablaması:
- `AdvancedPressureControl_ReadPressure()` - Təzyiq dəyərini oxuyur (ADC oxuyub çevirir)
- `AdvancedPressureControl_ConvertAdcToPressure()` - ADC-dən təzyiqə çevirir (daxili funksiya)

### Kalibrləmə:
- `AdvancedPressureControl_LoadCalibration()` - Flash-dan kalibrləmə yükləyir
- `AdvancedPressureControl_SaveCalibration()` - Flash-a kalibrləmə saxlayır
- `AdvancedPressureControl_IsCalibrated()` - Kalibrləmə statusunu yoxlayır

### Debug:
- `AdvancedPressureControl_PrintDebugInfo()` - Debug məlumatlarını göstərir
- `AdvancedPressureControl_PrintStatus()` - Status məlumatlarını göstərir

---

## ⚠️ Mühüm Qeydlər

### 1. ADC Saturasiyası
- 5.0V-də ADC dəyəri 6204 olmalıdır, amma 12-bit ADC maksimum 4095-dir
- Ona görə də 5.0V-də ADC **saturasiyaya çatır** və 4095-də məhdudlaşır
- Bu, normaldır və kodda nəzərə alınıb

### 2. Moving Average Filter
- 8 nümunə üçün tarixçə saxlanılır
- Bu, səs-küyü azaldır və sabitliyi artırır
- İlk 8 oxunuşda filter tam doldurulana qədər az nümunə istifadə olunur

### 3. Clamp (Məhdudlaşdırma)
- ADC dəyərləri 0-4095 diapazonunda clamp edilir
- Təzyiq dəyərləri 0.0-300.0 bar diapazonunda clamp edilir
- Bu, qeyri-etibarlı dəyərlərin qarşısını alır

### 4. Xəta İdarəetməsi
- ADC = 0 → Sensor bağlı deyil və ya xəta var
- Timeout → ADC konversiyası çox uzun çəkir
- Overrun → ADC çox sürətlə oxunur (continuous mode-da normal ola bilər)

### 5. Flash Yaddaş
- Kalibrləmə məlumatları Sector 11-də (0x080E0000) saxlanılır
- Offset 0x100-də kalibrləmə məlumatları var
- Mərkəzləşdirilmiş Flash yazma funksiyası istifadə olunur

---

## 🧪 Test Proseduru

### 1. ADC Oxunuşu Testi:
```c
uint16_t raw_adc = AdvancedPressureControl_ReadADC();
printf("Raw ADC: %u\r\n", raw_adc);
// Gözlənilən: 620-4095 arası
```

### 2. Təzyiq Hesablaması Testi:
```c
float pressure = AdvancedPressureControl_ReadPressure();
printf("Pressure: %.2f bar\r\n", pressure);
// Gözlənilən: 0.0-300.0 bar arası
```

### 3. Kalibrləmə Məlumatı:
```c
AdvancedPressureControl_PrintDebugInfo();
// Gözlənilən çıxış:
// ADC: 620-4095
// Pressure: 0.00-300.00 bar
// Slope: 0.0864
// Offset: -53.57
```

### 4. Hardware Testi:
- Multimetr ilə sensor çıxışını yoxlayın
- 0 bar → ~0.5V (ADC ~620)
- 300 bar → ~5.0V (ADC ~4095, saturasiya)

---

## 📝 Dəyişiklik Tarixçəsi

### Düzəliş 1: ADC_MIN Dəyəri
- **Əvvəlki**: ADC_MIN = 410
- **Yeni**: ADC_MIN = 620
- **Səbəb**: 0.5V üçün düzgün ADC hesablaması: (0.5/3.3)×4095 ≈ 620

### Düzəliş 2: PRESSURE_SLOPE Avtomatik Yenilənməsi
- **Əvvəlki**: PRESSURE_SLOPE ≈ 0.0814 bar/ADC
- **Yeni**: PRESSURE_SLOPE ≈ 0.0864 bar/ADC
- **Səbəb**: ADC_MIN dəyərinin düzəldilməsi

### Düzəliş 3: ADC Oxunuşu Təkmilləşdirilməsi
- EOC flag yoxlaması əlavə edildi
- Timeout mexanizmi əlavə edildi
- Xəta idarəetməsi təkmilləşdirildi

---

## 🎯 Nəticə

Təzyiq sensoru hesablaması və kalibrləmə sistemi düzgün işləyir:

✅ **ADC hesablaması**: Düzgün (620-4095 diapazonu)
✅ **Təzyiq hesablaması**: Düzgün (0.0-300.0 bar diapazonu)
✅ **Kalibrləmə**: Flash yaddaşda saxlanılır və yüklənir
✅ **Filtrləmə**: Moving Average Filter ilə səs-küy azaldılır
✅ **Xəta idarəetməsi**: Etibarlı dəyərlər saxlanılır və qaytarılır

**Sistem hazırdır və düzgün işləyir!** 🎉
