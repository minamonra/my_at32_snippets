#include "lcd_draw_u8g2.h"
#include "common.h"
#include "st7789.h"
#include "u8g2.h"
#include <string.h>

const uint8_t* u8g2_font_get_glyph_data(u8g2_t* u8g2, uint16_t encoding);
int16_t        u8g2_font_decode_glyph(u8g2_t* u8g2, const uint8_t* glyph_data);

u8g2_t                u8g2_display;
static const uint8_t* current_font = NULL;

// Цвета текущей строки (RGB565)
static uint16_t current_text_color;
static uint16_t current_bg_color;

// Кэшированные параметры шрифта
static uint8_t cached_font_height  = 20;
static uint8_t cached_font_ascent  = 16;
static uint8_t cached_font_descent = 4;

// ====================================================================
// RAM-БУФЕР ОДНОГО СИМВОЛА
// ====================================================================
// Идея оптимизации: раньше каждый штрих глифа (u8g2_DrawHVLine) сразу слался
// в SPI отдельной транзакцией — установка окна (CASET/RASET/RAMWR) на каждый
// маленький отрезок давала огромный оверхед (на 1-3 пикселя данных — 9+ байт
// служебных команд без DMA). Теперь весь символ (фон + сама буква) сначала
// собирается целиком в этом буфере в RAM, а затем ОДНИМ непрерывным DMA-блитом
// (lcd_blit_buffer) выгружается на экран — всего одна установка окна и один
// SPI-поток на весь символ, независимо от количества штрихов в глифе.
//
// Размер буфера подобран под используемые шрифты Terminus (макс. высота 32px).
// Если подключите шрифт крупнее — увеличьте константы (расход RAM = W*H*2 байта).
#define GLYPH_BUF_MAX_W 48
#define GLYPH_BUF_MAX_H 48
static uint16_t glyph_buf[GLYPH_BUF_MAX_W * GLYPH_BUF_MAX_H];

// Координаты левого-верхнего угла буфера в системе координат экрана
// (нужны, чтобы пересчитать абсолютные x,y из колбэка u8g2 в локальные x,y буфера)
static int16_t  glyph_origin_x = 0;
static int16_t  glyph_origin_y = 0;
static uint16_t glyph_buf_w    = 0;
static uint16_t glyph_buf_h    = 0;

// ====================================================================
// ОБНОВЛЕНИЕ КЭША ПАРАМЕТРОВ ШРИФТА
// ====================================================================
static void update_font_cache(void) {
  if (!current_font) return;

  // Смещения в заголовке шрифта U8g2 (23 байта)
  cached_font_height  = current_font[10];  // max_char_height
  cached_font_ascent  = current_font[13];  // ascent_A
  cached_font_descent = current_font[14];  // descent_g

  if (cached_font_height == 0) cached_font_height = 20;
  if (cached_font_ascent == 0) cached_font_ascent = 16;
  if (cached_font_descent == 0) cached_font_descent = 4;
}

// ====================================================================
// ФУНКЦИИ ПОЛУЧЕНИЯ ПАРАМЕТРОВ ШРИФТА
// ====================================================================
uint8_t lcd_u8g2_get_font_height(void) { return cached_font_height; }
uint8_t lcd_u8g2_get_font_ascent(void) { return cached_font_ascent; }
uint8_t lcd_u8g2_get_font_descent(void) { return cached_font_descent; }

// ====================================================================
// КОЛБЭК ОТРИСОВКИ ШТРИХОВ ГЛИФА (вызывается u8g2 много раз на один символ)
// ====================================================================
// ВАЖНО: раньше эта функция сама дёргала SPI (lcd_setwindows + побайтовая
// запись в цикле) — на каждый вызов. Теперь она НИЧЕГО не шлёт по SPI,
// а просто пишет пиксели штриха в локальный RAM-буфер glyph_buf[].
// Реальная пересылка на экран происходит один раз на весь символ —
// см. lcd_draw_u8g2_string() и вызов lcd_blit_buffer() в конце.
void u8g2_DrawHVLine(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len, uint8_t dir) {
  if (len == 0) return;

  // Как и в оригинале: цвет 0 — это пиксель буквы, остальное пропускаем
  if (u8g2_GetDrawColor(u8g2) != 0) {
    return;
  }

  // Переводим абсолютные координаты экрана в локальные координаты буфера
  int16_t lx = (int16_t)x - glyph_origin_x;
  int16_t ly = (int16_t)y - glyph_origin_y;

  for (uint16_t i = 0; i < len; i++) {
    int16_t px = (dir == 0) ? (int16_t)(lx + i) : lx;
    int16_t py = (dir == 1) ? (int16_t)(ly + i) : ly;

    // Защита от выхода за пределы буфера (например, если глиф крупнее
    // GLYPH_BUF_MAX_W/H, либо символ находится у самого края экрана)
    if (px < 0 || py < 0 || px >= (int16_t)glyph_buf_w || py >= (int16_t)glyph_buf_h) {
      continue;
    }

    glyph_buf[(uint32_t)py * glyph_buf_w + px] = current_text_color;
  }
}

