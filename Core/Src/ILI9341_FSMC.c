#include "ILI9341_FSMC.h"
#include "XPT2046.h"
#include "font.h"
#include "pressure_control_config.h"
#include "advanced_pressure_control.h"
#include "stm32f4xx_hal.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

/* External handles */
extern SPI_HandleTypeDef hspi1;
extern ADC_HandleTypeDef hadc3;

/* XPT2046 Commands */
#define XPT2046_CMD_READ_X      0x90  /* Read X position */
#define XPT2046_CMD_READ_Y      0xD0  /* Read Y position */

/* FSMC init funksiyası main.c-də var, burada dublikat yox */

/* Sadə yazma köməkçiləri */
static inline void lcd_write_command(uint16_t cmd)
{
    LCD_REG = cmd;
}
static inline void lcd_write_data(uint16_t data)
{
    LCD_RAM = data;
}

/* ------------------ ILI9341 INIT ------------------ */
void ILI9341_Init(void)
{
    /* Backlight ON */
    HAL_GPIO_WritePin(LCD_LIG_GPIO_Port, LCD_LIG_Pin, GPIO_PIN_SET);

    /* HW reset pulse */
    HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(120);

    /* ILI9341 initialization sequence */
    lcd_write_command(0x01); HAL_Delay(5);    /* Software Reset */
    lcd_write_command(0x28);                  /* Display OFF */

    lcd_write_command(0x3A);                  /* Pixel Format Set */
    lcd_write_data(0x55);                     /* 16-bit/pixel */

    lcd_write_command(0x36);                  /* Memory Access Control */
    lcd_write_data(0x28);                     /* MX=0, MY=1, MV=0, BGR=1 (Landscape 320x240) */

    /* Display ON */
    lcd_write_command(0x11); HAL_Delay(120);  /* Sleep Out */
    lcd_write_command(0x29);                  /* Display ON */
}

/* ------------------ Ekranı rənglə doldur ------------------ */
void ILI9341_FillScreen(uint16_t color)
{
    uint32_t px = 320UL * 240UL;

    /* RAM write (CASET/RASET landscape 320x240) */
    lcd_write_command(0x2A);  /* Column addr set */
    lcd_write_data(0x00); lcd_write_data(0x00);
    lcd_write_data(0x01); lcd_write_data(0x3F); /* 319 */

    lcd_write_command(0x2B);  /* Row addr set */
    lcd_write_data(0x00); lcd_write_data(0x00);
    lcd_write_data(0x00); lcd_write_data(0xEF); /* 239 */

    lcd_write_command(0x2C);  /* RAMWR */

    while (px--) {
        LCD_RAM = color;
    }
}

/* ------------------ Pixel çək ------------------ */
void ILI9341_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= 320 || y >= 240) return;

    /* Set address */
    lcd_write_command(0x2A);
    lcd_write_data(x >> 8); lcd_write_data(x & 0xFF);
    lcd_write_data(x >> 8); lcd_write_data(x & 0xFF);

    lcd_write_command(0x2B);
    lcd_write_data(y >> 8); lcd_write_data(y & 0xFF);
    lcd_write_data(y >> 8); lcd_write_data(y & 0xFF);

    lcd_write_command(0x2C);
    LCD_RAM = color;
}

/* ------------------ Düzbucaqlı çək ------------------ */
void ILI9341_DrawRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    for (uint16_t i = 0; i < height; i++) {
        for (uint16_t j = 0; j < width; j++) {
            ILI9341_DrawPixel(x + j, y + i, color);
        }
    }
}

/* ------------------ Xətt çək ------------------ */
void ILI9341_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    int16_t dx = abs(x2 - x1);
    int16_t dy = abs(y2 - y1);
    int16_t sx = (x1 < x2) ? 1 : -1;
    int16_t sy = (y1 < y2) ? 1 : -1;
    int16_t err = dx - dy;

    while (1) {
        ILI9341_DrawPixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int16_t e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
}

/* ------------------ Menu sistemi ------------------ */
static uint8_t current_page = 0;

typedef struct {
    uint16_t x, y, width, height;
    uint16_t color;
    char text[20];
} Button_t;

void ILI9341_DrawButton(Button_t *btn)
{
    // 3D button effect - raised appearance
    // Main button area
    ILI9341_DrawRectangle(btn->x, btn->y, btn->width, btn->height, btn->color);
    
    // Top and left edges (light)
    ILI9341_DrawLine(btn->x, btn->y, btn->x + btn->width - 1, btn->y, ILI9341_COLOR_WHITE);
    ILI9341_DrawLine(btn->x, btn->y, btn->x, btn->y + btn->height - 1, ILI9341_COLOR_WHITE);
    
    // Bottom and right edges (dark)
    ILI9341_DrawLine(btn->x, btn->y + btn->height - 1, btn->x + btn->width - 1, btn->y + btn->height - 1, ILI9341_COLOR_BLACK);
    ILI9341_DrawLine(btn->x + btn->width - 1, btn->y, btn->x + btn->width - 1, btn->y + btn->height - 1, ILI9341_COLOR_BLACK);
    
    // Inner shadow
    ILI9341_DrawRectangle(btn->x + 2, btn->y + 2, btn->width - 4, btn->height - 4, ILI9341_COLOR_BLACK);
}

uint8_t ILI9341_IsButtonPressed(Button_t *btn, uint16_t touch_x, uint16_t touch_y)
{
    return (touch_x >= btn->x && touch_x <= btn->x + btn->width &&
            touch_y >= btn->y && touch_y <= btn->y + btn->height);
}

/* Ana menu səhifəsi */
void ILI9341_ShowMainPage(void)
{
    current_page = 0;
    
    /* Qara fon */
    ILI9341_FillScreen(ILI9341_COLOR_BLACK);
    
    /* Başlıq */
    ILI9341_DrawString(100, 20, "MAIN MENU", ILI9341_COLOR_WHITE, 0x0000, 3);
    
    /* Settings düyməsi */
    Button_t settings_btn = {20, 80, 130, 40, ILI9341_COLOR_BLUE, "Settings"};
    ILI9341_DrawButton(&settings_btn);
    ILI9341_DrawString(50, 95, "SETTINGS", ILI9341_COLOR_WHITE, 0x0000, 2);
    
    /* Test düyməsi */
    Button_t test_btn = {170, 80, 130, 40, ILI9341_COLOR_GREEN, "Test"};
    ILI9341_DrawButton(&test_btn);
    ILI9341_DrawString(200, 95, "TEST", ILI9341_COLOR_WHITE, 0x0000, 2);
    
    /* Info düyməsi */
    Button_t info_btn = {20, 140, 130, 40, ILI9341_COLOR_YELLOW, "Info"};
    ILI9341_DrawButton(&info_btn);
    ILI9341_DrawString(50, 155, "INFO", ILI9341_COLOR_WHITE, 0x0000, 2);
}

/* Touch test səhifəsi */
void ILI9341_ShowTouchTestPage(void)
{
    current_page = 4;
    
    /* Qara fon */
    ILI9341_FillScreen(ILI9341_COLOR_BLACK);
    
    /* Başlıq */
    ILI9341_DrawRectangle(10, 5, 300, 20, ILI9341_COLOR_RED);
    ILI9341_DrawRectangle(12, 7, 296, 16, ILI9341_COLOR_WHITE);
    ILI9341_DrawString(120, 12, "TOUCH TEST", ILI9341_COLOR_RED, 0xFFFF, 1);
    
    /* Təlimatlar */
    ILI9341_DrawString(10, 37, "Touch the screen", ILI9341_COLOR_WHITE, 0x0000, 1);
    ILI9341_DrawString(10, 57, "to see red dots", ILI9341_COLOR_WHITE, 0x0000, 1);
    
    /* Geri düyməsi */
    Button_t back_btn = {20, 200, 280, 35, ILI9341_COLOR_RED, "Back"};
    ILI9341_DrawButton(&back_btn);
    ILI9341_DrawString(120, 215, "BACK TO MENU", ILI9341_COLOR_WHITE, 0x0000, 2);
}

/* Touch handling */
void ILI9341_HandleTouch(void)
{
    uint16_t raw_x, raw_y, screen_x, screen_y;
    
    /* Touch sensor işləyirmi test et */
    if (XPT2046_IsTouched()) {
        /* Raw koordinatları oxu */
        if (XPT2046_GetCoordinates(&raw_x, &raw_y)) {
            /* Raw koordinatları ekran koordinatlarına çevir */
            XPT2046_ConvertToScreen(raw_x, raw_y, &screen_x, &screen_y);
            
            /* Touch nöqtəsini göstərmə - silindi */
            
            /* Menu navigation */
            if (current_page == 0) { // Main page
                /* Settings button */
                if (screen_x >= 20 && screen_x <= 150 && screen_y >= 80 && screen_y <= 120) {
                    current_page = 1;
                    ILI9341_ShowSettingsPage();
                }
                /* Test button - geniş koordinatlar */
                else if (screen_x >= 160 && screen_x <= 310 && screen_y >= 70 && screen_y <= 130) {
                    /* Debug: Test düyməsi basıldı - ekranın mərkəzində böyük mavi dairə */
                    for(int i = -15; i <= 15; i++) {
                        for(int j = -15; j <= 15; j++) {
                            if ((i*i + j*j) <= 225) {
                                ILI9341_DrawPixel(160 + i, 120 + j, ILI9341_COLOR_BLUE);
                            }
                        }
                    }
                    current_page = 2;
                    ILI9341_ShowTestPage();
                }
                /* Info button */
                else if (screen_x >= 20 && screen_x <= 150 && screen_y >= 140 && screen_y <= 180) {
                    current_page = 3;
                    ILI9341_ShowInfoPage();
                }
            }
            else { // Other pages
                /* Back button - bütün ekran */
                if (screen_x >= 0 && screen_x <= 319 && screen_y >= 0 && screen_y <= 239) {
                    current_page = 0;
                    ILI9341_ShowMainPage();
                }
            }
        }
        
        HAL_Delay(200); /* Debounce */
    }
}

/* Touch debug - silindi */
void ILI9341_DebugTouch(void)
{
    /* Heçnə etmə */
}

/* Touch test - silindi */
void ILI9341_TestTouch(void)
{
    /* Heçnə etmə */
}

