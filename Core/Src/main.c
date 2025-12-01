/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body - SENSOR-SIZ EKRAN VERSİYASI
  ******************************************************************************
  * @attention
  *
  * VALEH High Pressure Control System v4.0
  * Touch sensor olmadan tam funksional ekran sistemi
  * PWM, PID və avtomatik rejim dəstəyi
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

/* =============== SİSTEM REJİMLƏRİ =============== */
typedef enum {
    MODE_IDLE,          // Boş - sistem gözləyir
    MODE_AUTO,          // Avtomatik - PID nəzarəti aktiv
    MODE_MANUAL,        // Manual - əl ilə idarəetmə
    MODE_SIMULATION     // Simulyasiya - sensor olmadan test
} OperationMode;

typedef enum {
    SCREEN_SPLASH,      // Açılış ekranı
    SCREEN_MAIN,        // Əsas ekran
    SCREEN_STATUS,      // Status ekranı
    SCREEN_SETTINGS,    // Ayarlar
    SCREEN_PID,         // PID parametrləri
    SCREEN_CALIBRATION  // Kalibrasiya
} ScreenType;

/* =============== ƏVVƏLCƏDƏN TƏYİN EDİLMİŞ SETPOINT-LƏR =============== */
typedef struct {
    float pressure;     // Təzyiq (bar)
    const char* name;   // Ad
    uint16_t color;     // Rəng
} PresetPoint_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define NUM_PRESETS 6
#define SCREEN_UPDATE_INTERVAL_MS 100
#define AUTO_CYCLE_INTERVAL_MS 5000
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
OperationMode g_operation_mode = MODE_IDLE;
ScreenType g_current_screen = SCREEN_SPLASH;

// Əvvəlcədən təyin edilmiş setpoint-lər
const PresetPoint_t g_presets[NUM_PRESETS] = {
    {50.0f,   "LOW",      ILI9341_COLOR_GREEN},
    {100.0f,  "MEDIUM",   ILI9341_COLOR_CYAN},
    {150.0f,  "HIGH",     ILI9341_COLOR_YELLOW},
    {200.0f,  "V.HIGH",   ILI9341_COLOR_ORANGE},
    {250.0f,  "EXTREME",  ILI9341_COLOR_RED},
    {300.0f,  "MAX",      ILI9341_COLOR_MAGENTA}
};

uint8_t g_current_preset = 0;
uint8_t g_auto_cycle_enabled = 0;
uint32_t g_last_cycle_time = 0;

// Ekran yeniləmə
uint32_t g_last_screen_update = 0;
uint8_t g_screen_needs_redraw = 1;

// Simulyasiya dəyişənləri
float g_sim_pressure = 0.0f;
float g_sim_target = 100.0f;
uint8_t g_sim_mode = 0;

// Touch dəyişənləri
uint8_t g_touch_detected = 0;
uint16_t g_touch_x = 0;
uint16_t g_touch_y = 0;

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
void Screen_ShowSplash(void);
void Screen_ShowMain(void);
void Screen_ShowStatus(void);
void Screen_Update(void);

/* Sistem funksiyaları */
void System_Init(void);
void System_ProcessInput(void);
void System_AutoCycle(void);
void System_UpdateSimulation(void);

/* Qrafik funksiyaları */
void Draw_PressureBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, 
                      float current, float target, float max_val);
void Draw_CircleGauge(uint16_t cx, uint16_t cy, uint16_t radius, 
                      float value, float max_val, uint16_t color);
void Draw_StatusPanel(uint16_t x, uint16_t y);
void Draw_ControlPanel(uint16_t x, uint16_t y);
void Draw_PresetButtons(uint16_t x, uint16_t y);
void Draw_ModeIndicator(uint16_t x, uint16_t y);
void Draw_PIDInfo(uint16_t x, uint16_t y);
void Draw_ValueBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h, 
                   const char* label, float value, const char* unit, uint16_t color);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* =============== QRAFİK FUNKSİYALARI =============== */

/**
 * @brief Təzyiq bar-ı çək
 */
