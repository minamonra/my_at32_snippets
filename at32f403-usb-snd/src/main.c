#include "at32f403a_407_clock.h"
#include "at32f403a_407_misc.h"
#include "at32f403a_407_usb.h"
#include "at32f403a_407_pwc.h"
#include "common.h"
#include "pcm5102.h"
#include "ui_task.h"
#include "ring_buffer.h"

// Интеграция официального USB ядра Artery чистого класса Audio
#include "usbd_core.h"
#include "audio_class.h"
#include "audio_desc.h"
#include "audio_codec.h"

#include "FreeRTOS.h"
#include "task.h"

// Глобальные объекты стека
usbd_core_type usb_core_dev;
ring_buffer_t  audio_ring_buf;

// Атомарные переменные для передачи состояния в таск дисплея
volatile uint8_t g_usb_volume_percent = 100;
volatile uint8_t g_usb_mute_status    = 0;

// Реализация задержки для оригинального usbd_core.c
void usb_delay_ms(uint32_t ms) {
  // Проверяем регистр IPSR: если он не равен 0, значит мы СТРОГО внутри аппаратного прерывания!
  if (__get_IPSR() != 0) {
    // Внутри прерывания делаем ТОЛЬКО жесткий пустой цикл (никаких vTaskDelay!)
    volatile uint32_t count = ms * 19200;  // Под частоту 192 МГц
    while (count--);
  } else {
    // Если мы в обычном таске FreeRTOS — спим красиво и отдаем такты другим задачам
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
      vTaskDelay(pdMS_TO_TICKS(ms));
    } else {
      volatile uint32_t count = ms * 19200;
      while (count--);
    }
  }
}