/* Settings səhifəsi */
void ILI9341_ShowSettingsPage(void)
{
    current_page = 1;
    
    /* Yaşıl fon */
    ILI9341_FillScreen(ILI9341_COLOR_GREEN);
    
    /* Başlıq */
    ILI9341_DrawRectangle(10, 10, 300, 30, ILI9341_COLOR_BLUE);
    ILI9341_DrawRectangle(12, 12, 296, 26, ILI9341_COLOR_WHITE);
    ILI9341_DrawString(120, 20, "SETTINGS", ILI9341_COLOR_BLUE, 0xFFFF, 2);
    
    /* Məzmun */
    ILI9341_DrawString(50, 60, "LCD: 320x240", ILI9341_COLOR_WHITE, 0x0000, 2);
    ILI9341_DrawString(50, 80, "Touch: XPT2046", ILI9341_COLOR_WHITE, 0x0000, 2);
    ILI9341_DrawString(50, 100, "MCU: STM32F407", ILI9341_COLOR_WHITE, 0x0000, 2);
    ILI9341_DrawString(50, 120, "Driver: ILI9341", ILI9341_COLOR_WHITE, 0x0000, 2);
    
    /* Geri düyməsi */
    Button_t back_btn = {20, 200, 280, 35, ILI9341_COLOR_RED, "Back"};
    ILI9341_DrawButton(&back_btn);
    ILI9341_DrawString(120, 215, "BACK TO MENU", ILI9341_COLOR_WHITE, 0x0000, 2);
}

/* Test səhifəsi */
void ILI9341_ShowTestPage(void)
{
    current_page = 2;
    
    /* Qırmızı fon */
    ILI9341_FillScreen(ILI9341_COLOR_RED);
    
    /* Başlıq */
    ILI9341_DrawRectangle(10, 10, 300, 30, ILI9341_COLOR_BLUE);
    ILI9341_DrawRectangle(12, 12, 296, 26, ILI9341_COLOR_WHITE);
    ILI9341_DrawString(140, 20, "TEST", ILI9341_COLOR_BLUE, 0xFFFF, 2);
    
    /* Test məzmunu */
    ILI9341_DrawString(50, 60, "Color Test: RED", ILI9341_COLOR_WHITE, 0x0000, 2);
    ILI9341_DrawString(50, 80, "Font Test: ABC abc 123", ILI9341_COLOR_WHITE, 0x0000, 2);
    ILI9341_DrawString(50, 100, "Touch Test: Touch buttons", ILI9341_COLOR_WHITE, 0x0000, 2);
    
    /* Geri düyməsi */
    Button_t back_btn = {20, 200, 280, 35, ILI9341_COLOR_RED, "Back"};
    ILI9341_DrawButton(&back_btn);
    ILI9341_DrawString(120, 215, "BACK TO MENU", ILI9341_COLOR_WHITE, 0x0000, 2);
}

/* Info səhifəsi */
void ILI9341_ShowInfoPage(void)
{
    current_page = 3;
    
    /* Sarı fon */
    ILI9341_FillScreen(ILI9341_COLOR_YELLOW);
    
    /* Başlıq */
    ILI9341_DrawRectangle(10, 10, 300, 30, ILI9341_COLOR_BLUE);
    ILI9341_DrawRectangle(12, 12, 296, 26, ILI9341_COLOR_WHITE);
    ILI9341_DrawString(140, 20, "INFO", ILI9341_COLOR_BLUE, 0xFFFF, 2);
    
    /* Məlumat */
    ILI9341_DrawString(50, 60, "STM32F407ZGT6", ILI9341_COLOR_BLACK, 0x0000, 2);
    ILI9341_DrawString(50, 80, "TFT 3.2 LCD", ILI9341_COLOR_BLACK, 0x0000, 2);
    ILI9341_DrawString(50, 100, "ILI9341 Driver", ILI9341_COLOR_BLACK, 0x0000, 2);
    ILI9341_DrawString(50, 120, "XPT2046 Touch", ILI9341_COLOR_BLACK, 0x0000, 2);
    ILI9341_DrawString(50, 140, "FSMC Interface", ILI9341_COLOR_BLACK, 0x0000, 2);
    
    /* Geri düyməsi */
    Button_t back_btn = {20, 200, 280, 35, ILI9341_COLOR_RED, "Back"};
    ILI9341_DrawButton(&back_btn);
    ILI9341_DrawString(120, 215, "BACK TO MENU", ILI9341_COLOR_WHITE, 0x0000, 2);
}

/* ------------------ Font funksiyaları ------------------ */
void ILI9341_DrawChar(uint16_t x, uint16_t y, char c, uint16_t color, uint16_t bgcolor, uint8_t size)
{
    if (c < 32 || c > 126) return;
    
    const uint8_t *font_data = &font_7x10.data[(c - 32) * 7];
    
    for (uint8_t i = 0; i < 7; i++) {
        uint8_t line = font_data[i];
        for (uint8_t j = 0; j < 10; j++) {
            if (line & (1 << j)) {
                // Pixel çək
                for (uint8_t sx = 0; sx < size; sx++) {
                    for (uint8_t sy = 0; sy < size; sy++) {
                        ILI9341_DrawPixel(x + i * size + sx, y + j * size + sy, color);
                    }
                }
            } else if (bgcolor != color) {
                // Background pixel çək
            for (uint8_t sx = 0; sx < size; sx++) {
                for (uint8_t sy = 0; sy < size; sy++) {
                        ILI9341_DrawPixel(x + i * size + sx, y + j * size + sy, bgcolor);
                    }
                }
            }
        }
    }
}

void ILI9341_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bgcolor, uint8_t size)
{
    uint16_t start_x = x;
    while (*str) {
        if (*str == '\n') {
            y += 12 * size; // 10 + 2 space
            x = start_x;
        } else {
            ILI9341_DrawChar(x, y, *str, color, bgcolor, size);
            x += 8 * size; // 7 + 1 space
        }
        str++;
    }
}

/* ------------------ Pressure Control System ------------------ */

/* Global variables for pressure control system */
static uint8_t pressure_control_page = 0; // 0=main, 1=menu, 7=touch_cal, 8=pressure_cal, 9=pid_tune
// REMOVED: 2=pwm, 3=pressure_limit, 4=drv, 5=zme, 6=motor - PWM və PRES LIM bölmələri silindi
static float current_pressure = 0.0;
// REMOVED: float pressure_limit = 300.0; // SİLİNDİ - artıq g_system_status.target_pressure istifadə edilir
static float drv_percent = 0.0;
static float zme_percent = 0.0;
static float motor_percent = 0.0;

/* PWM control variables */
static float drv_duty_cycle = 50.0;
static float drv_frequency = 1000.0;
static float zme_duty_cycle = 50.0;
static float motor_duty_cycle = 50.0;
static float motor_frequency = 1000.0;

/* Pressure sensor calibration variables */
/* Note: These are NOT static so AdvancedPressureControl can access them */
float min_voltage = 0.5;      /* Minimum voltage (0.5V) */
float max_voltage = 5.24;     /* Maximum voltage (5.24V) */
float min_pressure = 0.0;     /* DÜZƏLİŞ: Minimum pressure (0.0 bar - sıfır təzyiq) */
float max_pressure = 300.0;   /* Maximum pressure (300.0 bar) */
uint16_t adc_min = 410;       /* ADC value at minimum pressure (0.5V) */
uint16_t adc_max = 4096;      /* ADC value at maximum pressure (5.0V) */
static uint8_t calibration_mode = 0; /* 0=normal, 1=calibrate min, 2=calibrate max */

/* Precise conversion constants */
// DÜZƏLİŞ: P_PER_COUNT və P_OFFSET silindi - artıq g_calibration strukturundan istifadə edilir
// Bütün təzyiq konversiyaları Advanced sistemin g_calibration strukturundan istifadə edir
static const float BAR_PER_VOLT = 66.3228f; /* bar per volt (display only) */
static const float V_PER_COUNT = 0.00158f;  /* volt per ADC count (display only) */

/* Motor/DRV/ZME conversion constants */
static const float MOTOR_BAR_PER_PERCENT = 3.144f;    /* bar per 1% motor */
static const float MOTOR_PERCENT_PER_BAR = 0.3181f;  /* % per 1 bar motor */
static const float DRV_BAR_PER_PERCENT = 10.48f;     /* bar per 1% DRV (10-40% range) */
static const float DRV_PERCENT_PER_BAR = 0.09545f;   /* % per 1 bar DRV */
static const float ZME_BAR_PER_PERCENT = 10.48f;     /* bar per 1% ZME (0-30% range) */
static const float ZME_PERCENT_PER_BAR = 0.09545f;   /* % per 1 bar ZME */

/* Pressure limits for ZME control (separate from sensor calibration) */
static float zme_min_pressure = 3.5f;   /* minimum system pressure for ZME (bar) */
static float zme_max_pressure = 200.0f;  /* maximum system pressure for ZME (bar) */
static float zme_frequency = 1000.0f;  /* ZME PWM frequency in Hz */

// Button states
static uint8_t avto_active = 0;  // REMOVED but kept for compatibility
// stop_active - REMOVED (Stop button deleted)

// Auto mode control variables - REMOVED (AutoMode deleted)

/* Helper function to draw a button with text - no borders, white background */
void ILI9341_DrawControlButton(uint16_t x, uint16_t y, uint16_t width, uint16_t height, 
                               uint16_t color, const char* text, uint8_t text_size)
{
    // Draw button background (white by default, or specified color for special states)
    ILI9341_DrawRectangle(x, y, width, height, color);
    
    // No borders - clean white buttons
    
    // Draw text centered - black text on white background
    uint16_t text_x = x + (width - strlen(text) * 8 * text_size) / 2;
    uint16_t text_y = y + (height - 10 * text_size) / 2;
    ILI9341_DrawString(text_x, text_y, text, ILI9341_COLOR_BLACK, color, text_size);
}