void Draw_PressureBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, 
                      float current, float target, float max_val) {
    // Çərçivə
    ILI9341_DrawRect(x, y, width, height, ILI9341_COLOR_WHITE);
    ILI9341_DrawRect(x+1, y+1, width-2, height-2, ILI9341_COLOR_DARKGREY);
    
    // Hazırki dəyər bar-ı
    uint16_t bar_width = (uint16_t)((current / max_val) * (width - 4));
    if (bar_width > width - 4) bar_width = width - 4;
    
    // Rəng seçimi
    uint16_t bar_color;
    if (current < target * 0.8f) {
        bar_color = ILI9341_COLOR_CYAN;
    } else if (current < target * 1.1f) {
        bar_color = ILI9341_COLOR_GREEN;
    } else if (current < target * 1.2f) {
        bar_color = ILI9341_COLOR_YELLOW;
    } else {
        bar_color = ILI9341_COLOR_RED;
    }
    
    ILI9341_FillRect(x + 2, y + 2, bar_width, height - 4, bar_color);
    
    // Arxa plan
    if (bar_width < width - 4) {
        ILI9341_FillRect(x + 2 + bar_width, y + 2, width - 4 - bar_width, height - 4, 
                        ILI9341_COLOR_BLACK);
    }
    
    // Hədəf marker
    uint16_t target_x = x + 2 + (uint16_t)((target / max_val) * (width - 4));
    if (target_x < x + width - 2) {
        for (uint16_t i = 0; i < height - 4; i += 3) {
            ILI9341_DrawPixel(target_x, y + 2 + i, ILI9341_COLOR_WHITE);
        }
    }
}

/**
 * @brief Dairəvi gauge çək
 */
void Draw_CircleGauge(uint16_t cx, uint16_t cy, uint16_t radius, 
                      float value, float max_val, uint16_t color) {
    // Xarici dairə
    for (int angle = 0; angle < 360; angle++) {
        float rad = angle * 3.14159f / 180.0f;
        int x = cx + (int)(radius * cosf(rad));
        int y = cy + (int)(radius * sinf(rad));
        if (x >= 0 && x < 320 && y >= 0 && y < 240) {
            ILI9341_DrawPixel(x, y, ILI9341_COLOR_DARKGREY);
        }
    }
    
    // Dəyər qövsü (270° -dən başlayır, saat əqrəbi istiqamətində)
    float percent = value / max_val;
    if (percent > 1.0f) percent = 1.0f;
    int end_angle = (int)(percent * 270.0f);
    
    for (int angle = 0; angle < end_angle; angle++) {
        float rad = (angle - 135) * 3.14159f / 180.0f;
        for (int r = radius - 8; r < radius - 2; r++) {
            int x = cx + (int)(r * cosf(rad));
            int y = cy + (int)(r * sinf(rad));
            if (x >= 0 && x < 320 && y >= 0 && y < 240) {
                ILI9341_DrawPixel(x, y, color);
            }
        }
    }
    
    // Mərkəz dairəsi
    for (int r = 0; r < 5; r++) {
        for (int angle = 0; angle < 360; angle += 10) {
            float rad = angle * 3.14159f / 180.0f;
            int x = cx + (int)(r * cosf(rad));
            int y = cy + (int)(r * sinf(rad));
            if (x >= 0 && x < 320 && y >= 0 && y < 240) {
                ILI9341_DrawPixel(x, y, ILI9341_COLOR_WHITE);
            }
        }
    }
}

/**
 * @brief Dəyər qutusu çək
 */
void Draw_ValueBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h, 
                   const char* label, float value, const char* unit, uint16_t color) {
    // Çərçivə
    ILI9341_DrawRect(x, y, w, h, color);
    
    // Arxa plan
    ILI9341_FillRect(x + 1, y + 1, w - 2, h - 2, ILI9341_COLOR_BLACK);
    
    // Başlıq
    uint16_t label_x = x + 3;
    ILI9341_DrawString(label_x, y + 3, label, color, ILI9341_COLOR_BLACK, 1);
    
    // Dəyər
    char val_str[16];
    if (value < 10.0f) {
        sprintf(val_str, "%.2f", value);
    } else if (value < 100.0f) {
        sprintf(val_str, "%.1f", value);
    } else {
        sprintf(val_str, "%.0f", value);
    }
    ILI9341_DrawString(x + 3, y + 15, val_str, ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 2);
    
    // Vahid
    ILI9341_DrawString(x + w - 30, y + h - 12, unit, color, ILI9341_COLOR_BLACK, 1);
}

