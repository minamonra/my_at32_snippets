#include "st7789.h"
#include "common.h"
#include "at32f403a_407_dma.h"

// =========================================================================
// СИСТЕМНЫЕ МАКРОСЫ И КОМАНДЫ КОНТРОЛЛЕРА
// =========================================================================
#define SPI_STS_TDBE (1 << 1)
#define SPI_STS_BF   (1 << 7)

#define MADCTL_LANDSCAPE_BGR 0x78  // Бит 3 меняет порядок каналов на BGR
#define COLMOD_16BIT         0x05

// =========================================================================
// ТАЙМИНГИ И ЗАДЕРЖКИ (ВЫВЕРЕННЫЕ ПОЛЬЗОВАТЕЛЕМ)
// =========================================================================
#define DELAY_INIT_REG_MS 1
#define DELAY_RST_MS      1
#define DELAY_DISPON_MS   80  // Пауза от белых вспышек перед зажиганием BL

// Побайтовый хелпер для отправки 8-битных данных
static inline void spi_send_byte_8bit(uint8_t data) {
  while (!(SPI1->sts & SPI_STS_TDBE));
  SPI1->dt = data;
  while (SPI1->sts & SPI_STS_BF);
}

// Переключение разрядности кадра SPI1 (8-бит / 16-бит)
static void spi_set_16bit_mode(uint8_t enable) {
  SPI1->ctrl1 &= ~(1 << 6);  // Отключаем SPI перед изменением конфигурации
  if (enable) {
    SPI1->ctrl1 |= (1 << 11);  // DFF = 1: 16-бит режим кадра
  } else {
    SPI1->ctrl1 &= ~(1 << 11);  // DFF = 0: 8-бит режим кадра
  }
  SPI1->ctrl1 |= (1 << 6);  // Включаем SPI обратно
}

static void st7789_send_command(uint8_t cmd) {
  spi_set_16bit_mode(0);
  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_CS, FALSE);
  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_DC, FALSE);
  spi_send_byte_8bit(cmd);
  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_CS, TRUE);
}

static void st7789_send_data(uint8_t data) {
  spi_set_16bit_mode(0);
  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_CS, FALSE);
  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_DC, TRUE);
  spi_send_byte_8bit(data);
  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_CS, TRUE);
}

// Настройка рабочего окна со смещениями геометрии матрицы
void st7789_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  uint16_t x_start = x1 + TFT_X_OFFSET;
  uint16_t x_end   = x2 + TFT_X_OFFSET;
  uint16_t y_start = y1 + TFT_Y_OFFSET;
  uint16_t y_end   = y2 + TFT_Y_OFFSET;

  spi_set_16bit_mode(0);
  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_CS, FALSE);

  // Конфигурация столбцов (CASET)
  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_DC, FALSE);
  spi_send_byte_8bit(ST7789_CASET);
  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_DC, TRUE);
  spi_send_byte_8bit(x_start >> 8);
  spi_send_byte_8bit(x_start & 0xFF);
  spi_send_byte_8bit(x_end >> 8);
  spi_send_byte_8bit(x_end & 0xFF);

  // Конфигурация строк (PASET)
  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_DC, FALSE);
  spi_send_byte_8bit(ST7789_PASET);
  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_DC, TRUE);
  spi_send_byte_8bit(y_start >> 8);
  spi_send_byte_8bit(y_start & 0xFF);
  spi_send_byte_8bit(y_end >> 8);
  spi_send_byte_8bit(y_end & 0xFF);

  // Команда готовности записи в ОЗУ (RAMWR)
  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_DC, FALSE);
  spi_send_byte_8bit(ST7789_RAMWR);
  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_DC, TRUE);
}

