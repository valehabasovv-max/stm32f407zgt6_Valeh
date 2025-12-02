/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : VALEH High Pressure Control System v4.0
  *                   Touch Screen ilə Tam İdarəetmə Sistemi
  ******************************************************************************
  * @attention
  *
  * Xüsusiyyətlər:
  * - Touch ilə idarəetmə (XPT2046)
  * - Gözəl və modern UI dizaynı
  * - PWM idarəetməsi (Motor, ZME, DRV)
  * - PID nəzarəti
  * - Kalibrasiya sistemi
  * - Preset təzyiq səviyyələri
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ILI9341_FSMC.h"
#include "XPT2046.h"
#include "advanced_pressure_control.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* =============== EKRAN SƏHİFƏLƏRİ =============== */
typedef enum {
    PAGE_MAIN = 0,          /* Əsas idarəetmə ekranı */
    PAGE_MENU,              /* Menyu */
    PAGE_SETPOINT,          /* Setpoint ayarı */
    PAGE_PID_TUNE,          /* PID parametrləri */
    PAGE_CALIBRATION,       /* Sensor kalibrasiyası */
    PAGE_TOUCH_CAL,         /* Touch kalibrasiyası */
    PAGE_INFO               /* Sistem məlumatları */
} ScreenPage_t;

/* =============== DÜYMƏ STRUKTURU =============== */
typedef struct {
    uint16_t x, y;          /* Mövqe */
    uint16_t w, h;          /* Ölçü */
    uint16_t bg_color;      /* Arxa plan rəngi */
    uint16_t fg_color;      /* Mətn rəngi */
    const char* text;       /* Mətn */
    uint8_t font_size;      /* Font ölçüsü */
} Button_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   240

/* Rənglər - Modern palitra */
#define COLOR_BG_DARK       0x0821      /* Tünd arxa plan */
#define COLOR_BG_PANEL      0x1082      /* Panel arxa plan */
#define COLOR_ACCENT_BLUE   0x04DF      /* Vurğu mavi */
#define COLOR_ACCENT_GREEN  0x07E0      /* Yaşıl */
#define COLOR_ACCENT_RED    0xF800      /* Qırmızı */
#define COLOR_ACCENT_YELLOW 0xFFE0      /* Sarı */
#define COLOR_ACCENT_ORANGE 0xFD20      /* Narıncı */
#define COLOR_TEXT_WHITE    0xFFFF      /* Ağ mətn */
#define COLOR_TEXT_GREY     0x8410      /* Boz mətn */
#define COLOR_BORDER        0x4208      /* Çərçivə */

/* Touch parametrləri */
#define TOUCH_DEBOUNCE_MS   200
#define SCREEN_UPDATE_MS    100

/* Preset sayı */
#define NUM_PRESETS         6

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc3;
SPI_HandleTypeDef hspi1;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim6;
HCD_HandleTypeDef hhcd_USB_OTG_FS;
SRAM_HandleTypeDef hsram1;

/* USER CODE BEGIN PV */

/* =============== SİSTEM DƏYİŞƏNLƏRİ =============== */
static ScreenPage_t g_current_page = PAGE_MAIN;
static uint8_t g_needs_redraw = 1;
static uint32_t g_last_touch_time = 0;
static uint32_t g_last_screen_update = 0;
static uint8_t g_touch_was_pressed = 0;

/* Preset dəyərləri */
static const float g_presets[NUM_PRESETS] = {50.0f, 100.0f, 150.0f, 200.0f, 250.0f, 300.0f};
static const char* g_preset_names[NUM_PRESETS] = {"LOW", "MED", "HIGH", "V.HI", "EXT", "MAX"};
static const uint16_t g_preset_colors[NUM_PRESETS] = {
    COLOR_ACCENT_GREEN, ILI9341_COLOR_CYAN, COLOR_ACCENT_YELLOW,
    COLOR_ACCENT_ORANGE, COLOR_ACCENT_RED, ILI9341_COLOR_MAGENTA
};
static uint8_t g_current_preset = 1;  /* 100 bar default */

/* Sistem durumu */
static uint8_t g_system_running = 0;  /* 0=Stop, 1=Run */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_FSMC_Init(void);
static void MX_USB_OTG_FS_HCD_Init(void);
static void MX_ADC3_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM6_Init(void);
static void MX_SPI1_Init(void);

/* USER CODE BEGIN PFP */
/* Ekran funksiyaları */
void Screen_DrawSplash(void);
void Screen_DrawMain(void);
void Screen_DrawMenu(void);
void Screen_DrawSetpoint(void);
void Screen_DrawPIDTune(void);
void Screen_DrawCalibration(void);
void Screen_DrawInfo(void);
void Screen_Update(void);

/* Touch idarəetməsi */
void Touch_Process(void);
void Touch_HandleMain(uint16_t x, uint16_t y);
void Touch_HandleMenu(uint16_t x, uint16_t y);
void Touch_HandleSetpoint(uint16_t x, uint16_t y);
void Touch_HandlePIDTune(uint16_t x, uint16_t y);
void Touch_HandleCalibration(uint16_t x, uint16_t y);

/* UI komponentləri */
void UI_DrawButton(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                   const char* text, uint16_t bg_color, uint16_t fg_color, uint8_t size);
void UI_DrawPanel(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const char* title);
void UI_DrawProgressBar(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                        float value, float max_val, uint16_t color);
void UI_DrawGauge(uint16_t cx, uint16_t cy, uint16_t r, float value, float max_val);
void UI_DrawValueDisplay(uint16_t x, uint16_t y, const char* label, 
                         float value, const char* unit, uint16_t color);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* =============== UI KOMPONENTLƏR =============== */

/**
 * @brief Düymə çək
 */
void UI_DrawButton(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                   const char* text, uint16_t bg_color, uint16_t fg_color, uint8_t size) {
    /* Arxa plan */
    ILI9341_FillRect(x, y, w, h, bg_color);
    
    /* Çərçivə */
    ILI9341_DrawRect(x, y, w, h, COLOR_BORDER);
    
    /* 3D effekt - yuxarı kənar açıq */
    ILI9341_DrawLine(x + 1, y + 1, x + w - 2, y + 1, 
                     (bg_color == COLOR_BG_DARK) ? COLOR_BORDER : ILI9341_COLOR_WHITE);
    
    /* Mətn - ortada */
    uint16_t text_len = strlen(text) * 8 * size;
    uint16_t text_x = x + (w - text_len) / 2;
    uint16_t text_y = y + (h - 10 * size) / 2;
    ILI9341_DrawString(text_x, text_y, text, fg_color, bg_color, size);
}