/**
 * @brief Status paneli çək
 */
void Draw_StatusPanel(uint16_t x, uint16_t y) {
    SystemStatus_t* status = AdvancedPressureControl_GetStatus();
    
    // Başlıq
    ILI9341_DrawString(x, y, "SİSTEM STATUS", ILI9341_COLOR_CYAN, ILI9341_COLOR_BLACK, 1);
    
    // Xətlər
    y += 15;
    
    // Rejim
    const char* mode_str;
    uint16_t mode_color;
    switch (g_operation_mode) {
        case MODE_AUTO:
            mode_str = "AUTO";
            mode_color = ILI9341_COLOR_GREEN;
            break;
        case MODE_MANUAL:
            mode_str = "MANUAL";
            mode_color = ILI9341_COLOR_YELLOW;
            break;
        case MODE_SIMULATION:
            mode_str = "SIM";
            mode_color = ILI9341_COLOR_MAGENTA;
            break;
        default:
            mode_str = "IDLE";
            mode_color = ILI9341_COLOR_GREY;
    }
    
    char line[32];
    sprintf(line, "Mode: %s", mode_str);
    ILI9341_DrawString(x, y, line, mode_color, ILI9341_COLOR_BLACK, 1);
    
    y += 12;
    
    // PID Status
    if (status->control_enabled) {
        ILI9341_DrawString(x, y, "PID: ON ", ILI9341_COLOR_GREEN, ILI9341_COLOR_BLACK, 1);
    } else {
        ILI9341_DrawString(x, y, "PID: OFF", ILI9341_COLOR_RED, ILI9341_COLOR_BLACK, 1);
    }
    
    y += 12;
    
    // Safety
    if (status->safety_triggered) {
        ILI9341_DrawString(x, y, "SAFETY: TRIGGERED!", ILI9341_COLOR_RED, ILI9341_COLOR_BLACK, 1);
    } else {
        ILI9341_DrawString(x, y, "SAFETY: OK", ILI9341_COLOR_GREEN, ILI9341_COLOR_BLACK, 1);
    }
}

/**
 * @brief PWM dəyərlərini göstər
 */
void Draw_PWMInfo(uint16_t x, uint16_t y) {
    SystemStatus_t* status = AdvancedPressureControl_GetStatus();
    
    char line[32];
    
    // Motor
    sprintf(line, "MTR:%4.1f%%", status->motor_pwm_percent);
    ILI9341_DrawString(x, y, line, ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 1);
    
    // ZME
    sprintf(line, "ZME:%4.1f%%", status->zme_pwm_percent);
    ILI9341_DrawString(x, y + 12, line, ILI9341_COLOR_CYAN, ILI9341_COLOR_BLACK, 1);
    
    // DRV
    sprintf(line, "DRV:%4.1f%%", status->drv_pwm_percent);
    ILI9341_DrawString(x, y + 24, line, ILI9341_COLOR_GREEN, ILI9341_COLOR_BLACK, 1);
}

/**
 * @brief PID məlumatlarını göstər
 */
void Draw_PIDInfo(uint16_t x, uint16_t y) {
    SystemStatus_t* status = AdvancedPressureControl_GetStatus();
    
    char line[32];
    
    // Error
    sprintf(line, "ERR:%+6.1f", status->error);
    uint16_t err_color = (fabsf(status->error) < 2.0f) ? ILI9341_COLOR_GREEN : ILI9341_COLOR_YELLOW;
    ILI9341_DrawString(x, y, line, err_color, ILI9341_COLOR_BLACK, 1);
    
    // PID Output
    sprintf(line, "PID:%+6.1f", status->pid_output);
    ILI9341_DrawString(x, y + 12, line, ILI9341_COLOR_CYAN, ILI9341_COLOR_BLACK, 1);
    
    // Kp, Ki, Kd
    sprintf(line, "Kp:%.2f", g_pid_zme.Kp);
    ILI9341_DrawString(x, y + 24, line, ILI9341_COLOR_GREY, ILI9341_COLOR_BLACK, 1);
}

/**
 * @brief Preset düymələrini çək
 */
