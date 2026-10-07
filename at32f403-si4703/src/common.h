#ifndef __COMMON_H__
#define __COMMON_H__

#include "at32f403a_407_gpio.h"
#include "at32f403a_407_crm.h"

#define LED_PIN          GPIO_PINS_13
#define LED_GPIO_PORT    GPIOC
#define LED_GPIO_CRM_CLK CRM_GPIOC_PERIPH_CLOCK

#define LED_SYSTEM_OFF    gpio_bits_write(LED_GPIO_PORT, LED_PIN, TRUE)
#define LED_SYSTEM_TOGGLE gpio_bits_toggle(LED_GPIO_PORT, LED_PIN)

#define TFT_CRM_GPIO_CLK CRM_GPIOA_PERIPH_CLOCK
#define TFT_GPIO_PORT    GPIOA
#define TFT_PIN_SCK      GPIO_PINS_5
#define TFT_PIN_MISO     GPIO_PINS_6
#define TFT_PIN_MOSI     GPIO_PINS_7

#define TFT_PIN_CS  GPIO_PINS_4
#define TFT_PIN_DC  GPIO_PINS_1
#define TFT_PIN_RST GPIO_PINS_2
#define TFT_PIN_BL  GPIO_PINS_3

#define TFT_CRM_SPI_CLK CRM_SPI1_PERIPH_CLOCK
#define TFT_SPI_PORT    SPI1

void     init_bsp_gpio(void);
void     init_periph(void);
void     init_bsp_spi(void);
void     init_bsp_dma(void);
void     delay_ms(uint32_t ms);
void     delay_nop(uint32_t nops);  // Добавлено для микросекундных пауз
void     delay_us(uint32_t us);
uint16_t decode_utf8(const char** start);

#endif  // __COMMON_H__