/**
 * @brief Panel çək
 */
void UI_DrawPanel(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const char* title) {
    /* Arxa plan */
    ILI9341_FillRect(x, y, w, h, COLOR_BG_PANEL);
    
    /* Çərçivə */
    ILI9341_DrawRect(x, y, w, h, COLOR_ACCENT_BLUE);
    
    /* Başlıq */
    if (title && strlen(title) > 0) {
        ILI9341_FillRect(x + 1, y + 1, w - 2, 16, COLOR_ACCENT_BLUE);
        ILI9341_DrawString(x + 5, y + 3, title, COLOR_TEXT_WHITE, COLOR_ACCENT_BLUE, 1);
    }
}

/**
 * @brief Progress bar çək
 */
void UI_DrawProgressBar(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                        float value, float max_val, uint16_t color) {
    /* Çərçivə */
    ILI9341_DrawRect(x, y, w, h, COLOR_BORDER);
    
    /* Arxa plan */
    ILI9341_FillRect(x + 1, y + 1, w - 2, h - 2, COLOR_BG_DARK);
    
    /* Dəyər */
    float percent = value / max_val;
    if (percent > 1.0f) percent = 1.0f;
    if (percent < 0.0f) percent = 0.0f;
    
    uint16_t bar_w = (uint16_t)((w - 4) * percent);
    if (bar_w > 0) {
        ILI9341_FillRect(x + 2, y + 2, bar_w, h - 4, color);
    }
}

/**
 * @brief Dairəvi gauge çək
 */
void UI_DrawGauge(uint16_t cx, uint16_t cy, uint16_t r, float value, float max_val) {
    /* Xarici dairə */
    for (int a = -135; a <= 135; a += 3) {
        float rad = a * 3.14159f / 180.0f;
        int x1 = cx + (int)((r - 2) * cosf(rad));
        int y1 = cy + (int)((r - 2) * sinf(rad));
        ILI9341_DrawPixel(x1, y1, COLOR_BORDER);
    }
    
    /* Dəyər qövsü */
    float percent = value / max_val;
    if (percent > 1.0f) percent = 1.0f;
    if (percent < 0.0f) percent = 0.0f;
    
    int end_angle = -135 + (int)(percent * 270.0f);
    
    /* Rəng seçimi */
    uint16_t color;
    if (percent < 0.33f) {
        color = COLOR_ACCENT_GREEN;
    } else if (percent < 0.66f) {
        color = COLOR_ACCENT_YELLOW;
    } else {
        color = COLOR_ACCENT_RED;
    }
    
    for (int a = -135; a <= end_angle; a += 2) {
        float rad = a * 3.14159f / 180.0f;
        for (int dr = 6; dr <= 10; dr++) {
            int x1 = cx + (int)((r - dr) * cosf(rad));
            int y1 = cy + (int)((r - dr) * sinf(rad));
            if (x1 >= 0 && x1 < 320 && y1 >= 0 && y1 < 240) {
                ILI9341_DrawPixel(x1, y1, color);
            }
        }
    }
}

/**
 * @brief Dəyər göstəricisi çək
 */
void UI_DrawValueDisplay(uint16_t x, uint16_t y, const char* label, 
                         float value, const char* unit, uint16_t color) {
    /* Label */
    ILI9341_DrawString(x, y, label, COLOR_TEXT_GREY, COLOR_BG_DARK, 1);
    
    /* Dəyər */
    char val_str[16];
    if (value < 10.0f) {
        sprintf(val_str, "%.2f", value);
    } else if (value < 100.0f) {
        sprintf(val_str, "%.1f", value);
    } else {
        sprintf(val_str, "%.0f", value);
    }
    ILI9341_DrawString(x, y + 12, val_str, color, COLOR_BG_DARK, 2);
    
    /* Vahid */
    ILI9341_DrawString(x + strlen(val_str) * 16 + 5, y + 18, unit, COLOR_TEXT_GREY, COLOR_BG_DARK, 1);
}

/* =============== EKRAN SƏHİFƏLƏRİ =============== */

/**
 * @brief Açılış ekranı
 */
void Screen_DrawSplash(void) {
    ILI9341_FillScreen(COLOR_BG_DARK);
    
    /* Logo çərçivəsi */
    ILI9341_DrawRect(30, 40, 260, 90, COLOR_ACCENT_BLUE);
    ILI9341_DrawRect(32, 42, 256, 86, COLOR_ACCENT_BLUE);
    
    /* Başlıq */
    ILI9341_DrawString(95, 55, "VALEH", ILI9341_COLOR_CYAN, COLOR_BG_DARK, 4);
    ILI9341_DrawString(50, 100, "HIGH PRESSURE CONTROL", COLOR_TEXT_WHITE, COLOR_BG_DARK, 1);
    
    /* Versiya */
    ILI9341_DrawString(140, 145, "v4.0", COLOR_ACCENT_YELLOW, COLOR_BG_DARK, 2);
    
    /* Yükləmə animasiyası - daha sürətli */
    ILI9341_DrawRect(60, 180, 200, 20, COLOR_BORDER);
    for (int i = 0; i <= 100; i += 10) {
        ILI9341_FillRect(62, 182, i * 2 - 4, 16, COLOR_ACCENT_GREEN);
        HAL_Delay(20);  /* 30ms-dən 20ms-ə azaldıldı */
    }
    
    /* Mesaj */
    ILI9341_DrawString(100, 210, "Sistem hazir!", COLOR_ACCENT_GREEN, COLOR_BG_DARK, 1);
    HAL_Delay(300);  /* 500ms-dən 300ms-ə azaldıldı */
}

/**
 * @brief Əsas ekran
 */
