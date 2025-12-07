# STM32CubeIDE - Target Tab Tapmaq

## 📍 Target Tab Haradadır?

**Target** seçimi **"Debugger"** tab-ındadır!

## 🎯 Addım-Addım

### Addım 1: Debugger Tab-ına Keçin

1. Debug Configurations pəncərəsində (indi açıqdır)
2. Yuxarıda tab-lar görünür: **Main**, **Debugger**, **Startup**, **Source**, **Common**
3. **"Debugger"** tab-ına klikləyin

### Addım 2: Target Seçin

"Debugger" tab-ında:

1. **"Target"** bölməsini tapın
2. **"Target"** dropdown menu var
3. Orada seçimlər görünəcək:
   - **"Hardware"** (real board üçün)
   - **"Simulator"** (simulator üçün) ← **BUNU SEÇİN!**

### Addım 3: Simulator Seçin

1. **"Target"** dropdown-dan **"Simulator"** seçin
2. **"Apply"** düyməsinə basın
3. **"Debug"** düyməsinə basın

## 📸 Görünüş

Debugger tab-ında belə görünəcək:

```
┌─────────────────────────────────┐
│ Debugger Tab                    │
├─────────────────────────────────┤
│ Target: [Simulator ▼]           │ ← BURADADIR!
│                                  │
│ Debugger: ST-Link GDB Server    │
│ ...                             │
└─────────────────────────────────┘
```

## ⚠️ Qeyd

Əgər "Simulator" seçimi görünmürsə:
- STM32CubeIDE-nin son versiyasını quraşdırın
- Proyekt düzgün import olunduğundan əmin olun
- MCU düzgün seçildiyindən əmin olun (STM32F407ZG)

## 🔧 Alternativ

Əgər "Simulator" seçimi yoxdursa:
1. "Debugger" tab-ında
2. "Debugger" dropdown-dan "ST-Link GDB Server" seçin
3. "Target" bölməsində "Simulator" axtarın

## 📝 Sonrakı Addımlar

1. **"Debugger"** tab-ına keçin
2. **"Target"** dropdown-dan **"Simulator"** seçin
3. **"Apply"** → **"Debug"**

İndi simulator işə başlayacaq!

