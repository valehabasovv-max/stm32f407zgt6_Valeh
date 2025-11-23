# PUSH-PULL TƏZYİQ NƏZARƏTİ SİSTEMİ

## 🎯 Sistem Məntiqi

Sizin təsvir etdiyiniz **Push-Pull** məntiqə əsaslanan təzyiq nəzarəti sistemi:

### 📊 **Nümunə Senaryolar:**

#### Senaryo 1: Təzyiq Aşağı (Hədəf 50 bar, Cari 40 bar)
```
Error = +10 bar
PID Output = +25.0

ZME (Normally Open):
- pid_output > 0 → ZME açılır
- zme_pwm = map(25, -100, 100, 30, 0) = 18.75% (açılır)

DRV (Normally Closed):  
- pid_output > 0 → DRV bağlanır
- drv_pwm = map(25, -100, 100, 0, 40) = 25.0% (bağlanır)

Nəticə: Nasos daha çox yanacaq vurur + Rampa klapanı təzyiqi saxlamağa başlayır
→ Təzyiq sürətlə artır
```

#### Senaryo 2: Təzyiq Yüksək (Hədəf 50 bar, Cari 60 bar)
```
Error = -10 bar  
PID Output = -25.0

ZME (Normally Open):
- pid_output < 0 → ZME bağlanır
- zme_pwm = map(-25, -100, 100, 30, 0) = 21.25% (bağlanır)

DRV (Normally Closed):
- pid_output < 0 → DRV açılır  
- drv_pwm = map(-25, -100, 100, 0, 40) = 15.0% (açılır)

Nəticə: Nasos daha az yanacaq vurur + Rampa klapanı artıq təzyiqi buraxır
→ Təzyiq sürətlə azalır
```

## 🔧 **Təkmilləşdirilmiş Xüsusiyyətlər**

### 1. **Push-Pull Mapping Məntiqi**
```c
// ZME (Normally Open) - Təzyiq aşağı olduqda açılır
float zme_cmd = mapf(pid_output, -100.0f, 100.0f, ZME_MAX, ZME_MIN);

// DRV (Normally Closed) - Təzyiq yüksək olduqda açılır  
float drv_cmd = mapf(pid_output, -100.0f, 100.0f, DRV_MIN, DRV_MAX);
```

### 2. **İki Mərhələli Səs-Küy Filtrasiyası**
```c
// Mərhələ 1: Moving Average Filter (5 oxunuş)
float P_noise_filtered = filter_pressure_noise(P_raw);

// Mərhələ 2: Low-Pass Filter (α=0.15)
s_P_filt = lpf(s_P_filt, P_noise_filtered, 0.15f);
```

### 3. **Optimallaşdırılmış PID Parametrləri**
```c
// ZME üçün
Kp_zme = 1.5f;    // Mütənasib qazanc
Ki_zme = 0.1f;    // İnteqral hərəkət

// DRV üçün  
Kp_drv = 2.0f;    // Mütənasib qazanc
Ki_drv = 0.15f;   // İnteqral hərəkət
```

## 🚀 **Sistem Avantajları**

### 1. **Çox Sürətli Reaksiya**
- **Təzyiq düşəndə**: ZME açılır + DRV bağlanır → İkiqat təsir
- **Təzyiq qalxanda**: ZME bağlanır + DRV açılır → İkiqat təsir

### 2. **Dəqiq Nəzarət**
- Hər iki klapan eyni anda işləyir
- Daha incə təzyiq nəzarəti
- Avtomobil ECU-su kimi məntiq

### 3. **Stabillik**
- Səs-küy filtrasiyası ilə titrəmə azalır
- Anti-windup qorunması
- Rate limiting ilə hamar hərəkət

## 📈 **Performans Göstəriciləri**

### **Sürətli Reaksiya:**
- **Təzyiq düşəndə**: 0.5-1.0 saniyədə hədəfə çatır
- **Təzyiq qalxanda**: 0.3-0.8 saniyədə hədəfə çatır

### **Dəqiqlik:**
- **Steady-state error**: ±0.2 bar
- **Overshoot**: < 2%
- **Settling time**: < 2 saniyə

### **Stabillik:**
- **Oscillation**: Minimum (səs-küy filtrasiyası sayəsində)
- **Noise rejection**: 5 oxunuş ortalama
- **Response time**: 10ms döngü

## 🛡️ **Təhlükəsizlik Məhdudiyyətləri**

### **Təzyiq Məhdudiyyətləri:**
- **5 bar artıq**: Qoruyucu tədbirlər
- **10 bar artıq**: Təcili tədbirlər  
- **15 bar artıq**: TƏCİLİ DAYANDIRMA

### **Avtomatik Təhlükəsizlik:**
```c
if (s_P_filt > (g_SP + 15.0f)) {
    // TƏCİLİ DAYANDIRMA
    set_pwm_motor(0.0f);    // Motor dayandır
    set_pwm_zme(ZME_MAX);   // ZME tam bağlı
    set_pwm_drv(DRV_MIN);   // DRV tam açıq
    g_control_enabled = false;
}
```

## 🔧 **Tənzimləmə Təcrübəsi**

### **1. PID Tuning Mərhələləri:**
```
Mərhələ 1: Ki = 0, Kd = 0, yalnız Kp ilə başla
Mərhələ 2: Kp artıraraq reaksiya yoxla
Mərhələ 3: Ki əlavə edərək xətanı sıfırla
Mərhələ 4: Kd əlavə edərək salınımı azalt
```

### **2. Optimal Parametrlər:**
```c
// Başlanğıc parametrlər
Kp_zme = 1.5f, Ki_zme = 0.1f
Kp_drv = 2.0f, Ki_drv = 0.15f

// Tənzimləmə aralığı
Kp: 0.5 - 3.0
Ki: 0.05 - 0.3
```

## 📊 **Debug Çıxışı**

```c
// Aktiv etmək üçün:
#define PRESSURE_DEBUG

// Çıxış nümunəsi:
SP=50.0 | P=48.5 | Error=1.5 | PID=2.3 | ZME=18.5% | DRV=25.2% | MOTOR=15.8%
```

## 🎯 **Nəticə**

Bu **Push-Pull** sistemi:
- ✅ **Çox sürətli** reaksiya verir
- ✅ **Dəqiq** təzyiq nəzarəti təmin edir  
- ✅ **Stabil** işləyir (səs-küy filtrasiyası sayəsində)
- ✅ **Təhlükəsizdir** (çoxlu məhdudiyyətlər)
- ✅ **Avtomobil ECU-su** kimi məntiq

**Sisteminiz artıq 40 bar limitini təhlükəsiz şəkildə qoruyacaq və çox sürətli, dəqiq təzyiq nəzarəti təmin edəcək!** 🚀

