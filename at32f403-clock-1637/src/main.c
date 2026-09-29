#include "at32f403a_407_clock.h"
#include "at32f403a_407_pwc.h"
#include "tm1637.h"

// Конфигурация частоты ядра
#define SYS_CLK_HZ       240000000
#define TICK_PER_MS      (SYS_CLK_HZ / 1000)

// Настройки RTC календаря
#define RTC_START_TIME   43200
#define RTC_BPR_REG      BPR_DATA1
#define RTC_SIGNATURE    0x1234
#define SECONDS_IN_DAY   86400

// Конфигурация кнопки
#define BTN_PIN          GPIO_PINS_0
#define BTN_GPIO_PORT    GPIOA
#define BTN_GPIO_CRM_CLK CRM_GPIOA_PERIPH_CLOCK
#define BTN_UPD_MS       10
#define BTN_DEBOUNCE_MS  20
#define BTN_HOLD_MS      1000
#define BTN_SCROLL_MS    200

// Систик ms
volatile uint32_t ttms = 0;

// Как в stm32 :)
void SysTick_Handler(void) {
  ttms++;
}

// Инициализация систик таймера
void init_system_tick(void) {
  // Настройка на 1 мс при частоте 240 МГц
  SysTick->LOAD = TICK_PER_MS - 1;
  SysTick->VAL  = 0;

  // Включаем тактирование, разрешаем прерывание (TICKINT) и запускаем таймер
  SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

// Инициализация RTC с защитой от сброса (при прошивке в тч)
void rtc_init(void) {
  crm_periph_clock_enable(CRM_PWC_PERIPH_CLOCK, TRUE);
  crm_periph_clock_enable(CRM_BPR_PERIPH_CLOCK, TRUE);

  pwc_battery_powered_domain_access(TRUE);

  // Проверка сигнатуры и стабильности кварца LEXT
  if ((bpr_data_read(RTC_BPR_REG) == RTC_SIGNATURE) && (crm_flag_get(CRM_LEXT_STABLE_FLAG) == SET)) {
    rtc_wait_config_finish();
    rtc_wait_update_finish();
    return;
  }

  // Сброс домена при первом включении
  crm_battery_powered_domain_reset(TRUE);
  crm_battery_powered_domain_reset(FALSE);

  // Запуск часового кварца 32.768 кГц
  crm_clock_source_enable(CRM_CLOCK_SOURCE_LEXT, TRUE);
  while (crm_flag_get(CRM_LEXT_STABLE_FLAG) == RESET);

  // Выбор источника и включение RTC
  crm_rtc_clock_select(CRM_RTC_CLOCK_LEXT);
  crm_rtc_clock_enable(TRUE);

  rtc_wait_config_finish();
  rtc_wait_update_finish();

  // Предделитель на 1 секунду (32768 - 1)
  rtc_divider_set(32767);
  rtc_wait_config_finish();

  // Дефолтные 12:00
  rtc_counter_set(RTC_START_TIME);
  rtc_wait_config_finish();

  // Запись сигнатуры
  bpr_data_write(RTC_BPR_REG, RTC_SIGNATURE);
}

// Инициализация периферии
void init_periph(void) {
  gpio_init_type gpio_init_struct;

  crm_periph_clock_enable(BTN_GPIO_CRM_CLK, TRUE);

  gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
  gpio_init_struct.gpio_pull = GPIO_PULL_UP;
  gpio_init_struct.gpio_pins = BTN_PIN;
  gpio_init(BTN_GPIO_PORT, &gpio_init_struct);
}

int main(void) {
  // Настраиваем HSE на 240 МГц (BSP at32f403a_407_clock.h)
  system_clock_config();

  // Запускаем систик
  init_system_tick();

  init_periph();
  tm1637_init(1);
  rtc_init();

  uint8_t  btn_last_state   = TRUE;
  uint32_t btn_press_time   = 0;
  uint32_t last_btn_check   = 0;
  uint32_t last_fast_scroll = 0;

  while (1) {
    uint32_t rtc_seconds = rtc_counter_get();

    // Сброс суток
    if (rtc_seconds >= SECONDS_IN_DAY) {
      rtc_seconds %= SECONDS_IN_DAY;
      rtc_counter_set(rtc_seconds);
      rtc_wait_config_finish();
    }

    uint8_t hours   = rtc_seconds / 3600;
    uint8_t minutes = (rtc_seconds % 3600) / 60;

    // Опрос кнопки
    uint32_t current_millis = ttms;
    if (current_millis - last_btn_check >= BTN_UPD_MS) {
      last_btn_check = current_millis;

      uint8_t btn_current_state = gpio_input_data_bit_read(BTN_GPIO_PORT, BTN_PIN);

      // Фиксируем нажатие
      if (btn_last_state == TRUE && btn_current_state == RESET) {
        btn_press_time = 0;
      }

      // Если кнопка удерживается
      if (btn_current_state == RESET) {
        btn_press_time += BTN_UPD_MS;

        // Быстрая перемотка ЧАСЫ при удержании
        if (btn_press_time >= BTN_HOLD_MS) {
          if (current_millis - last_fast_scroll >= BTN_SCROLL_MS) {
            last_fast_scroll = current_millis;
            hours            = (hours + 1) % 24;
            rtc_seconds      = (hours * 3600) + (minutes * 60);
            rtc_counter_set(rtc_seconds);
            rtc_wait_config_finish();
          }
        }
      }

      // Фиксируем отпускание
      if (btn_last_state == RESET && btn_current_state == TRUE) {
        // Короткий клик — прибавляем МИНУТУ
        if (btn_press_time < BTN_HOLD_MS) {
          if (btn_press_time >= BTN_DEBOUNCE_MS) {
            minutes     = (minutes + 1) % 60;
            rtc_seconds = (hours * 3600) + (minutes * 60);
            rtc_counter_set(rtc_seconds);
            rtc_wait_config_finish();
          }
        }
        btn_press_time = 0;
      }
      btn_last_state = btn_current_state;
    }

    // Обновление 1637
    uint8_t colon_state = (rtc_seconds % 2);
    tm1637_display_time(hours, minutes, colon_state);
  }
}
