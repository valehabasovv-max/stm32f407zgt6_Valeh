#include "XPT2046.h"

/* Calibration variables */
static uint16_t cal_x_min = 200;
static uint16_t cal_x_max = 3800;
static uint16_t cal_y_min = 200;
static uint16_t cal_y_max = 3800;

/* Coordinate transformation mode */
static uint8_t coord_mode = 0; // 0=normal, 1=swap+invert, 2=invert only, 3=swap only

/* Pin tərifləri */
#define TP_CS_GPIO_Port    XPT2046_CS_PORT
#define TP_CS_Pin          XPT2046_CS_PIN
#define TP_IRQ_GPIO_Port   XPT2046_IRQ_PORT
#define TP_IRQ_Pin         XPT2046_IRQ_PIN
#define TP_SCK_GPIO_Port   GPIOB
#define TP_SCK_Pin         GPIO_PIN_1
#define TP_MISO_GPIO_Port  GPIOF
#define TP_MISO_Pin        GPIO_PIN_8
#define TP_MOSI_GPIO_Port  GPIOF
#define TP_MOSI_Pin        GPIO_PIN_9

/* Kiçik "delay" — SCK üçün stabil kənar */
static inline void tp_delay(void){ for(volatile int i=0;i<30;i++) __NOP(); }

/* Bit-bang SPI xfer (MSB-first) */
static uint8_t tp_xfer(uint8_t d)
{
    uint8_t r = 0;
    for(int i=7;i>=0;i--)
    {
        /* MOSI */
        HAL_GPIO_WritePin(TP_MOSI_GPIO_Port, TP_MOSI_Pin, (d & (1U<<i)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        tp_delay();
        /* SCK ↑ */
        HAL_GPIO_WritePin(TP_SCK_GPIO_Port, TP_SCK_Pin, GPIO_PIN_SET);
        tp_delay();
        /* MISO oxu */
        r <<= 1;
        if (HAL_GPIO_ReadPin(TP_MISO_GPIO_Port, TP_MISO_Pin) == GPIO_PIN_SET) r |= 1;
        /* SCK ↓ */
        HAL_GPIO_WritePin(TP_SCK_GPIO_Port, TP_SCK_Pin, GPIO_PIN_RESET);
        tp_delay();
    }
    return r;
}

/* 12-bit oxu (XPT2046-də: komandadan sonra 16 bit gəlir; üst 12-si məlumat) */
static uint16_t tp_read12(uint8_t cmd)
{
    (void)tp_xfer(cmd);             /* komanda */
    uint8_t h = tp_xfer(0x00);
    uint8_t l = tp_xfer(0x00);
    /* 12-bit: h: [11:4], l: [3:0] üst nibble */
    return ( ( (uint16_t)h << 5 ) | ( (uint16_t)l >> 3 ) ) & 0x0FFF;
}

/* GPIO-ları hazırla */
void XPT2046_Init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};

    /* CS, SCK, MOSI: çıxış */
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;

    g.Pin = TP_CS_Pin;   HAL_GPIO_Init(TP_CS_GPIO_Port, &g);
    g.Pin = TP_SCK_Pin;  HAL_GPIO_Init(TP_SCK_GPIO_Port, &g);
    g.Pin = TP_MOSI_Pin; HAL_GPIO_Init(TP_MOSI_GPIO_Port, &g);

    /* MISO: giriş */
    g.Mode = GPIO_MODE_INPUT;
    g.Pin  = TP_MISO_Pin;
    HAL_GPIO_Init(TP_MISO_GPIO_Port, &g);

    /* IRQ: giriş-pullup (modulda açıq kollektor olur) */
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLUP;
    g.Pin  = TP_IRQ_Pin;
    HAL_GPIO_Init(TP_IRQ_GPIO_Port, &g);

    /* İlkin səviyyələr */
    HAL_GPIO_WritePin(TP_CS_GPIO_Port,  TP_CS_Pin,  GPIO_PIN_SET);   /* CS=HIGH (de-assert) */
    HAL_GPIO_WritePin(TP_SCK_GPIO_Port, TP_SCK_Pin, GPIO_PIN_RESET); /* SCK=LOW */
    HAL_GPIO_WritePin(TP_MOSI_GPIO_Port,TP_MOSI_Pin,GPIO_PIN_RESET);
}

/* 0 = toxunmur, 1 = toxunur */
uint8_t XPT2046_IsTouched(void)
{
    /* IRQ pin LOW olmalıdır (toxunuşda aşağı çəkilir) */
    return (HAL_GPIO_ReadPin(TP_IRQ_GPIO_Port, TP_IRQ_Pin) == GPIO_PIN_RESET) ? 1U : 0U;
}

