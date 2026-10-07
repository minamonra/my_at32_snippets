#ifndef __LCD_DRAW_U8G2_H__
#define __LCD_DRAW_U8G2_H__

#include <stdint.h>
#include "u8g2.h"

// Глобальный экземпляр u8g2, используемый как "виртуальный" дисплей —
// реальный вывод на ST7796 идёт через колбэк u8g2_DrawHVLine() и буфер глифа.
extern u8g2_t u8g2_display;

// Выбор текущего шрифта (u8g2 font data, например u8g2_font_terminus_24b_cyr)
void lcd_u8g2_set_font(const uint8_t* font);

// Отрисовка строки UTF-8 (поддержка кириллицы через decode_utf8() из common.c)
// x, y      — базовая точка (y — базовая линия шрифта, как в u8g2)
// color     — цвет текста (RGB565)
// bg        — цвет фона под символами (RGB565)
void lcd_draw_u8g2_string(uint16_t x, uint16_t y, const char* str, uint16_t color, uint16_t bg);

// Кэшированные параметры текущего шрифта (обновляются в lcd_u8g2_set_font())
uint8_t lcd_u8g2_get_font_height(void);
uint8_t lcd_u8g2_get_font_ascent(void);
uint8_t lcd_u8g2_get_font_descent(void);

// Вычисление ширины строки в пикселях с учетом UTF-8 кириллицы
uint16_t lcd_u8g2_get_text_width(const char* str);

// Вывод UTF-8 строки с автоматическим переносом по словам в пределах max_w
void lcd_draw_u8g2_string_wrapped(uint16_t x, uint16_t y, const char* str, uint16_t max_w, uint16_t color, uint16_t bg);
// Вывод UTF-8 строки с автоматическим переносом по словам в пределах max_w
// и ограничением по количеству строк (max_lines)
void lcd_draw_u8g2_string_wrapped_line(uint16_t x, uint16_t y, const char* str, uint16_t max_w, uint16_t color, uint16_t bg, uint16_t max_lines);

#endif  // __LCD_DRAW_U8G2_H__