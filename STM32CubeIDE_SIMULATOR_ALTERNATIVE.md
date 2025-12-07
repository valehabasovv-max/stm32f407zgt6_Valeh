# STM32CubeIDE - Simulator Seçimi Yoxdursa

## ⚠️ Problem

"Debug probe" dropdown-da "Simulator" seçimi yoxdur. Bu, STM32CubeIDE-də STM32F407 üçün simulator dəstəyinin məhdud olduğunu göstərir.

## 🔍 Səbəb

STM32CubeIDE-də simulator dəstəyi:
- ✅ STM32F0, F1, F3 seriyaları üçün var
- ⚠️ STM32F4 seriyası üçün məhdud və ya yoxdur
- ❌ STM32F407 üçün tam simulator dəstəyi yoxdur

## 💡 Alternativ Həllər

### Həll 1: Renode Emulyator (Tövsiyə olunur)

Renode-da tam emulyasiya var:

```powershell
cd renode
.\run_emulator.ps1
```

**Avantajlar:**
- ✅ STM32F407 tam dəstəklənir
- ✅ CPU, Memory, UART işləyir
- ✅ Alqoritm testi üçün yaxşıdır

**Məhdudiyyətlər:**
- ⚠️ LCD və Touch tam simulyasiya olunmur

### Həll 2: STM32CubeIDE Hardware Debugging

Real hardware ilə debugging:

1. **"Debug probe"** dropdown-dan **"ST-LINK (ST-LINK GDB server)"** seçin
2. Board-u kompüterə qoşun
3. **"Debug"** düyməsinə basın
4. Real hardware-də işləyəcək

**Avantajlar:**
- ✅ Tam funksionallıq
- ✅ LCD və Touch işləyir
- ✅ Görüntü görünür

### Həll 3: QEMU (Məhdud)

QEMU-da STM32 simulyasiyası var, amma məhduddur.

## 🎯 Tövsiyə

**Praktik yanaşma:**

1. **Alqoritm Testi:** Renode emulyatorunda
   - PID, pressure control
   - UART output-da debug
   - State machine testi

2. **Visual Test:** Real hardware-də
   - LCD ekran
   - Touch düymələri
   - Screenshot almaq

3. **Development Cycle:**
   - Kod yazın → Renode-da test edin
   - Dəyişiklik edin → Real hardware-də yoxlayın
   - Screenshot alın → Dizaynı təkmilləşdirin

## 📝 Qeyd

STM32CubeIDE-də STM32F407 üçün simulator dəstəyi yoxdur. Bu normaldır - bütün MCU-lar üçün simulator yoxdur.

**Ən yaxşı həll:** Renode emulyator + Real hardware kombinasiyası

