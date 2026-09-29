#ifndef __TM1637_H__
#define __TM1637_H__

#include "at32f403a_407_clock.h"

#define TM_GPIO_PORT    GPIOB
#define TM_GPIO_CRM_CLK CRM_GPIOB_PERIPH_CLOCK
#define TM_CLK_PIN      GPIO_PINS_6
#define TM_DIO_PIN      GPIO_PINS_7

void tm1637_init(uint8_t brightness);
void tm1637_display_time(uint8_t hours, uint8_t minutes, uint8_t colon);
void tm1637_set_brightness(uint8_t brightness);

#endif  // __TM1637_H__