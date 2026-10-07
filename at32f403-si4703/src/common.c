#include "common.h"
#include "at32f403a_407_clock.h"
#include "at32f403a_407_dma.h"
#include "FreeRTOS.h"
#include "task.h"

void init_bsp_gpio(void) {
  gpio_init_type gpio_init_struct;

  // 1. Сначала строго включаем тактование мультиплексора (IOMUX) и порта дисплея
  crm_periph_clock_enable(CRM_IOMUX_PERIPH_CLOCK, TRUE);
  crm_periph_clock_enable(TFT_CRM_GPIO_CLK, TRUE);

  // 2. Освобождаем пины от JTAG (Включаем SWD, отключаем JTAG)
  gpio_pin_remap_config(SWJTAG_MUX_010, TRUE);

  // 3. И ТОЛЬКО ТЕПЕРЬ начисто настраиваем ноги дисплея, перебивая настройки JTAG
  gpio_init_struct.gpio_mode           = GPIO_MODE_MUX;
  gpio_init_struct.gpio_out_type       = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init_struct.gpio_pins           = TFT_PIN_SCK | TFT_PIN_MOSI;
  gpio_init(TFT_GPIO_PORT, &gpio_init_struct);

  gpio_init_struct.gpio_mode           = GPIO_MODE_OUTPUT;
  gpio_init_struct.gpio_out_type       = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init_struct.gpio_pins           = TFT_PIN_CS | TFT_PIN_DC | TFT_PIN_RST | TFT_PIN_BL;
  gpio_init(TFT_GPIO_PORT, &gpio_init_struct);

  // 4. Жестко поднимаем управляющие сигналы дисплея в HIGH
  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_CS, TRUE);
  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_DC, TRUE);
  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_RST, TRUE);
  gpio_bits_write(TFT_GPIO_PORT, TFT_PIN_BL, TRUE);  // Зажжет подсветку обратно!
}

void init_periph(void) {
  gpio_init_type gpio_init_struct;
  crm_periph_clock_enable(LED_GPIO_CRM_CLK, TRUE);

  gpio_init_struct.gpio_mode           = GPIO_MODE_OUTPUT;
  gpio_init_struct.gpio_out_type       = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init_struct.gpio_pins           = LED_PIN;
  gpio_init(LED_GPIO_PORT, &gpio_init_struct);

  LED_SYSTEM_OFF;
}

void init_bsp_spi(void) {
  spi_init_type spi_init_struct;

  crm_periph_clock_enable(TFT_CRM_SPI_CLK, TRUE);

  spi_default_para_init(&spi_init_struct);
  spi_init_struct.transmission_mode      = SPI_TRANSMIT_HALF_DUPLEX_TX;
  spi_init_struct.master_slave_mode      = SPI_MODE_MASTER;
  spi_init_struct.mclk_freq_division     = SPI_MCLK_DIV_2;
  spi_init_struct.first_bit_transmission = SPI_FIRST_BIT_MSB;
  spi_init_struct.frame_bit_num          = SPI_FRAME_16BIT;

  spi_init_struct.clock_polarity    = SPI_CLOCK_POLARITY_LOW;
  spi_init_struct.clock_phase       = SPI_CLOCK_PHASE_1EDGE;
  spi_init_struct.cs_mode_selection = SPI_CS_SOFTWARE_MODE;

  spi_init(TFT_SPI_PORT, &spi_init_struct);
  spi_enable(TFT_SPI_PORT, TRUE);
}

void init_bsp_dma(void) {
  crm_periph_clock_enable(CRM_DMA1_PERIPH_CLOCK, TRUE);
}

// - Замените функции delay_ms и добавьте delay_us
void delay_us(uint32_t us) {
  // Грубый, но стабильный подсчет микросекунд на NOP для AT32F407 на высоких частотах (до 240-288 МГц)
  volatile uint32_t count = (system_core_clock / 4000000) * us;
  if (count == 0) count = 1;
  while (count--) {
    __NOP();
  }
}

void delay_ms(uint32_t ms) {
  if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
    vTaskDelay(pdMS_TO_TICKS(ms));
  } else {
    while (ms--) {
      delay_us(1000);
    }
  }
}

// Потактовая задержка на NOPах
void delay_nop(uint32_t nops) {
  while (nops--) {
    __asm volatile("nop");
  }
}

// Декодирование UTF-8 символов (включая русскую кириллицу) для шрифтов U8g2
uint16_t decode_utf8(const char** start) {
  const char* ptr = *start;
  uint8_t     ch  = (uint8_t)(*ptr++);

  if (ch < 0x80) {
    *start = ptr;
    return ch;
  }

  uint16_t res = 0;
  if ((ch & 0xE0) == 0xC0) {
    res = (ch & 0x1F) << 6;
    res |= (*ptr++ & 0x3F);
  } else if ((ch & 0xF0) == 0xE0) {
    res = (ch & 0x0F) << 12;
    res |= (*ptr++ & 0x3F) << 6;
    res |= (*ptr++ & 0x3F);
  }

  *start = ptr;
  return res;
}