void Screen_DrawMain(void) {
    SystemStatus_t* status = AdvancedPressureControl_GetStatus();
    
    if (g_needs_redraw) {
        ILI9341_FillScreen(COLOR_BG_DARK);
        
        /* Başlıq paneli */
        ILI9341_FillRect(0, 0, 320, 22, COLOR_ACCENT_BLUE);
        ILI9341_DrawString(10, 5, "VALEH HPC", 
                          COLOR_TEXT_WHITE, COLOR_ACCENT_BLUE, 1);
        
        /* Sol üst künc - mode azaltma işarəsi */
        ILI9341_DrawString(0, 5, "<", ILI9341_COLOR_YELLOW, COLOR_ACCENT_BLUE, 1);
        
        /* Koordinat mode göstəricisi - sağ üst küncdə (toxunuş üçün böyük zona) */
        char mode_info[16];
        sprintf(mode_info, "M%d>", XPT2046_GetCoordMode());
        ILI9341_DrawString(290, 5, mode_info, ILI9341_COLOR_YELLOW, COLOR_ACCENT_BLUE, 1);
        
        /* Sol panel - Preset düymələri */
        UI_DrawPanel(5, 25, 105, 95, "PRESET");
        
        /* Sağ panel - PWM/PID info */
        UI_DrawPanel(210, 25, 105, 95, "STATUS");
        
        /* Alt panel - Control düymələri */
        UI_DrawPanel(5, 195, 310, 42, "CONTROL");
        
        g_needs_redraw = 0;
    }
    
    /* Preset düymələri */
    for (int i = 0; i < 6; i++) {
        uint16_t btn_x = 8 + (i % 3) * 34;
        uint16_t btn_y = 42 + (i / 3) * 38;
        uint16_t bg = (i == g_current_preset) ? g_preset_colors[i] : COLOR_BG_DARK;
        uint16_t fg = (i == g_current_preset) ? COLOR_BG_DARK : g_preset_colors[i];
        
        ILI9341_FillRect(btn_x, btn_y, 32, 35, bg);
        ILI9341_DrawRect(btn_x, btn_y, 32, 35, g_preset_colors[i]);
        
        char val[8];
        sprintf(val, "%.0f", g_presets[i]);
        ILI9341_DrawString(btn_x + 3, btn_y + 5, val, fg, bg, 1);
        ILI9341_DrawString(btn_x + 2, btn_y + 20, g_preset_names[i], fg, bg, 1);
    }
    
    /* Əsas təzyiq göstəricisi - mərkəz */
    float current_p = status->current_pressure;
    float target_p = status->target_pressure;
    
    /* Təzyiq dəyəri */
    ILI9341_FillRect(115, 30, 90, 50, COLOR_BG_DARK);
    char press_str[16];
    sprintf(press_str, "%.1f", current_p);
    
    /* Rəng seçimi */
    uint16_t press_color;
    if (fabsf(current_p - target_p) < 5.0f) {
        press_color = COLOR_ACCENT_GREEN;
    } else if (current_p > target_p) {
        press_color = COLOR_ACCENT_RED;
    } else {
        press_color = COLOR_ACCENT_YELLOW;
    }
    
    ILI9341_DrawString(115, 35, press_str, press_color, COLOR_BG_DARK, 3);
    ILI9341_DrawString(115, 65, "BAR", COLOR_TEXT_GREY, COLOR_BG_DARK, 1);
    
    /* Hədəf göstəricisi */
    char target_str[24];
    sprintf(target_str, "SP: %.0f bar", target_p);
    ILI9341_FillRect(115, 80, 90, 15, COLOR_BG_DARK);
    ILI9341_DrawString(115, 80, target_str, ILI9341_COLOR_CYAN, COLOR_BG_DARK, 1);
    
    /* Progress bar */
    UI_DrawProgressBar(115, 100, 90, 15, current_p, 300.0f, press_color);
    
    /* Status paneli - PWM dəyərləri */
    char line[20];
    ILI9341_FillRect(213, 42, 98, 75, COLOR_BG_PANEL);
    
    sprintf(line, "MTR:%5.1f%%", status->motor_pwm_percent);
    ILI9341_DrawString(215, 45, line, COLOR_ACCENT_YELLOW, COLOR_BG_PANEL, 1);
    
    sprintf(line, "ZME:%5.1f%%", status->zme_pwm_percent);
    ILI9341_DrawString(215, 60, line, ILI9341_COLOR_CYAN, COLOR_BG_PANEL, 1);
    
    sprintf(line, "DRV:%5.1f%%", status->drv_pwm_percent);
    ILI9341_DrawString(215, 75, line, COLOR_ACCENT_GREEN, COLOR_BG_PANEL, 1);
    
    sprintf(line, "ERR:%+5.1f", status->error);
    uint16_t err_color = (fabsf(status->error) < 3.0f) ? COLOR_ACCENT_GREEN : COLOR_ACCENT_YELLOW;
    ILI9341_DrawString(215, 95, line, err_color, COLOR_BG_PANEL, 1);
    
    /* Gauge */
    UI_DrawGauge(160, 155, 35, current_p, 300.0f);
    
    /* Control düymələri */
    /* START/STOP */
    if (g_system_running) {
        UI_DrawButton(10, 208, 70, 25, "STOP", COLOR_ACCENT_RED, COLOR_TEXT_WHITE, 1);
    } else {
        UI_DrawButton(10, 208, 70, 25, "START", COLOR_ACCENT_GREEN, COLOR_BG_DARK, 1);
    }
    
    /* MENU */
    UI_DrawButton(85, 208, 70, 25, "MENU", COLOR_ACCENT_BLUE, COLOR_TEXT_WHITE, 1);
    
    /* SP- */
    UI_DrawButton(160, 208, 35, 25, "-", COLOR_BG_PANEL, COLOR_TEXT_WHITE, 2);
    
    /* SP+ */
    UI_DrawButton(200, 208, 35, 25, "+", COLOR_BG_PANEL, COLOR_TEXT_WHITE, 2);
    
    /* PID indicator */
    if (status->control_enabled) {
        ILI9341_DrawString(245, 212, "PID:ON", COLOR_ACCENT_GREEN, COLOR_BG_PANEL, 1);
    } else {
        ILI9341_DrawString(245, 212, "PID:OFF", COLOR_ACCENT_RED, COLOR_BG_PANEL, 1);
    }
}

/**
 * @brief Menyu ekranı
 */