// Скоростная построчная DMA заливка
void st7789_fill_screen(uint16_t color) {
  dma_init_type                               dma_init_struct;
  __attribute__((aligned(4))) static uint16_t dma_buffer[TFT_WIDTH];

  for (uint16_t i = 0; i < TFT_WIDTH; i++) {
    dma_buffer[i] = color;
  }

  st7789_set_window(0, 0, TFT_WIDTH - 1, TFT_HEIGHT - 1);
  spi_set_16bit_mode(1);

  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_CS, FALSE);
  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_DC, TRUE);

  dma_reset(DMA1_CHANNEL3);

  dma_default_para_init(&dma_init_struct);
  dma_init_struct.peripheral_base_addr  = (uint32_t)&(SPI1->dt);
  dma_init_struct.memory_base_addr      = (uint32_t)dma_buffer;
  dma_init_struct.direction             = DMA_DIR_MEMORY_TO_PERIPHERAL;
  dma_init_struct.buffer_size           = TFT_WIDTH;
  dma_init_struct.peripheral_data_width = DMA_PERIPHERAL_DATA_WIDTH_HALFWORD;
  dma_init_struct.memory_data_width     = DMA_MEMORY_DATA_WIDTH_HALFWORD;
  dma_init_struct.peripheral_inc_enable = FALSE;
  dma_init_struct.memory_inc_enable     = TRUE;
  dma_init_struct.priority              = DMA_PRIORITY_HIGH;
  dma_init_struct.loop_mode_enable      = FALSE;

  for (uint16_t row = 0; row < TFT_HEIGHT; row++) {
    dma_init(DMA1_CHANNEL3, &dma_init_struct);
    SPI1->ctrl2 |= (1 << 1);  // TXDMAEN = 1

    dma_channel_enable(DMA1_CHANNEL3, TRUE);
    while (dma_flag_get(DMA1_FDT3_FLAG) == RESET);
    dma_flag_clear(DMA1_FDT3_FLAG);
    dma_channel_enable(DMA1_CHANNEL3, FALSE);
  }

  while (SPI1->sts & SPI_STS_BF);
  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_CS, TRUE);
  spi_set_16bit_mode(0);
}

// Инициализация контроллера
void st7789_init(void) {
  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_RST, FALSE);  // Опускаем пин RST для сброса дисплея
  delay_ms(DELAY_RST_MS);                              // Выдерживаем паузу в режиме сброса
  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_RST, TRUE);   // Поднимаем пин RST для запуска контроллера
  delay_ms(DELAY_RST_MS);                              // Пауза для стабилизации после сброса

  st7789_send_command(ST7789_SWRST);  // Команда программного сброса регистров
  delay_ms(DELAY_RST_MS);             // Пауза для выполнения перезапуска чипа

  st7789_send_command(ST7789_SLPOUT);  // Команда выхода из спящего режима
  delay_ms(DELAY_RST_MS);              // Пауза для запуска внутренних генераторов

  st7789_send_command(ST7789_COLMOD);  // Настройка формата передачи цвета
  st7789_send_data(COLMOD_16BIT);      // Задаем режим 16-бит на пиксель (RGB565)
  delay_ms(DELAY_RST_MS);              // Пауза для применения формата цвета

  st7789_send_command(ST7789_MADCTL);      // Настройка направления развертки матрицы
  st7789_send_data(MADCTL_LANDSCAPE_BGR);  // Задаем режим Landscape с цветовой картой BGR
  delay_ms(DELAY_RST_MS);                  // Пауза для перестройки адресации буфера

  st7789_send_command(ST7789_INVOFF);  // Отключаем инверсию цветов матрицы
  delay_ms(DELAY_RST_MS);              // Пауза для применения режима палитры

  st7789_fill_screen(ST7789_BLACK);  // Заливаем видеопамять черным цветом до включения

  st7789_send_command(ST7789_DISPON);  // Команда включения вывода картинки на экран
  delay_ms(DELAY_DISPON_MS);           // Выверенная пауза для предотвращения белой вспышки

  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_BL, FALSE);  // Включаем инверсную подсветку подачей логического нуля
}

// Перенаправление стандартного Alientek-инита на нашу готовую функцию инициализации
void lcd_init(void) {
  st7789_init();  // Вызываем готовую инициализацию дисплея
}

