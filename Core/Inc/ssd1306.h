#ifndef SSD1306_H
#define SSD1306_H

#include "main.h"

HAL_StatusTypeDef ssd1306_init(void);
void              ssd1306_clear(void);
HAL_StatusTypeDef ssd1306_update(void);

void ssd1306_set_pixel(uint8_t x, uint8_t y, uint8_t color);
void ssd1306_draw_char(uint8_t x, uint8_t y, char c, uint8_t scale);
void ssd1306_draw_string(uint8_t x, uint8_t y, const char *s, uint8_t scale);

#endif /* SSD1306_H */