void Screen_DrawMenu(void) {
    if (g_needs_redraw) {
        ILI9341_FillScreen(COLOR_BG_DARK);
        
        /* Başlıq */
        ILI9341_FillRect(0, 0, 320, 30, COLOR_ACCENT_BLUE);
        
        /* Sol üst - mode azalt */
        ILI9341_DrawString(5, 8, "<", ILI9341_COLOR_YELLOW, COLOR_ACCENT_BLUE, 2);
        
        ILI9341_DrawString(130, 8, "MENU", COLOR_TEXT_WHITE, COLOR_ACCENT_BLUE, 2);
        
        /* Sağ üst - mode artır */
        char mode_str[8];
        sprintf(mode_str, "M%d>", XPT2046_GetCoordMode());
        ILI9341_DrawString(280, 8, mode_str, ILI9341_COLOR_YELLOW, COLOR_ACCENT_BLUE, 1);
        
        /* Menyu düymələri */
        UI_DrawButton(40, 45, 240, 35, "SETPOINT", COLOR_BG_PANEL, ILI9341_COLOR_CYAN, 2);
        UI_DrawButton(40, 90, 240, 35, "PID TUNE", COLOR_BG_PANEL, COLOR_ACCENT_YELLOW, 2);
        UI_DrawButton(40, 135, 240, 35, "CALIBRATION", COLOR_BG_PANEL, COLOR_ACCENT_GREEN, 2);
        UI_DrawButton(40, 180, 240, 35, "BACK", COLOR_ACCENT_RED, COLOR_TEXT_WHITE, 2);
        
        g_needs_redraw = 0;
    }
}

/**
 * @brief Setpoint ayar ekranı
 */
void Screen_DrawSetpoint(void) {
    SystemStatus_t* status = AdvancedPressureControl_GetStatus();
    
    if (g_needs_redraw) {
        ILI9341_FillScreen(COLOR_BG_DARK);
        
        /* Başlıq */
        ILI9341_FillRect(0, 0, 320, 30, COLOR_ACCENT_BLUE);
        ILI9341_DrawString(80, 8, "SET PRESSURE", COLOR_TEXT_WHITE, COLOR_ACCENT_BLUE, 2);
        
        /* Panel */
        UI_DrawPanel(20, 40, 280, 100, "TARGET PRESSURE");
        
        /* Düymələr */
        UI_DrawButton(25, 100, 50, 35, "-10", COLOR_ACCENT_RED, COLOR_TEXT_WHITE, 2);
        UI_DrawButton(80, 100, 50, 35, "-1", COLOR_ACCENT_YELLOW, COLOR_BG_DARK, 2);
        UI_DrawButton(190, 100, 50, 35, "+1", COLOR_ACCENT_GREEN, COLOR_BG_DARK, 2);
        UI_DrawButton(245, 100, 50, 35, "+10", COLOR_ACCENT_GREEN, COLOR_TEXT_WHITE, 2);
        
        /* Preset düymələri */
        for (int i = 0; i < 6; i++) {
            uint16_t btn_x = 25 + i * 48;
            UI_DrawButton(btn_x, 150, 45, 25, g_preset_names[i], 
                         g_preset_colors[i], COLOR_BG_DARK, 1);
        }
        
        /* Back düymə */
        UI_DrawButton(100, 190, 120, 35, "BACK", COLOR_ACCENT_BLUE, COLOR_TEXT_WHITE, 2);
        
        g_needs_redraw = 0;
    }
    
    /* Hazırki dəyəri göstər */
    char val_str[16];
    sprintf(val_str, "%.0f BAR", status->target_pressure);
    ILI9341_FillRect(100, 60, 120, 30, COLOR_BG_PANEL);
    ILI9341_DrawString(110, 65, val_str, COLOR_ACCENT_YELLOW, COLOR_BG_PANEL, 2);
}

/**
 * @brief PID tənzimləmə ekranı
 */
void Screen_DrawPIDTune(void) {
    SystemStatus_t* status = AdvancedPressureControl_GetStatus();
    
    if (g_needs_redraw) {
        ILI9341_FillScreen(COLOR_BG_DARK);
        
        /* Başlıq */
        ILI9341_FillRect(0, 0, 320, 30, COLOR_ACCENT_BLUE);
        ILI9341_DrawString(100, 8, "PID TUNING", COLOR_TEXT_WHITE, COLOR_ACCENT_BLUE, 2);
        
        /* Kp sırası */
        ILI9341_DrawString(20, 45, "Kp:", COLOR_TEXT_WHITE, COLOR_BG_DARK, 2);
        UI_DrawButton(200, 40, 40, 30, "-", COLOR_BG_PANEL, COLOR_TEXT_WHITE, 2);
        UI_DrawButton(260, 40, 40, 30, "+", COLOR_BG_PANEL, COLOR_TEXT_WHITE, 2);
        
        /* Ki sırası */
        ILI9341_DrawString(20, 85, "Ki:", COLOR_TEXT_WHITE, COLOR_BG_DARK, 2);
        UI_DrawButton(200, 80, 40, 30, "-", COLOR_BG_PANEL, COLOR_TEXT_WHITE, 2);
        UI_DrawButton(260, 80, 40, 30, "+", COLOR_BG_PANEL, COLOR_TEXT_WHITE, 2);
        
        /* Kd sırası */
        ILI9341_DrawString(20, 125, "Kd:", COLOR_TEXT_WHITE, COLOR_BG_DARK, 2);
        UI_DrawButton(200, 120, 40, 30, "-", COLOR_BG_PANEL, COLOR_TEXT_WHITE, 2);
        UI_DrawButton(260, 120, 40, 30, "+", COLOR_BG_PANEL, COLOR_TEXT_WHITE, 2);
        
        /* SP sırası */
        ILI9341_DrawString(20, 165, "SP:", COLOR_TEXT_WHITE, COLOR_BG_DARK, 2);
        UI_DrawButton(200, 160, 40, 30, "-", COLOR_BG_PANEL, COLOR_TEXT_WHITE, 2);
        UI_DrawButton(260, 160, 40, 30, "+", COLOR_BG_PANEL, COLOR_TEXT_WHITE, 2);
        
        /* Back və Save */
        UI_DrawButton(20, 200, 80, 30, "BACK", COLOR_ACCENT_BLUE, COLOR_TEXT_WHITE, 1);
        UI_DrawButton(220, 200, 80, 30, "SAVE", COLOR_ACCENT_GREEN, COLOR_TEXT_WHITE, 1);
        
        g_needs_redraw = 0;
    }
    
    /* Dəyərləri göstər */
    char val_str[16];
    
    ILI9341_FillRect(80, 45, 100, 25, COLOR_BG_DARK);
    sprintf(val_str, "%.3f", g_pid_zme.Kp);
    ILI9341_DrawString(80, 45, val_str, COLOR_ACCENT_GREEN, COLOR_BG_DARK, 2);
    
    ILI9341_FillRect(80, 85, 100, 25, COLOR_BG_DARK);
    sprintf(val_str, "%.4f", g_pid_zme.Ki);
    ILI9341_DrawString(80, 85, val_str, COLOR_ACCENT_GREEN, COLOR_BG_DARK, 2);
    
    ILI9341_FillRect(80, 125, 100, 25, COLOR_BG_DARK);
    sprintf(val_str, "%.3f", g_pid_zme.Kd);
    ILI9341_DrawString(80, 125, val_str, COLOR_ACCENT_GREEN, COLOR_BG_DARK, 2);
    
    ILI9341_FillRect(80, 165, 100, 25, COLOR_BG_DARK);
    sprintf(val_str, "%.0f bar", status->target_pressure);
    ILI9341_DrawString(80, 165, val_str, COLOR_ACCENT_YELLOW, COLOR_BG_DARK, 2);
}