// ====================================================================
// ЗАГЛУШКИ ДЛЯ ЛИНКЕРА
// ====================================================================
uint8_t u8g2_IsIntersection(u8g2_t* u8g2, u8g2_uint_t x0, u8g2_uint_t y0, u8g2_uint_t x1, u8g2_uint_t y1) {
  return 1;
}
void     u8x8_utf8_init(u8x8_t* u8x8) { (void)u8x8; }
uint16_t u8x8_utf8_next(u8x8_t* u8x8, uint8_t b) { return b; }
uint16_t u8x8_ascii_next(u8x8_t* u8x8, uint8_t b) { return b; }
uint8_t  u8g2_GetKerning(u8g2_t* u8g2, u8g2_kerning_t* kerning, uint16_t e1, uint16_t e2) { return 0; }
uint8_t  u8g2_GetKerningByTable(u8g2_t* u8g2, const uint16_t* kt, uint16_t e1, uint16_t e2) { return 0; }

// ====================================================================
// ПУБЛИЧНЫЕ ФУНКЦИИ
// ====================================================================
void lcd_u8g2_set_font(const uint8_t* font) {
  if (font) {
    current_font = font;
    u8g2_SetFont(&u8g2_display, current_font);
    update_font_cache();
  }
}

void lcd_draw_u8g2_string(uint16_t x, uint16_t y, const char* str, uint16_t color, uint16_t bg) {
  if (!str || !current_font) return;

  current_text_color = color;
  current_bg_color   = bg;

  u8g2_SetFont(&u8g2_display, current_font);

  uint16_t    cursor_x = x;
  const char* ptr      = str;
  uint16_t    code;

  while (*ptr) {
    code = decode_utf8(&ptr);  // Ваш проверенный decode_utf8 из common.c
    if (code == 0) break;

    // Игнорируем управляющие символы Юникода
    if (code >= 0x007F && code <= 0x009F) {
      continue;
    }

    const uint8_t* glyph_data = u8g2_font_get_glyph_data(&u8g2_display, code);

    if (!glyph_data) {
      // Символа нет в шрифте — рисуем просто подложку нужной ширины.
      // Тут по-прежнему используется lcd_fillrect (одна DMA-заливка), это уже
      // и так быстро — отдельный буфер здесь не нужен.
      uint8_t space_width = cached_font_height / 4;
      if (space_width == 0) space_width = 6;

      lcd_fillrect(cursor_x, y - cached_font_ascent,
                   cursor_x + space_width - 1, y - cached_font_ascent + cached_font_height - 1,
                   bg);

      cursor_x += space_width;
      continue;
    }

    int8_t glyph_width = u8g2_GetGlyphWidth(&u8g2_display, code);
    if (glyph_width <= 0) glyph_width = 10;

    // Готовим буфер под символ: фон + позднее буква поверх.
    // Обрезаем по максимальному размеру буфера (см. комментарий у GLYPH_BUF_MAX_W/H) —
    // если реальный размер глифа больше, лишнее просто не поместится в буфер
    // (см. защиту от выхода за границы в u8g2_DrawHVLine выше).
    glyph_origin_x = cursor_x;
    glyph_origin_y = (int16_t)y - (int16_t)cached_font_ascent;

    glyph_buf_w = (glyph_width > GLYPH_BUF_MAX_W) ? GLYPH_BUF_MAX_W : (uint16_t)glyph_width;
    glyph_buf_h = (cached_font_height > GLYPH_BUF_MAX_H) ? GLYPH_BUF_MAX_H : cached_font_height;

    // Заливаем буфер цветом фона — это быстрая операция в RAM, без единого обращения к SPI
    uint32_t px_count = (uint32_t)glyph_buf_w * glyph_buf_h;
    for (uint32_t i = 0; i < px_count; i++) {
      glyph_buf[i] = bg;
    }

    // Рисуем сам глиф — u8g2 вызовет u8g2_DrawHVLine() много раз, но КАЖДЫЙ вызов
    // теперь пишет только в RAM-буфер выше, не трогая SPI вообще
    u8g2_display.font_decode.target_x = cursor_x;
    u8g2_display.font_decode.target_y = y;
    int16_t dx                        = u8g2_font_decode_glyph(&u8g2_display, glyph_data);

    // Единственная реальная передача на экран для всего символа целиком:
    // один DMA-блит вместо десятков мелких SPI-транзакций на каждый штрих
    lcd_blit_buffer((uint16_t)glyph_origin_x, (uint16_t)glyph_origin_y,
                    glyph_buf_w, glyph_buf_h, glyph_buf);

    cursor_x += (dx > 0) ? dx : glyph_width;
    if (cursor_x >= 480) break;
  }
}