// Поток мигания светодиодом (контроль работы FreeRTOS)
void v_blink_task(void* pv_parameters) {
  (void)pv_parameters;
  while (1) {
    LED_SYSTEM_TOGGLE;
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

// Критический калбэк: вызывается из прерывания DMA2 при опустошении половины пинг-понг буфера
void audio_buffer_ready_callback(uint16_t* buf) {
  size_t bytes_to_read = AUDIO_HALF_SIZE * sizeof(uint16_t);

  if (ring_buffer_get_available(&audio_ring_buf) >= bytes_to_read) {
    ring_buffer_read(&audio_ring_buf, (uint8_t*)buf, bytes_to_read);
  } else {
    for (uint32_t i = 0; i < AUDIO_HALF_SIZE; i++) {
      buf[i] = 0;
    }
  }
}

// Калбэки изменения громкости и Mute, вызываемые из оригинального аудио-кодека Artery
void my_codec_volume_handler(uint16_t vol_percent) {
  if (vol_percent > 100) vol_percent = 100;
  g_usb_volume_percent = (uint8_t)vol_percent;
}

void my_codec_mute_handler(uint8_t mute) {
  g_usb_mute_status = mute;
}

// Заводская функция автовыбора и деления частоты USB 48 МГц из примера ArteryTek
void usb_clock48m_select(void) {
  crm_clocks_freq_type clocks_struct;
  crm_clocks_freq_get(&clocks_struct);

  // При частоте ядра 192 МГц выставляем делитель 4.0 (2bits маска = CRM_USB_DIV_4)
  if (clocks_struct.sclk_freq == 192000000) {
    crm_usb_clock_div_set(CRM_USB_DIV_4);
  }
}

// Инициализация аппаратного тактирования и безопасных приоритетов блока USBFS
void usb_hardware_init(void) {
  // 1. Программный сброс линии DP для принудительного рестарта перечисления в Windows
  crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);
  gpio_init_type gpio_usb_reset;
  gpio_usb_reset.gpio_pins           = GPIO_PINS_12;
  gpio_usb_reset.gpio_mode           = GPIO_MODE_OUTPUT;
  gpio_usb_reset.gpio_out_type       = GPIO_OUTPUT_PUSH_PULL;
  gpio_usb_reset.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
  gpio_init(GPIOA, &gpio_usb_reset);

  gpio_bits_reset(GPIOA, GPIO_PINS_12);
  usb_delay_ms(200);

  // 2. Конфигурируем и выравниваем 48 МГц для USB блока по частотометру CRM
  usb_clock48m_select();

  // Включаем периферийное тактирование USB и модуля автокалибровки ACC
  crm_periph_clock_enable(CRM_USB_PERIPH_CLOCK, TRUE);
  crm_periph_clock_enable(CRM_PWC_PERIPH_CLOCK, TRUE);
  crm_periph_clock_enable(CRM_IOMUX_PERIPH_CLOCK, TRUE);

  // 3. Возвращаем пин в режим работы аппаратного USB
  gpio_usb_reset.gpio_mode = GPIO_MODE_INPUT;
  gpio_init(GPIOA, &gpio_usb_reset);

  // Безопасный приоритет прерывания USB для FreeRTOS (Приоритет 11)
  nvic_irq_enable(USBFS_L_CAN1_RX0_IRQn, 11, 0);
}

// Поток инициализации USB
void v_usb_init_task(void* pv_parameters) {
  (void)pv_parameters;

  vTaskDelay(pdMS_TO_TICKS(500));
  usb_hardware_init();

  // Включаем SOF обработчик обратно! Он критически важен заводскому USB-ядру
  // для внутренней синхронизации пакетов Audio DAC и предотвращения тайм-аутов хоста Windows
  // audio_class_handler.sof_handler = audio_class_sof_handler;

  // Инициализируем оригинальный стек чистым аудио-классом и дескрипторами из SDK
  usbd_core_init(&usb_core_dev, USB, &audio_class_handler, &audio_desc_handler, 0);
  vTaskDelay(pdMS_TO_TICKS(50));

  // Включаем внутреннюю подтяжку DP (Бит 18 регистра CTRL блока USB)
  USB->ctrl |= (1U << 18);
  vTaskDelay(pdMS_TO_TICKS(50));

  usbd_connect(&usb_core_dev);

  vTaskDelete(NULL);
}

// Вектор аппаратного прерывания USB
void usbd_irq_handler(usbd_core_type* udev);
void USBFS_L_CAN1_RX0_IRQHandler(void) {
  usbd_irq_handler(&usb_core_dev);
}

int main(void) {
  // Инициализация системной частоты ядра (192 МГц)
  system_clock_config();
  nvic_priority_group_config(NVIC_PRIORITY_GROUP_4);
  init_periph();

  // Настройка оригинальных указателей на функции кодека ArteryTek
  extern void (*audio_codec_spk_volume_set)(uint16_t volume);
  extern void (*audio_codec_spk_mute_set)(uint8_t mute);
  audio_codec_spk_volume_set = my_codec_volume_handler;
  audio_codec_spk_mute_set   = my_codec_mute_handler;

  // Инициализация аудиовыхода PCM5102 (Шина I2S2 Philips Master, 48 кГц)
  audio_callbacks_t audio_cbs = {.on_buffer_ready = audio_buffer_ready_callback};
  pcm5102_set_callbacks(&audio_cbs);
  pcm5102_init(48000);

  // Опускаем приоритет прерывания DMA I2S до безопасного для FreeRTOS уровня!
  nvic_irq_enable(DMA2_Channel4_5_IRQn, 6, 0);

  pcm5102_start();

  // Инициализируем кольцевой буфер перед запуском USB
  ring_buffer_init(&audio_ring_buf);

  // Создание операционных тасков FreeRTOS
  xTaskCreate(v_blink_task, "Blink", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
  xTaskCreate(v_tft_audio_ui_task, "TFT_UI", configMINIMAL_STACK_SIZE * 3, NULL, tskIDLE_PRIORITY + 2, NULL);
  xTaskCreate(v_usb_init_task, "USB_Init", configMINIMAL_STACK_SIZE * 3, NULL, tskIDLE_PRIORITY + 3, NULL);

  vTaskStartScheduler();

  while (1) {
  }
}