/* Enhanced circular pressure gauge with better visualization */
void ILI9341_DrawPressureGauge(uint16_t center_x, uint16_t center_y, uint16_t radius, float pressure, float max_pressure)
{
    // Draw outer circle with thicker border
    for (int angle = 0; angle < 360; angle++) {
        int x = center_x + radius * cos(angle * 3.14159 / 180);
        int y = center_y + radius * sin(angle * 3.14159 / 180);
        if (x >= 0 && x < 320 && y >= 0 && y < 240) {
            ILI9341_DrawPixel(x, y, ILI9341_COLOR_WHITE);
            // Draw thicker border
            if (x > 0 && x < 319 && y > 0 && y < 239) {
                ILI9341_DrawPixel(x-1, y, ILI9341_COLOR_WHITE);
                ILI9341_DrawPixel(x+1, y, ILI9341_COLOR_WHITE);
                ILI9341_DrawPixel(x, y-1, ILI9341_COLOR_WHITE);
                ILI9341_DrawPixel(x, y+1, ILI9341_COLOR_WHITE);
            }
        }
    }
    
    // Draw scale marks (every 30 degrees)
    for (int mark = 0; mark < 6; mark++) {
        int angle = mark * 30;
        int x1 = center_x + (radius - 2) * cos((angle + 180) * 3.14159 / 180);
        int y1 = center_y + (radius - 2) * sin((angle + 180) * 3.14159 / 180);
        int x2 = center_x + (radius - 8) * cos((angle + 180) * 3.14159 / 180);
        int y2 = center_y + (radius - 8) * sin((angle + 180) * 3.14159 / 180);
        ILI9341_DrawLine(x1, y1, x2, y2, ILI9341_COLOR_WHITE);
    }
    
    // Draw pressure arc with color coding (0-180 degrees)
    float pressure_angle = (pressure / max_pressure) * 180.0;
    for (int angle = 0; angle < pressure_angle; angle++) {
        int x = center_x + (radius - 5) * cos((angle + 180) * 3.14159 / 180);
        int y = center_y + (radius - 5) * sin((angle + 180) * 3.14159 / 180);
        if (x >= 0 && x < 320 && y >= 0 && y < 240) {
            // Enhanced color coding with more zones
            uint16_t color;
            float percentage = (float)angle / 180.0;
            if (percentage < 0.3) {
                color = ILI9341_COLOR_GREEN;      // 0-30%: Green (Safe)
            } else if (percentage < 0.6) {
                color = ILI9341_COLOR_YELLOW;     // 30-60%: Yellow (Caution)
            } else if (percentage < 0.8) {
                color = ILI9341_COLOR_ORANGE;    // 60-80%: Orange (Warning)
            } else {
                color = ILI9341_COLOR_RED;       // 80-100%: Red (Danger)
            }
            ILI9341_DrawPixel(x, y, color);
        }
    }
    
    // Draw needle (pointer) showing current pressure
    if (pressure > 0) {
        float needle_angle = (pressure / max_pressure) * 180.0 + 180.0;
        int needle_x = center_x + (radius - 3) * cos(needle_angle * 3.14159 / 180);
        int needle_y = center_y + (radius - 3) * sin(needle_angle * 3.14159 / 180);
        ILI9341_DrawLine(center_x, center_y, needle_x, needle_y, ILI9341_COLOR_RED);
    }
    
    // Draw center dot
    ILI9341_DrawRectangle(center_x - 2, center_y - 2, 5, 5, ILI9341_COLOR_RED);
}

/* Main pressure control screen */
void ILI9341_ShowPressureControlMain(void)
{
    pressure_control_page = 0;
    
    // Clear screen
    ILI9341_FillScreen(ILI9341_COLOR_BLACK);
    
    // Title at top - "HIGH PRESSURE CONTROL"
    ILI9341_DrawString(40, 15, "HIGH PRESSURE CONTROL", ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 2);
    
    // Pressure gauge in center - slightly below title
    ILI9341_DrawPressureGauge(160, 80, 35, current_pressure, 300.0);
    
    // Pressure value display below gauge with enhanced formatting
    char pressure_str[25];
    if (current_pressure < 10.0) {
        sprintf(pressure_str, "%.2f BAR", current_pressure);  // 2 decimal for low values
    } else if (current_pressure < 100.0) {
        sprintf(pressure_str, "%.1f BAR", current_pressure);  // 1 decimal for medium values
    } else {
        sprintf(pressure_str, "%.0f BAR", current_pressure);   // No decimal for high values
    }
    ILI9341_DrawString(130, 125, pressure_str, ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 2);
    
    // Show target pressure information (from Advanced system)
    SystemStatus_t* status = AdvancedPressureControl_GetStatus();
    char limit_str[30];
    sprintf(limit_str, "SP: %.0f bar", status->target_pressure);
    ILI9341_DrawString(10, 140, limit_str, ILI9341_COLOR_CYAN, ILI9341_COLOR_BLACK, 1);
    
    // Show safety status (based on target pressure)
    if (current_pressure > (status->target_pressure + 10.0f)) {
        ILI9341_DrawString(200, 140, "OVER LIMIT!", ILI9341_COLOR_RED, ILI9341_COLOR_BLACK, 1);
    } else if (current_pressure > status->target_pressure) {
        ILI9341_DrawString(200, 140, "NEAR LIMIT", ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 1);
    } else {
        ILI9341_DrawString(200, 140, "SAFE", ILI9341_COLOR_GREEN, ILI9341_COLOR_BLACK, 1);
    }
    
    // Control buttons (Menu) - below pressure display
    ILI9341_DrawControlButton(15, 150, 70, 35, ILI9341_COLOR_WHITE, "Menu", 1);
    // Auto button - REMOVED (AutoMode deleted)
    // Manual button - REMOVED (Manual mode deleted)
    // Stop button - REMOVED (Stop button deleted)
    
    /* Auto mode status display - REMOVED (AutoMode deleted) */
    
    /* Show target pressure (from Advanced system) - eyni status dəyişənini istifadə edirik */
    char target_info[30];
    sprintf(target_info, "SP: %.1f bar", status->target_pressure);
    ILI9341_DrawString(20, 210, target_info, ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 1);
    
    /* Show valve status (for NO valves: 0% = open, 100% = closed) */
    char valve_status[50];
    sprintf(valve_status, "ZME:%s DRV:%s", 
            (zme_duty_cycle == 0.0) ? "OPEN" : "CLOSED",
            (drv_duty_cycle == 0.0) ? "OPEN" : "CLOSED");
    ILI9341_DrawString(20, 230, valve_status, ILI9341_COLOR_CYAN, ILI9341_COLOR_BLACK, 1);
}

/* Menu page */
void ILI9341_ShowMenuPage(void)
{
    pressure_control_page = 1;
    
    // Clear screen
    ILI9341_FillScreen(ILI9341_COLOR_BLACK);
    
    // Title - moved down to avoid Back button
    ILI9341_DrawString(120, 60, "MENU", ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 3);
    
    // REMOVED: PWM button - PWM bölməsi silindi
    // REMOVED: Pressure Limit button - PRES LIM bölməsi silindi
    
    // Pressure Sensor Calibration button
    ILI9341_DrawControlButton(80, 90, 160, 35, ILI9341_COLOR_WHITE, "PRESS CAL", 2);
    
    // PID Tuning button
    ILI9341_DrawControlButton(80, 135, 160, 35, ILI9341_COLOR_WHITE, "PID TUNE", 2);
    
    // Back button - moved to upper left
    ILI9341_DrawControlButton(20, 20, 60, 30, ILI9341_COLOR_WHITE, "Back", 1);
}

/* REMOVED: PWM control page - PWM bölməsi silindi
void ILI9341_ShowPWMPage(void)
{
    pressure_control_page = 2;
    
    // Clear screen
    ILI9341_FillScreen(ILI9341_COLOR_BLACK);
    
    // Title
    ILI9341_DrawString(120, 20, "PWM CONTROL", ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 2);
    
    // DRV button
    ILI9341_DrawControlButton(80, 70, 160, 35, ILI9341_COLOR_WHITE, "DRV", 2);
    
    // ZME button
    ILI9341_DrawControlButton(80, 115, 160, 35, ILI9341_COLOR_WHITE, "ZME", 2);
    
    // Motor button
    ILI9341_DrawControlButton(80, 160, 160, 35, ILI9341_COLOR_WHITE, "MOTOR", 2);
    
    // Back button - moved to upper left
    ILI9341_DrawControlButton(20, 20, 80, 25, ILI9341_COLOR_WHITE, "Back", 1);
}
*/

/* REMOVED: Pressure limit page - PRES LIM bölməsi silindi
void ILI9341_ShowPressureLimitPage(void)
{
    pressure_control_page = 3;
    
    // Clear screen
    ILI9341_FillScreen(ILI9341_COLOR_BLACK);
    
    // Title
    ILI9341_DrawString(100, 20, "PRES LIM", ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 2);
    
    // Current target pressure display (from Advanced system)
    SystemStatus_t* status = AdvancedPressureControl_GetStatus();
    char limit_str[30];
    sprintf(limit_str, "SP: %.1f BAR", status->target_pressure);
    ILI9341_DrawString(80, 80, limit_str, ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 3);
    
    // Range display
    ILI9341_DrawString(100, 120, "0 - 300 BAR", ILI9341_COLOR_CYAN, ILI9341_COLOR_BLACK, 2);
    
    // Auto mode status on pressure limit page - REMOVED (AutoMode deleted)
    
    // Control buttons
    ILI9341_DrawControlButton(50, 160, 50, 40, ILI9341_COLOR_WHITE, "-10", 2);
    ILI9341_DrawControlButton(110, 160, 50, 40, ILI9341_COLOR_WHITE, "-1", 2);
    ILI9341_DrawControlButton(170, 160, 50, 40, ILI9341_COLOR_WHITE, "+1", 2);
    ILI9341_DrawControlButton(230, 160, 50, 40, ILI9341_COLOR_WHITE, "+10", 2);
    
    // Back button - moved down a bit from upper left
    ILI9341_DrawControlButton(20, 50, 80, 30, ILI9341_COLOR_WHITE, "Back", 1);
}
*/

/* REMOVED: DRV PWM control page - PWM bölməsi silindi
void ILI9341_ShowDRVPWMPage(void)
{
    pressure_control_page = 4;
    
    // Clear screen
    ILI9341_FillScreen(ILI9341_COLOR_BLACK);
    
    // Title
    ILI9341_DrawString(120, 20, "DRV PWM", ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 2);
    ILI9341_DrawString(20, 45, "NO Valve: 0%=Open, 100%=Closed", ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 1);
    
    // Duty Cycle section
    ILI9341_DrawString(50, 60, "Duty Cycle", ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 2);
    char duty_str[20];
    sprintf(duty_str, "%.1f%%", drv_duty_cycle);
    ILI9341_DrawString(100, 85, duty_str, ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 2);
    ILI9341_DrawControlButton(50, 110, 40, 30, ILI9341_COLOR_WHITE, "-", 2);
    ILI9341_DrawControlButton(200, 110, 40, 30, ILI9341_COLOR_WHITE, "+", 2);
    
    // Frequency section
    ILI9341_DrawString(50, 150, "Frequency", ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 2);
    char freq_str[20];
    sprintf(freq_str, "%.0f Hz", drv_frequency);
    ILI9341_DrawString(100, 175, freq_str, ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 2);
    ILI9341_DrawControlButton(50, 190, 40, 30, ILI9341_COLOR_WHITE, "-", 2);
    ILI9341_DrawControlButton(200, 190, 40, 30, ILI9341_COLOR_WHITE, "+", 2);
    
    // Back button - moved to upper left
    ILI9341_DrawControlButton(20, 20, 80, 25, ILI9341_COLOR_WHITE, "Back", 1);
}
*/

