# Virtual LCD - Renode İnteqrasiyası

## 🎯 Məqsəd

STM32-də monitorunda görünən proqramı virtual LCD-də də göstərmək.

## ⚠️ Çətinlik

FSMC LCD writes-ı hook etmək və interpretasiya etmək mürəkkəbdir çünki:

1. **ILI9341 Protocol:** LCD command/data protokolu kompleksdir
2. **FSMC Mapping:** FSMC address mapping-i anlamaq lazımdır
3. **Real-time:** Real-time simulyasiya performans tələb edir

## 💡 Praktik Həll

### Seçim 1: Screenshot + Virtual LCD (Tövsiyə olunur)

1. **Real hardware-də screenshot alın:**
   - Board-u bir dəfə qoşun
   - Proqramı upload edin
   - Telefon kamerası ilə screenshot alın
   - Və ya serial monitor-dan screenshot

2. **Virtual LCD-də göstərin:**
   - Screenshot-u virtual LCD-də açın
   - Dizayn dəyişikliklərini planlaşdırın

### Seçim 2: Framebuffer Capture

Proqramda framebuffer-i capture edib virtual LCD-yə göndərmək:

1. **Framebuffer export funksiyası əlavə edin:**
   ```c
   // main.c-də
   void ExportFramebuffer(void) {
       // Framebuffer-i UART-dan göndər
       // Və ya fayla yaz
   }
   ```

2. **Python script ilə oxuyun:**
   - UART-dan framebuffer oxuyun
   - Virtual LCD-yə göndərin

### Seçim 3: Renode FSMC Hook (Mürəkkəb)

Renode-da FSMC writes-ı hook etmək:

1. **Renode script-də hook qurun:**
   - `virtual_lcd_monitor.resc` faylında hook var
   - Amma ILI9341 protokolu kompleksdir

2. **Python bridge ilə bağlayın:**
   - `lcd_bridge.py` script-i istifadə edin
   - FSMC writes-ı virtual LCD-yə köçürün

## 🚀 Tez Həll (Tövsiyə olunur)

### Addım 1: Screenshot Alın

Real hardware-də (bir dəfə qoşun):
1. Proqramı upload edin
2. Screenshot alın (telefon kamerası)
3. Screenshot-u kompüterə köçürün

### Addım 2: Virtual LCD-də Göstərin

```python
# Screenshot-u virtual LCD-də açmaq üçün
from PIL import Image
import tkinter as tk

# Screenshot-u yüklə
img = Image.open("screenshot.jpg")
img = img.resize((320, 240))

# Virtual LCD-də göstər
# (lcd_monitor_gui.py-yə screenshot loader əlavə edin)
```

### Addım 3: Dizayn Dəyişiklikləri

1. Screenshot-u virtual LCD-də görün
2. Dizayn dəyişikliklərini planlaşdırın
3. Kodda dəyişiklik edin
4. Yenidən test edin

## 📝 Qeyd

**Vacib:** Tam real-time simulyasiya çətindir. Praktik yanaşma:
- Development: Screenshot + Virtual LCD
- Testing: Real hardware
- Final: Real hardware-də tam test

## 🔧 Növbəti Addımlar

1. Screenshot almaq üçün real hardware lazımdır (bir dəfə)
2. Screenshot-u virtual LCD-də göstərmək üçün script yarada bilərik
3. Dizayn dəyişikliklərini planlaşdırmaq üçün virtual LCD istifadə edin

