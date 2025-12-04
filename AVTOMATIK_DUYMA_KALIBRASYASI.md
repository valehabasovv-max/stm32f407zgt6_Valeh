# Avtomatik Düymə Kalibrasiya Sistemi

## Ümumi Məlumat

Bu sistem ekran düymələrini avtomatik olaraq kalibrasiya edir. İstifadəçi düyməyə toxunduqda, sistem öyrənir və sonrakı toxunuşlarda daha dəqiq nəticə verir.

## Necə İşləyir

### 1. Başlanğıc
- Sistem başladıqda, bütün düymələr qeydiyyata alınır (x, y, eni, hündürlüyü)
- Öyrənmə rejimi avtomatik olaraq aktivləşir
- 3 saniyə gözləyib əsas ekrana keçir

### 2. Öyrənmə Prosesi
- İstifadəçi ekrana toxunduqda, sistem toxunuşun hansı düyməyə yaxın olduğunu müəyyən edir
- Toxunuşun düymə mərkəzindən fərqini (offset) hesablayır
- Minimum 3 toxunuşdan sonra orta offset hesablanır
- Bu offset bütün gələcək toxunuşlara tətbiq edilir

### 3. Kalibrasiya Tamamlandı
- "CAL OK" yazısı yaşıl rəngdə göstərilir
- Offset dəyərləri ekranda göstərilir (məs: +15, -8)
- Sistem artıq düzəldilmiş koordinatlar istifadə edir

## Ekran Göstəriciləri

### Başlıq Paneli (Üst sıra)
```
<M     VALEH HPC    LEARN     M3>
```
- `<M` - Mode azalt (toxun)
- `LEARN` - Öyrənmə prosesi davam edir (sarı)
- `[RST]` - Kalibrasiya sıfırla (yaşıl, toxun)
- `M3>` - Mode artır (toxun, M0-M7)

### Status Göstəricisi (Sol alt)
```
CAL OK(+15,-8)   veya   LEARN 2/3
```
- `CAL OK(+X,+Y)` - Kalibrasiya tamamlandı, offset dəyərləri
- `LEARN N/3` - Öyrənmə prosesi, N nümunə toplandı

## Düymə Zonaları

### Əsas Ekran (PAGE_MAIN)
| Düymə | Mövqe | Ölçü | ID |
|-------|-------|------|-----|
| START/STOP | (10, 208) | 70x25 | 1 |
| MENU | (85, 208) | 70x25 | 2 |
| SP- | (160, 208) | 35x25 | 3 |
| SP+ | (200, 208) | 35x25 | 4 |
| PRESET 0-5 | Grid | 32x35 | 10-15 |

### Menyu Ekranı (PAGE_MENU)
| Düymə | Mövqe | Ölçü | ID |
|-------|-------|------|-----|
| SETPOINT | (40, 45) | 240x35 | 30 |
| PID TUNE | (40, 90) | 240x35 | 31 |
| CALIBRATION | (40, 135) | 240x35 | 32 |
| BACK | (40, 180) | 240x35 | 33 |

## Kalibrasiya Sıfırlama

Əgər kalibrasiya yanlış getsə:
1. Başlıq panelinin mərkəzinə toxunun (`[RST]` və ya `LEARN`)
2. "CAL RESET!" mesajı görünəcək
3. Sistem yenidən öyrənməyə başlayacaq

## Texniki Detallar

### Konfiqurasiya
```c
#define AUTO_CAL_MIN_SAMPLES    3     // Minimum nümunə sayı
#define AUTO_CAL_PROXIMITY      120   // Yaxınlıq həddi (piksel)
#define AUTO_CAL_MAX_OFFSET     150   // Maksimum offset
```

### API Funksiyaları
```c
void XPT2046_AutoCal_Init(void);              // Başlat
void XPT2046_AutoCal_RegisterButton(...);     // Düymə əlavə et
uint8_t XPT2046_AutoCal_ProcessTouch(...);    // Toxunuşu işlə
void XPT2046_AutoCal_Reset(void);             // Sıfırla
uint8_t XPT2046_AutoCal_IsCalibrated(void);   // Status yoxla
void XPT2046_AutoCal_GetOffset(...);          // Offset al
void XPT2046_AutoCal_PrintDebug(void);        // Debug çap et
```

## Üstünlüklər

1. **Manual kalibrasiya lazım deyil** - 3-nöqtəli kalibrasiya artıq tələb olunmur
2. **Real-time öyrənmə** - İstifadəçi istifadə etdikcə sistem özünü yaxşılaşdırır
3. **Sadə sıfırlama** - Yanlış getdikdə bir toxunuşla sıfırlama
4. **Vizual feedback** - Kalibrasiya vəziyyəti həmişə görünür

## Problemlərin Həlli

### Düymələr hələ də işləmir
1. Sistem başladıqda 3-5 dəfə düymələrə toxunun
2. `LEARN 3/3` mesajını gözləyin
3. `CAL OK` yaşıl olduqda düymələr işləyəcək

### Yanlış kalibrasiya
1. Başlıq panelinin mərkəzinə toxunun
2. `CAL RESET!` mesajını gözləyin
3. Yenidən düymələrə toxunun

### Hələ də problem varsa
1. Serial portdan debug mesajlarına baxın
2. `XPT2046_AutoCal_PrintDebug()` çağırın
3. Mode dəyişdirin (M0-M7 sınayın)