void Draw_PresetButtons(uint16_t x, uint16_t y) {
    for (int i = 0; i < NUM_PRESETS; i++) {
        uint16_t btn_x = x + (i % 3) * 55;
        uint16_t btn_y = y + (i / 3) * 30;
        
        uint16_t bg_color = (i == g_current_preset) ? g_presets[i].color : ILI9341_COLOR_BLACK;
        uint16_t fg_color = (i == g_current_preset) ? ILI9341_COLOR_BLACK : g_presets[i].color;
        
        // Düymə çərçivəsi
        ILI9341_DrawRect(btn_x, btn_y, 52, 27, g_presets[i].color);
        ILI9341_FillRect(btn_x + 1, btn_y + 1, 50, 25, bg_color);
        
        // Mətn
        char val_str[8];
        sprintf(val_str, "%.0f", g_presets[i].pressure);
        ILI9341_DrawString(btn_x + 8, btn_y + 4, val_str, fg_color, bg_color, 1);
        ILI9341_DrawString(btn_x + 3, btn_y + 15, g_presets[i].name, fg_color, bg_color, 1);
    }
}

/**
 * @brief Mode indikatoru çək
 */
void Draw_ModeIndicator(uint16_t x, uint16_t y) {
    const char* mode_names[] = {"IDLE", "AUTO", "MANUAL", "SIM"};
    uint16_t mode_colors[] = {ILI9341_COLOR_GREY, ILI9341_COLOR_GREEN, 
                              ILI9341_COLOR_YELLOW, ILI9341_COLOR_MAGENTA};
    
    for (int i = 0; i < 4; i++) {
        uint16_t btn_x = x + i * 40;
        uint16_t bg_color = (i == (int)g_operation_mode) ? mode_colors[i] : ILI9341_COLOR_BLACK;
        uint16_t fg_color = (i == (int)g_operation_mode) ? ILI9341_COLOR_BLACK : mode_colors[i];
        
        ILI9341_DrawRect(btn_x, y, 38, 18, mode_colors[i]);
        ILI9341_FillRect(btn_x + 1, y + 1, 36, 16, bg_color);
        ILI9341_DrawString(btn_x + 3, y + 4, mode_names[i], fg_color, bg_color, 1);
    }
}

/* =============== EKRAN FUNKSİYALARI =============== */

/**
 * @brief Açılış ekranını göstər
 */
void Screen_ShowSplash(void) {
    ILI9341_FillScreen(ILI9341_COLOR_BLACK);
    
    // Logo çərçivəsi
    ILI9341_DrawRect(20, 30, 280, 80, ILI9341_COLOR_CYAN);
    ILI9341_DrawRect(22, 32, 276, 76, ILI9341_COLOR_WHITE);
    
    // Başlıq
    ILI9341_DrawString(60, 50, "VALEH", ILI9341_COLOR_CYAN, ILI9341_COLOR_BLACK, 4);
    ILI9341_DrawString(55, 90, "HIGH PRESSURE CTRL", ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 1);
    
    // Versiya
    ILI9341_DrawString(130, 130, "v4.0", ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 2);
    
    // Yükləmə bar-ı
    ILI9341_DrawRect(40, 170, 240, 20, ILI9341_COLOR_DARKGREY);
    for (int i = 0; i < 10; i++) {
        ILI9341_FillRect(42, 172, i * 24, 16, ILI9341_COLOR_GREEN);
        HAL_Delay(100);
    }
    
    // Mesaj
    ILI9341_DrawString(70, 200, "Sistem hazirdir...", ILI9341_COLOR_GREEN, ILI9341_COLOR_BLACK, 1);
    HAL_Delay(500);
}

/**
 * @brief Əsas ekranı göstər
 */