/* X və Y raw oxu — 4 nümunə orta qiymət; 1=ok */
uint8_t XPT2046_ReadRaw(uint16_t *x, uint16_t *y)
{
    if (!x || !y) return 0;
    if (!XPT2046_IsTouched()) return 0;

    uint32_t xs=0, ys=0; int n=4;
    for(int i=0;i<n;i++)
    {
        /* CS ↓ */
        HAL_GPIO_WritePin(TP_CS_GPIO_Port, TP_CS_Pin, GPIO_PIN_RESET);
        tp_delay();

        /* Komandalar: 0xD0 = X, 0x90 = Y (12-bit, differential, PD=0b011) */
        uint16_t rx = tp_read12(0xD0);
        uint16_t ry = tp_read12(0x90);

        /* CS ↑ */
        HAL_GPIO_WritePin(TP_CS_GPIO_Port, TP_CS_Pin, GPIO_PIN_SET);

        xs += rx; ys += ry;
    }
    *x = (uint16_t)(xs / (uint32_t)n);
    *y = (uint16_t)(ys / (uint32_t)n);
    return 1;
}

/* Touch varsa koordinatları qaytar */
uint8_t XPT2046_GetCoordinates(uint16_t *x, uint16_t *y)
{
    if (XPT2046_IsTouched()) {
        return XPT2046_ReadRaw(x, y);
    }
    return 0;
}

/* Z pressure oxu */
uint16_t XPT2046_ReadZ(void)
{
    /* CS ↓ */
    HAL_GPIO_WritePin(TP_CS_GPIO_Port, TP_CS_Pin, GPIO_PIN_RESET);
    tp_delay();
    
    uint16_t z = tp_read12(XPT2046_CMD_READ_Z1);
    
    /* CS ↑ */
    HAL_GPIO_WritePin(TP_CS_GPIO_Port, TP_CS_Pin, GPIO_PIN_SET);
    
    return z;
}

/* Raw koordinatları ekran koordinatlarına çevir */
void XPT2046_ConvertToScreen(uint16_t raw_x, uint16_t raw_y, uint16_t *screen_x, uint16_t *screen_y)
{
    if (!screen_x || !screen_y) return;
    
    uint16_t temp_x = raw_x;
    uint16_t temp_y = raw_y;
    
    /* Apply calibration */
    if (temp_x < cal_x_min) temp_x = cal_x_min;
    if (temp_x > cal_x_max) temp_x = cal_x_max;
    if (temp_y < cal_y_min) temp_y = cal_y_min;
    if (temp_y > cal_y_max) temp_y = cal_y_max;
    
    /* Convert to screen coordinates based on transformation mode */
    switch (coord_mode) {
        case 0: // Normal mapping
            *screen_x = ((temp_x - cal_x_min) * 319) / (cal_x_max - cal_x_min);
            *screen_y = ((temp_y - cal_y_min) * 239) / (cal_y_max - cal_y_min);
            break;
            
        case 1: // Swap X and Y, then invert X
            *screen_x = 319 - ((temp_y - cal_y_min) * 319) / (cal_y_max - cal_y_min);
            *screen_y = ((temp_x - cal_x_min) * 239) / (cal_x_max - cal_x_min);
            break;
            
        case 2: // Invert both X and Y
            *screen_x = 319 - ((temp_x - cal_x_min) * 319) / (cal_x_max - cal_x_min);
            *screen_y = 239 - ((temp_y - cal_y_min) * 239) / (cal_y_max - cal_y_min);
            break;
            
        case 3: // Swap X and Y only
            *screen_x = ((temp_y - cal_y_min) * 319) / (cal_y_max - cal_y_min);
            *screen_y = ((temp_x - cal_x_min) * 239) / (cal_x_max - cal_x_min);
            break;
            
        default:
            *screen_x = ((temp_x - cal_x_min) * 319) / (cal_x_max - cal_x_min);
            *screen_y = ((temp_y - cal_y_min) * 239) / (cal_y_max - cal_y_min);
            break;
    }
    
    /* Ensure coordinates are within bounds */
    if (*screen_x > 319) *screen_x = 319;
    if (*screen_y > 239) *screen_y = 239;
}

/* Touch varsa ekran koordinatlarını qaytar */
uint8_t XPT2046_GetScreenCoordinates(uint16_t *screen_x, uint16_t *screen_y)
{
    uint16_t raw_x, raw_y;
    
    if (XPT2046_GetCoordinates(&raw_x, &raw_y)) {
        XPT2046_ConvertToScreen(raw_x, raw_y, screen_x, screen_y);
        return 1;
    }
    return 0;
}

/* Calibration function */
void XPT2046_Calibrate(void)
{
    // Calibration function - can be implemented later
}

/* Set calibration values */
void XPT2046_SetCalibration(uint16_t x_min, uint16_t x_max, uint16_t y_min, uint16_t y_max)
{
    cal_x_min = x_min;
    cal_x_max = x_max;
    cal_y_min = y_min;
    cal_y_max = y_max;
}

/* Get calibration values */
void XPT2046_GetCalibration(uint16_t *x_min, uint16_t *x_max, uint16_t *y_min, uint16_t *y_max)
{
    if (x_min) *x_min = cal_x_min;
    if (x_max) *x_max = cal_x_max;
    if (y_min) *y_min = cal_y_min;
    if (y_max) *y_max = cal_y_max;
}

/* Set coordinate transformation mode */
void XPT2046_SetCoordMode(uint8_t mode)
{
    coord_mode = mode;
}

/* Get coordinate transformation mode */
uint8_t XPT2046_GetCoordMode(void)
{
    return coord_mode;
}
