#include "at32f403a_407_clock.h"
#include "at32f403a_407_gpio.h"
#include "at32f403a_407_crm.h"

// (WeAct BlackPill PC13)
#define LED_PIN          GPIO_PINS_13
#define LED_GPIO_PORT    GPIOC
#define LED_GPIO_CRM_CLK CRM_GPIOC_PERIPH_CLOCK

#define LED_SYSTEM_OFF    gpio_bits_write(LED_GPIO_PORT, LED_PIN, TRUE)
#define LED_SYSTEM_ON     gpio_bits_write(LED_GPIO_PORT, LED_PIN, FALSE)
#define LED_SYSTEM_TOGGLE gpio_bits_toggle(LED_GPIO_PORT, LED_PIN)

// (PA0)
#define BTN_PIN          GPIO_PINS_0
#define BTN_GPIO_PORT    GPIOA
#define BTN_GPIO_CRM_CLK CRM_GPIOA_PERIPH_CLOCK

// Интервалы мигания без блокирующих задержек
#define BLINK_SLOW_MS   500  // Скорость в покое (медленно)
#define BLINK_FAST_MS   100  // Скорость при нажатии (быстро)
#define BTN_DEBOUNCE_MS 20   // Время антидребезга кнопки

// Систик ms
volatile uint32_t ttms = 0;

// Как в stm32 :)
void SysTick_Handler(void) {
  ttms++;
}

// (240 МГц)
void init_system_tick(void) {
  SysTick->LOAD = (240000000 / 1000) - 1;
  SysTick->VAL  = 0;
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

// Периферия
void init_periph(void) {
  gpio_init_type gpio_init_struct;

  // Включаем тактирование портов
  crm_periph_clock_enable(LED_GPIO_CRM_CLK, TRUE);
  crm_periph_clock_enable(BTN_GPIO_CRM_CLK, TRUE);

  // Настройка LED на выход Push-Pull
  gpio_init_struct.gpio_mode           = GPIO_MODE_OUTPUT;
  gpio_init_struct.gpio_out_type       = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init_struct.gpio_pins           = LED_PIN;
  gpio_init(LED_GPIO_PORT, &gpio_init_struct);

  LED_SYSTEM_OFF;  // Гасим по умолчанию

  // Настройка кнопки на вход без подтяжки
  gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
  gpio_init_struct.gpio_pins = BTN_PIN;
  gpio_init(BTN_GPIO_PORT, &gpio_init_struct);
}

int main(void) {
  system_clock_config();
  init_system_tick();
  init_periph();

  uint32_t last_blink_time = 0;      // Время последнего переключения LED
  uint32_t last_btn_time   = 0;      // Время последнего опроса кнопки
  uint8_t  btn_pressed     = FALSE;  // Текущий отфильтрованный статус кнопки

  while (1) {
    uint32_t current_time = ttms;

    // Опрос кнопки с программным антидребезгом
    if (current_time - last_btn_time >= BTN_DEBOUNCE_MS) {
      last_btn_time = current_time;

      // Читаем физическое состояние пина PA0
      uint8_t btn_raw_state = gpio_input_data_bit_read(BTN_GPIO_PORT, BTN_PIN);

      if (btn_raw_state == RESET) {
        btn_pressed = TRUE;  // Кнопка удерживается
      } else {
        btn_pressed = FALSE;  // Кнопка отпущена
      }
    }

    // Управление автоматом мигания на основе статуса кнопки
    uint32_t target_interval = btn_pressed ? BLINK_FAST_MS : BLINK_SLOW_MS;

    if (current_time - last_blink_time >= target_interval) {
      last_blink_time = current_time;
      LED_SYSTEM_TOGGLE;  // Инвертируем состояние
    }
  }
}