void Screen_ShowMain(void) {
    if (g_screen_needs_redraw) {
        ILI9341_FillScreen(ILI9341_COLOR_BLACK);
        
        // Başlıq
        ILI9341_DrawString(10, 5, "VALEH HIGH PRESSURE CONTROL", 
                          ILI9341_COLOR_CYAN, ILI9341_COLOR_BLACK, 1);
        
        // Separator xətt
        ILI9341_DrawLine(0, 18, 319, 18, ILI9341_COLOR_DARKGREY);
        
        // Preset düymələri
        Draw_PresetButtons(5, 22);
        
        // Mode indikatoru
        Draw_ModeIndicator(160, 22);
        
        g_screen_needs_redraw = 0;
    }
    
    // Dinamik məlumatları yenilə
    SystemStatus_t* status = AdvancedPressureControl_GetStatus();
    float current_p = g_sim_mode ? g_sim_pressure : status->current_pressure;
    float target_p = status->target_pressure;
    
    // Əsas təzyiq göstəricisi
    char pressure_str[16];
    sprintf(pressure_str, "%6.1f", current_p);
    
    // Köhnə dəyəri sil
    ILI9341_FillRect(5, 90, 150, 50, ILI9341_COLOR_BLACK);
    
    // Yeni dəyəri göstər
    ILI9341_DrawString(5, 90, pressure_str, ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 4);
    ILI9341_DrawString(125, 110, "BAR", ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 2);
    
    // Hədəf
    sprintf(pressure_str, "SP:%5.0f", target_p);
    ILI9341_FillRect(5, 145, 100, 15, ILI9341_COLOR_BLACK);
    ILI9341_DrawString(5, 145, pressure_str, ILI9341_COLOR_CYAN, ILI9341_COLOR_BLACK, 1);
    
    // Təzyiq bar-ı
    Draw_PressureBar(5, 165, 155, 20, current_p, target_p, 300.0f);
    
    // Sağ panel - PWM və PID
    Draw_PWMInfo(170, 90);
    Draw_PIDInfo(170, 135);
    
    // Status
    Draw_StatusPanel(5, 190);
    
    // Dairəvi gauge
    uint16_t gauge_color;
    if (current_p < target_p * 0.8f) gauge_color = ILI9341_COLOR_CYAN;
    else if (current_p < target_p * 1.1f) gauge_color = ILI9341_COLOR_GREEN;
    else if (current_p < target_p * 1.2f) gauge_color = ILI9341_COLOR_YELLOW;
    else gauge_color = ILI9341_COLOR_RED;
    
    Draw_CircleGauge(270, 190, 40, current_p, 300.0f, gauge_color);
}

/**
 * @brief Status ekranını göstər
 */
void Screen_ShowStatus(void) {
    if (g_screen_needs_redraw) {
        ILI9341_FillScreen(ILI9341_COLOR_BLACK);
        ILI9341_DrawString(100, 10, "SİSTEM STATUS", ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 2);
        g_screen_needs_redraw = 0;
    }
    
    SystemStatus_t* status = AdvancedPressureControl_GetStatus();
    char line[48];
    uint16_t y = 40;
    
    // Clear dinamik sahələr
    ILI9341_FillRect(10, 40, 300, 180, ILI9341_COLOR_BLACK);
    
    sprintf(line, "Pressure:   %7.2f bar", status->current_pressure);
    ILI9341_DrawString(10, y, line, ILI9341_COLOR_YELLOW, ILI9341_COLOR_BLACK, 1);
    y += 15;
    
    sprintf(line, "Target:     %7.2f bar", status->target_pressure);
    ILI9341_DrawString(10, y, line, ILI9341_COLOR_CYAN, ILI9341_COLOR_BLACK, 1);
    y += 15;
    
    sprintf(line, "Error:      %+7.2f bar", status->error);
    ILI9341_DrawString(10, y, line, ILI9341_COLOR_WHITE, ILI9341_COLOR_BLACK, 1);
    y += 20;
    
    sprintf(line, "Motor PWM:  %7.2f %%", status->motor_pwm_percent);
    ILI9341_DrawString(10, y, line, ILI9341_COLOR_GREEN, ILI9341_COLOR_BLACK, 1);
    y += 15;
    
    sprintf(line, "ZME PWM:    %7.2f %%", status->zme_pwm_percent);
    ILI9341_DrawString(10, y, line, ILI9341_COLOR_GREEN, ILI9341_COLOR_BLACK, 1);
    y += 15;
    
    sprintf(line, "DRV PWM:    %7.2f %%", status->drv_pwm_percent);
    ILI9341_DrawString(10, y, line, ILI9341_COLOR_GREEN, ILI9341_COLOR_BLACK, 1);
    y += 20;
    
    sprintf(line, "PID Kp:     %7.3f", g_pid_zme.Kp);
    ILI9341_DrawString(10, y, line, ILI9341_COLOR_MAGENTA, ILI9341_COLOR_BLACK, 1);
    y += 15;
    
    sprintf(line, "PID Ki:     %7.4f", g_pid_zme.Ki);
    ILI9341_DrawString(10, y, line, ILI9341_COLOR_MAGENTA, ILI9341_COLOR_BLACK, 1);
    y += 15;
    
    sprintf(line, "PID Kd:     %7.3f", g_pid_zme.Kd);
    ILI9341_DrawString(10, y, line, ILI9341_COLOR_MAGENTA, ILI9341_COLOR_BLACK, 1);
    y += 20;
    
    sprintf(line, "Raw ADC:    %7d", status->raw_adc_value);
    ILI9341_DrawString(10, y, line, ILI9341_COLOR_GREY, ILI9341_COLOR_BLACK, 1);
}