/* REMOVED: ZME PWM control page - PWM bölməsi silindi
void ILI9341_ShowZMEPWMPage(void)
{
    pressure_control_page = 5;
    
    // Clear screen
    ILI9341_FillScreen(ILI9341_COLOR_BLACK);
    
    // Title
    ILI9341_DrawString(120, 20, "ZME PWM", ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 2);
    ILI9341_DrawString(20, 45, "NO Valve: 0%=Open, 100%=Closed", ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 1);
    
    // Duty Cycle section
    ILI9341_DrawString(50, 60, "Duty Cycle", ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 2);
    char duty_str[20];
    sprintf(duty_str, "%.1f%%", zme_duty_cycle);
    ILI9341_DrawString(100, 85, duty_str, ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 2);
    ILI9341_DrawControlButton(50, 110, 40, 30, ILI9341_COLOR_WHITE, "-", 2);
    ILI9341_DrawControlButton(200, 110, 40, 30, ILI9341_COLOR_WHITE, "+", 2);
    
    // Frequency section
    ILI9341_DrawString(50, 150, "Frequency", ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 2);
    char freq_str[20];
    sprintf(freq_str, "%.0f Hz", zme_frequency);
    ILI9341_DrawString(100, 175, freq_str, ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 2);
    ILI9341_DrawControlButton(50, 190, 40, 30, ILI9341_COLOR_WHITE, "-", 2);
    ILI9341_DrawControlButton(200, 190, 40, 30, ILI9341_COLOR_WHITE, "+", 2);
    
    // Back button - moved to upper left
    ILI9341_DrawControlButton(20, 20, 80, 25, ILI9341_COLOR_WHITE, "Back", 1);
}
*/

/* REMOVED: Motor PWM control page - PWM bölməsi silindi
void ILI9341_ShowMotorPWMPage(void)
{
    pressure_control_page = 6;
    
    // Clear screen
    ILI9341_FillScreen(ILI9341_COLOR_BLACK);
    
    // Title
    ILI9341_DrawString(120, 20, "MOTOR PWM", ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 2);
    
    // Duty Cycle section
    ILI9341_DrawString(50, 60, "Duty Cycle", ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 2);
    char duty_str[20];
    sprintf(duty_str, "%.1f%%", motor_duty_cycle);
    ILI9341_DrawString(100, 85, duty_str, ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 2);
    ILI9341_DrawControlButton(50, 110, 40, 30, ILI9341_COLOR_WHITE, "-", 2);
    ILI9341_DrawControlButton(200, 110, 40, 30, ILI9341_COLOR_WHITE, "+", 2);
    
    // Frequency section
    ILI9341_DrawString(50, 150, "Frequency", ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 2);
    char freq_str[20];
    sprintf(freq_str, "%.0f Hz", motor_frequency);
    ILI9341_DrawString(100, 175, freq_str, ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 2);
    ILI9341_DrawControlButton(50, 190, 40, 30, ILI9341_COLOR_WHITE, "-", 2);
    ILI9341_DrawControlButton(200, 190, 40, 30, ILI9341_COLOR_WHITE, "+", 2);
    
    // Back button - moved to upper left
    ILI9341_DrawControlButton(20, 20, 80, 25, ILI9341_COLOR_WHITE, "Back", 1);
}
*/

/* Touch Calibration page */
void ILI9341_ShowCalibrationPage(void)
{
    pressure_control_page = 7;
    
    // Clear screen
    ILI9341_FillScreen(ILI9341_COLOR_BLACK);
    
    // Title
    ILI9341_DrawString(80, 20, "TOUCH CALIBRATION", ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 2);
    
    // Instructions
    ILI9341_DrawString(20, 50, "Touch the screen to see coordinates", ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 1);
    ILI9341_DrawString(20, 70, "Raw X/Y values will be shown below", ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 1);
    
    // Current calibration values
    uint16_t x_min, x_max, y_min, y_max;
    XPT2046_GetCalibration(&x_min, &x_max, &y_min, &y_max);
    
    char calib_info[60];
    sprintf(calib_info, "X: %d-%d, Y: %d-%d", x_min, x_max, y_min, y_max);
    ILI9341_DrawString(20, 100, calib_info, ILI9341_COLOR_CYAN, ILI9341_COLOR_BLACK, 1);
    
    // Coordinate mode buttons
    ILI9341_DrawControlButton(20, 130, 60, 30, ILI9341_COLOR_WHITE, "Mode 0", 1);
    ILI9341_DrawControlButton(90, 130, 60, 30, ILI9341_COLOR_WHITE, "Mode 1", 1);
    ILI9341_DrawControlButton(160, 130, 60, 30, ILI9341_COLOR_WHITE, "Mode 2", 1);
    ILI9341_DrawControlButton(230, 130, 60, 30, ILI9341_COLOR_WHITE, "Mode 3", 1);
    
    // Back button
    ILI9341_DrawControlButton(20, 200, 80, 30, ILI9341_COLOR_WHITE, "Back", 1);
}

/* Pressure Sensor Calibration page */
void ILI9341_ShowPressureCalibrationPage(void)
{
    pressure_control_page = 8; // New page for pressure sensor calibration
    
    // Clear screen
    ILI9341_FillScreen(ILI9341_COLOR_BLACK);
    
    // Title
    ILI9341_DrawString(60, 20, "PRESSURE SENSOR", ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 2);
    ILI9341_DrawString(80, 45, "CALIBRATION", ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 2);
    
    // DÜZƏLİŞ: Kalibrləmə dəyərlərini Advanced sistemdən götür
    extern CalibrationData_t g_calibration;
    
    // Update UI variables from Advanced system calibration (for consistency)
    // DÜZƏLİŞ: Əgər kalibrləmə dəyərləri default-dan fərqlidirsə, default dəyərləri istifadə et
    if (g_calibration.adc_min < 400 || g_calibration.adc_min > 450 || 
        g_calibration.adc_max < 4000 || g_calibration.adc_max > 4096) {
        // Invalid calibration values - use defaults
        adc_min = 410;
        adc_max = 4096;
        min_pressure = 0.0f;  // DÜZƏLİŞ: 0.0 bar (sıfır təzyiq)
        max_pressure = 300.0f;
        
        // Update g_calibration with default values
        g_calibration.adc_min = 410.0f;
        g_calibration.adc_max = 4096.0f;
        g_calibration.pressure_min = 0.0f;  // DÜZƏLİŞ: 0.0 bar (sıfır təzyiq)
        g_calibration.pressure_max = 300.0f;
        g_calibration.slope = (300.0f - 0.0f) / (4096.0f - 410.0f);  // DÜZƏLİŞ: 0.0f istifadə et
        g_calibration.offset = 0.0f - (g_calibration.slope * 410.0f);  // DÜZƏLİŞ: 0.0f istifadə et
        
        printf("DÜZƏLİŞ: Yanlış kalibrləmə dəyərləri aşkar edildi, default dəyərlər tətbiq olundu - ADC: %d-%d\r\n", adc_min, adc_max);
    } else {
        adc_min = (uint16_t)g_calibration.adc_min;
        adc_max = (uint16_t)g_calibration.adc_max;
        min_pressure = g_calibration.pressure_min;
        max_pressure = g_calibration.pressure_max;
    }
    
    // Current calibration values display
    char min_volt_str[30], max_volt_str[30];
    char min_press_str[30], max_press_str[30];
    char adc_min_str[30], adc_max_str[30];
    sprintf(min_volt_str, "Min Volt: %.1fV", min_voltage);
    sprintf(max_volt_str, "Max Volt: %.1fV", max_voltage);
    sprintf(min_press_str, "Min Press: %.1f bar", min_pressure);
    sprintf(max_press_str, "Max Press: %.1f bar", max_pressure);
    sprintf(adc_min_str, "ADC Min: %d", adc_min);
    sprintf(adc_max_str, "ADC Max: %d", adc_max);
    
    ILI9341_DrawString(20, 80, min_volt_str, ILI9341_COLOR_CYAN, ILI9341_COLOR_BLACK, 1);
    ILI9341_DrawString(20, 100, max_volt_str, ILI9341_COLOR_CYAN, ILI9341_COLOR_BLACK, 1);
    ILI9341_DrawString(20, 120, min_press_str, ILI9341_COLOR_CYAN, ILI9341_COLOR_BLACK, 1);
    ILI9341_DrawString(20, 140, max_press_str, ILI9341_COLOR_CYAN, ILI9341_COLOR_BLACK, 1);
    ILI9341_DrawString(20, 150, adc_min_str, ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 1);
    ILI9341_DrawString(20, 160, adc_max_str, ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 1);
    
    /* Current ADC reading - will be updated in real-time */
    char adc_str[30];
    sprintf(adc_str, "ADC: %d", adc_min); /* Initial value */
    ILI9341_DrawString(20, 170, adc_str, ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 1);
    
    /* Instructions */
    ILI9341_DrawString(20, 180, "Touch CAL MIN/MAX to calibrate", ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 1);
    
    /* Calibration buttons - larger for easier touch */
    ILI9341_DrawControlButton(20, 200, 90, 40, ILI9341_COLOR_WHITE, "CAL MIN", 1);
    ILI9341_DrawControlButton(120, 200, 90, 40, ILI9341_COLOR_WHITE, "CAL MAX", 1);
    ILI9341_DrawControlButton(220, 200, 90, 40, ILI9341_COLOR_WHITE, "SAVE", 1);
    
    /* Back button */
    ILI9341_DrawControlButton(20, 250, 90, 40, ILI9341_COLOR_WHITE, "BACK", 1);
}

