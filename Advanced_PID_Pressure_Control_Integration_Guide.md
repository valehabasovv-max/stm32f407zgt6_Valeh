# Advanced PID Pressure Control System - Integration Guide

## Valeh Injection System - Complete Control Solution

Bu sənəd STM32F407ZGT6 üçün hazırlanmış tam PID əsaslı təzyiq idarəetmə sisteminin inteqrasiyası üçün ətraflı rehberdir.

## 📋 Sistem Xüsusiyyətləri

### ✅ Tamamlanmış Funksiyalar
- **PID İdarəetmə**: ZME və DRV klapanları üçün dəqiq PID nəzarəti
- **Motor Sürət İdarəetməsi**: Təzyiq limitinə əsaslanan avtomatik motor sürəti tənzimləməsi
- **Təhlükəsizlik Sistemi**: Çox mərhələli təhlükəsizlik məntiqi və təcili dayandırma
- **Kalibrasiya Sistemi**: Sensor kalibrasiyası və parametr tənzimləməsi
- **Real-time Monitoring**: Həqiqi zamanlı status izləmə və debug çıxışı
- **Konfiqurasiya İdarəetməsi**: Flash yaddaşda parametr saxlanması

## 🏗️ Sistem Arxitekturası

### Fayl Strukturu
```
Core/
├── Inc/
│   ├── advanced_pressure_control.h      # Əsas PID idarəetmə sistemi
│   └── pressure_control_config.h        # Konfiqurasiya və kalibrasiya
├── Src/
│   ├── advanced_pressure_control.c      # PID idarəetmə implementasiyası
│   ├── pressure_control_config.c        # Konfiqurasiya implementasiyası
│   └── main.c                          # Ana proqram (yenilənmiş)
```

### Komponentlər
1. **Advanced Pressure Control System** - Əsas PID idarəetmə məntiqi
2. **Configuration Management** - Parametr və kalibrasiya idarəetməsi
3. **Safety System** - Təhlükəsizlik və təcili dayandırma
4. **Hardware Interface** - STM32 HAL inteqrasiyası

## 🔧 İnteqrasiya Addımları

### 1. Faylları Proyektə Əlavə Et
```bash
# Faylları STM32 proyektinizin Core/Inc və Core/Src qovluqlarına kopyalayın
Core/Inc/advanced_pressure_control.h
Core/Inc/pressure_control_config.h
Core/Src/advanced_pressure_control.c
Core/Src/pressure_control_config.c
```

### 2. Main.c Yeniləmələri
`main.c` faylı artıq yenilənib və aşağıdakı dəyişikliklər əlavə edilib:

```c
// Include əlavə edildi
#include "advanced_pressure_control.h"

// Timer 6 əlavə edildi (10ms control loop üçün)
TIM_HandleTypeDef htim6;

// Timer 6 konfiqurasiyası əlavə edildi
static void MX_TIM6_Init(void);

// Timer kəsilməsi handler əlavə edildi
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

// Sistem başlatma əlavə edildi
AdvancedPressureControl_Init();
HAL_TIM_Base_Start_IT(&htim6);
```

### 3. Timer 6 Konfiqurasiyası
Timer 6 10ms (100Hz) tezlikdə işləyir:
- **Prescaler**: 8399 (84MHz / 8400 = 10kHz)
- **Period**: 99 (10kHz / 100 = 100Hz)
- **Interrupt**: Aktiv

## 🎛️ İstifadə Rehberi

### Sistem Başlatma
```c
// Sistem başlatma
AdvancedPressureControl_Init();

// Hədəf təzyiq təyin et
AdvancedPressureControl_SetTargetPressure(70.0f); // 70 bar

// İdarəetməni aktivləşdir
SystemStatus_t* status = AdvancedPressureControl_GetStatus();
status->control_enabled = true;
```

### PID Parametrlərini Tənzimləmə
```c
// ZME PID parametrlərini təyin et
PressureControlConfig_SetPIDTuning(&g_pid_zme_tuning, 1.5f, 0.005f, 0.0f);

// Real-time parametr tənzimləməsi
PressureControlConfig_AdjustKP(0.1f);  // Kp artır
PressureControlConfig_AdjustKI(0.001f); // Ki artır
```

### Kalibrasiya Prosesi
```c
// Kalibrasiya başlat
PressureControlConfig_StartCalibration();

// Kalibrasiya nöqtələri əlavə et
PressureControlConfig_AddCalibrationPoint(500, 0.0f);    // 0 bar
PressureControlConfig_AddCalibrationPoint(3500, 314.6f); // 314.6 bar

// Kalibrasiyanı tamamla
PressureControlConfig_CompleteCalibration();
```

### Təhlükəsizlik Sistemi
```c
// Təhlükəsizlik limitlərini təyin et
PressureControlConfig_SetSafetyLimits(300.0f, 10.0f, 350.0f);

// Təcili dayandırma
AdvancedPressureControl_EmergencyStop();
```

## 📊 Sistem Parametrləri

### PID Parametrləri (İlkin Dəyərlər)
- **ZME Kp**: 1.5 (Mütənasib qazanc)
- **ZME Ki**: 0.005 (İnteqral qazanc)
- **ZME Kd**: 0.0 (Diferensial qazanc)
- **DRV**: İstifadə edilmir (0.0)

### Klapan Limitləri
- **ZME PWM**: 0% - 30% (0% açıq, 30% bağlı)
- **DRV PWM**: 0% - 40% (0% açıq, 40% bağlı)
- **Motor PWM**: 0% - 100%