/**
 * @brief Ekranı yenilə
 */
void Screen_Update(void) {
    uint32_t now = HAL_GetTick();
    
    if (now - g_last_screen_update < SCREEN_UPDATE_INTERVAL_MS) {
        return;
    }
    g_last_screen_update = now;
    
    switch (g_current_screen) {
        case SCREEN_MAIN:
            Screen_ShowMain();
            break;
        case SCREEN_STATUS:
            Screen_ShowStatus();
            break;
        default:
            Screen_ShowMain();
            break;
    }
}

/* =============== SİSTEM FUNKSİYALARI =============== */

/**
 * @brief Sistemi başlat
 */
void System_Init(void) {
    // Kalibrasiya yüklə
    AdvancedPressureControl_LoadCalibration();
    
    // Advanced sistemi başlat
    AdvancedPressureControl_Init();
    
    // İlkin setpoint təyin et
    AdvancedPressureControl_SetTargetPressure(g_presets[g_current_preset].pressure);
    
    // Timer 6 başlat (PID loop üçün)
    HAL_TIM_Base_Start_IT(&htim6);
    
    printf("System initialized. Touch sensor mode: %s\r\n", 
           g_sim_mode ? "SIMULATION" : "NORMAL");
}

/**
 * @brief Touch və ya digər inputları işlə
 */
void System_ProcessInput(void) {
    static uint32_t last_input_time = 0;
    static uint8_t input_debounce = 0;
    
    uint32_t now = HAL_GetTick();
    
    // Touch yoxla
    if (XPT2046_IsTouched()) {
        uint16_t raw_x, raw_y, screen_x, screen_y;
        
        if (XPT2046_GetCoordinates(&raw_x, &raw_y)) {
            XPT2046_ConvertToScreen(raw_x, raw_y, &screen_x, &screen_y);
            
            if (!input_debounce && (now - last_input_time > 200)) {
                g_touch_detected = 1;
                g_touch_x = screen_x;
                g_touch_y = screen_y;
                last_input_time = now;
                input_debounce = 1;
                
                // Touch işlə
                // Preset düymələri (5-165, 22-82)
                if (screen_y >= 22 && screen_y <= 82) {
                    for (int i = 0; i < NUM_PRESETS; i++) {
                        uint16_t btn_x = 5 + (i % 3) * 55;
                        uint16_t btn_y = 22 + (i / 3) * 30;
                        
                        if (screen_x >= btn_x && screen_x <= btn_x + 52 &&
                            screen_y >= btn_y && screen_y <= btn_y + 27) {
                            g_current_preset = i;
                            AdvancedPressureControl_SetTargetPressure(g_presets[i].pressure);
                            g_screen_needs_redraw = 1;
                            break;
                        }
                    }
                }
                
                // Mode düymələri (160-320, 22-40)
                if (screen_y >= 22 && screen_y <= 40 && screen_x >= 160) {
                    int mode_idx = (screen_x - 160) / 40;
                    if (mode_idx >= 0 && mode_idx <= 3) {
                        g_operation_mode = (OperationMode)mode_idx;
                        
                        if (g_operation_mode == MODE_SIMULATION) {
                            g_sim_mode = 1;
                        } else {
                            g_sim_mode = 0;
                        }
                        
                        if (g_operation_mode == MODE_AUTO) {
                            g_system_status.control_enabled = true;
                        } else if (g_operation_mode == MODE_IDLE) {
                            g_system_status.control_enabled = false;
                        }
                        
                        g_screen_needs_redraw = 1;
                    }
                }
            }
        }
    } else {
        input_debounce = 0;
        g_touch_detected = 0;
    }
    
    // Toxunuş sensoru işləmirsə, avtomatik olaraq simulyasiya rejiminə keç
    static uint32_t touch_check_time = 0;
    static uint8_t touch_fail_count = 0;
    
    if (now - touch_check_time > 2000) {
        touch_check_time = now;
        
        if (!g_touch_detected) {
            touch_fail_count++;
            
            if (touch_fail_count > 5 && g_operation_mode == MODE_IDLE) {
                // 10 saniyə touch yoxdursa, avtomatik rejimə keç
                g_operation_mode = MODE_AUTO;
                g_system_status.control_enabled = true;
                g_screen_needs_redraw = 1;
                printf("No touch detected. Auto-switching to AUTO mode.\r\n");
            }
        } else {
            touch_fail_count = 0;
        }
    }
}

