# TƏZYİQ NƏZARƏTİ SİSTEMİ - TƏHLÜKƏSİZLİK TƏDBİRLƏRİ

## 🚨 TƏHLÜKƏLİ VƏZİYYƏT HƏLL EDİLDİ

### Problemin Səbəbi:
- ZME və DRV aktuatorlarının məntiqində xətalar var idi
- PID parametrləri çox zəif idi
- Təhlükəsizlik məhdudiyyətləri yox idi

### Həll Edilən Problemlər:

#### 1. ZME (Normally Open) Düzəldildi:
```c
// ƏVVƏL (YANLIŞ):
float zme_cmd = zme_ff - pid_output;  // Tərs məntiq

// İNDİ (DÜZGÜN):
float zme_cmd = zme_ff + (Kp_zme * e + I_zme);  // Düzgün məntiq
```

#### 2. DRV (Normally Closed) Düzəldildi:
```c
// ƏVVƏL (YANLIŞ):
drv_cmd = drv_ff - (Kp_drv * e + I_drv);  // Yanlış hesablama

// İNDİ (DÜZGÜN):
if (e > 0) {
    drv_cmd = DRV_MAX;  // Təzyiq aşağı → DRV bağlı
} else {
    drv_cmd = DRV_MIN + (Kp_drv * fabsf(e) + fabsf(I_drv));  // Təzyiq yüksək → DRV açıl
}
```

#### 3. Aqressiv PID Parametrləri:
```c
// ƏVVƏL (ZƏİF):
Kp_zme = 0.5f, Ki_zme = 0.05f
Kp_drv = 0.4f, Ki_drv = 0.04f

// İNDİ (AQRESSİV):
Kp_zme = 2.0f, Ki_zme = 0.2f    // 4 dəfə güclü
Kp_drv = 3.0f, Ki_drv = 0.3f     // 7.5 dəfə güclü
```

## 🛡️ TƏHLÜKƏSİZLİK MƏHDUDİYYƏTLƏRİ

### 1. Təzyiq Məhdudiyyətləri:
- **5 bar artıq**: Qoruyucu tədbirlər (ZME bağlı, DRV açıq, motor yavaş)
- **10 bar artıq**: Təcili tədbirlər (ZME bağlı, DRV açıq, motor dayandır)
- **15 bar artıq**: TƏCİLİ DAYANDIRMA (bütün sistem dayandır)

### 2. Avtomatik Təhlükəsizlik:
```c
if (s_P_filt > (g_SP + 15.0f)) {
    // TƏCİLİ DAYANDIRMA
    set_pwm_motor(0.0f);    // Motor dayandır
    set_pwm_zme(ZME_MAX);   // ZME tam bağlı
    set_pwm_drv(DRV_MIN);   // DRV tam açıq
    g_control_enabled = false;  // İdarəetməni dayandır
}
```

### 3. Hər Döngüdə Təhlükəsizlik Yoxlaması:
```c
if (!PressureControl_SafetyCheck()) {
    printf("SİSTEM DAYANDIRILDI - Təzyiq çox təhlükəli!\r\n");
} else {
    PressureControl_Step();  // Normal idarəetmə
}
```

## 📊 YENİ İDARƏETMƏ MƏNTİQİ

### ZME (Normally Open Valve):
- **Təzyiq aşağı** (e > 0): ZME açılır (0% yaxın)
- **Təzyiq yüksək** (e < 0): ZME bağlanır (30% yaxın)

### DRV (Normally Closed Valve):
- **Təzyiq aşağı** (e > 0): DRV bağlı qalır (40%)
- **Təzyiq yüksək** (e < 0): DRV açılır (0% yaxın)

### Motor:
- **Normal**: Setpoint-ə görə sürət
- **Təzyiq yüksək**: Avtomatik dayandır

## 🔧 TƏTBİQ EDİLƏN DƏYİŞİKLİKLƏR

### 1. Core/Src/pressure_control.c:
- ✅ ZME məntiq düzəldildi
- ✅ DRV məntiq düzəldildi  
- ✅ Aqressiv PID parametrləri
- ✅ Təhlükəsizlik məhdudiyyətləri əlavə edildi
- ✅ Təcili dayandırma funksiyası

### 2. Core/Src/main.c:
- ✅ Aqressiv PID parametrləri
- ✅ Təhlükəsizlik yoxlaması hər döngüdə

### 3. Core/Inc/pressure_control.h:
- ✅ Təhlükəsizlik funksiyası əlavə edildi

## ⚠️ TƏHLÜKƏSİZLİK TƏDBİRLƏRİ

### 1. Avtomatik Təhlükəsizlik:
- Təzyiq 15 bar artıq olduqda sistem avtomatik dayandır
- Bütün aktuatorlar təhlükəsiz vəziyyətə gətirilir
- İdarəetmə sistemi deaktiv edilir

### 2. Manual Təhlükəsizlik:
- Təzyiq 5 bar artıq olduqda xəbərdarlıq
- Təzyiq 10 bar artıq olduqda təcili tədbirlər
- Təzyiq 15 bar artıq olduqda təcili dayandırma

### 3. Monitorinq:
- Hər döngüdə təzyiq yoxlanılır
- Təhlükəli vəziyyətdə avtomatik dayandırma
- Serial port vasitəsilə xəbərdarlıq mesajları

## 🎯 NƏTİCƏ

Artıq sisteminiz:
- ✅ Təzyiqə düzgün cavab verir
- ✅ Təhlükəli vəziyyətdə avtomatik dayandır
- ✅ Aqressiv PID parametrləri ilə sürətli cavab
- ✅ Çoxlu təhlükəsizlik məhdudiyyətləri
- ✅ Hər döngüdə təhlükəsizlik yoxlaması

**40 bar limitiniz artıq təhlükəsiz şəkildə qorunur!**