/**
 * @brief Kalibrasiya ekranı
 */
void Screen_DrawCalibration(void) {
    SystemStatus_t* status = AdvancedPressureControl_GetStatus();
    
    if (g_needs_redraw) {
        ILI9341_FillScreen(COLOR_BG_DARK);
        
        /* Başlıq */
        ILI9341_FillRect(0, 0, 320, 30, COLOR_ACCENT_BLUE);
        ILI9341_DrawString(80, 8, "CALIBRATION", COLOR_TEXT_WHITE, COLOR_ACCENT_BLUE, 2);
        
        /* Kalibrasiya düymələri */
        UI_DrawButton(20, 60, 130, 40, "CAL MIN", COLOR_ACCENT_YELLOW, COLOR_BG_DARK, 2);
        UI_DrawButton(170, 60, 130, 40, "CAL MAX", COLOR_ACCENT_GREEN, COLOR_TEXT_WHITE, 2);
        
        /* Save/Back */
        UI_DrawButton(20, 180, 100, 35, "BACK", COLOR_ACCENT_BLUE, COLOR_TEXT_WHITE, 2);
        UI_DrawButton(200, 180, 100, 35, "SAVE", COLOR_ACCENT_GREEN, COLOR_TEXT_WHITE, 2);
        
        g_needs_redraw = 0;
    }
    
    /* ADC və Pressure dəyərlərini göstər */
    char line[32];
    ILI9341_FillRect(20, 115, 280, 55, COLOR_BG_DARK);
    
    sprintf(line, "ADC:  %d", status->raw_adc_value);
    ILI9341_DrawString(30, 120, line, COLOR_ACCENT_YELLOW, COLOR_BG_DARK, 2);
    
    sprintf(line, "PRES: %.2f bar", status->current_pressure);
    ILI9341_DrawString(30, 145, line, COLOR_ACCENT_GREEN, COLOR_BG_DARK, 2);
}

/**
 * @brief Ekran yeniləməsi
 */
void Screen_Update(void) {
    uint32_t now = HAL_GetTick();
    if (now - g_last_screen_update < SCREEN_UPDATE_MS) {
        return;
    }
    g_last_screen_update = now;
    
    switch (g_current_page) {
        case PAGE_MAIN:
            Screen_DrawMain();
            break;
        case PAGE_MENU:
            Screen_DrawMenu();
            break;
        case PAGE_SETPOINT:
            Screen_DrawSetpoint();
            break;
        case PAGE_PID_TUNE:
            Screen_DrawPIDTune();
            break;
        case PAGE_CALIBRATION:
            Screen_DrawCalibration();
            break;
        default:
            Screen_DrawMain();
            break;
    }
}

/* =============== TOUCH İDARƏETMƏSİ =============== */

/**
 * @brief Touch işləmə - SADƏLƏŞDİRİLMİŞ VƏ ETİBARLI VERSİYA
 * @note Butonların düzgün işləməsi üçün optimallaşdırılmış
 */
void Touch_Process(void) {
    uint16_t tx, ty;
    uint16_t raw_x, raw_y;
    
    /* Touch varmı yoxla */
    if (!XPT2046_IsTouched()) {
        g_touch_was_pressed = 0;  /* Touch buraxıldı */
        return;
    }
    
    /* Debounce - əvvəlki touch-dan minimum vaxt keçməlidir */
    uint32_t now = HAL_GetTick();
    if (now - g_last_touch_time < TOUCH_DEBOUNCE_MS) {
        return;  /* Debounce vaxtı keçməyib */
    }
    
    /* Əgər artıq basılıbsa, buraxılana qədər gözlə */
    if (g_touch_was_pressed) {
        return;
    }
    
    /* Raw koordinatları oxu */
    if (!XPT2046_GetCoordinates(&raw_x, &raw_y)) {
        return;  /* Koordinat oxunmadı */
    }
    
    /* Screen koordinatlarına çevir */
    XPT2046_ConvertToScreen(raw_x, raw_y, &tx, &ty);
    
    /* Touch qeydiyyatı */
    g_last_touch_time = now;
    g_touch_was_pressed = 1;
    
    /* Debug: Koordinatları serial portda göstər */
    printf("TOUCH: screen(%d,%d) raw(%d,%d) page=%d\r\n", tx, ty, raw_x, raw_y, g_current_page);
    
    /* Səhifəyə görə touch işlə */
    switch (g_current_page) {
        case PAGE_MAIN:
            Touch_HandleMain(tx, ty);
            break;
        case PAGE_MENU:
            Touch_HandleMenu(tx, ty);
            break;
        case PAGE_SETPOINT:
            Touch_HandleSetpoint(tx, ty);
            break;
        case PAGE_PID_TUNE:
            Touch_HandlePIDTune(tx, ty);
            break;
        case PAGE_CALIBRATION:
            Touch_HandleCalibration(tx, ty);
            break;
        default:
            break;
    }
}

/**
 * @brief Əsas ekran touch - SADƏLƏŞDİRİLMİŞ VƏ ETİBARLI
 * @note Buton zonaları genişləndirilmiş - daha yaxşı toxunuş algılama
 */
