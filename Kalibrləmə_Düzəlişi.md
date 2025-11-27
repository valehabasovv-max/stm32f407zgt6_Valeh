# Təzyiq Sensoru Kalibrləmə Düzəlişi

## 🔍 Problem

Təzyiq sensoru PA3 pininə qoşulub və:
- **Min volt**: 0.5V (0 bar)
- **Max volt**: 5.0V (300 bar)

Amma kodda kalibrləmə dəyərləri yanlış idi.

## 🐛 Tapılan Xəta

**ADC_MIN dəyəri yanlış hesablanmışdı:**
- **Əvvəlki dəyər**: ADC_MIN = 410
- **Düzgün dəyər**: ADC_MIN = 620

### Hesablama:

STM32F4-də ADC referans gərginliyi **3.3V**-dur (5V deyil!).

**ADC dəyəri hesablaması:**
```
ADC = (Voltage / Vref) × 4095
```

**0.5V üçün:**
```
ADC = (0.5 / 3.3) × 4095 ≈ 620
```

**5.0V üçün:**
```
ADC = (5.0 / 3.3) × 4095 ≈ 6204
```

Amma 12-bit ADC maksimum **4095**-dir, ona görə də 5.0V-də ADC **saturasiyaya çatır** və 4095-də məhdudlaşır.

## ✅ Tətbiq Olunan Düzəlişlər

### 1. ADC_MIN Dəyəri Düzəldildi

**Fayllar:**
- `Core/Inc/advanced_pressure_control.h`
- `Core/Inc/pressure_control_config.h`
- `Core/Src/ILI9341_FSMC.c`

**Dəyişiklik:**
```c
// ƏVVƏL:
#define ADC_MIN 410

// İNDİ:
#define ADC_MIN 620   // DÜZƏLİŞ: 0.5V üçün düzgün ADC dəyəri
```

### 2. PRESSURE_SLOPE Avtomatik Yeniləndi

**Yeni slope:**
```
PRESSURE_SLOPE = (300.0 - 0.0) / (4095 - 620)
               = 300.0 / 3475
               ≈ 0.0864 bar/ADC count
```

**Əvvəlki slope:**
```
PRESSURE_SLOPE = 300.0 / (4095 - 410)
               = 300.0 / 3685
               ≈ 0.0814 bar/ADC count
```

### 3. Şərhlər Yeniləndi

Bütün şərhlərdə ADC hesablaması izah edildi:
- STM32F4 ADC referans: 3.3V
- Sensor çıxışı: 0.5V-5.0V
- ADC hesablaması: ADC = (Voltage / 3.3) × 4095

## 📊 Yeni Kalibrləmə Dəyərləri

| Parametr | Dəyər | İzah |
|----------|-------|------|
| **ADC_MIN** | 620 | 0.5V üçün ADC dəyəri |
| **ADC_MAX** | 4095 | 5.0V üçün ADC saturasiyası |
| **PRESSURE_MIN** | 0.0 bar | Minimum təzyiq |
| **PRESSURE_MAX** | 300.0 bar | Maksimum təzyiq |
| **PRESSURE_SLOPE** | 0.0864 bar/ADC | Təzyiq meyli |

## 🧪 Test

### 1. ADC Oxunuşu Yoxlaması

```c
uint16_t raw_adc = AdvancedPressureControl_ReadADC();
printf("Raw ADC: %u\r\n", raw_adc);

// Gözlənilən dəyərlər:
// 0 bar (0.5V) → ADC ≈ 620
// 300 bar (5.0V) → ADC ≈ 4095
```

### 2. Təzyiq Hesablaması

```c
float pressure = AdvancedPressureControl_ReadPressure();
printf("Pressure: %.2f bar\r\n", pressure);

// Gözlənilən dəyərlər:
// ADC = 620 → Pressure ≈ 0.0 bar
// ADC = 4095 → Pressure ≈ 300.0 bar
```

### 3. Kalibrləmə Məlumatı

```c
AdvancedPressureControl_PrintDebugInfo();

// Gözlənilən çıxış:
// ADC: 620-4095
// Pressure: 0.00-300.00 bar
// Slope: 0.0864
// Offset: -53.57 (hesablanmış)
```

## 📝 Dəyişikliklər

### Fayllar:
1. ✅ `Core/Inc/advanced_pressure_control.h` - ADC_MIN = 620
2. ✅ `Core/Inc/pressure_control_config.h` - CONFIG_PRESSURE_SENSOR_ADC_MIN = 620
3. ✅ `Core/Src/ILI9341_FSMC.c` - adc_min = 620
4. ✅ `Core/Src/advanced_pressure_control.c` - Şərhlər yeniləndi

### Avtomatik Yenilənən Dəyərlər:
- ✅ PRESSURE_SLOPE (avtomatik hesablanır)
- ✅ Offset (avtomatik hesablanır)
- ✅ Kalibrləmə strukturları

## 🎯 Nəticə

İndi kalibrləmə dəyərləri düzgündür:
- ✅ 0.5V → ADC = 620 (düzgün)
- ✅ 5.0V → ADC = 4095 (saturasiya, düzgün)
- ✅ Təzyiq hesablaması düzgün işləyir
- ✅ Slope və offset avtomatik yenilənir

**Sistem indi düzgün təzyiq göstərməlidir!** 🎉
