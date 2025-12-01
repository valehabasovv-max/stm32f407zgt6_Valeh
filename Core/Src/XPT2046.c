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
 * Hardware qoşulması:
 *   T_CS   (Chip Select)        -> PB12
 *   T_CLK  (SCK - Serial Clock) -> PB1
 *   T_DO   (MISO)               -> PF8
 *   T_DIN  (MOSI)               -> PF9
 *   T_IRQ  (Interrupt Request)  -> PF10
 */
#define TP_CS_GPIO_Port    XPT2046_CS_PORT
#define TP_CS_Pin          XPT2046_CS_PIN
#define TP_IRQ_GPIO_Port   XPT2046_IRQ_PORT
#define TP_IRQ_Pin         XPT2046_IRQ_PIN
#define TP_SCK_GPIO_Port   XPT2046_SCK_PORT
#define TP_SCK_Pin         XPT2046_SCK_PIN
#define TP_MISO_GPIO_Port  XPT2046_MISO_PORT
#define TP_MISO_Pin        XPT2046_MISO_PIN
#define TP_MOSI_GPIO_Port  XPT2046_MOSI_PORT
#define TP_MOSI_Pin        XPT2046_MOSI_PIN

/* Debug mode */
static uint8_t debug_mode = 0;

/* =============== 3-NÖQTƏLİ KALİBRASİYA DƏYİŞƏNLƏRİ =============== */
static TouchCalibration_t touch_cal = {
    .raw_x = {0, 0, 0},
    .raw_y = {0, 0, 0},
    .screen_x = {CAL_POINT1_X, CAL_POINT2_X, CAL_POINT3_X},
    .screen_y = {CAL_POINT1_Y, CAL_POINT2_Y, CAL_POINT3_Y},
    .a = 0.0f, .b = 0.0f, .c = 0.0f,
    .d = 0.0f, .e = 0.0f, .f = 0.0f,
    .calibrated = 0
};

/* 3-nöqtəli kalibrasiya bayrağı - bu aktiv olduqda matris istifadə edilir */
static uint8_t use_matrix_calibration = 0;

/**
 * @brief Default kalibrasiya dəyərlərini yüklə
 * @note 3-nöqtəli kalibrasiya uğursuz olduqda istifadə edilir
 */
void XPT2046_LoadDefaultCalibration(void)
{
    /* Tipik ILI9341 + XPT2046 üçün default dəyərlər */
    /* Bu dəyərlər əksər ekranlar üçün işləyir */
    /* DÜZƏLİŞ: Daha geniş diapazon və müxtəlif koordinat modları sınaqdan keçirildi */
    cal_x_min = 300;
    cal_x_max = 3800;
    cal_y_min = 300;
    cal_y_max = 3800;
    
    /* Koordinat mode seçimi - tipik 3.2" TFT ekranlar üçün */
    /* Mode 0: Normal */
    /* Mode 1: Swap X-Y + Invert X - ÇOX İSTİFADƏ OLUNAN landscape orientation */
    /* Mode 2: Invert both X and Y */
    /* Mode 3: Swap X-Y only */
    coord_mode = 1;  /* Default: Swap + Invert X */
    
    /* 3-nöqtəli kalibrasiyadan istifadə etmə, köhnə metod istifadə et */
    use_matrix_calibration = 0;
    touch_cal.calibrated = 0;
    
    printf("XPT2046: Default calibration loaded (fallback)\r\n");
    printf("  X range: %d - %d\r\n", cal_x_min, cal_x_max);
    printf("  Y range: %d - %d\r\n", cal_y_min, cal_y_max);
    printf("  Coord mode: %d (0=Normal, 1=SwapInvX, 2=InvBoth, 3=SwapOnly)\r\n", coord_mode);
}

/**
 * @brief Bütün koordinat modlarını sınaqdan keçir və ən uyğun olanı seç
 * @param raw_x, raw_y: Test üçün raw koordinatlar
 * @param expected_x, expected_y: Gözlənilən ekran koordinatları
 * @return Ən yaxşı koordinat modu (0-3)
 */
