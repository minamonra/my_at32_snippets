#include "ui_task.h"
#include "common.h"
#include "st7789.h"
#include "lcd_draw_u8g2.h"
#include "ring_buffer.h"
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"

extern const uint8_t u8g2_font_terminus_24b_cyr[];
extern ring_buffer_t audio_ring_buf;

// Ссылка на переменные из main.c
extern volatile uint8_t g_usb_volume_percent;
extern volatile uint8_t g_usb_mute_status;

static char ui_line1[32];
static char ui_line2[32];

void v_tft_audio_ui_task(void* pv_parameters) {
  (void)pv_parameters;

  // Инициализация графики
  init_bsp_gpio();
  init_bsp_spi();
  init_bsp_dma();
  lcd_init();
  lcd_u8g2_set_font(u8g2_font_terminus_24b_cyr);

  lcd_clear(ST7789_BLACK);

  // Рамка главного окна интерфейса
  lcd_drawrectangle(3, 3, 234, 100, ST7789_GREEN);

  while (1) {
    // 1. Формируем строку состояния подключения и Mute
    if (g_usb_mute_status) {
      snprintf(ui_line1, sizeof(ui_line1), "USB DAC: *MUTE* ");
    } else {
      snprintf(ui_line1, sizeof(ui_line1), "USB DAC: PLAYING");
    }

    // 2. Считываем реальную громкость, присланную из операционной системы
    snprintf(ui_line2, sizeof(ui_line2), "48.0kHz  VOL: %3d%%", g_usb_volume_percent);

    // Вывод текста на дисплей ST7789
    lcd_draw_u8g2_string(12, 32, ui_line1, ST7789_WHITE, ST7789_BLACK);
    lcd_draw_u8g2_string(12, 62, ui_line2, 0xAFE5, ST7789_BLACK);

    // 3. Динамический прогресс-бар заполнения кольцевого буфера (борьба с джиттером)
    size_t available_bytes = ring_buffer_get_available(&audio_ring_buf);
    // Переводим заполненность буфера (0..4096 байт) в пиксели на экране (ширина полосы 210 пикселей)
    uint32_t bar_width = (available_bytes * 210) / RING_BUFFER_SIZE;
    if (bar_width > 210) bar_width = 210;

    // Рисуем заполненную часть полосы буфера (голубая)
    if (bar_width > 0) {
      lcd_fillrect(12, 78, bar_width, 10, 0x07FF);
    }
    // Очищаем оставшуюся часть полосы буфера (черная)
    if (bar_width < 210) {
      lcd_fillrect(12 + bar_width, 78, 210 - bar_width, 10, ST7789_BLACK);
    }

    vTaskDelay(pdMS_TO_TICKS(100));  // Обновление экрана 10 раз в секунду
  }
}
