// - Скорректированная разметка под экран без вылетов за границы

#include "at32f403a_407_clock.h"
#include "at32f403a_407_gpio.h"
#include "at32f403a_407_crm.h"
#include "at32f403a_407_misc.h"

#include "common.h"
#include "st7789.h"
#include "si4703.h"
#include "lcd_draw_u8g2.h"

#include "FreeRTOS.h"
#include "task.h"

extern const uint8_t u8g2_font_terminus_24b_cyr[];
// Шаблоны строго фиксированной длины (ровно 19 символов + \0)
static char line1[] = "FM:  000.0 MHz     ";
static char line2[] = "SIG:00 [ST] ID:0000";  // Вернули ID для красоты или можете оставить пробелы

void v_blink_task(void* pv_parameters) {
  (void)pv_parameters;
  while (1) {
    LED_SYSTEM_TOGGLE;
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

// Ручной перевод RSSI в строку (Индексы 4 и 5)
static void parse_rssi_to_line(uint8_t rssi) {
  if (rssi > 99) rssi = 99;
  line2[4] = (char)('0' + (rssi / 10));
  line2[5] = (char)('0' + (rssi % 10));
}

// ЮВЕЛИРНО: Перевод Chip ID строго в индексы 15, 16, 17, 18 шаблона "SIG:00 [ST] ID:0000"
static void parse_chip_id_to_line(uint16_t chip_id) {
  const char hex_chars[] = "0123456789ABCDEF";
  line2[15]              = hex_chars[(chip_id >> 12) & 0x0F];
  line2[16]              = hex_chars[(chip_id >> 8) & 0x0F];
  line2[17]              = hex_chars[(chip_id >> 4) & 0x0F];
  line2[18]              = hex_chars[chip_id & 0x0F];
}

void v_tft_radio_task(void* pv_parameters) {
  (void)pv_parameters;

  uint16_t current_freq = 0;
  uint8_t  rssi         = 0;
  uint16_t chip_id      = 0;  // Заменили тестовую переменную обратно на chip_id

  init_bsp_gpio();
  init_bsp_spi();
  init_bsp_dma();
  lcd_init();
  lcd_u8g2_set_font(u8g2_font_terminus_24b_cyr);

  // Экран загрузки
  // lcd_clear(ST7789_BLACK);
  // lcd_drawrectangle(3, 3, 280, 72, ST7789_GREEN);
  // lcd_draw_u8g2_string(10, 45, "Инициализация...", ST7789_WHITE, ST7789_BLACK);
  // vTaskDelay(pdMS_TO_TICKS(100));

  // 1. Аппаратный сброс и тихий запуск чипа сразу на рабочей частоте 99.6 МГц
  si4703_init1();
  vTaskDelay(pdMS_TO_TICKS(50));

  // 2. Проверяем физическую связь с чипом через жесткий ID завода
  chip_id = si4703_get_chip_id();
  if (chip_id == 0x0000 || chip_id == 0xFFFF) {
    lcd_clear(ST7789_BLACK);
    lcd_drawrectangle(3, 3, 280, 72, ST7789_RED);
    lcd_draw_u8g2_string(10, 32, "ОШИБКА ШИНЫ I2C!", ST7789_RED, ST7789_BLACK);
    lcd_draw_u8g2_string(10, 62, "Чип не отвечает..", ST7789_WHITE, ST7789_BLACK);
    while (1) {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }

  si4703_set_band_fm();
  vTaskDelay(pdMS_TO_TICKS(50));

  // 3. Вызываем установку частоты (звук плавно откроется внутри функции строго после фиксации PLL)
  si4703_set_frequency(9450);
  si4703_set_volume(8);  // Устанавливаем громкость на 4 (из 15)
  vTaskDelay(pdMS_TO_TICKS(100));

  // Очистка экрана перед входом в цикл
  lcd_clear(ST7789_BLACK);
  lcd_drawrectangle(3, 3, 280, 72, ST7789_GREEN);

  // --- ОСНОВНОЙ ЦИКЛ ОБНОВЛЕНИЯ ЭКРАНА ---
  while (1) {
    current_freq = si4703_get_frequency();
    rssi         = si4703_get_rssi();
    chip_id      = si4703_get_chip_id();  // Читаем стабильный ID завода (например, 0x1253)

    // Разбор частоты по индексам шаблона "FM:  000.0 MHz     "
    uint16_t mhz = current_freq / 10;
    uint8_t  khz = current_freq % 10;

    line1[4] = (char)('0' + (mhz / 100));
    line1[5] = (char)('0' + ((mhz / 10) % 10));
    line1[6] = (char)('0' + (mhz % 10));
    line1[8] = (char)('0' + khz);

    if (line1[4] == '0') {
      line1[4] = ' ';
    }

    // Разбор RSSI и вывод реального Chip ID на экран
    parse_rssi_to_line(rssi);
    parse_chip_id_to_line(chip_id);

    // Индикация режима Стерео / Моно
    if (si4703_is_stereo()) {
      line2[8] = 'S';
      line2[9] = 'T';
    } else {
      line2[8] = 'M';
      line2[9] = 'O';
    }

    // Вывод на дисплей
    lcd_draw_u8g2_string(10, 32, line1, ST7789_WHITE, ST7789_BLACK);
    lcd_draw_u8g2_string(10, 62, line2, 0xAFE5, ST7789_BLACK);

    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

int main(void) {
  system_clock_config();
  nvic_priority_group_config(NVIC_PRIORITY_GROUP_4);
  init_periph();

  xTaskCreate(v_blink_task, "Blink", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
  xTaskCreate(v_tft_radio_task, "TFT_Task", configMINIMAL_STACK_SIZE * 3, NULL, tskIDLE_PRIORITY + 2, NULL);

  vTaskStartScheduler();

  while (1) {
  }
}