void Touch_HandleMain(uint16_t x, uint16_t y) {
    SystemStatus_t* status = AdvancedPressureControl_GetStatus();
    
    printf("Touch Main: x=%d, y=%d\r\n", x, y);
    
    /* ============================================
     * ALT PANEL - KONTROL DÜYMƏLƏRİ (y > 190)
     * Bu zona ən vacibdir - y > 190 pixel
     * ============================================ */
    if (y > 190) {
        /* START/STOP düyməsi (x: 10-80) */
        if (x < 90) {
            printf(">>> START/STOP\r\n");
            g_system_running = !g_system_running;
            status->control_enabled = g_system_running;
            g_needs_redraw = 1;
            return;
        }
        
        /* MENU düyməsi (x: 85-155) */
        if (x >= 70 && x < 170) {
            printf(">>> MENU\r\n");
            g_current_page = PAGE_MENU;
            g_needs_redraw = 1;
            return;
        }
        
        /* SP- düyməsi (x: 160-195) */
        if (x >= 150 && x < 210) {
            printf(">>> SP-\r\n");
            float new_sp = status->target_pressure - 10.0f;
            if (new_sp < 0.0f) new_sp = 0.0f;
            AdvancedPressureControl_SetTargetPressure(new_sp);
            g_needs_redraw = 1;
            return;
        }
        
        /* SP+ düyməsi (x: 200-235) */
        if (x >= 190) {
            printf(">>> SP+\r\n");
            float new_sp = status->target_pressure + 10.0f;
            if (new_sp > 300.0f) new_sp = 300.0f;
            AdvancedPressureControl_SetTargetPressure(new_sp);
            g_needs_redraw = 1;
            return;
        }
        return;
    }
    
    /* ============================================
     * PRESET DÜYMƏLƏRİ - SOL PANEL (x < 115, y: 25-120)
     * 3x2 grid - hər düymə ~34x38 pixel
     * ============================================ */
    if (x < 115 && y >= 25 && y < 125) {
        int col = x / 38;        /* 0, 1 və ya 2 */
        int row = (y - 25) / 45; /* 0 və ya 1 */
        
        if (col > 2) col = 2;
        if (row > 1) row = 1;
        
        int idx = row * 3 + col;
        if (idx >= 0 && idx < 6) {
            printf(">>> PRESET %d (%.0f bar)\r\n", idx, g_presets[idx]);
            g_current_preset = idx;
            AdvancedPressureControl_SetTargetPressure(g_presets[idx]);
            g_needs_redraw = 1;
        }
        return;
    }
    
    /* Digər zonalara toxunuş - heç nə etmə */
    printf("Touch outside buttons: x=%d, y=%d\r\n", x, y);
}

/**
 * @brief Menyu ekranı touch - SADƏLƏŞDİRİLMİŞ
 * @note Düymə zonaları Y koordinatına görə bölünüb
 */
void Touch_HandleMenu(uint16_t x, uint16_t y) {
    printf("Touch Menu: x=%d, y=%d\r\n", x, y);
    
    /* ============================================
     * MENYU DÜYMƏLƏRİ - Y KOORDINATINA GÖRƏ
     * Düymələr: SETPOINT(45), PID TUNE(90), CALIBRATION(135), BACK(180)
     * ============================================ */
    
    /* SETPOINT düyməsi (y: 40-85) */
    if (y >= 40 && y < 85) {
        printf(">>> SETPOINT\r\n");
        g_current_page = PAGE_SETPOINT;
        g_needs_redraw = 1;
        return;
    }
    
    /* PID TUNE düyməsi (y: 85-130) */
    if (y >= 85 && y < 130) {
        printf(">>> PID TUNE\r\n");
        g_current_page = PAGE_PID_TUNE;
        g_needs_redraw = 1;
        return;
    }
    
    /* CALIBRATION düyməsi (y: 130-175) */
    if (y >= 130 && y < 175) {
        printf(">>> CALIBRATION\r\n");
        g_current_page = PAGE_CALIBRATION;
        g_needs_redraw = 1;
        return;
    }
    
    /* BACK düyməsi (y >= 175) */
    if (y >= 175) {
        printf(">>> BACK\r\n");
        g_current_page = PAGE_MAIN;
        g_needs_redraw = 1;
        return;
    }
}

/**
 * @brief Setpoint ekranı touch - SADƏLƏŞDİRİLMİŞ
 */
void Touch_HandleSetpoint(uint16_t x, uint16_t y) {
    SystemStatus_t* status = AdvancedPressureControl_GetStatus();
    
    printf("Touch Setpoint: x=%d, y=%d\r\n", x, y);
    
    /* Düymə sırası (y: 95-140) */
    if (y >= 95 && y < 145) {
        if (x < 80) {
            /* -10 düyməsi */
            printf(">>> SP -10\r\n");
            float new_sp = status->target_pressure - 10.0f;
            if (new_sp < 0.0f) new_sp = 0.0f;
            AdvancedPressureControl_SetTargetPressure(new_sp);
        }
        else if (x < 140) {
            /* -1 düyməsi */
            printf(">>> SP -1\r\n");
            float new_sp = status->target_pressure - 1.0f;
            if (new_sp < 0.0f) new_sp = 0.0f;
            AdvancedPressureControl_SetTargetPressure(new_sp);
        }
        else if (x < 200) {
            /* +1 düyməsi */
            printf(">>> SP +1\r\n");
            float new_sp = status->target_pressure + 1.0f;
            if (new_sp > 300.0f) new_sp = 300.0f;
            AdvancedPressureControl_SetTargetPressure(new_sp);
        }
        else {
            /* +10 düyməsi */
            printf(">>> SP +10\r\n");
            float new_sp = status->target_pressure + 10.0f;
            if (new_sp > 300.0f) new_sp = 300.0f;
            AdvancedPressureControl_SetTargetPressure(new_sp);
        }
        return;
    }
    
    /* Preset düymələri (y: 145-180) */
    if (y >= 145 && y < 185) {
        int idx = x / 55;  /* 6 düymə, hər biri ~53 pixel */
        if (idx >= 6) idx = 5;
        printf(">>> Preset %d\r\n", idx);
        g_current_preset = idx;
        AdvancedPressureControl_SetTargetPressure(g_presets[idx]);
        return;
    }
    
    /* BACK düyməsi (y >= 185) */
    if (y >= 185) {
        printf(">>> BACK\r\n");
        g_current_page = PAGE_MAIN;
        g_needs_redraw = 1;
        return;
    }
}

/**
 * @brief PID tənzimləmə touch - SADƏLƏŞDİRİLMİŞ
 */