// Очистка экрана выбранным цветом через вызов базовой функции заливки
void lcd_clear(uint16_t color) {
  st7789_fill_screen(color);  // Вызываем полную заливку экрана через DMA
}

// Установка курсора в конкретную точку памяти дисплея
void lcd_setcursor(uint16_t x_pos, uint16_t y_pos) {
  st7789_set_window(x_pos, y_pos, x_pos, y_pos);  // Открываем окно размером в один пиксель
}

// Отрисовка одной точки выбранного цвета (теперь цвет берется из аргумента)
void lcd_drawpoint(uint16_t x, uint16_t y, uint16_t color) {
  st7789_set_window(x, y, x, y);                      // Устанавливаем окно в координаты пикселя
  spi_set_16bit_mode(0);                              // Переводим SPI в базовый 8-битный режим
  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_CS, FALSE);  // Опускаем CS для начала транзакции
  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_DC, TRUE);   // Выставляем режим передачи данных
  spi_send_byte_8bit(color >> 8);                     // Отправляем старший байт цвета точки
  spi_send_byte_8bit(color & 0xFF);                   // Отправляем младший байт цвета точки
  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_CS, TRUE);   // Поднимаем CS, завершая отрисовку
}

// Отрисовка контура прямоугольника выбранным цветом
void lcd_drawrectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
  uint16_t i;
  for (i = x1; i <= x2; i++) lcd_drawpoint(i, y1, color);
  for (i = x1; i <= x2; i++) lcd_drawpoint(i, y2, color);
  for (i = y1; i <= y2; i++) lcd_drawpoint(x1, i, color);
  for (i = y1; i <= y2; i++) lcd_drawpoint(x2, i, color);
}

// Обертка для установки границ окна адресации
void lcd_setwindows(uint16_t x_star, uint16_t y_star, uint16_t x_end, uint16_t y_end) {
  st7789_set_window(x_star, y_star, x_end, y_end);  // Вызываем настройку окна со смещениями матрицы
}

// Изменение направления развертки и ориентации экрана динамически
void lcd_direction(uint8_t direction) {
  st7789_send_command(ST7789_MADCTL);  // Отправляем команду контроля развертки
  st7789_send_data(direction);         // Записываем новую битовую маску ориентации
}

// Чтение идентификатора контроллера (заглушка для 4-проводного TX SPI)
uint16_t lcd_read_id(void) {
  return 0x7789;  // Возвращаем жестко задефайненный ID чипа ST7789
}

// Пакетный блит прямоугольника фиксированным цветом
void lcd_fillrect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
  dma_init_type                               dma_init_struct;
  uint16_t                                    w = x2 - x1 + 1;
  uint16_t                                    h = y2 - y1 + 1;
  __attribute__((aligned(4))) static uint16_t local_buf[TFT_WIDTH];
  uint16_t                                    i, row;

  for (i = 0; i < w; i++) {
    local_buf[i] = color;
  }

  st7789_set_window(x1, y1, x2, y2);
  spi_set_16bit_mode(1);

  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_CS, FALSE);
  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_DC, TRUE);
  SPI1->ctrl2 |= (1 << 1);

  for (row = 0; row < h; row++) {
    dma_reset(DMA1_CHANNEL3);
    dma_default_para_init(&dma_init_struct);
    dma_init_struct.peripheral_base_addr  = (uint32_t)&(SPI1->dt);
    dma_init_struct.memory_base_addr      = (uint32_t)local_buf;
    dma_init_struct.direction             = DMA_DIR_MEMORY_TO_PERIPHERAL;
    dma_init_struct.buffer_size           = w;
    dma_init_struct.peripheral_data_width = DMA_PERIPHERAL_DATA_WIDTH_HALFWORD;
    dma_init_struct.memory_data_width     = DMA_MEMORY_DATA_WIDTH_HALFWORD;
    dma_init_struct.peripheral_inc_enable = FALSE;
    dma_init_struct.memory_inc_enable     = TRUE;
    dma_init_struct.priority              = DMA_PRIORITY_HIGH;
    dma_init_struct.loop_mode_enable      = FALSE;

    dma_init(DMA1_CHANNEL3, &dma_init_struct);
    dma_channel_enable(DMA1_CHANNEL3, TRUE);
    while (dma_flag_get(DMA1_FDT3_FLAG) == RESET);
    dma_flag_clear(DMA1_FDT3_FLAG);
    dma_channel_enable(DMA1_CHANNEL3, FALSE);
  }

  while (SPI1->sts & SPI_STS_BF);
  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_CS, TRUE);
  spi_set_16bit_mode(0);
}