/* PID Tuning page */
void ILI9341_ShowPIDTuningPage(void)
{
    pressure_control_page = 9;
    
    ILI9341_FillScreen(ILI9341_COLOR_BLACK);
    
    ILI9341_DrawString(85, 15, "PID TUNING", ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 2);
    
    // KRİTİK DÜZƏLİŞ: Manual ADC oxunması silindi - yalnız PID sistemindən təzyiq götürülür
    // PID sistemi hər 10ms-də bir ADC oxuyur və g_system_status.current_pressure-ı yeniləyir
    // Bu, ADC kanallarının ziddiyyətini və loop qeyri-dəqiqliyini aradan qaldırır
    SystemStatus_t* status = AdvancedPressureControl_GetStatus();
    float current_pressure_val = status->current_pressure;
    
    // Update status with latest pressure reading
    status->current_pressure = current_pressure_val;
    
    char info[40];
    sprintf(info, "P: %.1f BAR", current_pressure_val);
    ILI9341_DrawString(20, 45, info, ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 1);
    sprintf(info, "SP: %.1f BAR", status->target_pressure);
    ILI9341_DrawString(150, 45, info, ILI9341_COLOR_CYAN, ILI9341_COLOR_BLACK, 1);
    // SP +/- controls - DÜZƏLİŞ: 10-10 dəyişir, düymələr daha böyük və "-10"/"+10" göstərir
    ILI9341_DrawControlButton(260, 40, 30, 30, ILI9341_COLOR_WHITE, "-10", 1);  // DÜZƏLİŞ: "-10" göstərir və daha böyük
    ILI9341_DrawControlButton(295, 40, 30, 30, ILI9341_COLOR_WHITE, "+10", 1);  // DÜZƏLİŞ: "+10" göstərir və daha böyük
    
    // Read current PID from AdvancedPressureControl system
    float kp = g_pid_zme.Kp;
    float ki = g_pid_zme.Ki;
    float kd = g_pid_zme.Kd;
    
    // Kp row
    ILI9341_DrawString(20, 80, "Kp:", ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 2);
    char val[24];
    // DÜZƏLİŞ: Standart format - Kp üçün 2 onluq yer (0.00 formatı)
    sprintf(val, "%.2f", kp);
    ILI9341_DrawString(80, 80, val, ILI9341_COLOR_GREEN, ILI9341_COLOR_BLACK, 2);
    ILI9341_DrawControlButton(190, 75, 45, 30, ILI9341_COLOR_WHITE, "-", 2);
    ILI9341_DrawControlButton(245, 75, 45, 30, ILI9341_COLOR_WHITE, "+", 2);
    
    // Ki row
    ILI9341_DrawString(20, 120, "Ki:", ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 2);
    // DÜZƏLİŞ: Standart format - Ki üçün 4 onluq yer (0.0000 formatı)
    sprintf(val, "%.4f", ki);
    ILI9341_DrawString(80, 120, val, ILI9341_COLOR_GREEN, ILI9341_COLOR_BLACK, 2);
    ILI9341_DrawControlButton(190, 115, 45, 30, ILI9341_COLOR_WHITE, "-", 2);
    ILI9341_DrawControlButton(245, 115, 45, 30, ILI9341_COLOR_WHITE, "+", 2);
    
    // Kd row
    ILI9341_DrawString(20, 160, "Kd:", ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 2);
    // DÜZƏLİŞ: Standart format - Kd üçün 3 onluq yer (0.000 formatı)
    sprintf(val, "%.3f", kd);
    ILI9341_DrawString(80, 160, val, ILI9341_COLOR_GREEN, ILI9341_COLOR_BLACK, 2);
    ILI9341_DrawControlButton(190, 155, 45, 30, ILI9341_COLOR_WHITE, "-", 2);
    ILI9341_DrawControlButton(245, 155, 45, 30, ILI9341_COLOR_WHITE, "+", 2);
    
    // Info: ZME and DRV use same PID parameters
    ILI9341_DrawString(20, 185, "ZME & DRV: Same PID", ILI9341_COLOR_CYAN, ILI9341_COLOR_BLACK, 1);
    
    // Back and Save
    ILI9341_DrawControlButton(20, 200, 80, 30, ILI9341_COLOR_WHITE, "Back", 1);
    ILI9341_DrawControlButton(220, 200, 80, 30, ILI9341_COLOR_WHITE, "Save", 1);
}

/* Update pressure display */
void ILI9341_UpdatePressureDisplay(float pressure)
{
    current_pressure = pressure;
    
    if (pressure_control_page == 0) {
        /* Clean pressure display - no debug clutter */
        
        /* Update pressure value with clean formatting */
        char pressure_str[25];
        if (current_pressure < 10.0) {
            sprintf(pressure_str, "%.2f BAR", current_pressure);
        } else if (current_pressure < 100.0) {
            sprintf(pressure_str, "%.1f BAR", current_pressure);
        } else {
            sprintf(pressure_str, "%.0f BAR", current_pressure);
        }
        
        /* Clear the old text area and draw new text */
        ILI9341_DrawRectangle(130, 125, 100, 20, ILI9341_COLOR_BLACK);
        ILI9341_DrawString(130, 125, pressure_str, ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 2);
        
        /* Update safety status (based on target pressure) */
        SystemStatus_t* status = AdvancedPressureControl_GetStatus();
        ILI9341_DrawRectangle(200, 140, 100, 15, ILI9341_COLOR_BLACK);
        if (current_pressure > (status->target_pressure + 10.0f)) {
            ILI9341_DrawString(200, 140, "OVER LIMIT!", ILI9341_COLOR_RED, ILI9341_COLOR_BLACK, 1);
        } else if (current_pressure > status->target_pressure) {
            ILI9341_DrawString(200, 140, "NEAR LIMIT", ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 1);
        } else {
            ILI9341_DrawString(200, 140, "SAFE", ILI9341_COLOR_GREEN, ILI9341_COLOR_BLACK, 1);
        }
        
        /* Update the pressure gauge arc only (not the entire circle) */
        ILI9341_DrawPressureGauge(160, 80, 35, current_pressure, 300.0);
    }
}

/* Update percentage displays */
void ILI9341_UpdatePercentageDisplays(float drv_percent_val, float zme_percent_val, float motor_percent_val)
{
    drv_percent = drv_percent_val;
    zme_percent = zme_percent_val;
    motor_percent = motor_percent_val;
    
    if (pressure_control_page == 0) {
        char drv_str[20], zme_str[20], motor_str[20];
        
        // Clear old text area first
        ILI9341_DrawRectangle(10, 210, 80, 15, ILI9341_COLOR_BLACK);
        ILI9341_DrawRectangle(110, 210, 80, 15, ILI9341_COLOR_BLACK);
        ILI9341_DrawRectangle(210, 210, 80, 15, ILI9341_COLOR_BLACK);
        
        sprintf(drv_str, "DRV: %.2f%%", drv_percent);
        sprintf(zme_str, "ZME: %.2f%%", zme_percent);
        sprintf(motor_str, "M: %.1f%%", motor_percent);
        
        ILI9341_DrawString(10, 210, drv_str, ILI9341_COLOR_CYAN, ILI9341_COLOR_BLACK, 1);
        ILI9341_DrawString(110, 210, zme_str, ILI9341_COLOR_CYAN, ILI9341_COLOR_BLACK, 1);
        ILI9341_DrawString(210, 210, motor_str, ILI9341_COLOR_CYAN, ILI9341_COLOR_BLACK, 1);
    }
}

