# SP VƏ PRESSURE_LIMIT AYIRMA REHBERİ

## 🚨 **PROBLEM: SP və pressure_limit bir-birinə qoşulub**

### **Səbəb:**
SP (Set Point) və pressure_limit dəyişənləri bir-birini dəyişdirirdi və bu, sonsuz dövrə yaradırdı.

## 🔧 **HƏLL EDİLƏN PROBLEMLƏR**

### **1. Sinxronlaşma Kodları Silindi**
```c
// ƏVVƏL (PROBLEMLİ):
pressure_limit = g_SP;  // pressure_limit-ni SP ilə sinxronlaşdır
g_SP = pressure_limit;  // SP-ni pressure_limit ilə sinxronlaşdır
// Sonsuz dövrə!

// İNDİ (DÜZGÜN):
// SP və pressure_limit müstəqil dəyişənlərdir
```

### **2. Müstəqil Dəyişənlər Yaratdı**
```c
// SP (Set Point) - PID nəzarəti üçün
float g_SP = 50.0f;  // pressure_control.c-də

// Pressure Limit - UI üçün
float pressure_limit = 50.0f;  // ILI9341_FSMC.c-də
```

### **3. Funksiyalar Ayrıldı**
```c
// SP dəyişdirmək üçün
void PressureControl_SetSetpoint(float sp_bar);

// Pressure limit dəyişdirmək üçün  
void PressureControl_SetPressureLimit(float limit_bar);
```

## 📊 **YENİ SİSTEM MƏNTİQİ**

### **SP (Set Point):**
- **Məqsəd**: PID nəzarət sistemi üçün hədəf təzyiq
- **Dəyişdirən**: `PressureControl_SetSetpoint()`
- **İstifadə**: PID hesablamalarında
- **Müstəqil**: pressure_limit-dən asılı deyil

### **Pressure Limit:**
- **Məqsəd**: UI-da göstərilən təzyiq limiti
- **Dəyişdirən**: Touch düymələri
- **İstifadə**: Ekranda göstərmək üçün
- **Müstəqil**: SP-dən asılı deyil

## 🧪 **TEST PROSEDURLARI**

### **Test 1: SP Dəyişdirmə**
```c
// Kodda çağırın:
PressureControl_SetSetpoint(60.0f);
// Gözlənilən: SP=60.0, pressure_limit dəyişmir
```

### **Test 2: Pressure Limit Dəyişdirmə**
1. **Ekranda "Menu" düyməsinə toxunun**
2. **"PRES LIM" düyməsinə toxunun**
3. **Düymələrə toxunun**
4. **Gözlənilən**: pressure_limit dəyişir, SP dəyişmir

### **Test 3: Müstəqillik Testi**
```c
// SP dəyişdir
PressureControl_SetSetpoint(70.0f);
// Pressure limit dəyişdir
PressureControl_SetPressureLimit(80.0f);
// Gözlənilən: SP=70.0, pressure_limit=80.0
```

## 📊 **GÖZLƏNİLƏN NƏTİCƏLƏR**

### **SP Dəyişdirmə:**
```
SP DƏYİŞDİRİLDİ: 60.0 bar
SP:60.0  P:25.0  ERR:35.0  PID:175.0  ZME:12.5%  DRV:17.5%  MOTOR:60.0%
```

### **Pressure Limit Dəyişdirmə:**
```
TOUCH: -10 button pressed
TOUCH: pressure_limit changed to 40.0
PRESSURE_LIMIT DƏYİŞDİRİLDİ: 40.0 bar (SP dəyişmir)
SHOWING PRESSURE LIMIT PAGE: pressure_limit=40.0, page=3
```

### **Müstəqillik:**
```
SP:70.0  P:25.0  ERR:45.0  PID:225.0  ZME:8.75%  DRV:22.5%  MOTOR:70.0%
Limit: 80.0 bar  // pressure_limit UI-da
```

## ⚙️ **KONFİQURASİYA**

### **SP Parametrləri:**
```c
// pressure_control.c
float g_SP = 50.0f;  // Default SP
// Dəyişdirən: PressureControl_SetSetpoint()
// İstifadə: PID hesablamalarında
```

### **Pressure Limit Parametrləri:**
```c
// ILI9341_FSMC.c  
float pressure_limit = 50.0f;  // Default pressure limit
// Dəyişdirən: Touch düymələri
// İstifadə: UI-da göstərmək üçün
```

## 🔍 **PROBLEM YOXLAMALARI**

### **1. SP Dəyişir?**
- `PressureControl_SetSetpoint()` çağırıldıqda SP dəyişir?
- pressure_limit dəyişmir?

### **2. Pressure Limit Dəyişir?**
- Touch düymələrinə toxunanda pressure_limit dəyişir?
- SP dəyişmir?

### **3. Müstəqillik Var?**
- SP və pressure_limit fərqli dəyərlərə sahib ola bilir?
- Bir-birini dəyişdirmir?

## 🚨 **PROBLEM HƏLLƏRİ**

### **Əgər SP Dəyişmir:**
1. **PressureControl_SetSetpoint() funksiyasını yoxlayın**
2. **g_SP dəyişənini yoxlayın**
3. **Debug çıxışını izləyin**

### **Əgər Pressure Limit Dəyişmir:**
1. **Touch handling-i yoxlayın**
2. **pressure_limit dəyişənini yoxlayın**
3. **Debug çıxışını izləyin**

### **Əgər Hələ də Qoşulub:**
1. **Sinxronlaşma kodlarını yoxlayın**
2. **Funksiya çağırışlarını yoxlayın**
3. **Debug çıxışını izləyin**

## 📞 **DƏSTƏK**

Əgər problemlər davam edərsə:
1. **Debug çıxışını izləyin**
2. **SP və pressure_limit dəyərlərini yoxlayın**
3. **Funksiya çağırışlarını yoxlayın**
4. **Sinxronlaşma kodlarını yoxlayın**

## 🎯 **NƏTİCƏ**

Bu həllər ilə:
1. **SP və pressure_limit müstəqildir**
2. **Sonsuz dövrə yoxdur**
3. **SP PID nəzarəti üçün istifadə edilir**
4. **Pressure limit UI üçün istifadə edilir**

## 🔧 **ƏSAS DƏYİŞİKLİKLƏR**

### **pressure_control.c:**
- Sinxronlaşma kodları silindi
- SP müstəqil oldu
- pressure_limit ilə əlaqə kəsildi

### **ILI9341_FSMC.c:**
- Touch handling təmizləndi
- pressure_limit müstəqil oldu
- SP ilə əlaqə kəsildi

### **main.c:**
- Sinxronlaşma kodları silindi
- Müstəqil dəyişənlər qaldı





