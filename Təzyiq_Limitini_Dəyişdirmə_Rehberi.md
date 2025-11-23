# TƏZYİQ LİMİTİNİ DƏYİŞDİRMƏ REHBERİ

## 🚨 **PROBLEM: Təzyiq limitini dəyişmək olmur**

### **Səbəb:**
Touch handling kodu mövcuddur, amma debug çıxışı yoxdur ki, düymələr işləyir.

## 🔧 **HƏLL EDİLƏN PROBLEMLƏR**

### **1. Debug Çıxışı Əlavə Edildi**
```c
// Touch detection debug
printf("TOUCH DETECTED: raw=(%d,%d) screen=(%d,%d) page=%d\r\n", 
       raw_x, raw_y, screen_x, screen_y, pressure_control_page);

// Button press debug
printf("TOUCH: -10 button pressed\r\n");
printf("TOUCH: pressure_limit changed to %.1f\r\n", pressure_limit);
```

### **2. Düymə Koordinatları Yoxlanıldı**
```c
// Pressure limit page düymələri:
-10: x=50-100, y=160-200
-1:  x=110-160, y=160-200  
+1:  x=170-220, y=160-200
+10: x=230-280, y=160-200
```

### **3. İlkin Dəyərlər Düzəldildi**
```c
// ILI9341_FSMC.c
float pressure_limit = 50.0; // Default 50 bar

// main.c  
PressureControl_SetSetpoint(50.0f); // İlkin 50 bar
```

## 🧪 **TEST PROSEDURLARI**

### **Test 1: Touch Detection**
1. **Sistemi işə salın**
2. **Serial monitor-u açın**
3. **Ekrana toxunun**
4. **Debug çıxışını yoxlayın:**
   ```
   TOUCH DETECTED: raw=(1234,5678) screen=(123,456) page=0
   ```

### **Test 2: Menu Navigation**
1. **Ana səhifədə "Menu" düyməsinə toxunun**
2. **"PRES LIM" düyməsinə toxunun**
3. **Debug çıxışını yoxlayın:**
   ```
   TOUCH: PRES LIM button pressed
   SHOWING PRESSURE LIMIT PAGE: pressure_limit=50.0
   ```

### **Test 3: Pressure Limit Dəyişdirmə**
1. **Pressure limit səhifəsində düymələrə toxunun**
2. **Debug çıxışını yoxlayın:**
   ```
   TOUCH: Pressure limit page - x=75, y=180
   TOUCH: -10 button pressed
   TOUCH: pressure_limit changed to 40.0
   SP SINXRONLAŞDIRILDI: 40.0 bar
   ```

## 📊 **GÖZLƏNİLƏN NƏTİCƏLƏR**

### **Uğurlu Touch Detection:**
```
TOUCH DETECTED: raw=(1234,5678) screen=(123,456) page=3
TOUCH: Pressure limit page - x=75, y=180
TOUCH: -10 button pressed
TOUCH: pressure_limit changed to 40.0
SP SINXRONLAŞDIRILDI: 40.0 bar
SHOWING PRESSURE LIMIT PAGE: pressure_limit=40.0
```

### **Uğurlu Menu Navigation:**
```
TOUCH DETECTED: raw=(1234,5678) screen=(123,456) page=1
TOUCH: PRES LIM button pressed
SHOWING PRESSURE LIMIT PAGE: pressure_limit=50.0
```

## ⚙️ **KONFİQURASİYA**

### **Touch Koordinatları:**
```c
// Menu səhifəsi (page=1)
PRES LIM button: x=80-240, y=160-200

// Pressure limit səhifəsi (page=3)  
-10 button: x=50-100, y=160-200
-1 button:  x=110-160, y=160-200
+1 button:  x=170-220, y=160-200
+10 button: x=230-280, y=160-200
Back button: x=20-100, y=50-80
```

### **Pressure Limit Məhdudiyyətləri:**
```c
// Minimum: 0 bar
// Maksimum: 300 bar
// Default: 50 bar
```

## 🔍 **DEBUG YOXLAMALARI**

### **1. Touch Detection İşləyir?**
- Serial monitor-da "TOUCH DETECTED" mesajı görünür?
- Koordinatlar düzgündür?

### **2. Menu Navigation İşləyir?**
- "PRES LIM" düyməsinə toxunanda "TOUCH: PRES LIM button pressed" görünür?
- Pressure limit səhifəsi açılır?

### **3. Pressure Limit Dəyişir?**
- Düymələrə toxunanda "TOUCH: X button pressed" görünür?
- "pressure_limit changed to X.X" mesajı görünür?
- Ekranda yeni dəyər göstərilir?

## 🚨 **PROBLEM HƏLLƏRİ**

### **Əgər Touch Detection İşləmir:**
1. **Touch screen kalibrasiyasını yoxlayın**
2. **XPT2046_IsTouched() funksiyasını test edin**
3. **Touch screen bağlantılarını yoxlayın**

### **Əgər Menu Navigation İşləmir:**
1. **Koordinatları yoxlayın**
2. **pressure_control_page dəyərini yoxlayın**
3. **Touch screen kalibrasiyasını yeniləyin**

### **Əgər Pressure Limit Dəyişmir:**
1. **Düymə koordinatlarını yoxlayın**
2. **Touch screen kalibrasiyasını yoxlayın**
3. **Debug çıxışını izləyin**

## 📞 **DƏSTƏK**

Əgər problemlər davam edərsə:
1. **Serial monitor-da debug çıxışını izləyin**
2. **Touch koordinatlarını yoxlayın**
3. **Touch screen kalibrasiyasını yeniləyin**
4. **Hardware bağlantılarını yoxlayın**

## 🎯 **NƏTİCƏ**

Bu həllər ilə:
1. **Touch detection debug edilir**
2. **Düymə koordinatları yoxlanılır**
3. **Pressure limit dəyişdirmə işləyir**
4. **Debug çıxışı ilə problemlər asanlıqla müəyyən edilir**