/**
 * @brief Avtomatik preset dəyişmə (test üçün)
 */
void System_AutoCycle(void) {
    if (!g_auto_cycle_enabled) return;
    
    uint32_t now = HAL_GetTick();
    
    if (now - g_last_cycle_time > AUTO_CYCLE_INTERVAL_MS) {
        g_last_cycle_time = now;
        
        g_current_preset = (g_current_preset + 1) % NUM_PRESETS;
        AdvancedPressureControl_SetTargetPressure(g_presets[g_current_preset].pressure);
        g_screen_needs_redraw = 1;
        
        printf("Auto-cycle: Preset %d (%s, %.0f bar)\r\n", 
               g_current_preset, g_presets[g_current_preset].name, 
               g_presets[g_current_preset].pressure);
    }
}

/**
 * @brief Simulyasiya rejimini yenilə
 */
void System_UpdateSimulation(void) {
    if (!g_sim_mode) return;
    
    float target = g_system_status.target_pressure;
    float diff = target - g_sim_pressure;
    
    // Təzyiq dəyişmə sürəti
    float rate = 0.5f; // bar/100ms
    
    if (fabsf(diff) < rate) {
        g_sim_pressure = target;
    } else if (diff > 0) {
        g_sim_pressure += rate;
    } else {
        g_sim_pressure -= rate;
    }
    
    // Random kiçik dalgalanma əlavə et
    g_sim_pressure += ((float)(rand() % 100) - 50.0f) / 500.0f;
    
    if (g_sim_pressure < 0.0f) g_sim_pressure = 0.0f;
    if (g_sim_pressure > 310.0f) g_sim_pressure = 310.0f;
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

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
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
  
  /* KRİTİK: ADC-ni Continuous Mode-da başlat */
  HAL_ADC_Start(&hadc3);
  
  MX_TIM3_Init();
  MX_TIM6_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  
  /* LCD başlat */
  HAL_Delay(50);
  ILI9341_Init();
  HAL_Delay(100);
  
  /* Açılış ekranı */
  Screen_ShowSplash();
  
  /* PWM başlat */
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);  // Motor
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);  // DRV
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);  // ZME
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);  // Ehtiyat
  
  /* Touch başlat (sensor varsa) */
  XPT2046_Init();
  
  /* Sistem başlat */
  System_Init();
  
  /* Əsas ekrana keç */
  g_current_screen = SCREEN_MAIN;
  g_screen_needs_redraw = 1;
  
  /* Avtomatik rejimə keç (sensor olmadan işləmək üçün) */
  g_operation_mode = MODE_AUTO;
  g_system_status.control_enabled = true;
  
  printf("=== VALEH High Pressure Control System v4.0 ===\r\n");
  printf("Touch Sensor Mode: Auto-detection\r\n");
  printf("Initial Preset: %d (%s, %.0f bar)\r\n", 
         g_current_preset, g_presets[g_current_preset].name, 
         g_presets[g_current_preset].pressure);
  
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    
    /* 1. Input işlə */
    System_ProcessInput();
    
    /* 2. Auto-cycle (əgər aktivdirsə) */
    System_AutoCycle();
    
    /* 3. Simulyasiya yenilə (əgər aktivdirsə) */
    System_UpdateSimulation();
    
    /* 4. Ekran yenilə */
    Screen_Update();
    
    /* 5. Kiçik gecikmə */
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
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

  /** Initializes the CPU, AHB and APB buses clocks
  */
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

  /** Enables the Clock Security System
  */
  HAL_RCC_EnableCSS();
}

