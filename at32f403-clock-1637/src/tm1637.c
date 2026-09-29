#include "tm1637.h"

#define ACK_NOPS 8
static uint8_t brightness_cmd = 0x88 + 4;

static const uint8_t digit_map[] = {
    0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f};

void delay_nop(uint32_t nops) {
  for (volatile uint32_t i = 0; i < nops; i++) {
    __NOP();
  }
}

static void tm1637_start(void) {
  gpio_bits_set(TM_GPIO_PORT, TM_CLK_PIN);
  gpio_bits_set(TM_GPIO_PORT, TM_DIO_PIN);
  delay_nop(ACK_NOPS);
  gpio_bits_reset(TM_GPIO_PORT, TM_DIO_PIN);
  delay_nop(ACK_NOPS);
}

static void tm1637_stop(void) {
  gpio_bits_reset(TM_GPIO_PORT, TM_CLK_PIN);
  gpio_bits_reset(TM_GPIO_PORT, TM_DIO_PIN);
  delay_nop(ACK_NOPS);
  gpio_bits_set(TM_GPIO_PORT, TM_CLK_PIN);
  delay_nop(ACK_NOPS);
  gpio_bits_set(TM_GPIO_PORT, TM_DIO_PIN);
  delay_nop(ACK_NOPS);
}

static void tm1637_write_byte(uint8_t b) {
  for (uint8_t i = 0; i < 8; i++) {
    gpio_bits_reset(TM_GPIO_PORT, TM_CLK_PIN);
    delay_nop(ACK_NOPS);
    if (b & 0x01) {
      gpio_bits_set(TM_GPIO_PORT, TM_DIO_PIN);
    } else {
      gpio_bits_reset(TM_GPIO_PORT, TM_DIO_PIN);
    }
    b >>= 1;
    delay_nop(ACK_NOPS);
    gpio_bits_set(TM_GPIO_PORT, TM_CLK_PIN);
    delay_nop(ACK_NOPS);
  }

  // Эмуляция импульса ACK
  gpio_bits_reset(TM_GPIO_PORT, TM_CLK_PIN);
  gpio_bits_set(TM_GPIO_PORT, TM_DIO_PIN);
  delay_nop(ACK_NOPS);
  gpio_bits_set(TM_GPIO_PORT, TM_CLK_PIN);
  delay_nop(ACK_NOPS);
  gpio_bits_reset(TM_GPIO_PORT, TM_CLK_PIN);
}

void tm1637_init(uint8_t brightness) {
  gpio_init_type gpio_struct;
  crm_periph_clock_enable(TM_GPIO_CRM_CLK, TRUE);

  gpio_default_para_init(&gpio_struct);
  gpio_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_struct.gpio_out_type       = GPIO_OUTPUT_PUSH_PULL;
  gpio_struct.gpio_mode           = GPIO_MODE_OUTPUT;
  gpio_struct.gpio_pull           = GPIO_PULL_NONE;
  gpio_struct.gpio_pins           = TM_CLK_PIN | TM_DIO_PIN;
  gpio_init(TM_GPIO_PORT, &gpio_struct);

  tm1637_set_brightness(brightness);
}

void tm1637_set_brightness(uint8_t brightness) {
  if (brightness > 7) brightness = 7;
  brightness_cmd = 0x88 + brightness;
}

void tm1637_display_time(uint8_t hours, uint8_t minutes, uint8_t colon) {
  uint8_t segments[4];

  segments[0] = digit_map[hours / 10];
  segments[1] = digit_map[hours % 10];
  segments[2] = digit_map[minutes / 10];
  segments[3] = digit_map[minutes % 10];

  if (colon) {
    segments[1] |= 0x80;
  }

  tm1637_start();
  tm1637_write_byte(0x40);
  tm1637_stop();

  tm1637_start();
  tm1637_write_byte(0xC0);
  for (uint8_t i = 0; i < 4; i++) {
    tm1637_write_byte(segments[i]);
  }
  tm1637_stop();

  tm1637_start();
  tm1637_write_byte(brightness_cmd);
  tm1637_stop();
}
