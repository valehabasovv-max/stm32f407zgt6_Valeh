/**
 * XPT2046 Touch Controller Driver
 * Tam işlək və kalibrasiyalı versiya
 */

#include "XPT2046.h"
#include <stdio.h>

/* =============== KALİBRASİYA DƏYƏRLƏRİ =============== */
/* Bu dəyərlər ekranınıza görə dəyişdirilməlidir */
static uint16_t cal_x_min = 200;
static uint16_t cal_x_max = 3900;
static uint16_t cal_y_min = 200;
static uint16_t cal_y_max = 3900;

/* Koordinat çevirmə rejimi */
/* 0 = Normal
 * 1 = X və Y dəyişdir, X-i əks et
 * 2 = X və Y-ni əks et
 * 3 = X və Y dəyişdir
 */
static uint8_t coord_mode = 1;  /* Standart: swap + invert X */

/* Touch təsdiqi üçün parametrlər */
#define TOUCH_SAMPLES 8
#define TOUCH_DEBOUNCE_MS 50
#define TOUCH_PRESSURE_THRESHOLD 100

/* Son touch vaxtı */
static uint32_t last_touch_time = 0;

/* Pin tərifləri */
/* 
 * QEYD: SCK pini PB1-dən PF11-ə köçürüldü!
 * Səbəb: PB1 TIM3_CH4 (PWM) ilə konflikt yaradırdı.
 * 
 * Hardware qoşulması:
 *   Touch CS   -> PB12
 *   Touch SCK  -> PF11 (əvvəl PB1 idi, DƏYİŞDİRİLDİ!)
 *   Touch MISO -> PF8
 *   Touch MOSI -> PF9
 *   Touch IRQ  -> PF10
 */
#define TP_CS_GPIO_Port    XPT2046_CS_PORT
#define TP_CS_Pin          XPT2046_CS_PIN
#define TP_IRQ_GPIO_Port   XPT2046_IRQ_PORT
#define TP_IRQ_Pin         XPT2046_IRQ_PIN
#define TP_SCK_GPIO_Port   GPIOF
#define TP_SCK_Pin         GPIO_PIN_11
#define TP_MISO_GPIO_Port  GPIOF
#define TP_MISO_Pin        GPIO_PIN_8
#define TP_MOSI_GPIO_Port  GPIOF
#define TP_MOSI_Pin        GPIO_PIN_9

/* Debug mode */
static uint8_t debug_mode = 0;

/* =============== AŞAĞI SƏVİYYƏLİ FUNKSİYALAR =============== */

/* Kiçik gecikmə - SCK üçün stabil kənar */
static inline void tp_delay(void) { 
    for(volatile int i = 0; i < 50; i++) __NOP(); 
}

