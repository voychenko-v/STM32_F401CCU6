#include "main.h"
#include <string.h>
#include "font5x7.h"
#include "ssd1306.h"

extern I2C_HandleTypeDef hi2c1;
#define OLED_ADDR (0x3C << 1)

#define OLED_CTRL_CMD   0x00
#define OLED_CTRL_DATA  0x40

#define OLED_W      128
#define OLED_H      64
#define OLED_PAGES  (OLED_H / 8)

#define OLED_COLOR_BLACK  0
#define OLED_COLOR_WHITE  1

#define FONT_W        5
#define FONT_H        7
#define FONT_FIRST    0
#define FONT_LAST     126

static uint8_t oled_buf[OLED_W * OLED_PAGES];

static HAL_StatusTypeDef oled_write_cmds(const uint8_t *cmds, uint16_t len)
{
    // control byte (0x00) іде першим байтом тієї ж транзакції
    return HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR, OLED_CTRL_CMD,
                             I2C_MEMADD_SIZE_8BIT,
                             (uint8_t*)cmds, len, 50);
}


HAL_StatusTypeDef ssd1306_init(void)
{
    static const uint8_t init_cmds[] = {
        0xAE,             // display off
        0xD5, 0x80,       // clock divide / osc freq
        0xA8, 0x3F,       // multiplex 64
        0xD3, 0x00,       // display offset
        0x40,             // start line = 0
        0x8D, 0x14,       // charge pump ON
        0x20, 0x00,       // horizontal addressing mode
        0xA1,             // segment remap (X mirror)
        0xC8,             // COM scan direction (Y mirror)
        0xDA, 0x12,       // COM pins config (для 128x64)
        0x81, 0xCF,       // contrast
        0xD9, 0xF1,       // pre-charge
        0xDB, 0x40,       // VCOMH
        0xA4,             // показувати вміст RAM
        0xA6,             // нормальний режим
        0xAF              // display on
    };

    HAL_Delay(100);
    return oled_write_cmds(init_cmds, sizeof(init_cmds));
}

void ssd1306_clear(void)
{
    memset(oled_buf, 0, sizeof(oled_buf));
}


HAL_StatusTypeDef ssd1306_update(void)
{
    static const uint8_t range[] = { 0x21, 0, 127,
                                     0x22, 0, 7 };

    HAL_StatusTypeDef st = oled_write_cmds(range, sizeof(range));
    if (st != HAL_OK) return st;

    return HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR, OLED_CTRL_DATA,
                             I2C_MEMADD_SIZE_8BIT,
                             oled_buf, sizeof(oled_buf), 100);
}

void ssd1306_set_pixel(uint8_t x, uint8_t y, uint8_t color)
{
    if (x >= OLED_W || y >= OLED_H) return;   // захист від виходу за межі масиву

    uint16_t index = x + (y / 8) * OLED_W;    // байт у буфері
    uint8_t  mask  = 1 << (y % 8);            // біт у байті (LSB зверху)

    if (color)
        oled_buf[index] |= mask;              // увімкнути піксель
    else
        oled_buf[index] &= ~mask;             // вимкнути піксель
}


void ssd1306_draw_char(uint8_t x, uint8_t y, char c, uint8_t scale)
{
    if (scale == 0) scale = 1;

    uint8_t code = (uint8_t)c;
    if (code < FONT_FIRST || code > FONT_LAST) code = '?';

    const uint8_t *glyph = &font5x7[code * 5];

    for (uint8_t col = 0; col < FONT_W; col++)
    {
        uint8_t bits = glyph[col];

        for (uint8_t row = 0; row < FONT_H; row++)
        {
            if (bits & (1 << row))
            {

                for (uint8_t dx = 0; dx < scale; dx++)
                    for (uint8_t dy = 0; dy < scale; dy++)
                        ssd1306_set_pixel(x + col * scale + dx,
                                          y + row * scale + dy, 1);
            }
        }
    }
}

void ssd1306_draw_string(uint8_t x, uint8_t y, const char *s, uint8_t scale)
{
    while (*s)
    {
        ssd1306_draw_char(x, y, *s, scale);
        x += (FONT_W + 1) * scale;     // 5 колонок символа + 1 колонка відступу
        s++;
    }
}