void Touch_HandlePIDTune(uint16_t x, uint16_t y) {
    SystemStatus_t* status = AdvancedPressureControl_GetStatus();
    
    printf("Touch PID: x=%d, y=%d\r\n", x, y);
    
    /* Sağ tərəf - +/- düymələri (x > 180) */
    if (x > 180) {
        uint8_t is_minus = (x < 250);  /* Sol tərəf = minus, sağ tərəf = plus */
        
        /* Kp (y: 35-75) */
        if (y >= 35 && y < 80) {
            float delta = is_minus ? -0.05f : 0.05f;
            float new_kp = g_pid_zme.Kp + delta;
            if (new_kp < 0.0f) new_kp = 0.0f;
            if (new_kp > 10.0f) new_kp = 10.0f;
            printf(">>> Kp: %.2f\r\n", new_kp);
            AdvancedPressureControl_SetPIDParams(&g_pid_zme, new_kp, g_pid_zme.Ki, g_pid_zme.Kd);
            AdvancedPressureControl_SetPIDParams(&g_pid_drv, new_kp, g_pid_drv.Ki, g_pid_drv.Kd);
            g_needs_redraw = 1;
            return;
        }
        
        /* Ki (y: 75-115) */
        if (y >= 75 && y < 120) {
            float delta = is_minus ? -0.005f : 0.005f;
            float new_ki = g_pid_zme.Ki + delta;
            if (new_ki < 0.0f) new_ki = 0.0f;
            if (new_ki > 1.0f) new_ki = 1.0f;
            printf(">>> Ki: %.4f\r\n", new_ki);
            AdvancedPressureControl_SetPIDParams(&g_pid_zme, g_pid_zme.Kp, new_ki, g_pid_zme.Kd);
            AdvancedPressureControl_SetPIDParams(&g_pid_drv, g_pid_drv.Kp, new_ki, g_pid_drv.Kd);
            g_needs_redraw = 1;
            return;
        }
        
        /* Kd (y: 115-155) */
        if (y >= 115 && y < 160) {
            float delta = is_minus ? -0.01f : 0.01f;
            float new_kd = g_pid_zme.Kd + delta;
            if (new_kd < 0.0f) new_kd = 0.0f;
            if (new_kd > 5.0f) new_kd = 5.0f;
            printf(">>> Kd: %.3f\r\n", new_kd);
            AdvancedPressureControl_SetPIDParams(&g_pid_zme, g_pid_zme.Kp, g_pid_zme.Ki, new_kd);
            AdvancedPressureControl_SetPIDParams(&g_pid_drv, g_pid_drv.Kp, g_pid_drv.Ki, new_kd);
            g_needs_redraw = 1;
            return;
        }
        
        /* SP (y: 155-195) */
        if (y >= 155 && y < 200) {
            float delta = is_minus ? -10.0f : 10.0f;
            float new_sp = status->target_pressure + delta;
            if (new_sp < 0.0f) new_sp = 0.0f;
            if (new_sp > 300.0f) new_sp = 300.0f;
            printf(">>> SP: %.0f\r\n", new_sp);
            AdvancedPressureControl_SetTargetPressure(new_sp);
            g_needs_redraw = 1;
            return;
        }
    }
    
    /* Alt sıra - BACK/SAVE (y >= 195) */
    if (y >= 195) {
        if (x < 150) {
            /* BACK */
            printf(">>> BACK\r\n");
            g_current_page = PAGE_MENU;
            g_needs_redraw = 1;
        } else {
            /* SAVE */
            printf(">>> SAVE\r\n");
            AdvancedPressureControl_SavePIDParamsToFlash();
            ILI9341_DrawString(130, 200, "SAVED!", COLOR_ACCENT_GREEN, COLOR_BG_DARK, 2);
            HAL_Delay(500);
            g_needs_redraw = 1;
        }
        return;
    }
}

/**
 * @brief Kalibrasiya touch - SADƏLƏŞDİRİLMİŞ
 */
void Touch_HandleCalibration(uint16_t x, uint16_t y) {
    SystemStatus_t* status = AdvancedPressureControl_GetStatus();
    
    printf("Touch Cal: x=%d, y=%d\r\n", x, y);
    
    /* CAL MIN/MAX sırası (y: 55-105) */
    if (y >= 55 && y < 110) {
        if (x < 160) {
            /* CAL MIN */
            printf(">>> CAL MIN: ADC=%d\r\n", status->raw_adc_value);
            g_calibration.adc_min = (float)status->raw_adc_value;
            g_calibration.pressure_min = 0.0f;
            ILI9341_DrawString(50, 110, "MIN SET!", COLOR_ACCENT_GREEN, COLOR_BG_DARK, 1);
            HAL_Delay(500);
            g_needs_redraw = 1;
        } else {
            /* CAL MAX */
            printf(">>> CAL MAX: ADC=%d\r\n", status->raw_adc_value);
            g_calibration.adc_max = (float)status->raw_adc_value;
            g_calibration.pressure_max = 300.0f;
            /* Slope və offset hesabla */
            if (g_calibration.adc_max > g_calibration.adc_min) {
                g_calibration.slope = (g_calibration.pressure_max - g_calibration.pressure_min) / 
                                      (g_calibration.adc_max - g_calibration.adc_min);
                g_calibration.offset = g_calibration.pressure_min - 
                                      (g_calibration.slope * g_calibration.adc_min);
            }
            ILI9341_DrawString(200, 110, "MAX SET!", COLOR_ACCENT_GREEN, COLOR_BG_DARK, 1);
            HAL_Delay(500);
            g_needs_redraw = 1;
        }
        return;
    }
    
    /* BACK/SAVE sırası (y >= 175) */
    if (y >= 175) {
        if (x < 160) {
            /* BACK */
            printf(">>> BACK\r\n");
            g_current_page = PAGE_MENU;
            g_needs_redraw = 1;
        } else {
            /* SAVE */
            printf(">>> SAVE\r\n");
            AdvancedPressureControl_SaveCalibration();
            ILI9341_DrawString(130, 160, "SAVED!", COLOR_ACCENT_GREEN, COLOR_BG_DARK, 2);
            HAL_Delay(500);
            g_needs_redraw = 1;
        }
        return;
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_FSMC_Init();
  MX_USB_OTG_FS_HCD_Init();
  MX_ADC3_Init();
  
  /* ADC başlat */
  HAL_ADC_Start(&hadc3);
  
  MX_TIM3_Init();
  MX_TIM6_Init();
  MX_SPI1_Init();
  
  /* USER CODE BEGIN 2 */
  
  /* LCD başlat */
  HAL_Delay(50);
  ILI9341_Init();
  HAL_Delay(100);
  
  /* Touch başlat - LCD-dən sonra, kalibrasiyadan əvvəl */
  XPT2046_Init();
  HAL_Delay(50);
  
  /* ============================================
   * 3-NÖQTƏLİ TOUCH KALİBRASİYA
   * Ekran açılmadan əvvəl touch koordinatlarını kalibrasiya et
   * ============================================ */
  ILI9341_RunTouchCalibration();
  
  /* PWM başlat */
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
  
  /* Açılış ekranı - kalibrasiyadan sonra */
  Screen_DrawSplash();
  
  /* Kalibrasiya yüklə */
  AdvancedPressureControl_LoadCalibration();
  
  /* PID sistemi başlat */
  AdvancedPressureControl_Init();
  
  /* İlkin setpoint */
  AdvancedPressureControl_SetTargetPressure(g_presets[g_current_preset]);
  
  /* KRİTİK: Əsas ekranı ÇƏK - Timer başlamadan ƏVVƏL */
  g_current_page = PAGE_MAIN;
  g_needs_redraw = 1;
  
  /* Sistemi başlat - Timer-dən ƏVVƏL */
  g_system_running = 1;
  g_system_status.control_enabled = true;
  
  /* İlk ekran yeniləməsini et - Timer-dən ƏVVƏL */
  Screen_DrawMain();
  
  /* Timer 6 başlat (PID loop) - Ekran çəkildikdən SONRA */
  HAL_TIM_Base_Start_IT(&htim6);
  
  printf("=== VALEH High Pressure Control v4.0 ===\r\n");
  printf("Touch Screen Interface Ready\r\n");
  
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    
    /* Touch işlə */
    Touch_Process();
    
    /* Ekran yenilə */
    Screen_Update();
    
    /* Kiçik gecikmə */
    HAL_Delay(10);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_RCC_EnableCSS();
}

/**
  * @brief ADC3 Initialization Function
  */
static void MX_ADC3_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  hadc3.Instance = ADC3;
  hadc3.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc3.Init.Resolution = ADC_RESOLUTION_12B;
  hadc3.Init.ScanConvMode = DISABLE;
  hadc3.Init.ContinuousConvMode = ENABLE;
  hadc3.Init.DiscontinuousConvMode = DISABLE;
  hadc3.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc3.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc3.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc3.Init.NbrOfConversion = 1;
  hadc3.Init.DMAContinuousRequests = DISABLE;
  hadc3.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc3) != HAL_OK)
  {
    Error_Handler();
  }

  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  */
