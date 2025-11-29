# 🔌 Voltage Divider Quraşdırma Təlimatı (300 bar üçün)

## ⚠️ NİYƏ LAZIMDIR?

**Problem:**
- Sensor çıxışı: **0.5V-5.0V** (0-300 bar)
- STM32 ADC maksimum: **3.3V**
- **3.3V-dən yuxarı gərginliklər ADC-ni zədələyə bilər!**
- **230 bar-dan yuxarı təzyiqlər ölçülə bilmir** (ADC saturated at 4095)

**Həll:**
- Voltage divider əlavə et
- Sensor çıxışını **yarıya endir** (0.5V-5.0V → 0.25V-2.5V)
- İndi **bütün 0-300 bar aralığı güvənli şəkildə ölçülə bilər** ✅

---

## 📋 LAZIMLI KOMPONENTLƏR

| Element | Dəyər | Miqdar | Qeydlər |
|---------|-------|--------|---------|
| Rezistor R1 | **10kΩ** | 1 ədəd | 1/4W, 1% tolerans |
| Rezistor R2 | **10kΩ** | 1 ədəd | 1/4W, 1% tolerans |
| Lehim | - | - | Bağlantılar üçün |
| İzolyasiya lent | - | - | Lehimləri qorumaq üçün |

**Alternativ dəyərlər:**
- 4.7kΩ + 4.7kΩ (eyni divisor)
- 22kΩ + 22kΩ (eyni divisor)

**Əhəmiyyətli:** İki rezistor **eyni dəyərdə** olmalıdır!

---

## 🔧 QURAŞDIRMA

### Addım 1: Cihazı Söndür

⚠️ **TƏHLÜKƏSİZLİK:** Sistemi söndür və enerji mənbəyindən ayır!

### Addım 2: Sensor Çıxış Kabelin Tap

Sensor çıxış kabelini tap (STM32 PA3 pininə gedən):
- Bu, **analog siqnal** kabelidir (0.5V-5.0V)
- Qidalanma kabeli **DEYİL** (5V power)

### Addım 3: Kabeli Kəs

Sensor çıxış kabelin **ortadan kəs**:

```
ƏVVƏL:
Sensor Output ──────────────────> STM32 PA3
(0.5V-5.0V)

SONRA:
Sensor Output ────╳────────────> STM32 PA3
                  ↑
                Burada kəs
```

### Addım 4: Voltage Divider Qur

**Sxem:**

```
     Sensor Output Wire
     (0.5V-5.0V)
          |
          |
       [R1: 10kΩ]
          |
          +-----------> STM32 PA3
          |            (0.25V-2.5V)
       [R2: 10kΩ]
          |
         GND
```

**Lehimləmə:**

1. **R1 (10kΩ)** - Sensor çıxış kabeli ilə PA3 arasına
2. **R2 (10kΩ)** - PA3 ilə GND arasına
3. Lehim bağlantılarını **izolyasiya lent** ilə qoru

### Addım 5: GND Bağlantısı

R2-nin aşağı ucu **GND**-yə bağlanmalıdır:
- STM32 GND pini
- Və ya Power Supply GND
- Və ya Sensor GND

**Diqqət:** Bütün GND-lər **ortaq** olmalıdır!

### Addım 6: Yoxla

Multimetrlə ölç:

| Sensor Çıxışı | PA3-də Olması Lazım |
|---------------|---------------------|
| 0.5V          | **0.25V** ✅        |
| 1.0V          | **0.50V** ✅        |
| 2.0V          | **1.00V** ✅        |
| 3.0V          | **1.50V** ✅        |
| 5.0V          | **2.50V** ✅        |

**Formula:** PA3 Voltage = Sensor Voltage × 0.5

---

## 💻 KOD KONFİQURASIYASI

### Aktiv Et

**Hal-hazırda KOD ARTIQ HAZIR!** Sadəcə flag-i aktiv etmək lazımdır:

Fayl: `Core/Inc/advanced_pressure_control.h`

```c
#define VOLTAGE_DIVIDER_ENABLED 1  // ✅ 1 = Voltage divider var (AKTIV)
```

**Deaktiv etmək üçün:**
```c
#define VOLTAGE_DIVIDER_ENABLED 0  // ❌ 0 = Yoxdur (DEAKTIV)
```

### Yenidən Kompilyasiya

Hardware quraşdırandan **SONRA**:

1. Kodu yenidən kompilyasiya et
2. MCU-ya yüklə
3. Sistemi başlat

Sistem avtomatik olaraq:
- ✅ Köhnə kalibrasiyanu aşkarlayacaq
- ✅ Yeni ADC dəyərləri (310-3103) yükləyəcək
- ✅ Flash-a yazacaq

---

## ✅ TEST PROSEDURU

### 1. Sistem Başlat

Serial/Terminal çıxışını aç və bu məlumatı gözlə:

```
*****************************************************************
*  ⚠ XƏBƏRDARLIQ: KALIBRASIYA SƏHV AŞKAR EDİLDİ!              *
*****************************************************************
*  Flash-dakı kalibrasiya validasiyadan keçmədi:
*    ADC: 620 - 4095 (Gözlənilən: 310 - 3103)
*
*  Voltage Divider aktiv (R1=R2=10kΩ)
*  Sensor 0.5V-5.0V → Divider 0.25V-2.5V → ADC 310-3103
*
*  Default kalibrasiya dəyərləri yüklənəcək və Flash-a yazılacaq.
*****************************************************************

--- FORCING RECALIBRATION ---
Setting calibration to defaults (Voltage Divider Mode):
  ADC: 310 - 3103
  Pressure: 0.0 - 300.0 bar
  NOTE: Voltage divider converts sensor 0.5V-5.0V → ADC 0.25V-2.5V

New calibration values:
  Slope: 0.107403 bar/ADC
  Offset: -33.294369 bar
```

### 2. ADC Dəyərlərini Yoxla

**Təzyiq yoxdur (0 bar):**
- Sensor: **0.5V**
- PA3: **0.25V**
- ADC: **~310**
- Ekran: **0 bar** ✅

**Orta təzyiq (150 bar):**
- Sensor: **2.75V**
- PA3: **1.375V**
- ADC: **~1705**
- Ekran: **~150 bar** ✅

**Maksimum təzyiq (300 bar):**
- Sensor: **5.0V**
- PA3: **2.5V**
- ADC: **~3103**
- Ekran: **~300 bar** ✅

### 3. Kalibrasiya (Opsional)

Əgər dəyərlər dəqiq deyilsə, UI-dən kalibrasiya et:

1. **CAL MIN:** Təzyiq yoxdur (0 bar) → düyməyə bas
2. **CAL MAX:** Maksimum təzyiq (300 bar) → düyməyə bas
3. **SAVE:** Kalibrasiyanu Flash-a yaz

---

## 🔄 GERİ DÖNMƏK (Voltage Divider-siz)

Əgər voltage divider-i çıxarmaq istəsəniz:

1. **Hardware:** R1 və R2-ni çıxar, kabeli birbaşa PA3-ə bağla
2. **Kod:** `VOLTAGE_DIVIDER_ENABLED 0` təyin et
3. **Yenidən kompilyasiya** et və yüklə

⚠️ **Diqqət:** Voltage divider olmadan maksimum **~230 bar** ölçülə bilər!

---

## ❓ PROBLEMLƏR VƏ HƏLL

### Problem: ADC hələ də 4095-də qalır

**Səbəblər:**
1. Voltage divider düzgün quraşdırılmayıb
2. GND bağlantısı yoxdur
3. Rezistor dəyərləri yanlışdır
4. Kod flag-i deaktivdir

**Həll:**
- Multimetrlə PA3 gərginliyini ölç (2.5V-dən aşağı olmalıdır)
- GND bağlantısını yoxla
- Rezistorları ölç (hər biri ~10kΩ olmalıdır)
- `VOLTAGE_DIVIDER_ENABLED 1` olduğunu yoxla

### Problem: Ekranda 0.00 bar qalır

**Səbəblər:**
1. Kod yenidən kompilyasiya edilməyib
2. Kalibrasiya Flash-a yazılmayıb

**Həll:**
- Kodu yenidən kompilyasiya et
- MCU-ya yüklə
- Sistemi yenidən başlat
- Serial output-da "FORCING RECALIBRATION" mesajını gözlə

### Problem: Dəyərlər dəqiq deyil

**Həll:**
- UI-dən kalibrasiya et (CAL MIN/MAX/SAVE)
- Sensor spesifikasiyasını yoxla (həqiqətən 0.5V-5.0V?)

---

## 📐 ALTERNATİV DİVİDER DİZAYNLARI

### 1:1 Divider (Hal-hazırda tətbiq edilib)

```
R1 = R2 = 10kΩ
Divisor = 0.5
Output = Input × 0.5
0.5V → 0.25V
5.0V → 2.50V ✅ Safe (< 3.3V)
```

### 3:2 Divider (Daha yüksək siqnal)

```
R1 = 10kΩ, R2 = 15kΩ
Divisor = 0.6
Output = Input × 0.6
0.5V → 0.30V
5.0V → 3.00V ✅ Safe (< 3.3V)
```

**Qeyd:** 3:2 divider daha yüksək ADC dəyərləri verir (daha yaxşı resolution), amma margin azdır.

---

## 📝 XÜLASƏİ

✅ **Hardware:** 2 ədəd 10kΩ rezistor əlavə et
✅ **Kod:** `VOLTAGE_DIVIDER_ENABLED 1` (artıq aktiv)
✅ **Test:** Sistem avtomatik kalibrasiya edəcək
✅ **Nəticə:** 0-300 bar tam ölçülür və güvənlidir

**UĞURLAR!** 🚀
