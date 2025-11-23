#ifndef FONT_H
#define FONT_H

#include <stdint.h>

// Font sizes
#define FONT_SIZE_SMALL    1
#define FONT_SIZE_MEDIUM   2
#define FONT_SIZE_LARGE    3
#define FONT_SIZE_XLARGE   4

// Font types
#define FONT_TYPE_5X7      0
#define FONT_TYPE_7X10     1
#define FONT_TYPE_11X18    2
#define FONT_TYPE_16X26    3
#define FONT_TYPE_20X32    4
#define FONT_TYPE_24X32    5

// Font colors
#define FONT_COLOR_WHITE   0xFFFF
#define FONT_COLOR_BLACK   0x0000
#define FONT_COLOR_RED     0xF800
#define FONT_COLOR_GREEN   0x07E0
#define FONT_COLOR_BLUE    0x001F
#define FONT_COLOR_YELLOW  0xFFE0
#define FONT_COLOR_CYAN    0x07FF
#define FONT_COLOR_MAGENTA 0xF81F

// Font structure
typedef struct {
    uint8_t width;
    uint8_t height;
    uint8_t first_char;
    uint8_t last_char;
    const uint8_t *data;
} Font_t;

// Function prototypes
void font_init(void);
uint8_t font_get_char_width(char c, uint8_t font_type, uint8_t scale);
uint8_t font_get_string_width(const char *str, uint8_t font_type, uint8_t scale);
uint8_t font_get_height(uint8_t font_type, uint8_t scale);

// Font data declarations
extern const Font_t font_5x7;
extern const Font_t font_7x10;
extern const Font_t font_8x12;
extern const Font_t font_12x16;

#endif // FONT_H



