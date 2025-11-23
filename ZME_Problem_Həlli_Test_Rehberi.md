# ZME PROBLEM HƏLLİ - TEST REHBERİ

## 🚨 **PROBLEM TƏHLİLİ**

### **Əsas Problem:**
- **"Cari təzyiq 30 bar olduqda təzyiq buraxılır 0.00 bara düşür"**
- Bu, ZME-nin qeyri-xətti (non-linear) davranışını göstərir
- 0%-dən 1%-ə keçid zamanı təzyiq qəfil sıfıra düşür

### **Səbəb:**
ZME-nin fiziki cavabı 0% PWM-də yanacağın çox keçməsinə icazə verir, lakin 1% və ya 2% PWM-də o, yanacağı tamamilə kəsir.

## 🔧 **HƏLL EDİLƏN PROBLEMLƏR**

### **1. ZME Qeyri-Xətti Davranış Kompensasiyası**
```c
// Yeni funksiya: ZME_CompensateNonlinearity()
// Bu funksiya ZME-nin qeyri-xətti davranışını kompensasiya edir
float ZME_CompensateNonlinearity(float desired_pwm);
```

### **2. Yeni Təhlükəsizlik Məntiqi**
```c
// Limit + 10 bar təhlükəsizlik qaydası
if (s_P_filt > (g_SP + 10.0f)) {
    // DRV-ni qəfil aç (0.0% PWM-ə - tam açıq)
    drv_cmd = DRV_MIN; // 0.0%
    
    // ZME-ni bağla (30.0% PWM-ə - tam bağlı)
    zme_cmd = ZME_MAX; // 30.0%
    
    // Motoru dayandır
    motor_cmd = 0.0f;
    
    // PID-nin inteqral hissəsini sıfırla
    I_zme = 0.0f;
    I_drv = 0.0f;
    
    return; // PID-nin qalan hissəsini keç
}
```

### **3. ZME Test Funksiyaları**
```c
// ZME-nin minimum işlək diapazonunu tapmaq üçün
float ZME_FindMinimumWorkingThreshold(void);

// Manual rejimdə ZME testi
void ZME_ManualTest(void);
```

## 🧪 **TEST PROSEDURLARI**

### **Test 1: ZME Manual Test**
1. **Ekranda sağ alt düyməyə toxunun** (200,150 - 300,200 koordinatları)
2. **ZME_ManualTest() funksiyası işə düşəcək**
3. **Test nəticələrini izləyin:**
   - ZME PWM 0.0%-dən 3.0%-ə qədər test ediləcək
   - Hər PWM dəyəri üçün 3 saniyə gözləyiləcək
   - Təzyiqin dəyişməsi izlənəcək
   - Problemli PWM dəyərləri müəyyən ediləcək

### **Test 2: ZME Minimum Threshold Test**
```c
// Kodda çağırın:
float min_threshold = ZME_FindMinimumWorkingThreshold();
printf("ZME minimum işlək PWM: %.1f%%\r\n", min_threshold);
```

### **Test 3: Təhlükəsizlik Məntiqi Test**
1. **Təzyiq limitini 50 bar təyin edin**
2. **Sistemi işə salın**
3. **Təzyiqin 60 bar-ı keçməsini gözləyin**
4. **Təhlükəsizlik rejiminin işə düşməsini yoxlayın:**
   - ZME = 30% (tam bağlı)
   - DRV = 0% (tam açıq)
   - Motor = 0% (dayandır)

## 📊 **GÖZLƏNİLƏN NƏTİCƏLƏR**

### **ZME Test Nəticələri:**
```
ZME TEST: PWM=0.0% test edilir...
ZME TEST: PWM=0.0%, P_before=25.0, P_after=28.5, Change=3.5
ZME TEST: PWM=0.0% işləyir! (Təzyiq artır)

ZME TEST: PWM=0.5% test edilir...
ZME TEST: PWM=0.5%, P_before=28.5, P_after=1.2, Change=-27.3
ZME TEST: PWM=0.5% PROBLEMLİ! (Təzyiq sıfıra düşdü)

ZME TEST: PWM=1.0% test edilir...
ZME TEST: PWM=1.0%, P_before=1.2, P_after=15.8, Change=14.6
ZME TEST: PWM=1.0% işləyir! (Təzyiq artır)
```

### **Təhlükəsizlik Rejimi Nəticələri:**
```
TƏHLÜKƏSİZLİK REJİMİ: Təzyiq çox yüksək! P=65.2 > SP+10=60.0
TƏHLÜKƏSİZLİK: ZME=30.0% (bağlı), DRV=0.0% (açıq), MOTOR=0.0% (dayandır)
```

## ⚙️ **KONFİQURASİYA**

### **ZME Kompensasiya Parametrləri:**
```c
// pressure_control.c faylında:
static float ZME_MIN_WORKING = 1.0f; // Test nəticələrinə görə dəyişdirin
```

### **Təhlükəsizlik Limitləri:**
```c
// Limit + 10 bar (əsas təhlükəsizlik)
if (s_P_filt > (g_SP + 10.0f)) { ... }

// Limit + 5 bar (xəbərdarlıq)
if (s_P_filt > (g_SP + 5.0f)) { ... }

// Limit + 50 bar (təcili dayandırma)
if (s_P_filt > max_safe_pressure) { ... }
```

## 🎯 **NƏTİCƏ**

Bu həllər ilə:
1. **ZME-nin qeyri-xətti davranışı kompensasiya edilir**
2. **"30 bar-dan 0 bar-a düşmə" problemi həll edilir**
3. **Təhlükəsizlik məntiqi gücləndirilir**
4. **Test funksiyaları ilə problemlər asanlıqla müəyyən edilir**

## 📞 **DƏSTƏK**

Əgər problemlər davam edərsə:
1. **ZME_ManualTest() nəticələrini yoxlayın**
2. **ZME_MIN_WORKING dəyərini tənzimləyin**
3. **Təhlükəsizlik limitlərini yoxlayın**
4. **Debug çıxışlarını izləyin**