// Вычисляет ширину строки в пикселях с учетом UTF-8
uint16_t lcd_u8g2_get_text_width(const char* str) {
  uint16_t    total_width = 0;
  const char* ptr         = str;
  while (*ptr != '\0') {
    uint16_t ch = decode_utf8(&ptr);
    if (ch == 0) break;
    total_width += 12;  // Базовая ширина символа для шрифта terminus_24b
  }
  return total_width;
}

// Выводит длинный текст с автоматическим переносом слов на следующую строку
// в пределах заданной ширины max_w
void lcd_draw_u8g2_string_wrapped(uint16_t x, uint16_t y, const char* str, uint16_t max_w, uint16_t color, uint16_t bg) {
  const char* current_ptr = str;
  const char* last_space  = NULL;

  char     line_buf[64];
  uint16_t line_idx  = 0;
  uint16_t current_w = 0;
  uint16_t current_y = y;

  uint8_t font_h = lcd_u8g2_get_font_height();

  while (*current_ptr != '\0') {
    const char* saved_ptr = current_ptr;
    uint16_t    ch        = decode_utf8(&current_ptr);
    if (ch == 0) break;

    uint16_t char_w = 12;  // Ширина одного символа для terminus_24b

    if (ch == ' ') {
      last_space = saved_ptr;
    }

    if (current_w + char_w > max_w) {
      if (last_space && last_space != str) {
        current_ptr = last_space + 1;
        for (int i = line_idx - 1; i >= 0; i--) {
          if (line_buf[i] == ' ') {
            line_buf[i] = '\0';
            break;
          }
        }
      } else {
        line_buf[line_idx] = '\0';
      }

      lcd_draw_u8g2_string(x, current_y, line_buf, color, bg);
      current_y += (font_h + 4);

      line_idx   = 0;
      current_w  = 0;
      last_space = NULL;

      while (*current_ptr == ' ') {
        decode_utf8(&current_ptr);
      }
      continue;
    }

    uint16_t bytes_to_copy = current_ptr - saved_ptr;
    for (uint16_t b = 0; b < bytes_to_copy; b++) {
      if (line_idx < 63) {
        line_buf[line_idx++] = saved_ptr[b];
      }
    }
    current_w += char_w;
  }

  if (line_idx > 0) {
    line_buf[line_idx] = '\0';
    lcd_draw_u8g2_string(x, current_y, line_buf, color, bg);
  }
}

// Выводит длинный текст с автоматическим переносом слов в пределах заданной ширины max_w и не более max_lines строк
void lcd_draw_u8g2_string_wrapped_line(uint16_t x, uint16_t y, const char* str, uint16_t max_w, uint16_t color, uint16_t bg, uint16_t max_lines) {
  if (max_lines == 0) return;  // Если 0 строк, сразу выходим

  const char* current_ptr = str;
  const char* last_space  = NULL;
  char        line_buf[64];
  uint16_t    line_idx    = 0;
  uint16_t    current_w   = 0;
  uint16_t    current_y   = y;
  uint16_t    lines_drawn = 0;  // Счетчик напечатанных строк
  uint8_t     font_h      = lcd_u8g2_get_font_height();

  while (*current_ptr != '\0') {
    const char* saved_ptr = current_ptr;
    uint16_t    ch        = decode_utf8(&current_ptr);
    if (ch == 0) break;

    uint16_t char_w = 12;  // Ширина одного символа для terminus_24b

    if (ch == ' ') {
      last_space = saved_ptr;
    }

    if (current_w + char_w > max_w) {
      if (last_space && last_space != str) {
        current_ptr = last_space + 1;
        for (int i = line_idx - 1; i >= 0; i--) {
          if (line_buf[i] == ' ') {
            line_buf[i] = '\0';
            break;
          }
        }
      } else {
        line_buf[line_idx] = '\0';
      }

      lcd_draw_u8g2_string(x, current_y, line_buf, color, bg);
      lines_drawn++;

      // Проверка лимита строк
      if (lines_drawn >= max_lines) {
        return;
      }

      current_y += (font_h + 4);
      line_idx   = 0;
      current_w  = 0;
      last_space = NULL;

      while (*current_ptr == ' ') {
        decode_utf8(&current_ptr);
      }
      continue;
    }

    uint16_t bytes_to_copy = current_ptr - saved_ptr;
    for (uint16_t b = 0; b < bytes_to_copy; b++) {
      if (line_idx < 63) {
        line_buf[line_idx++] = saved_ptr[b];
      }
    }
    current_w += char_w;
  }

  // Печать последней строки, если лимит еще не исчерпан
  if (line_idx > 0 && lines_drawn < max_lines) {
    line_buf[line_idx] = '\0';
    lcd_draw_u8g2_string(x, current_y, line_buf, color, bg);
  }
}