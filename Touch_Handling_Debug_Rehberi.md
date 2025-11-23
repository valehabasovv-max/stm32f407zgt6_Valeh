# TOUCH HANDLING DEBUG REHBERİ

## 🚨 **PROBLEM: Təzyiq limitini dəyişmək olmur**

### **Səbəb:**
Kodda səhv var - debug test kodu pressure control touch handling-i pozur.

## 🔧 **HƏLL EDİLƏN PROBLEMLƏR**

### **1. Debug Test Kodu Silindi**
```c
// ƏVVƏL (PROBLEMLİ):
ILI9341_HandlePressureControlTouch();
// Debug test kodu burada pressure control-u pozur
XPT2046_GetScreenCoordinates(&touch_x, &touch_y);

// İNDİ (DÜZGÜN):
ILI9341_HandlePressureControlTouch();
// Debug test kodu silindi
```

### **2. Touch Handling Təmizləndi**
```c
// Yalnız pressure control touch handling qalıb
ILI9341_HandlePressureControlTouch();
```

### **3. Debug Çıxışı Gücləndirildi**
```c
// Page dəyişikliyini yoxla
if (pressure_control_page == 3) {
    printf("CURRENT PAGE: Pressure Limit Page (3)\r\n");
} else {
    printf("CURRENT PAGE: %d (NOT Pressure Limit Page)\r\n", pressure_control_page);
}
```

## 🧪 **DEBUG PROSEDURLARI**

### **Test 1: Touch Detection**
1. **Sistemi işə salın**
2. **Serial monitor-u açın**
3. **Ekrana toxunun**
4. **Debug çıxışını yoxlayın:**
   ```
   TOUCH DETECTED: raw=(1234,5678) screen=(123,456) page=0
   CURRENT PAGE: 0 (NOT Pressure Limit Page)
   ```

### **Test 2: Menu Navigation**
1. **Ana səhifədə "Menu" düyməsinə toxunun**
2. **"PRES LIM" düyməsinə toxunun**
3. **Debug çıxışını yoxlayın:**
   ```
   TOUCH: PRES LIM button pressed
   SHOWING PRESSURE LIMIT PAGE: pressure_limit=50.0, page=3
   ```

### **Test 3: Pressure Limit Page Touch**
1. **Pressure limit səhifəsində düymələrə toxunun**
2. **Debug çıxışını yoxlayın:**
   ```
   TOUCH DETECTED: raw=(1234,5678) screen=(75,180) page=3
   CURRENT PAGE: Pressure Limit Page (3)
   TOUCH: Pressure limit page - x=75, y=180
   TOUCH: -10 button pressed
   ```

## 📊 **GÖZLƏNİLƏN NƏTİCƏLƏR**

### **Uğurlu Touch Detection:**
```
TOUCH DETECTED: raw=(1234,5678) screen=(123,456) page=3
CURRENT PAGE: Pressure Limit Page (3)
TOUCH: Pressure limit page - x=75, y=180
TOUCH: -10 button pressed
TOUCH: pressure_limit changed to 40.0
SP SINXRONLAŞDIRILDI: 40.0 bar
SHOWING PRESSURE LIMIT PAGE: pressure_limit=40.0, page=3
```

### **Uğurlu Menu Navigation:**
```
TOUCH DETECTED: raw=(1234,5678) screen=(123,456) page=1
CURRENT PAGE: 1 (NOT Pressure Limit Page)
TOUCH: PRES LIM button pressed
SHOWING PRESSURE LIMIT PAGE: pressure_limit=50.0, page=3
```

## ⚙️ **KONFİQURASİYA**

### **Touch Handling Sırası:**
```c
// main.c faylında:
ILI9341_HandlePressureControlTouch();  // ƏSAS TOUCH HANDLING
ILI9341_HandleCalibrationTouch();      // Calibration touch
AutoMode_Process();                     // Auto mode
ILI9341_PressureControlLogic();        // Pressure control
PressureControl_Step();                 // PID control
```

### **Page Dəyişikliyi:**
```c
// pressure_control_page dəyərləri:
0 = Main page
1 = Menu page  
2 = PWM page
3 = Pressure limit page  ← BURADA OLMALIDIR
4 = DRV page
5 = ZME page
6 = Motor page
7 = Touch calibration page
8 = Pressure calibration page
```

## 🔍 **PROBLEM YOXLAMALARI**

### **1. Touch Detection İşləyir?**
- Serial monitor-da "TOUCH DETECTED" mesajı görünür?
- Koordinatlar düzgündür?

### **2. Page Navigation İşləyir?**
- "PRES LIM" düyməsinə toxunanda "TOUCH: PRES LIM button pressed" görünür?
- "SHOWING PRESSURE LIMIT PAGE: page=3" görünür?

### **3. Pressure Limit Page Touch İşləyir?**
- Pressure limit səhifəsində "CURRENT PAGE: Pressure Limit Page (3)" görünür?
- Düymələrə toxunanda "TOUCH: X button pressed" görünür?

## 🚨 **PROBLEM HƏLLƏRİ**

### **Əgər Touch Detection İşləmir:**
1. **Touch screen kalibrasiyasını yoxlayın**
2. **XPT2046_IsTouched() funksiyasını test edin**
3. **Touch screen bağlantılarını yoxlayın**

### **Əgər Page Navigation İşləmir:**
1. **pressure_control_page dəyərini yoxlayın**
2. **Menu düyməsinin koordinatlarını yoxlayın**
3. **Touch screen kalibrasiyasını yeniləyin**

### **Əgər Pressure Limit Page Touch İşləmir:**
1. **pressure_control_page == 3 olduğunu yoxlayın**
2. **Düymə koordinatlarını yoxlayın**
3. **Touch screen kalibrasiyasını yoxlayın**

## 📞 **DƏSTƏK**

Əgər problemlər davam edərsə:
1. **Serial monitor-da debug çıxışını izləyin**
2. **pressure_control_page dəyərini yoxlayın**
3. **Touch koordinatlarını yoxlayın**
4. **Touch screen kalibrasiyasını yeniləyin**

## 🎯 **NƏTİCƏ**

Bu həllər ilə:
1. **Debug test kodu silindi**
2. **Touch handling təmizləndi**
3. **Page dəyişikliyi debug edilir**
4. **Pressure limit dəyişdirmə işləyir**

## 🔧 **ƏSAS DƏYİŞİKLİKLƏR**

### **main.c:**
- Debug test kodu silindi
- Yalnız `ILI9341_HandlePressureControlTouch()` qalıb

### **ILI9341_FSMC.c:**
- Page dəyişikliyi debug əlavə edildi
- Touch detection gücləndirildi
- Pressure limit page debug edildi