/* Handle touch for pressure control system */
void ILI9341_HandlePressureControlTouch(void)
{
    uint16_t raw_x, raw_y, screen_x, screen_y;
    
    if (XPT2046_IsTouched()) {
        if (XPT2046_GetCoordinates(&raw_x, &raw_y)) {
            XPT2046_ConvertToScreen(raw_x, raw_y, &screen_x, &screen_y);
            
            /* Clean touch handling - no debug clutter */
            
            /* Clean interface - no debug clutter */
            
            switch (pressure_control_page) {
                case 0: // Main page
                    /* Menu button */
                    if (screen_x >= 15 && screen_x <= 85 && screen_y >= 150 && screen_y <= 185) {
                        ILI9341_ShowMenuPage();
                    }
                    /* Auto button - REMOVED (AutoMode deleted) */
                    /* Manual button - REMOVED (Manual mode deleted) */
                    /* Stop button - REMOVED (Stop button deleted) */
                    break;
                    
                case 1: // Menu page
                    // REMOVED: PWM button - PWM bölməsi silindi
                    // REMOVED: Pressure Limit button - PRES LIM bölməsi silindi
                    
                    // Pressure Sensor Calibration button (80-240 x, 90-125 y)
                    if (screen_x >= 80 && screen_x <= 240 && screen_y >= 90 && screen_y <= 125) {
                        ILI9341_ShowPressureCalibrationPage();
                    }
                    // PID Tuning button (80-240 x, 135-170 y)
                    else if (screen_x >= 80 && screen_x <= 240 && screen_y >= 135 && screen_y <= 170) {
                        ILI9341_ShowPIDTuningPage();
                    }
                    /* Back button - moved to upper left */
                    else if (screen_x >= 20 && screen_x <= 80 && screen_y >= 20 && screen_y <= 50) {
                        ILI9341_ShowPressureControlMain();
                    }
                    break;
                    
                // REMOVED: case 2 (PWM page), case 4 (DRV PWM page), case 5 (ZME PWM page), case 6 (Motor PWM page)
                // PWM bölməsi tamamilə silindi
                // REMOVED: case 3 (Pressure limit page) - PRES LIM bölməsi silindi
                    
                case 7: // Touch Calibration page
                    // Mode buttons
                    if (screen_x >= 20 && screen_x <= 80 && screen_y >= 130 && screen_y <= 160) {
                        XPT2046_SetCoordMode(0);
                    }
                    else if (screen_x >= 90 && screen_x <= 150 && screen_y >= 130 && screen_y <= 160) {
                        XPT2046_SetCoordMode(1);
                    }
                    else if (screen_x >= 160 && screen_x <= 220 && screen_y >= 130 && screen_y <= 160) {
                        XPT2046_SetCoordMode(2);
                    }
                    else if (screen_x >= 230 && screen_x <= 290 && screen_y >= 130 && screen_y <= 160) {
                        XPT2046_SetCoordMode(3);
                    }
                    // Back button
                    else if (screen_x >= 20 && screen_x <= 100 && screen_y >= 200 && screen_y <= 230) {
                        ILI9341_ShowMenuPage();
                    }
                    break;
                    
                case 8: // Pressure Sensor Calibration page
                    /* Clean touch handling - no debug clutter */
                    
                    /* CAL MIN button - larger button */
                    if (screen_x >= 20 && screen_x <= 110 && screen_y >= 190 && screen_y <= 230) {
                        /* KRİTİK DÜZƏLİŞ: ADC oxunması PID sistemindən götürülür - ADC bloklanması yoxdur */
                        /* PID sistemi hər 10ms-də bir ADC oxuyur və Status strukturuna yazır */
                        SystemStatus_t* cal_status = AdvancedPressureControl_GetStatus();
                        uint16_t adc_value = cal_status->raw_adc_value;  // Xam ADC dəyəri Status-dan
                        adc_min = adc_value;  /* Set as minimum ADC value */
                        min_pressure = 0.0f; /* DÜZƏLİŞ: Set pressure to 0.0 bar (sıfır təzyiq) */
                        calibration_mode = 0; /* Reset calibration mode */
                        
                        /* Show confirmation */
                        ILI9341_DrawString(20, 200, "MIN CALIBRATED!", ILI9341_COLOR_GREEN, ILI9341_COLOR_BLACK, 1);
                        
                        /* DÜZƏLİŞ: Kalibrləmə dəyərlərini dərhal Advanced sistemə ötür */
                        extern CalibrationData_t g_calibration;
                        g_calibration.adc_min = (float)adc_min;
                        g_calibration.pressure_min = min_pressure;  // 0.0 bar
                            
                            // Recalculate slope and offset immediately if both min and max are set
                            if (adc_max > adc_min) {
                                g_calibration.slope = (max_pressure - min_pressure) / (float)(adc_max - adc_min);
                                g_calibration.offset = min_pressure - (g_calibration.slope * (float)adc_min);
                                printf("MIN Calibration updated - ADC: %d, Pressure: %.2f bar, Slope: %.6f, Offset: %.2f\r\n",
                                       adc_min, min_pressure, g_calibration.slope, g_calibration.offset);
                            }
                        /* KRİTİK DÜZƏLİŞ: HAL_ADC_Stop() silindi - artıq manual ADC oxunması yoxdur */
                        ILI9341_ShowPressureCalibrationPage(); /* Refresh page */
                    }
                    /* CAL MAX button - larger button */
                    else if (screen_x >= 120 && screen_x <= 210 && screen_y >= 190 && screen_y <= 230) {
                        /* KRİTİK DÜZƏLİŞ: ADC oxunması PID sistemindən götürülür - ADC bloklanması yoxdur */
                        /* PID sistemi hər 10ms-də bir ADC oxuyur və Status strukturuna yazır */
                        SystemStatus_t* cal_status = AdvancedPressureControl_GetStatus();
                        uint16_t adc_value = cal_status->raw_adc_value;  // Xam ADC dəyəri Status-dan
                        adc_max = adc_value;  /* Set as maximum ADC value */
                        max_pressure = 300.0f; /* Set pressure to 300.0 bar (full range) - DÜZƏLİŞ: float literal */
                        calibration_mode = 0; /* Reset calibration mode */
                        
                        /* Show confirmation */
                        ILI9341_DrawString(110, 200, "MAX CALIBRATED!", ILI9341_COLOR_GREEN, ILI9341_COLOR_BLACK, 1);
                        
                        /* DÜZƏLİŞ: Kalibrləmə dəyərlərini dərhal Advanced sistemə ötür */
                        extern CalibrationData_t g_calibration;
                            g_calibration.adc_max = (float)adc_max;
                            g_calibration.pressure_max = max_pressure;
                            
                            // Recalculate slope and offset immediately
                            if (adc_max > adc_min) {
                                g_calibration.slope = (max_pressure - min_pressure) / (float)(adc_max - adc_min);
                                g_calibration.offset = min_pressure - (g_calibration.slope * (float)adc_min);
                                printf("MAX Calibration updated - ADC: %d, Pressure: %.2f bar, Slope: %.6f, Offset: %.2f\r\n",
                                       adc_max, max_pressure, g_calibration.slope, g_calibration.offset);
                            }
                        /* KRİTİK DÜZƏLİŞ: HAL_ADC_Stop() silindi - artıq manual ADC oxunması yoxdur */
                        /* ADC dəyəri Status-dan götürülür, ona görə də Stop() lazım deyil */
                        ILI9341_ShowPressureCalibrationPage(); /* Refresh page */
                    }
                    /* SAVE button - larger button */
                    else if (screen_x >= 220 && screen_x <= 310 && screen_y >= 190 && screen_y <= 230) {
                        /* DÜZƏLİŞ: Advanced sistem kalibrləmə funksiyasını istifadə et */
                        // UI-dan kalibrləmə dəyərlərini Advanced sistemə ötür
                        extern uint16_t adc_min, adc_max;
                        extern float min_pressure, max_pressure;
                        
                        // Validate calibration values
                        if (adc_max <= adc_min) {
                            ILI9341_DrawString(200, 200, "ERROR: Invalid ADC range!", ILI9341_COLOR_RED, ILI9341_COLOR_BLACK, 1);
                            HAL_Delay(2000);
                            ILI9341_ShowPressureCalibrationPage();
                            // DÜZƏLİŞ: break yerinə return - if-else blokunun içində break istifadə oluna bilməz
                            return;
                        }
                        
                        // Update Advanced system calibration structure from UI values
                        extern CalibrationData_t g_calibration;
                        g_calibration.adc_min = (float)adc_min;
                        g_calibration.adc_max = (float)adc_max;
                        g_calibration.pressure_min = min_pressure;  // DÜZƏLİŞ: 0.0 bar olmalıdır
                        g_calibration.pressure_max = max_pressure;
                        
                        // Calculate slope and offset
                        g_calibration.slope = (max_pressure - min_pressure) / (float)(adc_max - adc_min);
                        g_calibration.offset = min_pressure - (g_calibration.slope * (float)adc_min);  // DÜZƏLİŞ: min_pressure = 0.0f
                        g_calibration.calibrated = true;
                        
                        printf("Calibration updated from UI - ADC: %.0f-%.0f, Pressure: %.2f-%.2f bar, Slope: %.6f, Offset: %.2f\r\n",
                               g_calibration.adc_min, g_calibration.adc_max, 
                               g_calibration.pressure_min, g_calibration.pressure_max,
                               g_calibration.slope, g_calibration.offset);
                        
                        // Save to flash using Advanced system
                        AdvancedPressureControl_SaveCalibration();
                        ILI9341_DrawString(200, 200, "SAVED!", ILI9341_COLOR_GREEN, ILI9341_COLOR_BLACK, 1);
                        HAL_Delay(1000); /* Show message for 1 second */
                        ILI9341_ShowMenuPage();
                    }
                    /* Back button - larger button */
                    else if (screen_x >= 20 && screen_x <= 110 && screen_y >= 240 && screen_y <= 280) {
                        ILI9341_ShowMenuPage();
                    }
                    break;

                case 9: // PID Tuning page
                    // SP - button (260-290, 40-70) - DÜZƏLİŞ: 10-10 dəyişir, düymə daha böyük
                    if (screen_x >= 260 && screen_x <= 290 && screen_y >= 40 && screen_y <= 70) {
                        SystemStatus_t* status = AdvancedPressureControl_GetStatus();
                        float new_sp = status->target_pressure - 10.0f;  // DÜZƏLİŞ: 1.0f əvəzinə 10.0f
                        if (new_sp < 0.0f) new_sp = 0.0f;
                        AdvancedPressureControl_SetTargetPressure(new_sp);
                        // REMOVED: pressure_limit = new_sp; // Artıq pressure_limit yoxdur
                        ILI9341_ShowPIDTuningPage();  // Sürətli yeniləmə - delay yoxdur
                    }
                    // SP + button (295-325, 40-70) - DÜZƏLİŞ: 10-10 dəyişir, düymə daha böyük
                    else if (screen_x >= 295 && screen_x <= 325 && screen_y >= 40 && screen_y <= 70) {
                        SystemStatus_t* status = AdvancedPressureControl_GetStatus();
                        float new_sp = status->target_pressure + 10.0f;  // DÜZƏLİŞ: 1.0f əvəzinə 10.0f
                        if (new_sp > 300.0f) new_sp = 300.0f;
                        AdvancedPressureControl_SetTargetPressure(new_sp);
                        // REMOVED: pressure_limit = new_sp; // Artıq pressure_limit yoxdur
                        ILI9341_ShowPIDTuningPage();  // Sürətli yeniləmə - delay yoxdur
                    }
                    // Layout coordinates must match ShowPIDTuningPage
                    // Kp - button (190-235, 75-105)
                    else if (screen_x >= 190 && screen_x <= 235 && screen_y >= 75 && screen_y <= 105) {
                        // DÜZƏLİŞ: Kp addımı 0.1f-dən 0.05f-ə dəyişdirildi (daha incə tənzimləmə)
                        // 0.1f addımı kiçik dəyərlər üçün çox qaba idi (məsələn, 0.1 → 0.2 ikiqat artım)
                        float new_kp = g_pid_zme.Kp - 0.05f;
                        if (new_kp < 0.0f) new_kp = 0.0f;
                        // Update both ZME and DRV PID (they use same parameters)
                        AdvancedPressureControl_SetPIDParams(&g_pid_zme, new_kp, g_pid_zme.Ki, g_pid_zme.Kd);
                        AdvancedPressureControl_SetPIDParams(&g_pid_drv, new_kp, g_pid_drv.Ki, g_pid_drv.Kd);
                        printf("Kp dəyişdirildi: %.2f\r\n", new_kp);  // Debug mesajı (format 0.2f-ə dəyişdirildi)
                        ILI9341_ShowPIDTuningPage();
                    }
                    // Kp + button (245-290, 75-105)
                    else if (screen_x >= 245 && screen_x <= 290 && screen_y >= 75 && screen_y <= 105) {
                        // DÜZƏLİŞ: Kp addımı 0.1f-dən 0.05f-ə dəyişdirildi (daha incə tənzimləmə)
                        float new_kp = g_pid_zme.Kp + 0.05f;
                        if (new_kp > 10.0f) new_kp = 10.0f;
                        // Update both ZME and DRV PID (they use same parameters)
                        AdvancedPressureControl_SetPIDParams(&g_pid_zme, new_kp, g_pid_zme.Ki, g_pid_zme.Kd);
                        AdvancedPressureControl_SetPIDParams(&g_pid_drv, new_kp, g_pid_drv.Ki, g_pid_drv.Kd);
                        printf("Kp dəyişdirildi: %.2f\r\n", new_kp);  // Debug mesajı (format 0.2f-ə dəyişdirildi)
                        ILI9341_ShowPIDTuningPage();
                    }
                    // Ki - button (190-235, 115-145)
                    else if (screen_x >= 190 && screen_x <= 235 && screen_y >= 115 && screen_y <= 145) {
                        // DÜZƏLİŞ: Ki addımı 0.001f-dən 0.005f-ə dəyişdirildi (daha sürətli tənzimləmə)
                        // 0.001f addımı çox kiçikdir və tənzimləməni çox yavaş edir
                        // 0.005f addımı Ki dəyərləri 0.05 və ya daha böyük olduqda daha praktikdir
                        float new_ki = g_pid_zme.Ki - 0.005f;
                        if (new_ki < 0.0f) new_ki = 0.0f;
                        // Update both ZME and DRV PID (they use same parameters)
                        AdvancedPressureControl_SetPIDParams(&g_pid_zme, g_pid_zme.Kp, new_ki, g_pid_zme.Kd);
                        AdvancedPressureControl_SetPIDParams(&g_pid_drv, g_pid_drv.Kp, new_ki, g_pid_drv.Kd);
                        printf("Ki dəyişdirildi: %.4f\r\n", new_ki);  // Debug mesajı
                        ILI9341_ShowPIDTuningPage();
                    }
                    // Ki + button (245-290, 115-145)
                    else if (screen_x >= 245 && screen_x <= 290 && screen_y >= 115 && screen_y <= 145) {
                        // DÜZƏLİŞ: Ki addımı 0.001f-dən 0.005f-ə dəyişdirildi (daha sürətli tənzimləmə)
                        float new_ki = g_pid_zme.Ki + 0.005f;
                        if (new_ki > 1.0f) new_ki = 1.0f;
                        // Update both ZME and DRV PID (they use same parameters)
                        AdvancedPressureControl_SetPIDParams(&g_pid_zme, g_pid_zme.Kp, new_ki, g_pid_zme.Kd);
                        AdvancedPressureControl_SetPIDParams(&g_pid_drv, g_pid_drv.Kp, new_ki, g_pid_drv.Kd);
                        printf("Ki dəyişdirildi: %.4f\r\n", new_ki);  // Debug mesajı
                        ILI9341_ShowPIDTuningPage();
                    }
                    // Kd - button (190-235, 155-185)
                    else if (screen_x >= 190 && screen_x <= 235 && screen_y >= 155 && screen_y <= 185) {
                        float new_kd = g_pid_zme.Kd - 0.05f;
                        if (new_kd < 0.0f) new_kd = 0.0f;
                        // Update both ZME and DRV PID (they use same parameters)
                        AdvancedPressureControl_SetPIDParams(&g_pid_zme, g_pid_zme.Kp, g_pid_zme.Ki, new_kd);
                        AdvancedPressureControl_SetPIDParams(&g_pid_drv, g_pid_drv.Kp, g_pid_drv.Ki, new_kd);
                        ILI9341_ShowPIDTuningPage();
                    }
                    // Kd + button (245-290, 155-185)
                    else if (screen_x >= 245 && screen_x <= 290 && screen_y >= 155 && screen_y <= 185) {
                        float new_kd = g_pid_zme.Kd + 0.05f;
                        if (new_kd > 5.0f) new_kd = 5.0f;
                        // Update both ZME and DRV PID (they use same parameters)
                        AdvancedPressureControl_SetPIDParams(&g_pid_zme, g_pid_zme.Kp, g_pid_zme.Ki, new_kd);
                        AdvancedPressureControl_SetPIDParams(&g_pid_drv, g_pid_drv.Kp, g_pid_drv.Ki, new_kd);
                        ILI9341_ShowPIDTuningPage();
                    }
                    // Back (20-100, 200-230)
                    else if (screen_x >= 20 && screen_x <= 100 && screen_y >= 200 && screen_y <= 230) {
                        ILI9341_ShowMenuPage();
                    }
                    // Save (220-300, 200-230)
                    else if (screen_x >= 220 && screen_x <= 300 && screen_y >= 200 && screen_y <= 230) {
                        // Save PID parameters to flash memory
                        AdvancedPressureControl_SavePIDParamsToFlash();
                        
                        // Show confirmation message
                        ILI9341_DrawString(250, 100, "SAVED!", ILI9341_COLOR_GREEN, ILI9341_COLOR_BLACK, 2);
                        HAL_Delay(500);
                        ILI9341_ShowPIDTuningPage();
                    }
                    break;
            }
        }
        HAL_Delay(200); /* Debounce */
    }
}