### Təhlükəsizlik Limitləri
- **Maksimum Təzyiq**: 300 bar
- **Over-limit Marjası**: 10 bar
- **Təcili Dayandırma**: 350 bar

## 🔍 Debug və Monitoring

### Status İzləmə
```c
// Cari statusu al
SystemStatus_t* status = AdvancedPressureControl_GetStatus();

// Debug məlumatlarını çap et
AdvancedPressureControl_PrintDebugInfo();

// Konfiqurasiya məlumatlarını çap et
PressureControlConfig_PrintCurrentConfig();
```

### Real-time Çıxış
Sistem hər 10ms-də aşağıdakı məlumatları çap edir:
```
SP:70.0  P:68.5  ERR:1.5  PID:2.3  ZME:15.2%  DRV:8.5%  MOTOR:75.0%
```

## ⚙️ Konfiqurasiya Seçənəkləri

### Sistem Konfiqurasiyası
```c
// Debug rejimini aktivləşdir/deaktivləşdir
PressureControlConfig_SetDebugMode(true);

// Avtomatik rejimi aktivləşdir/deaktivləşdir
PressureControlConfig_SetAutoMode(true);

// Təhlükəsizlik sistemini aktivləşdir/deaktivləşdir
PressureControlConfig_EnableSafety(true);
```

### Klapan Konfiqurasiyası
```c
// ZME limitlərini təyin et
PressureControlConfig_SetZMELimits(0.0f, 30.0f, 2.0f);

// DRV limitlərini təyin et
PressureControlConfig_SetDRVLimits(0.0f, 40.0f);

// Motor limitlərini təyin et
PressureControlConfig_SetMotorLimits(0.0f, 100.0f);
```

## 🚨 Təhlükəsizlik Xüsusiyyətləri

### 1. Təzyiq Limit Yoxlaması
- Təzyiq hədəf + 10 bar-ı keçərsə təhlükəsizlik rejimi aktivləşir
- DRV açılır (0%), ZME bağlanır (30%), Motor dayandırılır

### 2. Maksimum Təzyiq Limit
- Təzyiq 300 bar-ı keçərsə təcili dayandırma
- Bütün sistem təhlükəsiz vəziyyətə gətirilir

### 3. Təcili Dayandırma
- Təzyiq 350 bar-ı keçərsə təcili dayandırma
- Bütün aktuatorlar təhlükəsiz vəziyyətə gətirilir

## 📈 Performans Optimallaşdırması

### PID Tənzimləməsi
1. **Kp Artırma**: Daha sürətli cavab, amma daha çox osilasiya
2. **Ki Artırma**: Sabit xətanı azaldır, amma overshoot artırır
3. **Kd Artırma**: Osilasiyanı azaldır, amma səs-küyə həssasdır

### ZME Qeyri-xəttiliyi
Sistem ZME-nin qeyri-xətti davranışını avtomatik kompensasiya edir:
- Minimum işlək threshold: 1.0%
- Kompensasiya faktoru: 1.0% üçün hesablanır

## 🔧 Xəta Aradan Qaldırma

### Ümumi Problemlər
1. **Sistem işləmir**: Timer 6-nın aktiv olduğunu yoxlayın
2. **Təzyiq oxunmur**: ADC3 konfiqurasiyasını yoxlayın
3. **Klapanlar işləmir**: PWM kanallarının aktiv olduğunu yoxlayın
4. **PID işləmir**: Kalibrasiyanın tamamlandığını yoxlayın

### Debug Məlumatları
```c
// Sistem statusunu yoxla
AdvancedPressureControl_PrintDebugInfo();

// Kalibrasiya statusunu yoxla
PressureControlConfig_PrintCalibrationStatus();

// Tuning statusunu yoxla
PressureControlConfig_PrintTuningStatus();
```

## 📝 Nümunə Kod

### Tam İnteqrasiya Nümunəsi
```c
int main(void) {
    // STM32 başlatma
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_ADC3_Init();
    MX_TIM3_Init();
    MX_TIM6_Init();
    
    // PWM kanallarını başlat
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1); // Motor
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2); // DRV
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3); // ZME
    
    // Timer 6-nı başlat (10ms control loop)
    HAL_TIM_Base_Start_IT(&htim6);
    
    // Advanced Pressure Control System başlat
    AdvancedPressureControl_Init();
    
    // Hədəf təzyiq təyin et
    AdvancedPressureControl_SetTargetPressure(70.0f);
    
    // İdarəetməni aktivləşdir
    SystemStatus_t* status = AdvancedPressureControl_GetStatus();
    status->control_enabled = true;
    
    // Ana dövr
    while (1) {
        // Timer kəsilməsində avtomatik çağırılır
        // AdvancedPressureControl_TimerCallback();
        
        HAL_Delay(50);
    }
}

// Timer kəsilməsi handler
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM6) {
        AdvancedPressureControl_TimerCallback();
    }
}
```

## 🎯 Nəticə

Bu sistem sizin təklif etdiyiniz bütün tələbləri qarşılayır:

✅ **ZME və DRV idarəetməsi** - PID əsaslı dəqiq idarəetmə
✅ **Motor sürət idarəetməsi** - Təzyiq limitinə əsaslanan avtomatik tənzimləmə
✅ **Təhlükəsizlik sistemi** - Çox mərhələli təhlükəsizlik məntiqi
✅ **Kalibrasiya sistemi** - Sensor kalibrasiyası və parametr tənzimləməsi
✅ **Real-time monitoring** - Həqiqi zamanlı status izləmə
✅ **STM32 inteqrasiyası** - Tam HAL kitabxanası dəstəyi

Sistem hazırdır və dərhal istifadəyə başlaya bilərsiniz!




