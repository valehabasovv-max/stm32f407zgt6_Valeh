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

#ifdef __cplusplus
}
#endif

#endif /* __XPT2046_H */