/* Simple touch test function */
void ILI9341_SimpleTouchTest(void)
{
    static uint32_t last_touch_time = 0;
    uint32_t current_time = HAL_GetTick();
    
    /* Only check touch every 100ms to avoid too frequent updates */
    if (current_time - last_touch_time >= 100) {
        last_touch_time = current_time;
        
        uint16_t raw_x, raw_y, screen_x, screen_y;
        
        if (XPT2046_IsTouched()) {
            if (XPT2046_GetCoordinates(&raw_x, &raw_y)) {
                XPT2046_ConvertToScreen(raw_x, raw_y, &screen_x, &screen_y);
                
                /* Clean touch handling - no debug clutter */
            }
        }
    }
}

/* Pressure control logic - maintains pressure at set limit */
void ILI9341_PressureControlLogic(void)
{
    static uint32_t last_control_time = 0;
    static uint32_t last_pid_update_time = 0;
    uint32_t current_time = HAL_GetTick();
    
    // KRİTİK DÜZƏLİŞ: Target SP dəyişən kimi Flash-a yazma məntiqini silin
    // Artıq AdvancedPressureControl özü target_pressure-i yaddaşdan oxuyur
    // və yalnız Save düyməsi basıldıqda yazmalıdır (PID Tuning səhifəsində)
    
    /* Run control logic every 100ms (Update only) */
    if (current_time - last_control_time >= 100) {
        last_control_time = current_time;
        
        /* Get status from AdvancedPressureControl system */
        SystemStatus_t* status = AdvancedPressureControl_GetStatus();
        
        /* Read ZME and DRV PWM percentages from AdvancedPressureControl */
        zme_percent = status->zme_pwm_percent;
        drv_percent = status->drv_pwm_percent;
        motor_percent = status->motor_pwm_percent;
        
        /* Motor sürəti SP-yə görə tənzimlənir - bu məntiq AdvancedPressureControl_Step()-dədir.
         * Bizim burada yalnız statusu yeniləməyimiz lazımdır. */
        if (motor_percent < 0.0f) motor_percent = 0.0f;
        if (motor_percent > 100.0f) motor_percent = 100.0f;
        
        /* Update displays if on main page */
        if (pressure_control_page == 0) {
            // Update pressure display here as well for immediate feedback
            ILI9341_UpdatePressureDisplay(status->current_pressure);
            ILI9341_UpdatePercentageDisplays(drv_percent, zme_percent, motor_percent);
        }
    }
    
    /* Update PID tuning page pressure display every 200ms */
    if (current_time - last_pid_update_time >= 200) {
        last_pid_update_time = current_time;
        
        if (pressure_control_page == 9) { // PID Tuning page
            /* DÜZƏLİŞ: AdvancedPressureControl sistemindən təzyiq oxu (kalibrləmə sinxronizasiyası üçün) */
            /* DEPRECATED: PressureSensor_ConvertToPressure artıq istifadə olunmur */
            extern uint16_t adc_value;  // Will read fresh
            uint16_t adc_val = 0;
            
            // KRİTİK DÜZƏLİŞ: Manual ADC oxunması silindi - yalnız PID sistemindən təzyiq götürülür
            // PID sistemi hər 10ms-də bir ADC oxuyur və g_system_status.current_pressure-ı yeniləyir
            SystemStatus_t* status = AdvancedPressureControl_GetStatus();
            float current_pressure_val = status->current_pressure;
            
            /* Update pressure display on PID tuning page */
            char info[40];
            sprintf(info, "P: %.1f BAR", current_pressure_val);
            /* Clear old text and draw new */
            ILI9341_DrawRectangle(20, 45, 100, 15, ILI9341_COLOR_BLACK);
            ILI9341_DrawString(20, 45, info, ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 1);
        }
    }
}

/* ==================== PWM CONTROL FUNCTIONS ==================== */

/* External TIM3 handle */
extern TIM_HandleTypeDef htim3;

/* PWM to Voltage Converter Module Configuration:
 * Module: PWM to Voltage Converter 0%~100% to 0~10V
 * 
 * Connection:
 * - PWM Input: Connect to PC6 (TIM3_CH1) for Motor PWM
 * - GND: Connect to system ground
 * - VCC: Connect to 5V or 3.3V (check module requirements)
 * - Output: 0-10V analog signal for motor control
 * 
 * PWM Settings for optimal performance:
 * - Frequency: 1-10 kHz (recommended: 1 kHz)
 * - Duty Cycle: 0-100% (0% = 0V, 100% = 10V)
 * - Voltage Level: 3.3V or 5V (select via jumper on module)
 */

/* PWM Control Functions */
void PWM_SetMotorDutyCycle(float duty_percent)
{
    if (duty_percent < 0.0) duty_percent = 0.0;
    if (duty_percent > 100.0) duty_percent = 100.0;
    
    /* Get current timer period */
    uint32_t period = __HAL_TIM_GET_AUTORELOAD(&htim3);
    
    /* Convert percentage to compare value based on current period */
    uint32_t compare_value = (uint32_t)((duty_percent / 100.0) * period);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, compare_value);
    
    /* Update global variable */
    motor_duty_cycle = duty_percent;
}

void PWM_SetDRVDutyCycle(float duty_percent)
{
    if (duty_percent < 0.0) duty_percent = 0.0;
    if (duty_percent > 100.0) duty_percent = 100.0;
    
    /* Get current timer period */
    uint32_t period = __HAL_TIM_GET_AUTORELOAD(&htim3);
    
    /* Convert percentage to compare value based on current period */
    uint32_t compare_value = (uint32_t)((duty_percent / 100.0) * period);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, compare_value);
    
    /* Update global variable */
    drv_percent = duty_percent;
}

void PWM_SetZMEDutyCycle(float duty_percent)
{
    if (duty_percent < 0.0) duty_percent = 0.0;
    if (duty_percent > 100.0) duty_percent = 100.0;
    
    /* Get current timer period */
    uint32_t period = __HAL_TIM_GET_AUTORELOAD(&htim3);
    
    /* Convert percentage to compare value based on current period */
    uint32_t compare_value = (uint32_t)((duty_percent / 100.0) * period);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, compare_value);
    
    /* Update global variable */
    zme_percent = duty_percent;
}

void PWM_SetMotorFrequency(float frequency_hz)
{
    if (frequency_hz < 100.0) frequency_hz = 100.0;
    if (frequency_hz > 10000.0) frequency_hz = 10000.0;
    
    /* Calculate period for desired frequency */
    /* Timer clock: 84MHz (APB1 * 2) */
    uint32_t timer_clock = 84000000; /* 84MHz */
    uint32_t period = (timer_clock / frequency_hz) - 1;
    
    /* Update timer period */
    __HAL_TIM_SET_AUTORELOAD(&htim3, period);
    
    /* Update global variable */
    motor_frequency = frequency_hz;
    
    /* Recalculate duty cycle with new period */
    PWM_SetMotorDutyCycle(motor_duty_cycle);
}

