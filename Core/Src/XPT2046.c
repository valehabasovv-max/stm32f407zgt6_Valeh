/**
 * XPT2046 Touch Controller Driver
 * Tam işlək və kalibrasiyalı versiya
 */

#include "XPT2046.h"
#include <stdio.h>

/* =============== KALİBRASİYA DƏYƏRLƏRİ =============== */
/* İstifadəçinin işləyən kodundan götürülüb */
static uint16_t cal_x_min = 200;
static uint16_t cal_x_max = 3800;
static uint16_t cal_y_min = 200;
static uint16_t cal_y_max = 3800;

/* Koordinat çevirmə rejimi */
/* 0 = Normal - touch və LCD eyni istiqamətdə
 * 1 = X və Y dəyişdir, X-i əks et
 * 2 = X və Y-ni əks et
 * 3 = X və Y dəyişdir (swap only) - ən çox işləyən
 * 4 = Invert X only
 * 5 = Invert Y only
 * 6 = Swap X-Y + Invert Y
 * 7 = Swap X-Y + Invert both
 * 
 * KRİTİK DÜZƏLİŞ: LCD landscape mode-dadır (Memory Access Control = 0x28, MV=1)
 * Bu səbəbdən touch koordinatları da swap edilməlidir.
 * Mode 3 (Swap X-Y only) landscape LCD üçün ən uyğundur.
 */
static uint8_t coord_mode = 1;  /* DÜZƏLİŞ: Mode 1 - Yalnız Mode 1 istifadə olunur */

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
 * @note Mode seçimi avtomatik həll edilir
 */
void XPT2046_LoadDefaultCalibration(void)
{
    /* İstifadəçinin işləyən kodundan götürülmüş dəyərlər */
    cal_x_min = 200;
    cal_x_max = 3800;
    cal_y_min = 200;
    cal_y_max = 3800;
    
    /* ============================================
     * KOORDİNAT MODE SEÇİMİ - LANDSCAPE LCD ÜÇÜN
     * ILI9341 Memory Access Control = 0x28 (landscape, MV=1)
     * 
     * Mode 0: Normal - touch və LCD eyni istiqamətdə
     * Mode 1: Swap X-Y + Invert X
     * Mode 2: Invert both X and Y - 180° fırlanmış
     * Mode 3: Swap X-Y only - landscape LCD üçün ən uyğun
     * Mode 4: Invert X only
     * Mode 5: Invert Y only
     * Mode 6: Swap X-Y + Invert Y
     * Mode 7: Swap X-Y + Invert both
     * 
     * KRİTİK DÜZƏLİŞ: LCD MV=1 (landscape) olduğu üçün
     * touch koordinatları da swap edilməlidir - Mode 3
     * ============================================ */
    
    /* Mode 1 - Yalnız Mode 1 istifadə olunur */
    coord_mode = 1;
    
    /* 3-nöqtəli kalibrasiyadan istifadə etmə, köhnə metod istifadə et */
    use_matrix_calibration = 0;
    touch_cal.calibrated = 0;
    
    printf("XPT2046: Default calibration loaded\r\n");
    printf("  X range: %d - %d\r\n", cal_x_min, cal_x_max);
    printf("  Y range: %d - %d\r\n", cal_y_min, cal_y_max);
    printf("  Coord mode: %d\r\n", coord_mode);
    printf("\r\n");
    printf("*** Butonlar işləmirsə, sağ üst küncə toxunun (mode dəyişdirmə)\r\n");
}

/**
 * @brief Bütün koordinat modlarını sınaqdan keçir və ən uyğun olanı seç
 * @param raw_x, raw_y: Test üçün raw koordinatlar
 * @param expected_x, expected_y: Gözlənilən ekran koordinatları
 * @return Ən yaxşı koordinat modu (0-7)
 */