uint8_t XPT2046_FindBestCoordMode(uint16_t raw_x, uint16_t raw_y,
                                   uint16_t expected_x, uint16_t expected_y)
{
    uint8_t best_mode = 1;  /* Default */
    int32_t best_error = 100000;
    
    for (uint8_t mode = 0; mode <= 3; mode++) {
        uint8_t old_mode = coord_mode;
        coord_mode = mode;
        
        uint16_t test_sx, test_sy;
        XPT2046_ConvertToScreen(raw_x, raw_y, &test_sx, &test_sy);
        
        int32_t dx = (int32_t)test_sx - (int32_t)expected_x;
        int32_t dy = (int32_t)test_sy - (int32_t)expected_y;
        int32_t error = dx * dx + dy * dy;
        
        printf("  Mode %d: got(%d,%d) expected(%d,%d) error=%ld\r\n",
               mode, test_sx, test_sy, expected_x, expected_y, error);
        
        if (error < best_error) {
            best_error = error;
            best_mode = mode;
        }
        
        coord_mode = old_mode;
    }
    
    printf("Best coord mode: %d (error=%ld)\r\n", best_mode, best_error);
    return best_mode;
}

/* =============== AŞAĞI SƏVİYYƏLİ FUNKSİYALAR =============== */

/* Kiçik gecikmə - SCK üçün stabil kənar */
/* STM32F407 168MHz-də təxminən 1us gecikmə */
static inline void tp_delay(void) { 
    for(volatile int i = 0; i < 100; i++) __NOP(); 
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
    printf("\r\n=== XPT2046 Touch Controller Init ===\r\n");
    
    /* GPIO saatlarını aktivləşdir */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    
    /* ============================================
     * KRİTİK: SPI1 Hardware-ni TAMAMILƏ deaktiv et!
     * MX_SPI1_Init() bu pinləri hardware SPI rejiminə qoyub.
     * Biz bit-bang SPI istifadə etdiyimiz üçün GPIO rejiminə keçirməliyik.
     * ============================================ */
    printf("Disabling SPI1 hardware peripheral...\r\n");
    
    /* SPI1 periferalını deaktiv et */
    __HAL_RCC_SPI1_CLK_ENABLE();  /* Əvvəlcə clock aktiv olmalıdır */
    
    /* SPI1-i tamamilə sıfırla */
    SPI1->CR1 = 0;   /* Control register 1 sıfırla */
    SPI1->CR2 = 0;   /* Control register 2 sıfırla */
    
    __HAL_RCC_SPI1_FORCE_RESET();    /* SPI1-i force reset et */
    __HAL_RCC_SPI1_RELEASE_RESET();  /* Reset-i burax */
    __HAL_RCC_SPI1_CLK_DISABLE();    /* SPI1 clock-u söndür */
    
    /* GPIO Alternate Function-u sıfırla */
    /* PA5, PA6 üçün MODER registrini birbaşa dəyişdir */
    GPIOA->MODER &= ~(GPIO_MODER_MODER5 | GPIO_MODER_MODER6);  /* Input mode */
    GPIOA->AFR[0] &= ~(0xFFU << 20);  /* PA5 və PA6 üçün AF sıfırla */
    
    /* PB5 üçün MODER registrini dəyişdir */
    GPIOB->MODER &= ~GPIO_MODER_MODER5;  /* Input mode */
    GPIOB->AFR[0] &= ~(0xFU << 20);  /* PB5 üçün AF sıfırla */
    
    /* Əvvəlcə pinləri resetlə */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5);  /* PA5 - SCK */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_6);  /* PA6 - MISO */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_5);  /* PB5 - MOSI */
    
    HAL_Delay(5);  /* Stabilləşmə üçün kiçik gecikmə */
    
    GPIO_InitTypeDef g = {0};

    /* CS: çıxış (PB12) */
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    g.Pin = TP_CS_Pin;   
    HAL_GPIO_Init(TP_CS_GPIO_Port, &g);
    
    /* SCK: çıxış (PA5) */
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    g.Pin = TP_SCK_Pin;  
    HAL_GPIO_Init(TP_SCK_GPIO_Port, &g);
    
    /* MOSI: çıxış (PB5) */
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    g.Pin = TP_MOSI_Pin; 
    HAL_GPIO_Init(TP_MOSI_GPIO_Port, &g);

    /* MISO: giriş (PA6) - pull-up ilə */
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLUP;
    g.Pin  = TP_MISO_Pin;
    HAL_GPIO_Init(TP_MISO_GPIO_Port, &g);

    /* IRQ: giriş-pullup (PF10) - modulda açıq kollektor olur */
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLUP;
    g.Pin  = TP_IRQ_Pin;
    HAL_GPIO_Init(TP_IRQ_GPIO_Port, &g);

    /* İlkin səviyyələr */
    HAL_GPIO_WritePin(TP_CS_GPIO_Port,  TP_CS_Pin,  GPIO_PIN_SET);   /* CS=HIGH (deaktiv) */
    HAL_GPIO_WritePin(TP_SCK_GPIO_Port, TP_SCK_Pin, GPIO_PIN_RESET); /* SCK=LOW */
    HAL_GPIO_WritePin(TP_MOSI_GPIO_Port, TP_MOSI_Pin, GPIO_PIN_RESET);
    
    /* Test communication with XPT2046 */
    HAL_Delay(10);
    
    /* Bir neçə dummy oxuma - XPT2046-nı oyanmaq üçün */
    HAL_GPIO_WritePin(TP_CS_GPIO_Port, TP_CS_Pin, GPIO_PIN_RESET);
    tp_delay();
    tp_xfer(0x00);  /* Dummy command */
    tp_xfer(0x00);
    HAL_GPIO_WritePin(TP_CS_GPIO_Port, TP_CS_Pin, GPIO_PIN_SET);
    tp_delay();
    
    /* Touch-un işlədiyini yoxla */
    uint8_t irq_state = HAL_GPIO_ReadPin(TP_IRQ_GPIO_Port, TP_IRQ_Pin);
    
    printf("XPT2046 Touch Controller initialized\r\n");
    printf("Pin config: CS=PB12, SCK=PA5, MISO=PA6, MOSI=PB5, IRQ=PF10\r\n");
    printf("Calibration: X[%d-%d], Y[%d-%d], Mode=%d\r\n", 
           cal_x_min, cal_x_max, cal_y_min, cal_y_max, coord_mode);
    printf("IRQ Pin (PF10) state: %s\r\n", irq_state ? "HIGH (ready)" : "LOW (touched or problem)");
    printf("=== XPT2046 Init Complete ===\r\n\r\n");
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
        
        /* Daha geniş diapazon qəbul et (50-4050 arası) */
        /* Ekran kənarlarında dəyərlər daha aşağı/yüksək ola bilər */
        if (rx > 50 && rx < 4050 && ry > 50 && ry < 4050) {
            x_samples[valid_samples] = rx;
            y_samples[valid_samples] = ry;
            valid_samples++;
        }
    }
    
    /* Ən azı 2 etibarlı nümunə kifayətdir (daha az sərt) */
    if (valid_samples < 2) {
        if (debug_mode) {
            printf("XPT2046: Not enough valid samples (%d)\r\n", valid_samples);
        }
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
    
    /* 3-nöqtəli kalibrasiya aktiv olduqda matris istifadə et */
    if (touch_cal.calibrated && use_matrix_calibration) {
        XPT2046_ConvertWithMatrix(raw_x, raw_y, screen_x, screen_y);
        
        /* Debug çıxışı */
        if (debug_mode) {
            printf("Touch: raw(%d,%d) -> screen(%d,%d) [matrix]\r\n", 
                   raw_x, raw_y, *screen_x, *screen_y);
        }
        return;
    }
    
    /* Köhnə kalibrasiya metodu (fallback) */
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

/* =============== 3-NÖQTƏLİ KALİBRASİYA FUNKSİYALARI =============== */

/**
 * @brief Kalibrasiya matrisini 3 nöqtədən hesabla
 * @note Affine transformasiya: screen = A * raw + offset
 *       | screen_x |   | a  b | | raw_x |   | c |
 *       | screen_y | = | d  e | | raw_y | + | f |
 */
static void CalculateCalibrationMatrix(void)
{
    /* 3 nöqtənin raw və screen koordinatları */
    float rx[3], ry[3], sx[3], sy[3];
    
    for (int i = 0; i < 3; i++) {
        rx[i] = (float)touch_cal.raw_x[i];
        ry[i] = (float)touch_cal.raw_y[i];
        sx[i] = (float)touch_cal.screen_x[i];
        sy[i] = (float)touch_cal.screen_y[i];
    }
    
    /* Determinant hesabla */
    float div = (rx[0] - rx[2]) * (ry[1] - ry[2]) - (rx[1] - rx[2]) * (ry[0] - ry[2]);
    
    if (div == 0.0f) {
        printf("Calibration error: determinant is zero!\r\n");
        touch_cal.calibrated = 0;
        return;
    }
    
    /* Affine matris əmsallarını hesabla - X üçün */
    touch_cal.a = ((sx[0] - sx[2]) * (ry[1] - ry[2]) - (sx[1] - sx[2]) * (ry[0] - ry[2])) / div;
    touch_cal.b = ((rx[0] - rx[2]) * (sx[1] - sx[2]) - (rx[1] - rx[2]) * (sx[0] - sx[2])) / div;
    touch_cal.c = (sx[0] * (rx[1] * ry[2] - rx[2] * ry[1]) - 
                   sx[1] * (rx[0] * ry[2] - rx[2] * ry[0]) + 
                   sx[2] * (rx[0] * ry[1] - rx[1] * ry[0])) / div;
    
    /* Affine matris əmsallarını hesabla - Y üçün */
    touch_cal.d = ((sy[0] - sy[2]) * (ry[1] - ry[2]) - (sy[1] - sy[2]) * (ry[0] - ry[2])) / div;
    touch_cal.e = ((rx[0] - rx[2]) * (sy[1] - sy[2]) - (rx[1] - rx[2]) * (sy[0] - sy[2])) / div;
    touch_cal.f = (sy[0] * (rx[1] * ry[2] - rx[2] * ry[1]) - 
                   sy[1] * (rx[0] * ry[2] - rx[2] * ry[0]) + 
                   sy[2] * (rx[0] * ry[1] - rx[1] * ry[0])) / div;
    
    touch_cal.calibrated = 1;
    use_matrix_calibration = 1;
    
    printf("Calibration complete!\r\n");
    printf("Matrix: a=%.4f, b=%.4f, c=%.4f\r\n", touch_cal.a, touch_cal.b, touch_cal.c);
    printf("Matrix: d=%.4f, e=%.4f, f=%.4f\r\n", touch_cal.d, touch_cal.e, touch_cal.f);
}

/**
 * @brief Matris ilə raw koordinatları ekran koordinatlarına çevir
 */
void XPT2046_ConvertWithMatrix(uint16_t raw_x, uint16_t raw_y,
                               uint16_t *screen_x, uint16_t *screen_y)
{
    if (!screen_x || !screen_y) return;
    
    if (!touch_cal.calibrated || !use_matrix_calibration) {
        /* Kalibrasiya yoxdursa, köhnə üsulla çevir */
        XPT2046_ConvertToScreen(raw_x, raw_y, screen_x, screen_y);
        return;
    }
    
    /* Affine transformasiya tətbiq et */
    float fx = (float)raw_x;
    float fy = (float)raw_y;
    
    float sx = touch_cal.a * fx + touch_cal.b * fy + touch_cal.c;
    float sy = touch_cal.d * fx + touch_cal.e * fy + touch_cal.f;
    
    /* Ekran sərhədləri içində saxla */
    if (sx < 0.0f) sx = 0.0f;
    if (sx > (TOUCH_SCREEN_WIDTH - 1)) sx = TOUCH_SCREEN_WIDTH - 1;
    if (sy < 0.0f) sy = 0.0f;
    if (sy > (TOUCH_SCREEN_HEIGHT - 1)) sy = TOUCH_SCREEN_HEIGHT - 1;
    
    *screen_x = (uint16_t)(sx + 0.5f);
    *screen_y = (uint16_t)(sy + 0.5f);
}

/**
 * @brief Kalibrasiya olunubmu yoxla
 */
uint8_t XPT2046_IsCalibrated(void)
{
    return touch_cal.calibrated && use_matrix_calibration;
}

/**
 * @brief Kalibrasiya məlumatlarını al
 */
TouchCalibration_t* XPT2046_GetCalibrationData(void)
{
    return &touch_cal;
}

/**
 * @brief Kalibrasiya matrisini xaricdən təyin et
 */
void XPT2046_SetCalibrationMatrix(TouchCalibration_t* cal)
{
    if (!cal) return;
    
    touch_cal.a = cal->a;
    touch_cal.b = cal->b;
    touch_cal.c = cal->c;
    touch_cal.d = cal->d;
    touch_cal.e = cal->e;
    touch_cal.f = cal->f;
    touch_cal.calibrated = cal->calibrated;
    
    if (cal->calibrated) {
        use_matrix_calibration = 1;
    }
}

/**
 * @brief 3-nöqtəli touch kalibrasiyası
 * @note Bu funksiya LCD funksiyalarını çağırır - ILI9341_FSMC.h include olunmalıdır
 * @return 1 uğurlu, 0 ləğv edildi
 */
uint8_t XPT2046_Calibrate3Point(void)
{
    /* Bu funksiya main.c-dən çağırılır və LCD istifadə edir */
    /* İmplementasiya ILI9341_FSMC.c-də olacaq çünki LCD funksiyaları lazımdır */
    printf("3-Point Calibration started...\r\n");
    printf("Please use XPT2046_RunCalibration() from ILI9341_FSMC module\r\n");
    return 0;
}

/**
 * @brief Kalibrasiya nöqtəsinin raw dəyərini təyin et
 * @param point_idx: 0, 1 və ya 2 (3 nöqtə)
 * @param raw_x, raw_y: Raw touch koordinatları
 */
void XPT2046_SetCalibrationPoint(uint8_t point_idx, uint16_t raw_x, uint16_t raw_y)
{
    if (point_idx >= 3) return;
    
    touch_cal.raw_x[point_idx] = raw_x;
    touch_cal.raw_y[point_idx] = raw_y;
    
    printf("Calibration point %d: raw(%d, %d) -> screen(%d, %d)\r\n",
           point_idx, raw_x, raw_y, 
           touch_cal.screen_x[point_idx], touch_cal.screen_y[point_idx]);
}

/**
 * @brief Kalibrasiya matrisini yoxla və doğrula
 * @return 1 uğurlu, 0 uğursuz
 */
static uint8_t VerifyCalibrationMatrix(void)
{
    /* Test: Məlum raw dəyərləri üçün screen koordinatlarını hesabla */
    /* Sol üst künc test (cal point 1) */
    uint16_t test_sx, test_sy;
    XPT2046_ConvertWithMatrix(touch_cal.raw_x[0], touch_cal.raw_y[0], &test_sx, &test_sy);
    
    /* Gözlənilən koordinatlarla müqayisə - 30 piksel tolerans */
    int16_t dx1 = (int16_t)test_sx - (int16_t)touch_cal.screen_x[0];
    int16_t dy1 = (int16_t)test_sy - (int16_t)touch_cal.screen_y[0];
    
    if (dx1 < -30 || dx1 > 30 || dy1 < -30 || dy1 > 30) {
        printf("Calibration verification failed for point 1: expected(%d,%d) got(%d,%d)\r\n",
               touch_cal.screen_x[0], touch_cal.screen_y[0], test_sx, test_sy);
        return 0;
    }
    
    /* Sağ üst künc test (cal point 2) */
    XPT2046_ConvertWithMatrix(touch_cal.raw_x[1], touch_cal.raw_y[1], &test_sx, &test_sy);
    
    int16_t dx2 = (int16_t)test_sx - (int16_t)touch_cal.screen_x[1];
    int16_t dy2 = (int16_t)test_sy - (int16_t)touch_cal.screen_y[1];
    
    if (dx2 < -30 || dx2 > 30 || dy2 < -30 || dy2 > 30) {
        printf("Calibration verification failed for point 2: expected(%d,%d) got(%d,%d)\r\n",
               touch_cal.screen_x[1], touch_cal.screen_y[1], test_sx, test_sy);
        return 0;
    }
    
    /* Mərkəz alt künc test (cal point 3) */
    XPT2046_ConvertWithMatrix(touch_cal.raw_x[2], touch_cal.raw_y[2], &test_sx, &test_sy);
    
    int16_t dx3 = (int16_t)test_sx - (int16_t)touch_cal.screen_x[2];
    int16_t dy3 = (int16_t)test_sy - (int16_t)touch_cal.screen_y[2];
    
    if (dx3 < -30 || dx3 > 30 || dy3 < -30 || dy3 > 30) {
        printf("Calibration verification failed for point 3: expected(%d,%d) got(%d,%d)\r\n",
               touch_cal.screen_x[2], touch_cal.screen_y[2], test_sx, test_sy);
        return 0;
    }
    
    printf("Calibration verification PASSED!\r\n");
    return 1;
}

/**
 * @brief Bütün nöqtələr alındıqdan sonra matrisi hesabla
 */
void XPT2046_FinishCalibration(void)
{
    CalculateCalibrationMatrix();
    
    /* Matrisi doğrula */
    if (touch_cal.calibrated && use_matrix_calibration) {
        if (!VerifyCalibrationMatrix()) {
            printf("WARNING: Matrix calibration failed verification! Falling back to simple method.\r\n");
            
            /* Raw dəyərlərdən sadə kalibrasiya dəyərlərini hesabla */
            /* X range: raw_x[0] (sol) və raw_x[1] (sağ) arasında */
            uint16_t min_raw_x = touch_cal.raw_x[0] < touch_cal.raw_x[1] ? touch_cal.raw_x[0] : touch_cal.raw_x[1];
            uint16_t max_raw_x = touch_cal.raw_x[0] > touch_cal.raw_x[1] ? touch_cal.raw_x[0] : touch_cal.raw_x[1];
            uint16_t min_raw_y = touch_cal.raw_y[0] < touch_cal.raw_y[2] ? touch_cal.raw_y[0] : touch_cal.raw_y[2];
            uint16_t max_raw_y = touch_cal.raw_y[0] > touch_cal.raw_y[2] ? touch_cal.raw_y[0] : touch_cal.raw_y[2];
            
            /* 10% margin əlavə et */
            uint16_t x_range = max_raw_x - min_raw_x;
            uint16_t y_range = max_raw_y - min_raw_y;
            
            cal_x_min = min_raw_x > (x_range / 10) ? min_raw_x - (x_range / 10) : 50;
            cal_x_max = max_raw_x + (x_range / 10);
            if (cal_x_max > 4050) cal_x_max = 4050;
            
            cal_y_min = min_raw_y > (y_range / 10) ? min_raw_y - (y_range / 10) : 50;
            cal_y_max = max_raw_y + (y_range / 10);
            if (cal_y_max > 4050) cal_y_max = 4050;
            
            /* Koordinat mode-u avtomatik müəyyən et - bütün modları sınaqdan keçir */
            printf("Testing all coord modes to find best match...\r\n");
            
            /* İlk kalibrasiya nöqtəsi ilə test et */
            coord_mode = XPT2046_FindBestCoordMode(
                touch_cal.raw_x[0], touch_cal.raw_y[0],
                touch_cal.screen_x[0], touch_cal.screen_y[0]
            );
            
            /* Matrisi deaktiv et */
            use_matrix_calibration = 0;
            touch_cal.calibrated = 0;
            
            printf("Fallback calibration: X[%d-%d], Y[%d-%d], Mode=%d\r\n",
                   cal_x_min, cal_x_max, cal_y_min, cal_y_max, coord_mode);
        }
    }
}