/**
  * @brief ADC3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC3_Init(void)
{

  /* USER CODE BEGIN ADC3_Init 0 */

  /* USER CODE END ADC3_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC3_Init 1 */

  /* USER CODE END ADC3_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
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

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC3_Init 2 */

  /* USER CODE END ADC3_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
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
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
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
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief TIM6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM6_Init(void)
{

  /* USER CODE BEGIN TIM6_Init 0 */

  /* USER CODE END TIM6_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM6_Init 1 */

  /* USER CODE END TIM6_Init 1 */
  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 8399;  // 84MHz / 8400 = 10kHz
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 99;       // 10kHz / 100 = 100Hz (10ms period)
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
  /* USER CODE BEGIN TIM6_Init 2 */
  HAL_NVIC_SetPriority(TIM6_DAC_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
  /* USER CODE END TIM6_Init 2 */

}

/**
  * @brief USB_OTG_FS Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_OTG_FS_HCD_Init(void)
{

  /* USER CODE BEGIN USB_OTG_FS_Init 0 */

  /* USER CODE END USB_OTG_FS_Init 0 */

  /* USER CODE BEGIN USB_OTG_FS_Init 1 */

  /* USER CODE END USB_OTG_FS_Init 1 */
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
  /* USER CODE BEGIN USB_OTG_FS_Init 2 */

  /* USER CODE END USB_OTG_FS_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(Lcd_RST_GPIO_Port, Lcd_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(Lcd_LIG_GPIO_Port, Lcd_LIG_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : Lcd_RST_Pin */
  GPIO_InitStruct.Pin = Lcd_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Lcd_RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Lcd_LIG_Pin */
  GPIO_InitStruct.Pin = Lcd_LIG_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Lcd_LIG_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : XPT2046 SPI pins */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1|GPIO_PIN_12, GPIO_PIN_SET);  /* SCK, TCS */
  HAL_GPIO_WritePin(GPIOF, GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10, GPIO_PIN_SET);  /* MISO, MOSI, TIRQ */

  /*Configure GPIO pins : XPT2046 SPI pins */
  GPIO_InitStruct.Pin = GPIO_PIN_1;  /* SCK */
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_12;  /* TCS */
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_8;  /* MISO */
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_9;  /* MOSI */
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_10;  /* TIRQ */
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* FSMC initialization function */
static void MX_FSMC_Init(void)
{

  /* USER CODE BEGIN FSMC_Init 0 */
  
  /* FSMC Clock Enable */
  __HAL_RCC_FSMC_CLK_ENABLE();

  /* USER CODE END FSMC_Init 0 */

  FSMC_NORSRAM_TimingTypeDef Timing = {0};

  /* USER CODE BEGIN FSMC_Init 1 */

  /* USER CODE END FSMC_Init 1 */

  /** Perform the SRAM1 memory initialization sequence
  */
  hsram1.Instance = FSMC_NORSRAM_DEVICE;
  hsram1.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;
  /* hsram1.Init */
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
  /* Timing */
  Timing.AddressSetupTime = 15;
  Timing.AddressHoldTime = 15;
  Timing.DataSetupTime = 5;
  Timing.BusTurnAroundDuration = 1;
  Timing.CLKDivision = 16;
  Timing.DataLatency = 17;
  Timing.AccessMode = FSMC_ACCESS_MODE_A;
  /* ExtTiming */

  if (HAL_SRAM_Init(&hsram1, &Timing, NULL) != HAL_OK)
  {
    Error_Handler( );
  }

  /* USER CODE BEGIN FSMC_Init 2 */

  /* USER CODE END FSMC_Init 2 */
}

/* USER CODE BEGIN 4 */

/**
 * @brief Timer Period Elapsed Callback - PID Loop
 * @param htim: Timer handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM6) {
        // PID step - hər 10ms-də bir çağırılır
        if (g_operation_mode == MODE_AUTO || g_operation_mode == MODE_MANUAL) {
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
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
          ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
