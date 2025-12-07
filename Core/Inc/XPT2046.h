/**
  ******************************************************************************
  * @file    XPT2046.h
  * @brief   XPT2046 Touch Controller Driver Header
  *          Tam işlək və kalibrasiyalı versiya
  ******************************************************************************
  */

#ifndef __XPT2046_H
#define __XPT2046_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes */
#include "stm32f4xx_hal.h"

/* =============== GPIO PIN TƏRİFLƏRİ =============== */
/* 
 * Bu pinləri öz hardware-ınıza görə dəyişdirin
 * 
 * PIN KONFİQURASİYASI:
 *   T_CS   (Chip Select)        -> PB12
 *   T_CLK  (SCK - Serial Clock) -> PB1
 *   T_DO   (MISO)               -> PF8
 *   T_DIN  (MOSI)               -> PF9
 *   T_IRQ  (Interrupt Request)  -> PF10
 */
#define XPT2046_CS_PIN          GPIO_PIN_12
#define XPT2046_CS_PORT         GPIOB
#define XPT2046_IRQ_PIN         GPIO_PIN_10
#define XPT2046_IRQ_PORT        GPIOF
#define XPT2046_SCK_PIN         GPIO_PIN_1
#define XPT2046_SCK_PORT        GPIOB
#define XPT2046_MISO_PIN        GPIO_PIN_8
#define XPT2046_MISO_PORT       GPIOF
#define XPT2046_MOSI_PIN        GPIO_PIN_9
#define XPT2046_MOSI_PORT       GPIOF

/* =============== XPT2046 KOMANDLARI =============== */
#define XPT2046_CMD_READ_X      0xD0  /* X mövqeyini oxu */
#define XPT2046_CMD_READ_Y      0x90  /* Y mövqeyini oxu */
#define XPT2046_CMD_READ_Z1     0xB0  /* Z1 basqısını oxu */
#define XPT2046_CMD_READ_Z2     0xC0  /* Z2 basqısını oxu */

/* =============== DEFAULT KALİBRASİYA DƏYƏRLƏRİ =============== */
/* Bu dəyərlər ekranınıza görə kalibrasiya olunmalıdır */
#define XPT2046_X_MIN           200
#define XPT2046_X_MAX           3900
#define XPT2046_Y_MIN           200
#define XPT2046_Y_MAX           3900
#define XPT2046_Z_THRESHOLD     50

/* =============== 3-NÖQTƏLİ KALİBRASİYA =============== */
/* Ekran ölçüləri */
#define TOUCH_SCREEN_WIDTH      320
#define TOUCH_SCREEN_HEIGHT     240

/* Kalibrasiya nöqtələri mövqeyi (piksel) */
#define CAL_POINT1_X            30      /* Sol üst */
#define CAL_POINT1_Y            30
#define CAL_POINT2_X            290     /* Sağ üst */
#define CAL_POINT2_Y            30
#define CAL_POINT3_X            160     /* Mərkəz alt */
#define CAL_POINT3_Y            210

/* Kalibrasiya nöqtə ölçüsü */
#define CAL_TARGET_SIZE         10
#define CAL_TARGET_COLOR        0xF800  /* Qırmızı */

/* Kalibrasiya strukturu */
typedef struct {
    /* 3 kalibrasiya nöqtəsinin raw dəyərləri */
    uint16_t raw_x[3];
    uint16_t raw_y[3];
    
    /* Ekran koordinatları (hədəf) */
    uint16_t screen_x[3];
    uint16_t screen_y[3];
    
    /* Kalibrasiya matrisi əmsalları */
    float a, b, c;  /* X üçün: screen_x = a*raw_x + b*raw_y + c */
    float d, e, f;  /* Y üçün: screen_y = d*raw_x + e*raw_y + f */
    
    /* Kalibrasiya bayrağı */
    uint8_t calibrated;
} TouchCalibration_t;

/* =============== AVTOMATİK DÜYMƏ KALİBRASİYASI =============== */
/* 
 * Bu sistem düyməyə toxunanda avtomatik olaraq kalibrasiya edir.
 * İstifadəçi düyməyə toxunduqda, sistem:
 * 1. Toxunuşun hansı düyməyə yaxın olduğunu müəyyən edir
 * 2. Raw koordinatları həmin düymənin mərkəzinə uyğunlaşdırır
 * 3. Offset-i hesablayır və kalibrasiyaya tətbiq edir
 */

