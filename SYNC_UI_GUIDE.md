# UI Sinxronizasiyası - Bir Yerdə Dəyişiklik

## 🎯 Məqsəd

**Yalnız C kodlarında dəyişiklik etməklə həm simulyatorda, həm də board-da eyni dəyişiklikləri görmək.**

## ✅ Necə İşləyir

### 1. C Kodlarında Dəyişiklik

```c
// Core/Src/main.c

// Rəng dəyişikliyi
#define COLOR_ACCENT_BLUE   0x1234  // Yeni rəng

// Preset dəyəri dəyişikliyi
static const float g_presets[NUM_PRESETS] = {
    50.0f, 100.0f, 150.0f, 200.0f, 250.0f, 300.0f
};

// Preset adı dəyişikliyi
static const char* g_preset_names[NUM_PRESETS] = {
    "LOW", "MED", "HIGH", "V.HI", "EXT", "MAX"
};
```

### 2. UI Simulator Avtomatik Oxuyur

UI Simulator işə salındıqda:
- C kodlarını avtomatik parse edir
- Rəngləri, preset dəyərlərini, ölçüləri oxuyur
- Eyni konfiqurasiya ilə UI göstərir

### 3. Board-da Upload

C kodlarını build edib upload etdikdə:
- Eyni konfiqurasiya board-da işləyir
- Simulyator və board-da eyni UI

## 📋 Dəstəklənən Parametrlər

### Rənglər (avtomatik parse)
- `COLOR_BG_DARK`
- `COLOR_BG_PANEL`
- `COLOR_ACCENT_BLUE`
- `COLOR_ACCENT_GREEN`
- `COLOR_ACCENT_RED`
- `COLOR_ACCENT_YELLOW`
- `COLOR_ACCENT_ORANGE`
- `COLOR_TEXT_WHITE`
- `COLOR_TEXT_GREY`
- `COLOR_BORDER`

### Ölçülər (avtomatik parse)
- `SCREEN_WIDTH`
- `SCREEN_HEIGHT`

### Preset Dəyərləri (avtomatik parse)
- `g_presets[]` array
- `g_preset_names[]` array

## 🚀 İstifadə

### 1. C Kodlarında Dəyişiklik

```c
// main.c-də dəyişiklik
#define COLOR_ACCENT_BLUE   0x5678  // Yeni rəng
```

### 2. UI Simulator-da Test

```powershell
python renode/ui_simulator.py
```

**Nəticə:** Yeni rəng avtomatik görünəcək!

### 3. Board-a Upload

```powershell
cd Debug
make
# STM32CubeProgrammer ilə upload
```

**Nəticə:** Board-da da eyni rəng görünəcək!

## 💡 Nümunə

### Dəyişiklik: Preset dəyəri

**C kodunda:**
```c
static const float g_presets[NUM_PRESETS] = {
    60.0f, 110.0f, 160.0f, 210.0f, 260.0f, 310.0f  // Dəyişiklik
};
```

**Nəticə:**
- ✅ UI Simulator-da yeni dəyərlər görünür
- ✅ Board-a upload edildikdə yeni dəyərlər görünür
- ✅ Bir yerdə dəyişiklik, hər yerdə tətbiq olunur!

## ⚠️ Qeyd

- UI Simulator işə salındıqda C kodlarını yenidən parse edir
- C kodlarında dəyişiklik etdikdən sonra UI Simulator-u yenidən işə salın
- Build prosesi eyni qalır (make)

## ✅ Avantajlar

1. **Bir yerdə dəyişiklik** - Yalnız C kodlarında
2. **Avtomatik sinxronizasiya** - UI Simulator avtomatik oxuyur
3. **Eyni UI** - Simulyator və board-da eyni görüntü
4. **Asan test** - Board olmadan UI test edə bilərsiniz

