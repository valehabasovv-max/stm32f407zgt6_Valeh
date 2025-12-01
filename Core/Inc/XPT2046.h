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
 * ŞÜHÜNDİ PIN XƏYYANMASI:
 *   Touch CS   -> PB12 (Chip Select)
 *   Touch SCK  -> PF11 (Serial Clock) - DƏYİŞDİRİLDİ! (əvvəl PB1 idi)
 *   Touch MISO -> PF8  (Master In Slave Out)
 *   Touch MOSI -> PF9  (Master Out Slave In)
 *   Touch IRQ  -> PF10 (Interrupt Request)
 *
 * QEYD: SCK pini PB1-dən PF11-ə köçürüldü çünki
 * PB1 TIM3_CH4 (PWM çıxışı) ilə konflikt yaradırdı.
 * 
 * Hardware-da tel dəyişikliyi lazımdır:
 *   Touch moduldakı CLK/SCK pinini PB1-dən ayırıb PF11-ə bağlayın
 */
#define XPT2046_CS_PIN          GPIO_PIN_12
#define XPT2046_CS_PORT         GPIOB
#define XPT2046_IRQ_PIN         GPIO_PIN_10
#define XPT2046_IRQ_PORT        GPIOF
#define XPT2046_SCK_PIN         GPIO_PIN_11
#define XPT2046_SCK_PORT        GPIOF

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