/* Düymə strukturu - kalibrasiya üçün */
typedef struct {
    uint16_t x;          /* Düymənin sol kənarı */
    uint16_t y;          /* Düymənin üst kənarı */
    uint16_t w;          /* Düymənin eni */
    uint16_t h;          /* Düymənin hündürlüyü */
    const char* name;    /* Düymənin adı (debug üçün) */
    uint8_t id;          /* Düymənin ID-si */
} AutoCalButton_t;

/* Avtomatik kalibrasiya strukturu */
typedef struct {
    /* Öyrənilmiş offset dəyərləri */
    int16_t offset_x;        /* X offset (piksel) */
    int16_t offset_y;        /* Y offset (piksel) */
    
    /* Öyrənmə prosesi */
    int32_t sum_offset_x;    /* Kumulativ X offset */
    int32_t sum_offset_y;    /* Kumulativ Y offset */
    uint16_t sample_count;   /* Nümunə sayı */
    
    /* Kalibrasiya vəziyyəti */
    uint8_t is_calibrated;   /* Kalibrasiya olunubmu? */
    uint8_t learning_mode;   /* Öyrənmə rejimi aktivdir? */
    uint8_t last_button_id;  /* Son basılan düymənin ID-si */
    
    /* Konfiqurasiya */
    uint16_t min_samples;    /* Minimum nümunə sayı (stabil kalibrasiya üçün) */
    uint16_t proximity_threshold; /* Düymə yaxınlıq həddi (piksel) */
    
    /* Statistika */
    uint32_t total_touches;  /* Ümumi toxunuş sayı */
    uint32_t matched_touches;/* Düyməyə uyğun gələn toxunuş sayı */
} AutoCalibration_t;

/* Maksimum düymə sayı */
#define AUTO_CAL_MAX_BUTTONS    32

/* Default konfiqurasiya dəyərləri */
#define AUTO_CAL_MIN_SAMPLES        3     /* Minimum 3 nümunə lazımdır - daha sürətli öyrənmə */
#define AUTO_CAL_PROXIMITY          120   /* 120 piksel yaxınlıq həddi - daha tolerant */
#define AUTO_CAL_MAX_OFFSET         150   /* Maksimum qəbul edilən offset - böyük sapmalar üçün */

/* =============== FUNKSİYA PROTOTİPLƏRİ =============== */

/* Başlatma */
void XPT2046_Init(void);

/* Raw oxuma */
uint8_t XPT2046_ReadRaw(uint16_t *x, uint16_t *y);
uint16_t XPT2046_ReadZ(void);

/* Touch algılama */
uint8_t XPT2046_IsTouched(void);
uint8_t XPT2046_GetCoordinates(uint16_t *x, uint16_t *y);

/* Ekran koordinatları */
void XPT2046_ConvertToScreen(uint16_t raw_x, uint16_t raw_y, 
                             uint16_t *screen_x, uint16_t *screen_y);
uint8_t XPT2046_GetScreenCoordinates(uint16_t *screen_x, uint16_t *screen_y);

/* Kalibrasiya */
void XPT2046_Calibrate(void);
void XPT2046_SetCalibration(uint16_t x_min, uint16_t x_max, 
                            uint16_t y_min, uint16_t y_max);
void XPT2046_GetCalibration(uint16_t *x_min, uint16_t *x_max, 
                            uint16_t *y_min, uint16_t *y_max);

/* 3-nöqtəli kalibrasiya */
uint8_t XPT2046_Calibrate3Point(void);                      /* 3 nöqtəli kalibrasiya başlat */
void XPT2046_SetCalibrationMatrix(TouchCalibration_t* cal); /* Kalibrasiya matrisini təyin et */
TouchCalibration_t* XPT2046_GetCalibrationData(void);       /* Kalibrasiya məlumatlarını al */
uint8_t XPT2046_IsCalibrated(void);                         /* Kalibrasiya olunubmu? */
void XPT2046_ConvertWithMatrix(uint16_t raw_x, uint16_t raw_y,
                               uint16_t *screen_x, uint16_t *screen_y); /* Matris ilə çevir */
void XPT2046_SetCalibrationPoint(uint8_t point_idx, uint16_t raw_x, uint16_t raw_y); /* Kalibrasiya nöqtəsi təyin et */
void XPT2046_FinishCalibration(void);                       /* Kalibrasiya tamamla */
void XPT2046_LoadDefaultCalibration(void);                  /* Default kalibrasiya yüklə */
uint8_t XPT2046_FindBestCoordMode(uint16_t raw_x, uint16_t raw_y,
                                   uint16_t expected_x, uint16_t expected_y); /* Ən yaxşı koordinat modunu tap */

/* Koordinat rejimi */
void XPT2046_SetCoordMode(uint8_t mode);
uint8_t XPT2046_GetCoordMode(void);

