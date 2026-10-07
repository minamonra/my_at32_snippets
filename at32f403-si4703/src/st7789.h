#ifndef ST7789_H
#define ST7789_H

#include <stdint.h>

// Физические размеры матрицы 2.25" полосы SONGXIN LIGHT
#define TFT_WIDTH  284
#define TFT_HEIGHT 76

// Аппаратные смещения памяти CGRAM контроллера для режима 0x70
#define TFT_X_OFFSET 18
#define TFT_Y_OFFSET 82

// BGR-матрица использует формат цвета BGR565 (где старшие 5 бит отвечают за синий цвет, а младшие 5 — за красный)
#define ST7789_BLACK 0x0000
#define ST7789_WHITE 0xFFFF
#define ST7789_BLUE  0xF800
#define ST7789_GREEN 0x07E0
#define ST7789_RED   0x001F
// Основные составные цвета
#define ST7789_YELLOW  0x07FF  // Желтый (Красный + Зеленый)
#define ST7789_CYAN    0xFFE0  // Циан / Бирюзовый (Синий + Зеленый)
#define ST7789_MAGENTA 0xF81F  // Пурпурный / Маджента (Синий + Красный)
// Оттенки серого
#define ST7789_LIGHTGRAY 0xD69A  // Светло-серый
#define ST7789_GRAY      0x8410  // Серый
#define ST7789_DARKGRAY  0x4208  // Темно-серый
// Расширенная палитра
#define ST7789_ORANGE 0x053F  // Оранжевый
#define ST7789_PINK   0xCE1F  // Розовый
#define ST7789_PURPLE 0x8010  // Фиолетовый
#define ST7789_BROWN  0x1231  // Коричневый
#define ST7789_GOLD   0x06BF  // Золотой
// Темные оттенки
#define ST7789_NAVY      0x8000  // Темно-синий
#define ST7789_DARKGREEN 0x03E0  // Темно-зеленый
#define ST7789_MAROON    0x0010  // Тёмно-бордовый
#define ST7789_OLIVE     0x0410  // Оливковый

// Команды контроллера ST7789
#define ST7789_SWRST   0x01  // Программный сброс
#define ST7789_SLPOUT  0x11  // Выход из спящего режима
#define ST7789_INVOFF  0x20  // Выключение инверсии цвета
#define ST7789_INVON   0x21  // Включение инверсии цвета
#define ST7789_PORCTRL 0xB2  // Управление таймингами развертки (Porch Setting)
#define ST7789_CASET   0x2A  // Установка адреса столбцов
#define ST7789_PASET   0x2B  // Установка адреса строк
#define ST7789_RAMWR   0x2C  // Запись в видеопамять RAM
#define ST7789_MADCTL  0x36  // Контроль направления развертки матрицы
#define ST7789_COLMOD  0x3A  // Настройка интерфейса цвета
#define ST7789_DISPON  0x29  // Включение отображения дисплея

// Стабильные референсные Porch-тайминги кадровой развертки вытянутых панелей
#define ST7789_PORCH_V_BACK_FORWARD 0x0C  // Вертикальные задержки
#define ST7789_PORCH_V_IDLE         0x00  // Вертикальный режим простоя
#define ST7789_PORCH_H_BACK_FORWARD 0x33  // Горизонтальные задержки

// Функции управления дисплеем
void st7789_init(void);
void st7789_fill_screen(uint16_t color);
void st7789_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

// Прототипы функций Alientek-стиля и нового блиттера для заголовочного файла src/st7789.h
void lcd_init(void);
void lcd_clear(uint16_t color);
void lcd_setcursor(uint16_t x_pos, uint16_t y_pos);
void lcd_drawpoint(uint16_t x, uint16_t y, uint16_t color);  // Добавлен аргумент color

// Находим эту строку в src/st7789.h и заменяем на:
void lcd_drawrectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

void lcd_setwindows(uint16_t x_star, uint16_t y_star, uint16_t x_end, uint16_t y_end);
void lcd_direction(uint8_t direction);

void lcd_fillrect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void lcd_blit_buffer(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h, const uint16_t* buf);

#endif  // ST7789_H