uint8_t XPT2046_FindBestCoordMode(uint16_t raw_x, uint16_t raw_y,
                                   uint16_t expected_x, uint16_t expected_y)
{
    uint8_t best_mode = 0;  /* Default */
    int32_t best_error = 1000000;
    uint8_t old_mode = coord_mode;
    
    printf("Testing all 8 coord modes for raw(%d,%d) -> expected(%d,%d):\r\n",
           raw_x, raw_y, expected_x, expected_y);
    
    /* Bütün 8 modu sına */
    for (uint8_t mode = 0; mode < 8; mode++) {
        coord_mode = mode;
        
        uint16_t test_sx, test_sy;
        XPT2046_ConvertToScreen(raw_x, raw_y, &test_sx, &test_sy);
        
        int32_t dx = (int32_t)test_sx - (int32_t)expected_x;
        int32_t dy = (int32_t)test_sy - (int32_t)expected_y;
        int32_t error = dx * dx + dy * dy;
        
        printf("  M%d: (%d,%d) err=%ld\r\n", mode, test_sx, test_sy, error);
        
        if (error < best_error) {
            best_error = error;
            best_mode = mode;
        }
    }
    
    coord_mode = old_mode;  /* Əvvəlki mode-u bərpa et */
    
    printf("==> Best mode: %d (error=%ld)\r\n", best_mode, best_error);
    return best_mode;
}

/* =============== AŞAĞI SƏVİYYƏLİ FUNKSİYALAR =============== */

/* Kiçik gecikmə - SCK üçün stabil kənar */
/* İstifadəçinin işləyən kodundan - 30 NOP kifayətdir */
static inline void tp_delay(void) { 
    for(volatile int i = 0; i < 30; i++) __NOP(); 
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
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    
    /* ============================================
     * QEYD: Touch kontroller bit-bang SPI istifadə edir
     * Pinlər: CS=PB12, SCK=PB1, MISO=PF8, MOSI=PF9, IRQ=PF10
     * Bu pinlər SPI1-dən fərqlidir, ona görə SPI1 deaktiv etmək lazım deyil
     * ============================================ */
    
    GPIO_InitTypeDef g = {0};

    /* CS: çıxış (PB12) */
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    g.Pin = TP_CS_Pin;   
    HAL_GPIO_Init(TP_CS_GPIO_Port, &g);
    
    /* SCK: çıxış (PB1) */
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    g.Pin = TP_SCK_Pin;  
    HAL_GPIO_Init(TP_SCK_GPIO_Port, &g);
    
    /* MOSI: çıxış (PF9) */
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;
    g.Pin = TP_MOSI_Pin; 
    HAL_GPIO_Init(TP_MOSI_GPIO_Port, &g);

    /* MISO: giriş (PF8) - pull-up ilə */
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
    
    /* XPT2046 ilə kommunikasiya testi */
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
    printf("Pin config: CS=PB12, SCK=PB1, MISO=PF8, MOSI=PF9, IRQ=PF10\r\n");
    printf("Calibration: X[%d-%d], Y[%d-%d], Mode=%d\r\n", 
           cal_x_min, cal_x_max, cal_y_min, cal_y_max, coord_mode);
    printf("IRQ Pin (PF10) state: %s\r\n", irq_state ? "HIGH (ready)" : "LOW (touched or problem)");
    printf("=== XPT2046 Init Complete ===\r\n\r\n");
}

/* =============== TOXUNUŞ ALGILAMA =============== */

/**
 * @brief Touch varmı yoxla - GÜCLƏNDİRİLMİŞ
 * @return 1 = toxunuş var, 0 = toxunuş yox
 * @note IRQ pin LOW olduqda touch var
 * @note İki dəfə yoxlama - daha etibarlı
 */
uint8_t XPT2046_IsTouched(void)
{
    /* İlk yoxlama */
    if (HAL_GPIO_ReadPin(TP_IRQ_GPIO_Port, TP_IRQ_Pin) != GPIO_PIN_RESET) {
        return 0;  /* Touch yox */
    }
    
    /* Kiçik gecikmə və ikinci yoxlama - debounce */
    for (volatile int i = 0; i < 50; i++) __NOP();
    
    /* İkinci yoxlama */
    if (HAL_GPIO_ReadPin(TP_IRQ_GPIO_Port, TP_IRQ_Pin) != GPIO_PIN_RESET) {
        return 0;  /* Səhv alarm - touch yox */
    }
    
    return 1;  /* Touch var */
}