/* Debug */
void XPT2046_SetDebugMode(uint8_t enable);

/* Köməkçi funksiyalar */
uint8_t XPT2046_IsButtonPressed(uint16_t btn_x, uint16_t btn_y, 
                                uint16_t btn_w, uint16_t btn_h,
                                uint16_t touch_x, uint16_t touch_y);
uint8_t XPT2046_GetTouchDebounced(uint16_t *screen_x, uint16_t *screen_y, 
                                  uint32_t debounce_ms);

/* Self-test: read raw X/Y samples ignoring IRQ and print to serial (debug only) */
void XPT2046_SelfTest(uint8_t samples);

/* =============== AVTOMATİK DÜYMƏ KALİBRASİYA FUNKSİYALARI =============== */

/**
 * @brief Avtomatik kalibrasiya sistemini başlat
 */
void XPT2046_AutoCal_Init(void);

/**
 * @brief Düymə əlavə et (kalibrasiya üçün)
 * @param x, y: Düymənin sol üst küncü
 * @param w, h: Düymənin ölçüsü
 * @param name: Düymənin adı (debug)
 * @param id: Düymənin unikal ID-si
 * @return 1 uğurlu, 0 xəta
 */
uint8_t XPT2046_AutoCal_RegisterButton(uint16_t x, uint16_t y, 
                                        uint16_t w, uint16_t h,
                                        const char* name, uint8_t id);

/**
 * @brief Bütün düymələri sil
 */
void XPT2046_AutoCal_ClearButtons(void);

/**
 * @brief Toxunuşu işlə və avtomatik kalibrasiya et
 * @param raw_x, raw_y: Raw touch koordinatları
 * @param screen_x, screen_y: Çevrilmiş ekran koordinatları (düzəldilmiş)
 * @return Uyğun gələn düymənin ID-si (0 = heç bir düymə)
 */
uint8_t XPT2046_AutoCal_ProcessTouch(uint16_t raw_x, uint16_t raw_y,
                                      uint16_t *screen_x, uint16_t *screen_y);

/**
 * @brief Öyrənmə rejimini aktiv/deaktiv et
 * @param enable: 1 = aktiv, 0 = deaktiv
 */
void XPT2046_AutoCal_SetLearning(uint8_t enable);

/**
 * @brief Kalibrasiya olunubmu yoxla
 * @return 1 kalibrasiya olunub, 0 olunmayıb
 */
uint8_t XPT2046_AutoCal_IsCalibrated(void);

/**
 * @brief Kalibrasiyani sıfırla
 */
void XPT2046_AutoCal_Reset(void);

/**
 * @brief Offset dəyərlərini al
 * @param offset_x, offset_y: Offset dəyərləri üçün pointer
 */
void XPT2046_AutoCal_GetOffset(int16_t *offset_x, int16_t *offset_y);

/**
 * @brief Offset dəyərlərini manual təyin et
 * @param offset_x, offset_y: Yeni offset dəyərləri
 */
void XPT2046_AutoCal_SetOffset(int16_t offset_x, int16_t offset_y);

/**
 * @brief Statistikanı al
 * @param total: Ümumi toxunuş sayı
 * @param matched: Düyməyə uyğun gələn sayı
 * @param samples: Kalibrasiya nümunə sayı
 */
void XPT2046_AutoCal_GetStats(uint32_t *total, uint32_t *matched, uint16_t *samples);

/**
 * @brief Ən yaxın düyməni tap
 * @param screen_x, screen_y: Ekran koordinatları
 * @param btn_id: Tapılan düymənin ID-si
 * @param distance: Məsafə
 * @return 1 düymə tapıldı, 0 tapılmadı
 */
uint8_t XPT2046_AutoCal_FindNearestButton(uint16_t screen_x, uint16_t screen_y,
                                           uint8_t *btn_id, uint16_t *distance);

/**
 * @brief Düzəldilmiş ekran koordinatlarını al
 * @param raw_x, raw_y: Raw koordinatlar
 * @param screen_x, screen_y: Düzəldilmiş ekran koordinatları
 */
void XPT2046_AutoCal_GetCorrectedCoords(uint16_t raw_x, uint16_t raw_y,
                                         uint16_t *screen_x, uint16_t *screen_y);

/**
 * @brief Proximity threshold təyin et
 * @param threshold: Yeni threshold dəyəri (piksel)
 */
void XPT2046_AutoCal_SetProximity(uint16_t threshold);

/**
 * @brief Debug məlumatlarını çap et
 */
void XPT2046_AutoCal_PrintDebug(void);

#ifdef __cplusplus
}
#endif

#endif /* __XPT2046_H */
