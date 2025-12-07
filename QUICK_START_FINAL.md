# 🚀 Tez Başlanğıc - Bir Komanda Hər Şey

## ✨ Avtomatik Script

Artıq bir komanda ilə hər şeyi işə sala bilərsiniz!

```powershell
cd renode
.\run_all.ps1
```

Bu script **avtomatik olaraq:**
1. ✅ Proyekti build edir
2. ✅ Renode emulyatoru işə salır
3. ✅ UI Simulator-ı açır

## 📋 Seçimlər

### Hamısını işə sal (Tövsiyə olunur)
```powershell
.\run_all.ps1
```

### Yalnız build et
```powershell
.\run_all.ps1 -BuildOnly
```

### Yalnız emulyator
```powershell
.\run_all.ps1 -EmulatorOnly
```

### Yalnız UI Simulator
```powershell
.\run_all.ps1 -UISimulatorOnly
```

## 🎯 Nə Görəcəksiniz

1. **Build prosesi** - Proyekt build olunur
2. **Renode konsolu** - Emulyator işləyir (background-da)
3. **UI Simulator pəncərəsi** - Proqramın UI-si görünür

## 💡 İpucu

Hər dəfə kod dəyişdikdən sonra:
```powershell
.\run_all.ps1
```

Script avtomatik build edəcək və yenidən işə salacaq!

## 📝 Qeyd

- Build uğursuz olsa, mövcud ELF faylından istifadə edəcək
- Renode yoxdursa, yalnız UI Simulator işləyəcək
- Python yoxdursa, yalnız emulyator işləyəcək

## 🔧 Problemlərin Həlli

### Problem: Build uğursuz
**Həll:** Script mövcud ELF faylından istifadə edəcək (əgər varsa)

### Problem: Renode tapılmır
**Həll:** Script emulyatoru atlayacaq, UI Simulator işləyəcək

### Problem: Python tapılmır
**Həll:** Script UI Simulator-u atlayacaq, emulyator işləyəcək

## ✅ Hazır!

İndi bir komanda ilə hər şeyi işə sala bilərsiniz!

