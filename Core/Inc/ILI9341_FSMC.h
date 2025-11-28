#ifndef ILI9341_FSMC_H
#define ILI9341_FSMC_H

#include "main.h"

/* ---------------- FSMC ünvanları (NE4 + A10) ---------------- */
#define LCD_BASE_ADDRESS   ((uint32_t)0x6C000000U)
#define LCD_REG            (*(__IO uint16_t *)(LCD_BASE_ADDRESS))
#define LCD_RAM            (*(__IO uint16_t *)(LCD_BASE_ADDRESS | 0x0800U))  /* A10=1 => +0x0800 */

/* ---------------- İstifadəçi GPIO-ları ---------------- */
#define LCD_RST_GPIO_Port   GPIOC
#define LCD_RST_Pin         GPIO_PIN_5

#define LCD_LIG_GPIO_Port   GPIOB
#define LCD_LIG_Pin         GPIO_PIN_0

/* ---------------- Rənglər (RGB565) ---------------- */
#define ILI9341_COLOR_BLACK   0x0000
#define ILI9341_COLOR_WHITE   0xFFFF
#define ILI9341_COLOR_RED     0xF800
#define ILI9341_COLOR_GREEN   0x07E0
#define ILI9341_COLOR_BLUE    0x001F
#define ILI9341_COLOR_YELLOW  0xFFE0
#define ILI9341_COLOR_CYAN    0x07FF
#define ILI9341_COLOR_ORANGE  0xFD20
#define ILI9341_COLOR_MAGENTA 0xF81F

/* İctimai funksiyalar */
void ILI9341_Init(void);
void ILI9341_FillScreen(uint16_t color);
void ILI9341_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void ILI9341_DrawRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void ILI9341_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void ILI9341_DrawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bgcolor, uint8_t size);
void ILI9341_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bgcolor, uint8_t size);

/* Menu funksiyaları */
void ILI9341_ShowMainPage(void);
void ILI9341_ShowSettingsPage(void);
void ILI9341_ShowTestPage(void);
void ILI9341_ShowInfoPage(void);
void ILI9341_ShowTouchTestPage(void);
void ILI9341_HandleTouch(void);
void ILI9341_DebugTouch(void);
void ILI9341_TestTouch(void);

/* Pressure Control System Functions */
void ILI9341_ShowPressureControlMain(void);
void ILI9341_ShowMenuPage(void);
/* REMOVED: PWM page functions - PWM bölməsi silindi
void ILI9341_ShowPWMPage(void);
void ILI9341_ShowDRVPWMPage(void);
void ILI9341_ShowZMEPWMPage(void);
void ILI9341_ShowMotorPWMPage(void);
*/
/* REMOVED: Pressure Limit page function - PRES LIM bölməsi silindi
void ILI9341_ShowPressureLimitPage(void);
*/
void ILI9341_ShowCalibrationPage(void);
void ILI9341_UpdatePressureDisplay(float pressure);
void ILI9341_UpdatePercentageDisplays(float drv_percent, float zme_percent, float motor_percent);
void ILI9341_HandlePressureControlTouch(void);
void ILI9341_PressureControlLogic(void);
void ILI9341_SimpleTouchTest(void);

/* PWM Control Functions */
void PWM_SetMotorDutyCycle(float duty_percent);
void PWM_SetDRVDutyCycle(float duty_percent);
void PWM_SetZMEDutyCycle(float duty_percent);
void PWM_SetMotorFrequency(float frequency_hz);
void PWM_UpdateAllChannels(void);
void PWM_TestSequence(void);

/* Pressure Sensor Calibration Functions */
void ILI9341_ShowCalibrationPage(void);
void ILI9341_ShowPressureCalibrationPage(void);
void ILI9341_ShowPIDTuningPage(void);
void ILI9341_HandleCalibrationTouch(void);
/* 
 * REMOVED: PressureSensor_* funksiyaları silindi
 * 
 * Bu funksiyalar dublikat kalibrləmə məntiqinə səbəb olurdu.
 * Bunun əvəzinə AdvancedPressureControl_* funksiyalarını istifadə edin:
 * - AdvancedPressureControl_LoadCalibration() - Flash-dan kalibrləmə yükləmə
 * - AdvancedPressureControl_SaveCalibration() - Flash-a kalibrləmə yazma
 * - AdvancedPressureControl_ReadPressure() - Təzyiq oxuma
 * 
 * Kalibrləmə məntiqi yalnız Advanced sistem daxilində (advanced_pressure_control.c)
 * saxlanılmalıdır ki, vahid kalibrləmə məlumatları istifadə edilsin.
 */
void PressureSensor_DebugStatus(void);
void PressureSensor_CheckPinConfiguration(void);

/* Precise conversion functions */
float PressureToMotorPercent(float pressure);
float MotorPercentToPressure(float motor_percent);
float PressureToDRVPercent(float pressure);
float PressureToZMEPercent(float pressure);

/* Auto Mode Control Functions - REMOVED (AutoMode deleted) */

/* Global variables */
extern float pressure_limit;  /* Pressure limit (SP) value - can be modified externally */

#endif /* ILI9341_FSMC_H */