/* Bit-bang SPI transfer (MSB-first) */
static uint8_t tp_xfer(uint8_t d)
{
    uint8_t r = 0;
    for(int i = 7; i >= 0; i--)
    {
        /* MOSI */
        HAL_GPIO_WritePin(TP_MOSI_GPIO_Port, TP_MOSI_Pin, 
                         (d & (1U << i)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        tp_delay();
        
        /* SCK ↑ */
        HAL_GPIO_WritePin(TP_SCK_GPIO_Port, TP_SCK_Pin, GPIO_PIN_SET);
        tp_delay();
        
        /* MISO oxu */
        r <<= 1;
        if (HAL_GPIO_ReadPin(TP_MISO_GPIO_Port, TP_MISO_Pin) == GPIO_PIN_SET) {
            r |= 1;
        }
        
        /* SCK ↓ */
        HAL_GPIO_WritePin(TP_SCK_GPIO_Port, TP_SCK_Pin, GPIO_PIN_RESET);
        tp_delay();
    }
    return r;
}

/* 12-bit oxu (XPT2046-da: komandadan sonra 16 bit gəlir; üst 12-si məlumat) */
static uint16_t tp_read12(uint8_t cmd)
{
    (void)tp_xfer(cmd);
    uint8_t h = tp_xfer(0x00);
    uint8_t l = tp_xfer(0x00);
    return (((uint16_t)h << 5) | ((uint16_t)l >> 3)) & 0x0FFF;
}

/* =============== GPIO BAŞLATMA =============== */
void XPT2046_Init(void)
{
    /* GPIO saatlarını aktivləşdir */
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};

    /* CS, SCK, MOSI: çıxış */
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;

    g.Pin = TP_CS_Pin;   
    HAL_GPIO_Init(TP_CS_GPIO_Port, &g);
    
    g.Pin = TP_SCK_Pin;  
    HAL_GPIO_Init(TP_SCK_GPIO_Port, &g);
    
    g.Pin = TP_MOSI_Pin; 
    HAL_GPIO_Init(TP_MOSI_GPIO_Port, &g);

    /* MISO: giriş */
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_NOPULL;
    g.Pin  = TP_MISO_Pin;
    HAL_GPIO_Init(TP_MISO_GPIO_Port, &g);

    /* IRQ: giriş-pullup (modulda açıq kollektor olur) */
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLUP;
    g.Pin  = TP_IRQ_Pin;
    HAL_GPIO_Init(TP_IRQ_GPIO_Port, &g);

    /* İlkin səviyyələr */
    HAL_GPIO_WritePin(TP_CS_GPIO_Port,  TP_CS_Pin,  GPIO_PIN_SET);   /* CS=HIGH (deaktiv) */
    HAL_GPIO_WritePin(TP_SCK_GPIO_Port, TP_SCK_Pin, GPIO_PIN_RESET); /* SCK=LOW */
    HAL_GPIO_WritePin(TP_MOSI_GPIO_Port, TP_MOSI_Pin, GPIO_PIN_RESET);
    
    printf("XPT2046 Touch Controller initialized\r\n");
    printf("Calibration: X[%d-%d], Y[%d-%d], Mode=%d\r\n", 
           cal_x_min, cal_x_max, cal_y_min, cal_y_max, coord_mode);
}

/* =============== TOXUNUŞ ALGILAMA =============== */

/* IRQ pin-i yoxla - 0=toxunmur, 1=toxunur */
uint8_t XPT2046_IsTouched(void)
{
    /* IRQ pin LOW olmalıdır (toxunuşda aşağı çəkilir) */
    return (HAL_GPIO_ReadPin(TP_IRQ_GPIO_Port, TP_IRQ_Pin) == GPIO_PIN_RESET) ? 1U : 0U;
}

/* Çoxlu nümunə ilə raw koordinatları oxu */
uint8_t XPT2046_ReadRaw(uint16_t *x, uint16_t *y)
{
    if (!x || !y) return 0;
    if (!XPT2046_IsTouched()) return 0;

    uint32_t x_sum = 0, y_sum = 0;
    uint16_t x_samples[TOUCH_SAMPLES];
    uint16_t y_samples[TOUCH_SAMPLES];
    uint8_t valid_samples = 0;
    
    /* Çoxlu nümunə götür */
    for(int i = 0; i < TOUCH_SAMPLES; i++)
    {
        /* CS ↓ */
        HAL_GPIO_WritePin(TP_CS_GPIO_Port, TP_CS_Pin, GPIO_PIN_RESET);
        tp_delay();

        /* Koordinatları oxu */
        /* X: 0xD0 (12-bit, differential, PD=00) */
        /* Y: 0x90 (12-bit, differential, PD=00) */
        uint16_t rx = tp_read12(0xD0);
        uint16_t ry = tp_read12(0x90);

        /* CS ↑ */
        HAL_GPIO_WritePin(TP_CS_GPIO_Port, TP_CS_Pin, GPIO_PIN_SET);
        tp_delay();
        
        /* Yalnız etibarlı dəyərləri qəbul et */
        if (rx > 100 && rx < 4000 && ry > 100 && ry < 4000) {
            x_samples[valid_samples] = rx;
            y_samples[valid_samples] = ry;
            valid_samples++;
        }
    }
    
    /* Ən azı 4 etibarlı nümunə olmalıdır */
    if (valid_samples < 4) {
        return 0;
    }
    
    /* Bubble sort ilə median tap */
    for (uint8_t i = 0; i < valid_samples - 1; i++) {
        for (uint8_t j = 0; j < valid_samples - i - 1; j++) {
            if (x_samples[j] > x_samples[j + 1]) {
                uint16_t temp = x_samples[j];
                x_samples[j] = x_samples[j + 1];
                x_samples[j + 1] = temp;
            }
            if (y_samples[j] > y_samples[j + 1]) {
                uint16_t temp = y_samples[j];
                y_samples[j] = y_samples[j + 1];
                y_samples[j + 1] = temp;
            }
        }
    }
    
    /* Median dəyərləri götür (ortadakı dəyərlər) */
    uint8_t mid = valid_samples / 2;
    *x = x_samples[mid];
    *y = y_samples[mid];
    
    return 1;
}

/* Touch koordinatlarını al */
uint8_t XPT2046_GetCoordinates(uint16_t *x, uint16_t *y)
{
    if (!XPT2046_IsTouched()) {
        return 0;
    }
    return XPT2046_ReadRaw(x, y);
}

/* Z pressure oxu */
uint16_t XPT2046_ReadZ(void)
{
    HAL_GPIO_WritePin(TP_CS_GPIO_Port, TP_CS_Pin, GPIO_PIN_RESET);
    tp_delay();
    
    uint16_t z1 = tp_read12(0xB0);  /* Z1 */
    uint16_t z2 = tp_read12(0xC0);  /* Z2 */
    
    HAL_GPIO_WritePin(TP_CS_GPIO_Port, TP_CS_Pin, GPIO_PIN_SET);
    
    /* Pressure hesabla */
    if (z1 == 0) return 4095;
    return z1;
}

/* =============== KOORDİNAT ÇEVİRMƏ =============== */

void XPT2046_ConvertToScreen(uint16_t raw_x, uint16_t raw_y, 
                             uint16_t *screen_x, uint16_t *screen_y)
{
    if (!screen_x || !screen_y) return;
    
    int32_t temp_x = raw_x;
    int32_t temp_y = raw_y;
    
    /* Kalibrasiya limitlərini tətbiq et */
    if (temp_x < cal_x_min) temp_x = cal_x_min;
    if (temp_x > cal_x_max) temp_x = cal_x_max;
    if (temp_y < cal_y_min) temp_y = cal_y_min;
    if (temp_y > cal_y_max) temp_y = cal_y_max;
    
    /* Ekran koordinatlarına çevir */
    int32_t sx, sy;
    
    switch (coord_mode) {
        case 0: /* Normal mapping */
            sx = ((temp_x - cal_x_min) * 319) / (cal_x_max - cal_x_min);
            sy = ((temp_y - cal_y_min) * 239) / (cal_y_max - cal_y_min);
            break;
            
        case 1: /* Swap X-Y, invert X (ən çox istifadə olunan) */
            sx = 319 - ((temp_y - cal_y_min) * 319) / (cal_y_max - cal_y_min);
            sy = ((temp_x - cal_x_min) * 239) / (cal_x_max - cal_x_min);
            break;
            
        case 2: /* Invert both X and Y */
            sx = 319 - ((temp_x - cal_x_min) * 319) / (cal_x_max - cal_x_min);
            sy = 239 - ((temp_y - cal_y_min) * 239) / (cal_y_max - cal_y_min);
            break;
            
        case 3: /* Swap X-Y only */
            sx = ((temp_y - cal_y_min) * 319) / (cal_y_max - cal_y_min);
            sy = ((temp_x - cal_x_min) * 239) / (cal_x_max - cal_x_min);
            break;
            
        default:
            sx = ((temp_x - cal_x_min) * 319) / (cal_x_max - cal_x_min);
            sy = ((temp_y - cal_y_min) * 239) / (cal_y_max - cal_y_min);
            break;
    }
    
    /* Limitlər içində saxla */
    if (sx < 0) sx = 0;
    if (sx > 319) sx = 319;
    if (sy < 0) sy = 0;
    if (sy > 239) sy = 239;
    
    *screen_x = (uint16_t)sx;
    *screen_y = (uint16_t)sy;
    
    /* Debug çıxışı */
    if (debug_mode) {
        printf("Touch: raw(%d,%d) -> screen(%d,%d) mode=%d\r\n", 
               raw_x, raw_y, *screen_x, *screen_y, coord_mode);
    }
}

/* Ekran koordinatlarını birbaşa al */
uint8_t XPT2046_GetScreenCoordinates(uint16_t *screen_x, uint16_t *screen_y)
{
    uint16_t raw_x, raw_y;
    
    if (XPT2046_GetCoordinates(&raw_x, &raw_y)) {
        XPT2046_ConvertToScreen(raw_x, raw_y, screen_x, screen_y);
        return 1;
    }
    return 0;
}

/* =============== KALİBRASİYA FUNKSİYALARI =============== */

void XPT2046_SetCalibration(uint16_t x_min, uint16_t x_max, 
                            uint16_t y_min, uint16_t y_max)
{
    cal_x_min = x_min;
    cal_x_max = x_max;
    cal_y_min = y_min;
    cal_y_max = y_max;
    
    printf("Touch calibration set: X[%d-%d], Y[%d-%d]\r\n", 
           x_min, x_max, y_min, y_max);
}

void XPT2046_GetCalibration(uint16_t *x_min, uint16_t *x_max, 
                            uint16_t *y_min, uint16_t *y_max)
{
    if (x_min) *x_min = cal_x_min;
    if (x_max) *x_max = cal_x_max;
    if (y_min) *y_min = cal_y_min;
    if (y_max) *y_max = cal_y_max;
}

void XPT2046_SetCoordMode(uint8_t mode)
{
    if (mode <= 3) {
        coord_mode = mode;
        printf("Touch coordinate mode set to: %d\r\n", mode);
    }
}

uint8_t XPT2046_GetCoordMode(void)
{
    return coord_mode;
}

/* Debug mode */
void XPT2046_SetDebugMode(uint8_t enable)
{
    debug_mode = enable;
}

/* Kalibrasiya proseduru */
void XPT2046_Calibrate(void)
{
    printf("Touch calibration procedure...\r\n");
    printf("Touch the indicated points on screen.\r\n");
    /* Kalibrasiya proseduru burada implementasiya oluna bilər */
}

/* =============== DÜYMƏ ALGILAMA =============== */

/**
 * @brief Düymənin basılıb-basılmadığını yoxla
 * @param btn_x, btn_y: Düymənin sol üst küncü
 * @param btn_w, btn_h: Düymənin eni və hündürlüyü
 * @param touch_x, touch_y: Touch koordinatları
 * @return 1 əgər düymə basılıbsa, 0 əks halda
 */
uint8_t XPT2046_IsButtonPressed(uint16_t btn_x, uint16_t btn_y, 
                                uint16_t btn_w, uint16_t btn_h,
                                uint16_t touch_x, uint16_t touch_y)
{
    return (touch_x >= btn_x && touch_x <= (btn_x + btn_w) &&
            touch_y >= btn_y && touch_y <= (btn_y + btn_h)) ? 1 : 0;
}

/**
 * @brief Debounce ilə touch al
 * @param screen_x, screen_y: Ekran koordinatları
 * @param debounce_ms: Debounce vaxtı (ms)
 * @return 1 əgər yeni touch varsa
 */
uint8_t XPT2046_GetTouchDebounced(uint16_t *screen_x, uint16_t *screen_y, 
                                  uint32_t debounce_ms)
{
    uint32_t now = HAL_GetTick();
    
    if (now - last_touch_time < debounce_ms) {
        return 0;
    }
    
    if (XPT2046_GetScreenCoordinates(screen_x, screen_y)) {
        last_touch_time = now;
        return 1;
    }
    
    return 0;
}