// Пакетный блит буфера произвольных 16-битных пикселей одним DMA-проходом (с инкрементом источника)
void lcd_blit_buffer(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h, const uint16_t* buf) {
  dma_init_type dma_init_struct;      // Создаем дескриптор инициализации DMA
  uint32_t      total_words = w * h;  // Вычисляем полный объем массива в 16-битных словах

  st7789_set_window(x1, y1, x1 + w - 1, y1 + h - 1);  // Открываем окно точно под геометрию массива
  spi_set_16bit_mode(1);                              // Включаем скоростной 16-битный режим кадра

  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_CS, FALSE);  // Активируем линию выбора чипа CS
  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_DC, TRUE);   // Выставляем режим отправки массива данных
  SPI1->ctrl2 |= (1 << 1);                            // Разрешаем SPI1 использовать транзакции DMA

  dma_reset(DMA1_CHANNEL3);                                                    // Полностью сбрасываем конфигурацию канала 3
  dma_default_para_init(&dma_init_struct);                                     // Инициализация структуры параметрами по умолчанию
  dma_init_struct.peripheral_base_addr  = (uint32_t)&(SPI1->dt);               // Регистр назначения — буфер данных SPI1
  dma_init_struct.memory_base_addr      = (uint32_t)buf;                       // Адрес источника — переданный указатель на массив buf
  dma_init_struct.direction             = DMA_DIR_MEMORY_TO_PERIPHERAL;        // Вектор пересылки: из ОЗУ в периферию
  dma_init_struct.buffer_size           = total_words;                         // Пересылаем весь объем за один непрерывный проход
  dma_init_struct.peripheral_data_width = DMA_PERIPHERAL_DATA_WIDTH_HALFWORD;  // Ширина ячейки приемника (16 бит)
  dma_init_struct.memory_data_width     = DMA_MEMORY_DATA_WIDTH_HALFWORD;      // Ширина ячейки источника (16 бит)
  dma_init_struct.peripheral_inc_enable = FALSE;                               // Адрес SPI регистра не инкрементируем
  dma_init_struct.memory_inc_enable     = TRUE;                                // ИНКРЕМЕНТИРУЕМ адрес буфера для чтения новых пикселей
  dma_init_struct.priority              = DMA_PRIORITY_HIGH;                   // Назначаем высокий приоритет для захвата шины
  dma_init_struct.loop_mode_enable      = FALSE;                               // Режим одиночной пакетной передачи

  dma_init(DMA1_CHANNEL3, &dma_init_struct);  // Прописываем конфигурацию в регистры DMA контроллера
  dma_channel_enable(DMA1_CHANNEL3, TRUE);    // Запускаем аппаратный транзит всего массива данных

  while (dma_flag_get(DMA1_FDT3_FLAG) == RESET);  // Ожидаем окончания передачи всей структуры пикселей кадра
  dma_flag_clear(DMA1_FDT3_FLAG);                 // Очищаем системный флаг завершения работы канала
  dma_channel_enable(DMA1_CHANNEL3, FALSE);       // Останавливаем работу канала DMA

  while (SPI1->sts & SPI_STS_BF);                    // Дожидаемся физического завершения передачи по SPI шине
  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_CS, TRUE);  // Поднимаем линию CS, освобождая шину дисплея
  spi_set_16bit_mode(0);                             // Возвращаем SPI в базовый 8-битный формат кадра
}