/**
 * @brief Raw touch koordinatlarını oxu - GÜCLƏNDİRİLMİŞ
 * @param x, y: Raw koordinatlar üçün pointer
 * @return 1 = uğurlu, 0 = xəta
 * @note Median filtrasiya ilə səs-küyü azaldır
 */
uint8_t XPT2046_ReadRaw(uint16_t *x, uint16_t *y)
{
    if (!x || !y) return 0;

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
        
        /* Çox geniş diapazon qəbul et - bütün etibarlı dəyərlər */
        uint8_t accepted = 0;
        if (rx > 100 && rx < 4000 && ry > 100 && ry < 4000) {
            x_samples[valid_samples] = rx;
            y_samples[valid_samples] = ry;
            valid_samples++;
            accepted = 1;
        }

        /* Debug: nümunələri çap et (əgər debug_mode aktivdirsə) */
        if (debug_mode) {
            printf("XPT2046 sample %d: rx=%u ry=%u %s\r\n", i, rx, ry, accepted ? "ACCEPT" : "REJECT");
        }
    }
    
    /* Ən azı 1 etibarlı nümunə kifayətdir */
    if (valid_samples < 1) {
        return 0;
    }
    
    /* Əgər 1 nümunə varsa, birbaşa istifadə et */
    if (valid_samples == 1) {
        *x = x_samples[0];
        *y = y_samples[0];
        if (debug_mode) printf("XPT2046 final (1 sample): x=%u y=%u\r\n", *x, *y);
        return 1;
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

    if (debug_mode) printf("XPT2046 median result: valid=%d mid=%d x=%u y=%u\r\n", valid_samples, mid, *x, *y);

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

/**
 * @brief Raw touch koordinatlarını ekran koordinatlarına çevir
 * @param raw_x, raw_y: Raw touch koordinatları
 * @param screen_x, screen_y: Ekran koordinatları üçün pointer
 * @note coord_mode dəyişəninə görə çevirmə aparılır
 */
void XPT2046_ConvertToScreen(uint16_t raw_x, uint16_t raw_y, 
                             uint16_t *screen_x, uint16_t *screen_y)
{
    if (!screen_x || !screen_y) return;
    
    /* 3-nöqtəli kalibrasiya aktiv olduqda matris istifadə et */
    if (touch_cal.calibrated && use_matrix_calibration) {
        XPT2046_ConvertWithMatrix(raw_x, raw_y, screen_x, screen_y);
        return;
    }
    
    /* Köhnə kalibrasiya metodu (fallback) */
    int32_t temp_x = raw_x;
    int32_t temp_y = raw_y;
    
    /* Kalibrasiya limitlərini tətbiq et */
    if (temp_x < (int32_t)cal_x_min) temp_x = cal_x_min;
    if (temp_x > (int32_t)cal_x_max) temp_x = cal_x_max;
    if (temp_y < (int32_t)cal_y_min) temp_y = cal_y_min;
    if (temp_y > (int32_t)cal_y_max) temp_y = cal_y_max;
    
    /* Ekran koordinatlarına çevir */
    int32_t sx, sy;
    int32_t x_range = cal_x_max - cal_x_min;
    int32_t y_range = cal_y_max - cal_y_min;
    
    /* Sıfıra bölmədən qorun */
    if (x_range <= 0) x_range = 1;
    if (y_range <= 0) y_range = 1;
    
    switch (coord_mode) {
        case 0: /* Normal */
            sx = ((temp_x - cal_x_min) * 319) / x_range;
            sy = ((temp_y - cal_y_min) * 239) / y_range;
            break;
            
        case 1: /* Swap X-Y + Invert X */
            sx = 319 - ((temp_y - cal_y_min) * 319) / y_range;
            sy = ((temp_x - cal_x_min) * 239) / x_range;
            break;
            
        case 2: /* Invert both X and Y */
            sx = 319 - ((temp_x - cal_x_min) * 319) / x_range;
            sy = 239 - ((temp_y - cal_y_min) * 239) / y_range;
            break;
            
        case 3: /* Swap X-Y only */
            sx = ((temp_y - cal_y_min) * 319) / y_range;
            sy = ((temp_x - cal_x_min) * 239) / x_range;
            break;
            
        case 4: /* Invert X only */
            sx = 319 - ((temp_x - cal_x_min) * 319) / x_range;
            sy = ((temp_y - cal_y_min) * 239) / y_range;
            break;
            
        case 5: /* Invert Y only */
            sx = ((temp_x - cal_x_min) * 319) / x_range;
            sy = 239 - ((temp_y - cal_y_min) * 239) / y_range;
            break;
            
        case 6: /* Swap X-Y + Invert Y */
            sx = ((temp_y - cal_y_min) * 319) / y_range;
            sy = 239 - ((temp_x - cal_x_min) * 239) / x_range;
            break;
            
        case 7: /* Swap X-Y + Invert both */
            sx = 319 - ((temp_y - cal_y_min) * 319) / y_range;
            sy = 239 - ((temp_x - cal_x_min) * 239) / x_range;
            break;
            
        default:
            sx = ((temp_x - cal_x_min) * 319) / x_range;
            sy = ((temp_y - cal_y_min) * 239) / y_range;
            break;
    }
    
    /* Limitlər içində saxla */
    if (sx < 0) sx = 0;
    if (sx > 319) sx = 319;
    if (sy < 0) sy = 0;
    if (sy > 239) sy = 239;
    
    *screen_x = (uint16_t)sx;
    *screen_y = (uint16_t)sy;
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
    /* DÜZƏLİŞ: Yalnız Mode 1 istifadə olunur, digər modlar silinib */
    coord_mode = 1;  /* Həmişə Mode 1 */
    printf("Touch coordinate mode fixed to: 1\r\n");
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
 * @brief Self-test: read raw X/Y samples ignoring IRQ and print them
 * @param samples: number of samples to read and print
 *
 * Use this when IRQ may be unreliable to check if the controller responds.
 */
void XPT2046_SelfTest(uint8_t samples)
{
    if (samples == 0) return;

    printf("XPT2046 SelfTest: reading %u samples (ignoring IRQ)\r\n", samples);

    for (uint8_t i = 0; i < samples; i++) {
        /* Start transfer */
        HAL_GPIO_WritePin(TP_CS_GPIO_Port, TP_CS_Pin, GPIO_PIN_RESET);
        tp_delay();

        uint16_t rx = tp_read12(XPT2046_CMD_READ_X);
        uint16_t ry = tp_read12(XPT2046_CMD_READ_Y);

        HAL_GPIO_WritePin(TP_CS_GPIO_Port, TP_CS_Pin, GPIO_PIN_SET);

        printf("SelfTest %u: raw_x=%u raw_y=%u\r\n", i, rx, ry);
        HAL_Delay(20);
    }

    printf("XPT2046 SelfTest done\r\n");
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

/* =============== AVTOMATİK DÜYMƏ KALİBRASİYA SİSTEMİ =============== */
/*
 * Bu sistem düyməyə toxunanda avtomatik olaraq kalibrasiya edir.
 * 
 * NECƏ İŞLƏYİR:
 * 1. Düymələr qeydiyyata alınır (button x, y, w, h)
 * 2. İstifadəçi ekrana toxunduqda, sistem ən yaxın düyməni tapır
 * 3. Toxunuşun düymə mərkəzindən fərqini (offset) hesablayır
 * 4. Bir neçə toxunuşdan sonra orta offset təyin edilir
 * 5. Bu offset bütün gələcək toxunuşlara tətbiq edilir
 * 
 * ÜSTÜNLÜKLƏR:
 * - İstifadəçi düyməyə toxunmağa çalışdıqca sistem özü öyrənir
 * - 3-nöqtəli kalibrasiya lazım deyil
 * - Real-time düzəliş
 */

/* Qlobal avtomatik kalibrasiya dəyişənləri */
static AutoCalibration_t g_auto_cal = {
    .offset_x = 0,
    .offset_y = 0,
    .sum_offset_x = 0,
    .sum_offset_y = 0,
    .sample_count = 0,
    .is_calibrated = 0,
    .learning_mode = 1,  /* Öyrənmə rejimi default olaraq aktiv */
    .last_button_id = 0,
    .min_samples = AUTO_CAL_MIN_SAMPLES,
    .proximity_threshold = AUTO_CAL_PROXIMITY,
    .total_touches = 0,
    .matched_touches = 0
};

/* Qeydiyyata alınmış düymələr */
static AutoCalButton_t g_buttons[AUTO_CAL_MAX_BUTTONS];
static uint8_t g_button_count = 0;

/**
 * @brief Avtomatik kalibrasiya sistemini başlat
 */
void XPT2046_AutoCal_Init(void)
{
    g_auto_cal.offset_x = 0;
    g_auto_cal.offset_y = 0;
    g_auto_cal.sum_offset_x = 0;
    g_auto_cal.sum_offset_y = 0;
    g_auto_cal.sample_count = 0;
    g_auto_cal.is_calibrated = 0;
    g_auto_cal.learning_mode = 1;
    g_auto_cal.last_button_id = 0;
    g_auto_cal.min_samples = AUTO_CAL_MIN_SAMPLES;
    g_auto_cal.proximity_threshold = AUTO_CAL_PROXIMITY;
    g_auto_cal.total_touches = 0;
    g_auto_cal.matched_touches = 0;
    
    g_button_count = 0;
    
    printf("AutoCal: System initialized, learning mode ON\r\n");
}

/**
 * @brief Düymə əlavə et (kalibrasiya üçün)
 */
uint8_t XPT2046_AutoCal_RegisterButton(uint16_t x, uint16_t y, 
                                        uint16_t w, uint16_t h,
                                        const char* name, uint8_t id)
{
    if (g_button_count >= AUTO_CAL_MAX_BUTTONS) {
        printf("AutoCal: Max buttons reached!\r\n");
        return 0;
    }
    
    /* Eyni ID ilə düymə varsa, yeniləyirik */
    for (uint8_t i = 0; i < g_button_count; i++) {
        if (g_buttons[i].id == id) {
            g_buttons[i].x = x;
            g_buttons[i].y = y;
            g_buttons[i].w = w;
            g_buttons[i].h = h;
            g_buttons[i].name = name;
            return 1;
        }
    }
    
    /* Yeni düymə əlavə et */
    g_buttons[g_button_count].x = x;
    g_buttons[g_button_count].y = y;
    g_buttons[g_button_count].w = w;
    g_buttons[g_button_count].h = h;
    g_buttons[g_button_count].name = name;
    g_buttons[g_button_count].id = id;
    g_button_count++;
    
    return 1;
}

/**
 * @brief Bütün düymələri sil
 */
void XPT2046_AutoCal_ClearButtons(void)
{
    g_button_count = 0;
    printf("AutoCal: All buttons cleared\r\n");
}

/**
 * @brief Integer square root - Newton's method
 * @param n: Kök hesablanacaq ədəd
 * @return n-in tam hissəli kökü
 */
static uint16_t isqrt(uint32_t n)
{
    if (n == 0) return 0;
    
    uint32_t x = n;
    uint32_t y = (x + 1) / 2;
    
    while (y < x) {
        x = y;
        y = (x + n / x) / 2;
    }
    
    return (uint16_t)x;
}

/**
 * @brief Ən yaxın düyməni tap
 * @param screen_x, screen_y: Ekran koordinatları
 * @param btn_id: Tapılan düymənin ID-si
 * @param distance: Düyməyə məsafə
 * @return 1 düymə tapıldı, 0 tapılmadı
 * 
 * KRİTİK DÜZƏLİŞ: Məsafə hesablaması optimallaşdırıldı
 * Əvvəlki while loop çox yavaş idi, indi Newton's method istifadə edilir
 */
uint8_t XPT2046_AutoCal_FindNearestButton(uint16_t screen_x, uint16_t screen_y,
                                           uint8_t *btn_id, uint16_t *distance)
{
    if (g_button_count == 0) {
        return 0;
    }
    
    uint32_t min_dist_sq = 0xFFFFFFFF;  /* Minimum məsafənin kvadratı */
    uint8_t nearest_id = 0;
    uint8_t found = 0;
    
    for (uint8_t i = 0; i < g_button_count; i++) {
        /* Düymənin mərkəzini hesabla */
        uint16_t btn_cx = g_buttons[i].x + g_buttons[i].w / 2;
        uint16_t btn_cy = g_buttons[i].y + g_buttons[i].h / 2;
        
        /* Məsafəni hesabla */
        int16_t dx = (int16_t)screen_x - (int16_t)btn_cx;
        int16_t dy = (int16_t)screen_y - (int16_t)btn_cy;
        
        /* Euclidean distance squared - kök hesablamadan müqayisə */
        uint32_t dist_sq = (uint32_t)(dx * dx) + (uint32_t)(dy * dy);
        
        /* Ən kiçik məsafəni tap */
        if (dist_sq < min_dist_sq) {
            min_dist_sq = dist_sq;
            nearest_id = g_buttons[i].id;
            found = 1;
        }
    }
    
    /* Yalnız tapıldıqda nəticəni qaytar */
    if (found) {
        if (btn_id) *btn_id = nearest_id;
        if (distance) *distance = isqrt(min_dist_sq);  /* Optimallaşdırılmış kök hesablaması */
    }
    
    return found;
}

/**
 * @brief Düymə daxilindəmi yoxla
 */
static uint8_t IsInsideButton(uint16_t x, uint16_t y, AutoCalButton_t* btn)
{
    return (x >= btn->x && x <= btn->x + btn->w &&
            y >= btn->y && y <= btn->y + btn->h);
}

/**
 * @brief Düyməni ID-yə görə tap
 */
static AutoCalButton_t* FindButtonById(uint8_t id)
{
    for (uint8_t i = 0; i < g_button_count; i++) {
        if (g_buttons[i].id == id) {
            return &g_buttons[i];
        }
    }
    return NULL;
}

/**
 * @brief Toxunuşu işlə və avtomatik kalibrasiya et
 */
uint8_t XPT2046_AutoCal_ProcessTouch(uint16_t raw_x, uint16_t raw_y,
                                      uint16_t *screen_x, uint16_t *screen_y)
{
    if (!screen_x || !screen_y) return 0;
    
    g_auto_cal.total_touches++;
    
    /* Əvvəlcə standart çevirmə */
    uint16_t sx, sy;
    XPT2046_ConvertToScreen(raw_x, raw_y, &sx, &sy);
    
    /* Offset tətbiq et */
    int16_t corrected_x = (int16_t)sx + g_auto_cal.offset_x;
    int16_t corrected_y = (int16_t)sy + g_auto_cal.offset_y;
    
    /* Limitlər */
    if (corrected_x < 0) corrected_x = 0;
    if (corrected_x > 319) corrected_x = 319;
    if (corrected_y < 0) corrected_y = 0;
    if (corrected_y > 239) corrected_y = 239;
    
    *screen_x = (uint16_t)corrected_x;
    *screen_y = (uint16_t)corrected_y;
    
    /* Öyrənmə rejimində deyilsə, sadəcə düzəldilmiş koordinatları qaytar */
    if (!g_auto_cal.learning_mode) {
        /* Ən yaxın düyməni tap (qayıdış dəyəri üçün) */
        uint8_t btn_id = 0;
        uint16_t dist = 0;
        if (XPT2046_AutoCal_FindNearestButton(*screen_x, *screen_y, &btn_id, &dist)) {
            if (dist <= g_auto_cal.proximity_threshold) {
                return btn_id;
            }
        }
        return 0;
    }
    
    /* ========== ÖYRƏNİŞ REJİMİ ========== */
    
    /* Düzəldilməmiş koordinatla ən yaxın düyməni tap */
    uint8_t btn_id = 0;
    uint16_t dist = 0;
    
    if (!XPT2046_AutoCal_FindNearestButton(sx, sy, &btn_id, &dist)) {
        /* Düymə tapılmadı */
        return 0;
    }
    
    /* Yaxınlıq həddini yoxla */
    if (dist > g_auto_cal.proximity_threshold) {
        /* Çox uzaq, öyrənməyə daxil etmə */
        return 0;
    }
    
    g_auto_cal.matched_touches++;
    g_auto_cal.last_button_id = btn_id;
    
    /* Düyməni tap */
    AutoCalButton_t* btn = FindButtonById(btn_id);
    if (!btn) return 0;
    
    /* Düymənin mərkəzi */
    uint16_t btn_cx = btn->x + btn->w / 2;
    uint16_t btn_cy = btn->y + btn->h / 2;
    
    /* Offset hesabla: fərq = hədəf - faktik */
    int16_t delta_x = (int16_t)btn_cx - (int16_t)sx;
    int16_t delta_y = (int16_t)btn_cy - (int16_t)sy;
    
    /* Çox böyük offset-ləri rədd et */
    if (delta_x > AUTO_CAL_MAX_OFFSET || delta_x < -AUTO_CAL_MAX_OFFSET ||
        delta_y > AUTO_CAL_MAX_OFFSET || delta_y < -AUTO_CAL_MAX_OFFSET) {
        printf("AutoCal: Offset too large (%d,%d), ignoring\r\n", delta_x, delta_y);
        return btn_id;
    }
    
    /* Kumulativ offset-ə əlavə et */
    g_auto_cal.sum_offset_x += delta_x;
    g_auto_cal.sum_offset_y += delta_y;
    g_auto_cal.sample_count++;
    
    printf("AutoCal: Button '%s' (id=%d), delta(%d,%d), samples=%d\r\n",
           btn->name ? btn->name : "?", btn_id, delta_x, delta_y, g_auto_cal.sample_count);
    
    /* Minimum nümunə sayına çatdıqda kalibrasiya et */
    if (g_auto_cal.sample_count >= g_auto_cal.min_samples) {
        /* Orta offset hesabla */
        g_auto_cal.offset_x = (int16_t)(g_auto_cal.sum_offset_x / g_auto_cal.sample_count);
        g_auto_cal.offset_y = (int16_t)(g_auto_cal.sum_offset_y / g_auto_cal.sample_count);
        g_auto_cal.is_calibrated = 1;
        
        printf("AutoCal: CALIBRATED! Offset = (%d, %d) from %d samples\r\n",
               g_auto_cal.offset_x, g_auto_cal.offset_y, g_auto_cal.sample_count);
        
        /* Düzəldilmiş koordinatları yenilə */
        corrected_x = (int16_t)sx + g_auto_cal.offset_x;
        corrected_y = (int16_t)sy + g_auto_cal.offset_y;
        
        if (corrected_x < 0) corrected_x = 0;
        if (corrected_x > 319) corrected_x = 319;
        if (corrected_y < 0) corrected_y = 0;
        if (corrected_y > 239) corrected_y = 239;
        
        *screen_x = (uint16_t)corrected_x;
        *screen_y = (uint16_t)corrected_y;
    }
    
    return btn_id;
}

/**
 * @brief Öyrənmə rejimini aktiv/deaktiv et
 */
void XPT2046_AutoCal_SetLearning(uint8_t enable)
{
    g_auto_cal.learning_mode = enable;
    printf("AutoCal: Learning mode %s\r\n", enable ? "ON" : "OFF");
}

/**
 * @brief Kalibrasiya olunubmu yoxla
 */
uint8_t XPT2046_AutoCal_IsCalibrated(void)
{
    return g_auto_cal.is_calibrated;
}

/**
 * @brief Kalibrasiyani sıfırla
 */
void XPT2046_AutoCal_Reset(void)
{
    g_auto_cal.offset_x = 0;
    g_auto_cal.offset_y = 0;
    g_auto_cal.sum_offset_x = 0;
    g_auto_cal.sum_offset_y = 0;
    g_auto_cal.sample_count = 0;
    g_auto_cal.is_calibrated = 0;
    g_auto_cal.total_touches = 0;
    g_auto_cal.matched_touches = 0;
    
    printf("AutoCal: Reset complete\r\n");
}

/**
 * @brief Offset dəyərlərini al
 */
void XPT2046_AutoCal_GetOffset(int16_t *offset_x, int16_t *offset_y)
{
    if (offset_x) *offset_x = g_auto_cal.offset_x;
    if (offset_y) *offset_y = g_auto_cal.offset_y;
}

/**
 * @brief Offset dəyərlərini manual təyin et
 */
void XPT2046_AutoCal_SetOffset(int16_t offset_x, int16_t offset_y)
{
    g_auto_cal.offset_x = offset_x;
    g_auto_cal.offset_y = offset_y;
    g_auto_cal.is_calibrated = 1;
    
    printf("AutoCal: Offset manually set to (%d, %d)\r\n", offset_x, offset_y);
}

/**
 * @brief Statistikanı al
 */
void XPT2046_AutoCal_GetStats(uint32_t *total, uint32_t *matched, uint16_t *samples)
{
    if (total) *total = g_auto_cal.total_touches;
    if (matched) *matched = g_auto_cal.matched_touches;
    if (samples) *samples = g_auto_cal.sample_count;
}

/**
 * @brief Düzəldilmiş ekran koordinatlarını al
 */
void XPT2046_AutoCal_GetCorrectedCoords(uint16_t raw_x, uint16_t raw_y,
                                         uint16_t *screen_x, uint16_t *screen_y)
{
    if (!screen_x || !screen_y) return;
    
    /* Standart çevirmə */
    uint16_t sx, sy;
    XPT2046_ConvertToScreen(raw_x, raw_y, &sx, &sy);
    
    /* Offset tətbiq et */
    int16_t corrected_x = (int16_t)sx + g_auto_cal.offset_x;
    int16_t corrected_y = (int16_t)sy + g_auto_cal.offset_y;
    
    /* Limitlər */
    if (corrected_x < 0) corrected_x = 0;
    if (corrected_x > 319) corrected_x = 319;
    if (corrected_y < 0) corrected_y = 0;
    if (corrected_y > 239) corrected_y = 239;
    
    *screen_x = (uint16_t)corrected_x;
    *screen_y = (uint16_t)corrected_y;
}

/**
 * @brief Proximity threshold təyin et
 */
void XPT2046_AutoCal_SetProximity(uint16_t threshold)
{
    g_auto_cal.proximity_threshold = threshold;
    printf("AutoCal: Proximity threshold set to %d pixels\r\n", threshold);
}

/**
 * @brief Debug məlumatlarını çap et
 */
void XPT2046_AutoCal_PrintDebug(void)
{
    printf("\r\n=== AUTO CALIBRATION DEBUG ===\r\n");
    printf("Status: %s\r\n", g_auto_cal.is_calibrated ? "CALIBRATED" : "NOT CALIBRATED");
    printf("Learning: %s\r\n", g_auto_cal.learning_mode ? "ON" : "OFF");
    printf("Offset: (%d, %d)\r\n", g_auto_cal.offset_x, g_auto_cal.offset_y);
    printf("Samples: %d/%d\r\n", g_auto_cal.sample_count, g_auto_cal.min_samples);
    printf("Touches: total=%lu, matched=%lu\r\n", g_auto_cal.total_touches, g_auto_cal.matched_touches);
    printf("Buttons registered: %d\r\n", g_button_count);
    
    for (uint8_t i = 0; i < g_button_count; i++) {
        printf("  [%d] '%s': (%d,%d) %dx%d\r\n", 
               g_buttons[i].id,
               g_buttons[i].name ? g_buttons[i].name : "?",
               g_buttons[i].x, g_buttons[i].y,
               g_buttons[i].w, g_buttons[i].h);
    }
    printf("==============================\r\n\n");
}
