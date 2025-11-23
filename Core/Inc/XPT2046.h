/**
  ******************************************************************************
  * @file    XPT2046.h
  * @brief   XPT2046 Touch controller driver header for STM32F407
  ******************************************************************************
  */

#ifndef __XPT2046_H
#define __XPT2046_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes */
#include "stm32f4xx_hal.h"

/* GPIO pin definitions */
#define XPT2046_CS_PIN          GPIO_PIN_12
#define XPT2046_CS_PORT         GPIOB
#define XPT2046_IRQ_PIN         GPIO_PIN_10
#define XPT2046_IRQ_PORT        GPIOF

/* XPT2046 Commands */
#define XPT2046_CMD_READ_X      0xD0  /* Read X position */
#define XPT2046_CMD_READ_Y      0x90  /* Read Y position */
#define XPT2046_CMD_READ_Z1     0xB0  /* Read Z1 pressure */
#define XPT2046_CMD_READ_Z2     0xC0  /* Read Z2 pressure */

/* Calibration values (adjust for your touch panel) */
#define XPT2046_X_MIN           379
#define XPT2046_X_MAX           3756
#define XPT2046_Y_MIN           335
#define XPT2046_Y_MAX           3774
#define XPT2046_Z_THRESHOLD     50

/* Function prototypes */
void XPT2046_Init(void);
uint8_t XPT2046_ReadRaw(uint16_t *x, uint16_t *y);
uint16_t XPT2046_ReadZ(void);
uint8_t XPT2046_IsTouched(void);
uint8_t XPT2046_GetCoordinates(uint16_t *x, uint16_t *y);
void XPT2046_ConvertToScreen(uint16_t raw_x, uint16_t raw_y, uint16_t *screen_x, uint16_t *screen_y);
uint8_t XPT2046_GetScreenCoordinates(uint16_t *screen_x, uint16_t *screen_y);
void XPT2046_Calibrate(void);
void XPT2046_SetCalibration(uint16_t x_min, uint16_t x_max, uint16_t y_min, uint16_t y_max);
void XPT2046_GetCalibration(uint16_t *x_min, uint16_t *x_max, uint16_t *y_min, uint16_t *y_max);
void XPT2046_SetCoordMode(uint8_t mode);
uint8_t XPT2046_GetCoordMode(void);

#ifdef __cplusplus
}
#endif
#endif /* __XPT2046_H */

