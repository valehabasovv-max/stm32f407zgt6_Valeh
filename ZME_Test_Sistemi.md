# ZME TEST SİSTEMİ

## 🔍 **PROBLEM TƏHLİLİ**

### **Mövcud Vəziyyət:**
- **Təzyiq**: 50.0 BAR (40 BAR limitindən yüksək)
- **Error**: +6.5 (təzyiq yüksək)
- **ZME**: 11.3% (yanlış - 30% olmalıdır)
- **DRV**: 24.9% (yanlış - 0% olmalıdır)

### **Problemlər:**
1. **ZME mapping yanlış**: 11.3% əvəzinə 30% olmalıdır
2. **DRV mapping yanlış**: 24.9% əvəzinə 0% olmalıdır
3. **PID məntiq qarışıq**: ZME və DRV eyni anda işləyir

## 🔧 **HƏLL STRATEGİYASI**

### **1. Yalnız ZME ilə Test:**
```c
// TEST: Yalnız ZME ilə başlayaq
if (pid_output < 0) {
    // Təzyiq yüksək → ZME bağlanmalıdır
    zme_cmd = ZME_MAX;  // 30% - tam bağlı
} else {
    // Təzyiq aşağı → ZME açılmalıdır
    zme_cmd = ZME_MIN;  // 0% - tam açıq
}

// DRV: Hələlik deaktiv (test üçün)
drv_cmd = DRV_MAX;  // 40% - tam bağlı (deaktiv)
```

### **2. Düzgün Məntiq:**
- **Təzyiq yüksək** (Error < 0): ZME bağlanmalıdır (30%)
- **Təzyiq aşağı** (Error > 0): ZME açılmalıdır (0%)
- **DRV deaktiv**: Test zamanı işləmir

## 📊 **GÖZLƏNİLƏN NƏTİCƏLƏR**

### **Təzyiq 50 BAR (Error = +6.5):**
```
pid_output = 3.0 * 6.5 = +19.5 (müsbət)
ZME = 0% (açıq) - yanacaq verir
DRV = 40% (bağlı) - deaktiv
Motor = 22.2% - işləyir
→ Təzyiq artmalıdır (yanlış!)
```

### **Düzgün Məntiq:**
```
pid_output = 3.0 * 6.5 = +19.5 (müsbət)
ZME = 30% (bağlı) - yanacaq vermir
DRV = 40% (bağlı) - deaktiv
Motor = 0% - dayandır
→ Təzyiq azalmalıdır (düzgün!)
```

## 🎯 **TEST PROSEDURU**

### **1. İlk Test:**
- **Təzyiq**: 50 BAR
- **Setpoint**: 40 BAR
- **Gözlənilən**: ZME = 30% (bağlı)
- **Nəticə**: Təzyiq azalmalıdır

### **2. İkinci Test:**
- **Təzyiq**: 35 BAR
- **Setpoint**: 40 BAR
- **Gözlənilən**: ZME = 0% (açıq)
- **Nəticə**: Təzyiq artmalıdır

### **3. Üçüncü Test:**
- **Təzyiq**: 40 BAR
- **Setpoint**: 40 BAR
- **Gözlənilən**: ZME = 15% (orta)
- **Nəticə**: Təzyiq sabit qalmalıdır

## 🔧 **TƏTBİQ EDİLƏN DƏYİŞİKLİKLƏR**

### **1. Core/Src/pressure_control.c:**
- ✅ Yalnız ZME ilə test
- ✅ DRV deaktiv
- ✅ Sadə if-else məntiq
- ✅ Debug çıxışı aktiv

### **2. Test Parametrləri:**
- **ZME**: 0% (açıq) və 30% (bağlı)
- **DRV**: 40% (deaktiv)
- **Motor**: Normal işləmə
- **Debug**: Həmişə aktiv

## 📈 **GÖZLƏNİLƏN NƏTİCƏLƏR**

### **Təzyiq 50 BAR olduqda:**
```
SP=40.0 | P=50.0 | Error=-10.0 | PID=-30.0 | ZME=30.0% | DRV=40.0% | MOTOR=0.0%
```
- **ZME**: 30% (bağlı) - yanacaq vermir
- **Nəticə**: Təzyiq azalmalıdır

### **Təzyiq 35 BAR olduqda:**
```
SP=40.0 | P=35.0 | Error=+5.0 | PID=+15.0 | ZME=0.0% | DRV=40.0% | MOTOR=15.0%
```
- **ZME**: 0% (açıq) - yanacaq verir
- **Nəticə**: Təzyiq artmalıdır

## 🎯 **NƏTİCƏ**

### **Test Məqsədi:**
1. **ZME məntiqini yoxla**: Təzyiq yüksək olduqda bağlanır
2. **DRV deaktiv**: Qarışıqlıq yox
3. **Sadə məntiq**: if-else ilə test
4. **Debug çıxışı**: Hər döngüdə göstər

### **Gözlənilən Nəticə:**
- **Təzyiq 50 BAR**: ZME = 30% (bağlı) → Təzyiq azalır
- **Təzyiq 35 BAR**: ZME = 0% (açıq) → Təzyiq artır
- **Təzyiq 40 BAR**: ZME = 15% (orta) → Təzyiq sabit

**Test tamamlandıqdan sonra DRV də aktiv ediləcək!** 🎯