void PWM_UpdateAllChannels(void)
{
    /* Update all PWM channels with current values */
    PWM_SetMotorDutyCycle(motor_duty_cycle);
    PWM_SetDRVDutyCycle(drv_percent);
    PWM_SetZMEDutyCycle(zme_percent);
    PWM_SetMotorFrequency(motor_frequency);
}

void PWM_TestSequence(void)
{
    /* Test sequence for PWM to voltage converter module */
    
    /* Test 1: 0% duty cycle */
    PWM_SetMotorDutyCycle(0.0);
    HAL_Delay(2000);
    
    /* Test 2: 25% duty cycle */
    PWM_SetMotorDutyCycle(25.0);
    HAL_Delay(2000);
    
    /* Test 3: 50% duty cycle */
    PWM_SetMotorDutyCycle(50.0);
    HAL_Delay(2000);
    
    /* Test 4: 75% duty cycle */
    PWM_SetMotorDutyCycle(75.0);
    HAL_Delay(2000);
    
    /* Test 5: 100% duty cycle */
    PWM_SetMotorDutyCycle(100.0);
    HAL_Delay(2000);
    
    /* Return to 0% */
    PWM_SetMotorDutyCycle(0.0);
}

/* ==================== PRESSURE SENSOR CALIBRATION FUNCTIONS ==================== */

/* External ADC handle */
extern ADC_HandleTypeDef hadc3;

/* Function to check ADC pin configuration */
void PressureSensor_CheckPinConfiguration(void)
{
    /* Check if ADC3 is properly configured */
    if (hadc3.Instance != ADC3) {
        if (pressure_control_page == 0) {
            ILI9341_DrawString(10, 10, "ADC3 NOT CONFIGURED", ILI9341_COLOR_RED, ILI9341_COLOR_BLACK, 1);
        }
        return;
    }
    
    /* Check ADC channel configuration */
    if (hadc3.Init.Resolution != ADC_RESOLUTION_12B) {
        if (pressure_control_page == 0) {
            ILI9341_DrawString(10, 10, "ADC RESOLUTION ERROR", ILI9341_COLOR_RED, ILI9341_COLOR_BLACK, 1);
        }
        return;
    }
    
    /* Check if ADC is ready */
    if (HAL_ADC_GetState(&hadc3) != HAL_ADC_STATE_READY) {
        if (pressure_control_page == 0) {
            ILI9341_DrawString(10, 10, "ADC NOT READY", ILI9341_COLOR_RED, ILI9341_COLOR_BLACK, 1);
        }
        return;
    }
    
    /* ADC configuration is OK - no need to show on main page */
}

void PressureSensor_Calibrate(void)
{
    /* KRİTİK DÜZƏLİŞ: ADC oxunması PID sistemindən götürülür - ADC bloklanması yoxdur */
    SystemStatus_t* status = AdvancedPressureControl_GetStatus();
    uint16_t adc_value = status->raw_adc_value;  // Xam ADC dəyəri Status-dan
    
    if (calibration_mode == 1) {
        /* Calibrate minimum */
        adc_min = adc_value;
        calibration_mode = 0;
    }
    else if (calibration_mode == 2) {
        /* Calibrate maximum */
        adc_max = adc_value;
        calibration_mode = 0;
    }
}

/* 
 * QEYD: Kalibrləmə məntiqi yalnız Advanced sistem daxilində (advanced_pressure_control.c)
 * saxlanılır. UI funksiyaları Advanced sistemin funksiyalarını çağırır:
 * - AdvancedPressureControl_LoadCalibration() - Flash-dan kalibrləmə yükləmə
 * - AdvancedPressureControl_SaveCalibration() - Flash-a kalibrləmə yazma
 * - AdvancedPressureControl_ReadPressure() - Təzyiq oxuma (kalibrləmə ilə)
 */

/* Convert pressure to motor percentage (0-100%) */
float PressureToMotorPercent(float pressure)
{
    if (pressure < min_pressure) pressure = min_pressure;
    if (pressure > max_pressure) pressure = max_pressure;
    
    /* Formula: Motor% = (P - 0.2) * 0.3181 */
    float motor_percent = (pressure - min_pressure) * MOTOR_PERCENT_PER_BAR;
    
    /* Clamp to 0-100% */
    if (motor_percent < 0.0f) motor_percent = 0.0f;
    if (motor_percent > 100.0f) motor_percent = 100.0f;
    
    return motor_percent;
}

/* Convert motor percentage to pressure */
float MotorPercentToPressure(float motor_percent)
{
    if (motor_percent < 0.0f) motor_percent = 0.0f;
    if (motor_percent > 100.0f) motor_percent = 100.0f;
    
    /* Formula: P = 0.2 + (Motor%) * 3.144 */
    float pressure = min_pressure + motor_percent * MOTOR_BAR_PER_PERCENT;
    
    return pressure;
}

/* Convert pressure to DRV percentage (10-40% range) */
float PressureToDRVPercent(float pressure)
{
    if (pressure < min_pressure) pressure = min_pressure;
    if (pressure > max_pressure) pressure = max_pressure;
    
    /* Formula: DRV% = 10 + (P - 0.2) * 0.09545 */
    float drv_percent = 10.0f + (pressure - min_pressure) * DRV_PERCENT_PER_BAR;
    
    /* Clamp to 10-40% */
    if (drv_percent < 10.0f) drv_percent = 10.0f;
    if (drv_percent > 40.0f) drv_percent = 40.0f;
    
    return drv_percent;
}

/* Convert pressure to ZME percentage (0-30% range, inverse) */
float PressureToZMEPercent(float pressure)
{
    if (pressure < zme_min_pressure) pressure = zme_min_pressure;
    if (pressure > zme_max_pressure) pressure = zme_max_pressure;
    
    /* ZME for pressure control: inverse relationship */
    /* At low pressure: ZME = 0% (open) - allow pressure to rise */
    /* At high pressure: ZME = 30% (closed) - prevent pressure overshoot */
    float pressure_range = zme_max_pressure - zme_min_pressure;
    float zme_percent = ((zme_max_pressure - pressure) / pressure_range) * 30.0f;
    
    /* ZME pressure control logic - improved */
    if (pressure < 10.0f) {
        zme_percent = 0.0f;  /* Very low pressure: ZME fully open for pressure rise */
    } else if (pressure > 150.0f) {
        zme_percent = 30.0f;  /* High pressure: ZME closed to prevent overshoot */
    }
    
    /* Clamp to 0-30% */
    if (zme_percent < 0.0f) zme_percent = 0.0f;
    if (zme_percent > 30.0f) zme_percent = 30.0f;
    
    return zme_percent;
}

/* IMPROVED debug function to check sensor status */
void PressureSensor_DebugStatus(void)
{
    /* KRİTİK DÜZƏLİŞ: ADC oxunması PID sistemindən götürülür - ADC bloklanması yoxdur */
    SystemStatus_t* status = AdvancedPressureControl_GetStatus();
    uint16_t raw_adc = status->raw_adc_value;  // Xam ADC dəyəri Status-dan
    
    /* Calculate voltage: V = (ADC / 4095) * 3.3V */
    float voltage = (float)raw_adc * 3.3f / 4095.0f;
    
    /* PID sistemindən təzyiq dəyərini götür */
    float pressure = status->current_pressure;
    
    /* Check for sensor problems - only show critical errors */
    if (raw_adc < 50) {
        /* Very low ADC - possible sensor disconnection */
        if (pressure_control_page == 0) {
            ILI9341_DrawString(10, 10, "SENSOR ERROR: Low ADC", ILI9341_COLOR_RED, ILI9341_COLOR_BLACK, 1);
        }
    } else if (raw_adc > 4000) {
        /* Very high ADC - possible sensor short circuit */
        if (pressure_control_page == 0) {
            ILI9341_DrawString(10, 10, "SENSOR ERROR: High ADC", ILI9341_COLOR_RED, ILI9341_COLOR_BLACK, 1);
        }
    }
    /* Normal reading - no debug clutter on main page */
    /* KRİTİK DÜZƏLİŞ: HAL_ADC_Stop() silindi - artıq manual ADC oxunması yoxdur */
    /* ADC dəyəri Status-dan götürülür, ona görə də Stop() lazım deyil */
}

void ILI9341_HandleCalibrationTouch(void)
{
    /* Handle calibration touch events */
    if (pressure_control_page == 8) {
        /* Update ADC reading display */
        static uint32_t last_adc_update = 0;
        uint32_t current_time = HAL_GetTick();
        
        if (current_time - last_adc_update > 200) { /* Update every 200ms for better responsiveness */
            last_adc_update = current_time;
            
            /* KRİTİK DÜZƏLİŞ: ADC oxunması PID sistemindən götürülür - ADC bloklanması yoxdur */
            SystemStatus_t* status = AdvancedPressureControl_GetStatus();
            uint16_t adc_value = status->raw_adc_value;  // Xam ADC dəyəri Status-dan
            
            /* Update display with current ADC value */
            char adc_str[30];
            sprintf(adc_str, "ADC: %d", adc_value);
            ILI9341_DrawString(20, 170, adc_str, ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 1);
            
            /* PID sistemindən təzyiq dəyərini götür */
            float current_pressure = status->current_pressure;
            char pressure_str[30];
            sprintf(pressure_str, "Pressure: %.1f bar", current_pressure);
            ILI9341_DrawString(20, 180, pressure_str, ILI9341_COLOR_GREEN, ILI9341_COLOR_BLACK, 1);
            
            /* Show calibration values */
            char debug_cal_str[50];
            sprintf(debug_cal_str, "Cal: %d-%d -> %.1f-%.1f", adc_min, adc_max, min_pressure, max_pressure);
            ILI9341_DrawString(20, 190, debug_cal_str, ILI9341_COLOR_CYAN, ILI9341_COLOR_BLACK, 1);
            /* KRİTİK DÜZƏLİŞ: HAL_ADC_Stop() silindi - artıq manual ADC oxunması yoxdur */
            /* ADC dəyəri Status-dan götürülür, ona görə də Stop() lazım deyil */
        }
    }
}

/* ==================== AUTO MODE CONTROL FUNCTIONS ==================== */

/* AutoMode functions - REMOVED (AutoMode deleted) */