static void MX_SPI1_Init(void)
{
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM3 Initialization Function
  */
static void MX_TIM3_Init(void)
{
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_TIM_MspPostInit(&htim3);
}

/**
  * @brief TIM6 Initialization Function
  */
static void MX_TIM6_Init(void)
{
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 8399;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 99;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
}

/**
  * @brief USB_OTG_FS Initialization Function
  */
static void MX_USB_OTG_FS_HCD_Init(void)
{
  hhcd_USB_OTG_FS.Instance = USB_OTG_FS;
  hhcd_USB_OTG_FS.Init.Host_channels = 8;
  hhcd_USB_OTG_FS.Init.speed = HCD_SPEED_FULL;
  hhcd_USB_OTG_FS.Init.dma_enable = DISABLE;
  hhcd_USB_OTG_FS.Init.phy_itface = HCD_PHY_EMBEDDED;
  hhcd_USB_OTG_FS.Init.Sof_enable = DISABLE;
  if (HAL_HCD_Init(&hhcd_USB_OTG_FS) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();

  HAL_GPIO_WritePin(Lcd_RST_GPIO_Port, Lcd_RST_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(Lcd_LIG_GPIO_Port, Lcd_LIG_Pin, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = Lcd_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Lcd_RST_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = Lcd_LIG_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Lcd_LIG_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : XPT2046 Touch - Software SPI (bit-bang) */
  /* İlkin səviyyələr: CS=HIGH (deaktiv), SCK=LOW, MOSI=LOW */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);   /* TCS = HIGH */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);  /* SCK = LOW */
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_9, GPIO_PIN_RESET);  /* MOSI = LOW */

  /* PB1: SCK - çıxış */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* PB12: TCS (Chip Select) - çıxış */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* PF8: MISO - giriş, pull-up ilə */
  GPIO_InitStruct.Pin = GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /* PF9: MOSI - çıxış */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /* PF10: TIRQ - giriş, pull-up ilə (open-drain output) */
  GPIO_InitStruct.Pin = GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
}

/* FSMC initialization function */
static void MX_FSMC_Init(void)
{
  __HAL_RCC_FSMC_CLK_ENABLE();

  FSMC_NORSRAM_TimingTypeDef Timing = {0};

  hsram1.Instance = FSMC_NORSRAM_DEVICE;
  hsram1.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;
  hsram1.Init.NSBank = FSMC_NORSRAM_BANK4;
  hsram1.Init.DataAddressMux = FSMC_DATA_ADDRESS_MUX_DISABLE;
  hsram1.Init.MemoryType = FSMC_MEMORY_TYPE_SRAM;
  hsram1.Init.MemoryDataWidth = FSMC_NORSRAM_MEM_BUS_WIDTH_16;
  hsram1.Init.BurstAccessMode = FSMC_BURST_ACCESS_MODE_DISABLE;
  hsram1.Init.WaitSignalPolarity = FSMC_WAIT_SIGNAL_POLARITY_LOW;
  hsram1.Init.WrapMode = FSMC_WRAP_MODE_DISABLE;
  hsram1.Init.WaitSignalActive = FSMC_WAIT_TIMING_BEFORE_WS;
  hsram1.Init.WriteOperation = FSMC_WRITE_OPERATION_ENABLE;
  hsram1.Init.WaitSignal = FSMC_WAIT_SIGNAL_DISABLE;
  hsram1.Init.ExtendedMode = FSMC_EXTENDED_MODE_DISABLE;
  hsram1.Init.AsynchronousWait = FSMC_ASYNCHRONOUS_WAIT_DISABLE;
  hsram1.Init.WriteBurst = FSMC_WRITE_BURST_DISABLE;
  hsram1.Init.PageSize = FSMC_PAGE_SIZE_NONE;

  Timing.AddressSetupTime = 15;
  Timing.AddressHoldTime = 15;
  Timing.DataSetupTime = 5;
  Timing.BusTurnAroundDuration = 1;
  Timing.CLKDivision = 16;
  Timing.DataLatency = 17;
  Timing.AccessMode = FSMC_ACCESS_MODE_A;

  if (HAL_SRAM_Init(&hsram1, &Timing, NULL) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/**
 * @brief Timer Period Elapsed Callback - PID Loop
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM6) {
        if (g_system_running) {
            AdvancedPressureControl_Step();
        }
    }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif
