# Kalibrləmə Debug Təlimatı

Bu sənəd kalibrləmə problemini həll etmək üçün addım-addım təlimat verir.

## 🔍 Problemin Təsviri

Təzyiq sensoru kalibrləmə olunmur - 67.1 bar yazır və sıfırlanmır.

## 📋 Debug Addımları

### 1. Serial Port-dan Debug Məlumatlarını Yoxlayın

Sistem başladıqda serial port-dan aşağıdakı məlumatları yoxlayın:

```
Loading pressure sensor calibration from flash...
Flash address: 0x080E1000
Read magic: 0xXXXXXXXX (expected: 0x12345678)
```

**Əgər magic dəyəri 0x12345678 deyilsə:**
- Flash-da kalibrləmə məlumatları yoxdur
- Default kalibrləmə dəyərləri istifadə olunur

**Əgər magic dəyəri 0x12345678-dirsə:**
- Checksum-u yoxlayın
- Əgər checksum uyğun gəlmirsə, flash-da korlanmış məlumat var

### 2. Kalibrləmə Prosesini Test Edin

1. **UI-dan kalibrləmə:**
   - Pressure Sensor Calibration səhifəsinə gedin
   - CAL MIN düyməsinə basın (sıfır təzyiqdə)
   - CAL MAX düyməsinə basın (maksimum təzyiqdə)
   - SAVE düyməsinə basın

2. **Serial port-dan yoxlayın:**
   ```
   Calibration updated from UI - ADC: XXX-XXX, Pressure: X.XX-X.XX bar, Slope: X.XXXXXX, Offset: X.XX
   Saving pressure sensor calibration to flash...
   DEBUG: Saving calibration - ADC: XXX-XXX, Pressure: X.XX-X.XX bar
   AdvancedPressureControl: Calibration saved and verified - ADC: XXX-XXX, Pressure: X.XX-X.XX bar
   ```

### 3. Sistem Yenidən Başladıqda Yoxlayın

Sistem yenidən başladıqda serial port-dan yoxlayın:

```
DEBUG: Calibration values - ADC: XXX-XXX, Pressure: X.XX-X.XX bar, Slope: X.XXXXXX, Offset: X.XX
```

**Əgər dəyərlər düzgündürsə:**
- Kalibrləmə düzgün yüklənib
- Problem təzyiq hesablamasında ola bilər

**Əgər dəyərlər default-dursa:**
- Flash-dan kalibrləmə oxunmur
- Flash-a yazma problemi ola bilər

### 4. Təzyiq Hesablamasını Yoxlayın

Hər 100 çağırışda bir dəfə serial port-dan yoxlayın:

```
DEBUG: ADC=XXX, Clamped=XXX, Pressure=X.XX bar (Offset=X.XX, Slope=X.XXXXXX)
```

**Əgər ADC dəyəri düzgündürsə, amma təzyiq düzgün deyilsə:**
- Slope və ya offset düzgün hesablanmır
- Kalibrləmə dəyərləri düzgün tətbiq olunmur

## 🛠️ Həll Yolları

### Həll 1: Flash Sektoru Silin

Əgər flash-da korlanmış məlumat varsa, sektoru silin:

1. STM32CubeProgrammer istifadə edin
2. Flash Sector 11-i silin (0x080E0000 - 0x080FFFFF)
3. Sistem yenidən başladın
4. Kalibrləməni yenidən edin

### Həll 2: Kalibrləməni Manuel Test Edin

1. UI-dan kalibrləmə edin
2. Serial port-dan yoxlayın ki, kalibrləmə dəyərləri düzgün yazılıb
3. Sistem yenidən başladın
4. Serial port-dan yoxlayın ki, kalibrləmə dəyərləri düzgün yüklənib

### Həll 3: Default Kalibrləmə Dəyərlərini Yoxlayın

Əgər flash-da kalibrləmə yoxdursa, default dəyərlər istifadə olunur:
- ADC_MIN = 500
- ADC_MAX = 3500
- PRESSURE_MIN = 0.2 bar
- PRESSURE_MAX = 314.6 bar
- Slope = 0.1048 bar/ADC count
- Offset = 0.2 - (0.1048 * 500) = -52.2 bar

**QEYD:** Default offset mənfi ola bilər, bu normaldır.

## 📊 Debug Məlumatları

Debug məlumatları serial port-dan göstərilir:
- İlk çağırışda kalibrləmə dəyərləri
- Hər 100 çağırışda bir dəfə ADC və təzyiq dəyərləri
- Kalibrləmə yazılanda və yüklənəndə məlumatlar

## 🎯 Nəticə

Əgər problem davam edirsə, serial port-dan debug məlumatlarını göndərin ki, daha dəqiq diaqnoz qura bilək.

