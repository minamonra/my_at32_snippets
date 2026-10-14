#include "pcm5102.h"
#include "at32f403a_407_dma.h"
#include "at32f403a_407_spi.h"

/* Наш циклический пинг-понг буфер */
uint16_t                 audio_buffer[AUDIO_BUF_SIZE];
static audio_callbacks_t callbacks = {0};

uint16_t* pcm5102_get_buffer1(void) { return &audio_buffer[0]; }
uint16_t* pcm5102_get_buffer2(void) { return &audio_buffer[AUDIO_HALF_SIZE]; }

void pcm5102_set_callbacks(audio_callbacks_t* cb) {
  if (cb) callbacks.on_buffer_ready = cb->on_buffer_ready;
}

void pcm5102_init(uint32_t sample_rate) {
  gpio_init_type gpio_init_struct;
  i2s_init_type  i2s_init_struct;
  dma_init_type  dma_init_struct;

  /* 1. Включение тактирования шин GPIO B, Альтернативных функций (IOMUX), I2S2 (блок SPI2) и DMA2 */
  crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, TRUE);
  crm_periph_clock_enable(CRM_IOMUX_PERIPH_CLOCK, TRUE);
  crm_periph_clock_enable(CRM_SPI2_PERIPH_CLOCK, TRUE);
  crm_periph_clock_enable(CRM_DMA2_PERIPH_CLOCK, TRUE);

  /* 2. Конфигурация пинов: PB12 (WS), PB13 (BCK), PB15 (SD) как мультиплексированный Push-Pull */
  gpio_init_struct.gpio_pins           = GPIO_PINS_12 | GPIO_PINS_13 | GPIO_PINS_15;
  gpio_init_struct.gpio_mode           = GPIO_MODE_MUX;
  gpio_init_struct.gpio_out_type       = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init(GPIOB, &gpio_init_struct);

  /* 3. Конфигурация DMA2_CHANNEL5 (Канал аппаратного передатчика I2S2_TX) */
  dma_reset(DMA2_CHANNEL5);
  dma_default_para_init(&dma_init_struct);
  dma_init_struct.peripheral_base_addr  = (uint32_t)&(SPI2->dt);
  dma_init_struct.memory_base_addr      = (uint32_t)audio_buffer;
  dma_init_struct.direction             = DMA_DIR_MEMORY_TO_PERIPHERAL;
  dma_init_struct.buffer_size           = AUDIO_BUF_SIZE;
  dma_init_struct.peripheral_inc_enable = FALSE;
  dma_init_struct.memory_inc_enable     = TRUE;
  dma_init_struct.peripheral_data_width = DMA_PERIPHERAL_DATA_WIDTH_HALFWORD;
  dma_init_struct.memory_data_width     = DMA_MEMORY_DATA_WIDTH_HALFWORD;
  dma_init_struct.loop_mode_enable      = TRUE; /* Циклический режим (Circular) */
  dma_init_struct.priority              = DMA_PRIORITY_VERY_HIGH;
  dma_init(DMA2_CHANNEL5, &dma_init_struct);

  /* Включение прерываний по половине и полному проходу буфера */
  dma_interrupt_enable(DMA2_CHANNEL5, DMA_FDT_INT | DMA_HDT_INT, TRUE);

  /* Регистрация вектора прерывания DMA2 для каналов 4 и 5 */
  nvic_irq_enable(DMA2_Channel4_5_IRQn, 1, 0);

  /* 4. Конфигурация шины I2S2 (Philips Master, 16 бит данных в 32-бит слоте) */
  i2s_default_para_init(&i2s_init_struct);
  i2s_init_struct.audio_protocol      = I2S_AUDIO_PROTOCOL_PHILLIPS;
  i2s_init_struct.data_channel_format = I2S_DATA_16BIT_CHANNEL_32BIT;
  i2s_init_struct.mclk_output_enable  = FALSE; /* ИСПРАВЛЕНО: confirm_state принимает TRUE/FALSE */
  i2s_init_struct.audio_sampling_freq = sample_rate;
  i2s_init_struct.operation_mode      = I2S_MODE_MASTER_TX;
  i2s_init_struct.clock_polarity      = I2S_CLOCK_POLARITY_LOW;
  i2s_init(SPI2, &i2s_init_struct);
}

void pcm5102_start(void) {
  dma_channel_enable(DMA2_CHANNEL5, TRUE);    /* 1. Стартуем DMA канал */
  i2s_enable(SPI2, TRUE);                     /* 2. Включаем шину I2S2 */
  spi_i2s_dma_transmitter_enable(SPI2, TRUE); /* 3. Открываем аппаратные запросы DMA */
}

void pcm5102_stop(void) {
  spi_i2s_dma_transmitter_enable(SPI2, FALSE);
  dma_channel_enable(DMA2_CHANNEL5, FALSE);
  i2s_enable(SPI2, FALSE);
}

/* Прерывание объединенной группы DMA2 (Каналы 4 и 5) */
void DMA2_Channel4_5_IRQHandler(void) {
  if (dma_interrupt_flag_get(DMA2_HDT5_FLAG) != RESET) {
    dma_flag_clear(DMA2_HDT5_FLAG);
    if (callbacks.on_buffer_ready) {
      callbacks.on_buffer_ready(pcm5102_get_buffer1());
    }
  }
  if (dma_interrupt_flag_get(DMA2_FDT5_FLAG) != RESET) {
    dma_flag_clear(DMA2_FDT5_FLAG);
    if (callbacks.on_buffer_ready) {
      callbacks.on_buffer_ready(pcm5102_get_buffer2());
    }
  }
}
