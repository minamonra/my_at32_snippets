#include "si4703.h"
#include "common.h"
#include "at32f403a_407_i2c.h"

// Дополнительные инклюды FreeRTOS
#include "FreeRTOS.h"
#include "task.h"

#define I2C_STS2_BUSY (1 << 1)

// Настройки диапазона и шага, строго скопированные с GitHub (в десятках кГц)
static uint16_t _freqLow   = 8750;  // 87.5 МГц = 8750 десятков кГц
static uint8_t  _freqSteps = 10;    // Шаг 100 кГц = 10 десятков кГц

// Единый массив для хранения всех 16 регистров чипа
static uint16_t registers[16];

// Жесткая Big-Endian запись конфигурационных регистров 0x02..0x07 (как в _saveRegisters)
void si4703_write_registers(void) {
  i2c_type* i2c_port = I2C1;
  uint32_t  timeout;

  timeout = 50000;
  while (i2c_port->sts2 & I2C_STS2_BUSY) {
    if (timeout-- == 0) return;
  }

  i2c_start_generate(i2c_port);
  timeout = 50000;
  while (i2c_flag_get(i2c_port, I2C_STARTF_FLAG) == RESET) {
    if (timeout-- == 0) return;
  }

  // Отправка адреса устройства на запись (0x20 для Artery)
  i2c_7bit_address_send(i2c_port, SI4703_I2C_ADDR, 0);
  timeout = 50000;
  while (i2c_flag_get(i2c_port, I2C_ADDR7F_FLAG) == RESET) {
    if (timeout-- == 0) return;
  }

  (void)i2c_port->sts1;
  (void)i2c_port->sts2;

  // Выдаем регистры строго подряд с 0x02 по 0x07
  for (int regSpot = 0x02; regSpot < 0x08; regSpot++) {
    uint16_t reg_val = registers[regSpot];
    uint8_t  hi      = (uint8_t)(reg_val >> 8);
    uint8_t  lo      = (uint8_t)(reg_val & 0xFF);

    // Отправляем старший байт и ЖДЕМ его физической отправки
    i2c_data_send(i2c_port, hi);
    timeout = 50000;
    while (i2c_flag_get(i2c_port, I2C_TDBE_FLAG) == RESET) {
      if (timeout-- == 0) return;
    }

    // Отправляем младший байт и ЖДЕМ его физической отправки
    i2c_data_send(i2c_port, lo);
    timeout = 50000;
    while (i2c_flag_get(i2c_port, I2C_TDBE_FLAG) == RESET) {
      if (timeout-- == 0) return;
    }
  }

  // Финальное ожидание освобождения буфера перед СТОП-битом
  timeout = 50000;
  while (i2c_flag_get(i2c_port, I2C_TDBE_FLAG) == RESET) {
    if (timeout-- == 0) return;
  }

  i2c_stop_generate(i2c_port);
  delay_us(10);
}

// Надежное циклическое чтение 32 байт (16 регистров), начиная с 0x0A (как в _readRegisters)
uint8_t si4703_read_registers(void) {
  i2c_type* i2c_port = I2C1;
  uint32_t  timeout;

  timeout = 50000;
  while (i2c_port->sts2 & I2C_STS2_BUSY) {
    if (timeout-- == 0) return 0;
  }

  i2c_start_generate(i2c_port);
  timeout = 50000;
  while (i2c_flag_get(i2c_port, I2C_STARTF_FLAG) == RESET) {
    if (timeout-- == 0) return 0;
  }

  // Отправка адреса устройства на чтение (направление 1)
  i2c_7bit_address_send(i2c_port, SI4703_I2C_ADDR, 1);
  timeout = 50000;
  while (i2c_flag_get(i2c_port, I2C_ADDR7F_FLAG) == RESET) {
    if (timeout-- == 0) return 0;
  }

  (void)i2c_port->sts1;
  (void)i2c_port->sts2;

  int x = 0x0A;
  for (int count = 0; count < 16; count++) {
    if (x == 0x10) x = 0;

    if (count < 15) {
      i2c_ack_enable(i2c_port, TRUE);
    }

    // Чтение старшего байта (HI)
    timeout = 50000;
    while (i2c_flag_get(i2c_port, I2C_RDBF_FLAG) == RESET) {
      if (timeout-- == 0) return 0;
    }
    uint8_t hi = i2c_data_receive(i2c_port);

    // Перед чтением самого последнего байта выставляем NACK (count == 15)
    if (count == 15) {
      i2c_ack_enable(i2c_port, FALSE);
    }

    // Чтение младшего байта (LO)
    timeout = 50000;
    while (i2c_flag_get(i2c_port, I2C_RDBF_FLAG) == RESET) {
      if (timeout-- == 0) return 0;
    }
    uint8_t lo = i2c_data_receive(i2c_port);

    registers[x] = ((uint16_t)hi << 8) | lo;
    x++;
  }

  i2c_stop_generate(i2c_port);
  return 1;
}

// Быстрое чтение только статусного регистра 0x0A (как в _readRegister0A)
static void si4703_read_register0A(void) {
  i2c_type* i2c_port = I2C1;
  uint32_t  timeout;

  timeout = 50000;
  while (i2c_port->sts2 & I2C_STS2_BUSY) {
    if (timeout-- == 0) return;
  }

  i2c_start_generate(i2c_port);
  timeout = 50000;
  while (i2c_flag_get(i2c_port, I2C_STARTF_FLAG) == RESET) {
    if (timeout-- == 0) return;
  }

  i2c_7bit_address_send(i2c_port, SI4703_I2C_ADDR, 1);
  timeout = 50000;
  while (i2c_flag_get(i2c_port, I2C_ADDR7F_FLAG) == RESET) {
    if (timeout-- == 0) return;
  }

  (void)i2c_port->sts1;
  (void)i2c_port->sts2;

  i2c_ack_enable(i2c_port, FALSE);

  timeout = 50000;
  while (i2c_flag_get(i2c_port, I2C_RDBF_FLAG) == RESET) {
    if (timeout-- == 0) return;
  }
  uint8_t hi = i2c_data_receive(i2c_port);

  timeout = 50000;
  while (i2c_flag_get(i2c_port, I2C_RDBF_FLAG) == RESET) {
    if (timeout-- == 0) return;
  }
  uint8_t lo = i2c_data_receive(i2c_port);

  registers[0x0A] = ((uint16_t)hi << 8) | lo;
  i2c_stop_generate(i2c_port);
}

// Автомат ожидания окончания настройки частоты с GitHub (_waitEnd) с защитой от зависания
static void si4703_wait_end(void) {
  uint32_t safety_timeout;

  // 1. Ждем поднятия флага STC в единицу
  safety_timeout = 200;
  do {
    delay_ms(5);
    si4703_read_register0A();
    if (safety_timeout-- == 0) break;
  } while ((registers[0x0A] & STC) == 0);

  // 2. Снимаем биты TUNE и SEEK БЕЗ предварительного полного чтения, чтобы не затереть ОЗУ
  registers[0x02] &= ~(1 << SEEK);
  registers[0x03] &= ~(1 << TUNE);
  si4703_write_registers();

  delay_ms(20);

  // 3. Ждем, пока чип подтвердит сброс и опустит флаг STC в ноль
  safety_timeout = 100;
  do {
    delay_ms(5);
    si4703_read_registers();
    if (safety_timeout-- == 0) break;
  } while ((registers[0x0A] & STC) != 0);
}

// Полный аналог комбинации методов init() + setBand() с GitHub, но с MUTE
uint8_t si4703_init(void) {
  gpio_init_type gpio_init_struct;

  crm_periph_clock_enable(SI4703_CRM_GPIO_CLK, TRUE);
  crm_periph_clock_enable(CRM_I2C1_PERIPH_CLOCK, TRUE);

  gpio_default_para_init(&gpio_init_struct);
  gpio_init_struct.gpio_mode           = GPIO_MODE_OUTPUT;
  gpio_init_struct.gpio_out_type       = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init_struct.gpio_pins           = SI4703_PIN_RST | SI4703_PIN_SDIO | SI4703_PIN_SCLK | GPIO_PINS_4;
  gpio_init(SI4703_GPIO_PORT, &gpio_init_struct);

  // Сброс: SEN = 1, SDIO = 0
  gpio_bits_write(SI4703_GPIO_PORT, GPIO_PINS_4, TRUE);
  gpio_bits_write(SI4703_GPIO_PORT, SI4703_PIN_SCLK, FALSE);
  gpio_bits_write(SI4703_GPIO_PORT, SI4703_PIN_SDIO, FALSE);
  gpio_bits_write(SI4703_GPIO_PORT, SI4703_PIN_RST, FALSE);
  delay_ms(20);

  gpio_bits_write(SI4703_GPIO_PORT, SI4703_PIN_RST, TRUE);
  delay_ms(20);

  gpio_init_struct.gpio_mode     = GPIO_MODE_MUX;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_OPEN_DRAIN;
  gpio_init_struct.gpio_pins     = SI4703_PIN_SCLK | SI4703_PIN_SDIO;
  gpio_init(SI4703_GPIO_PORT, &gpio_init_struct);

  gpio_bits_write(SI4703_GPIO_PORT, GPIO_PINS_4, TRUE);

  i2c_init(I2C1, 0, 100000);
  i2c_enable(I2C1, TRUE);

  for (int i = 0; i < 16; i++) registers[i] = 0x0000;

  // Шаг 1: Активация генератора опорного кварца (XOSCEN = 1 в 0x07)
  si4703_read_registers();
  registers[0x07] = 0x8100;
  si4703_write_registers();
  vTaskDelay(pdMS_TO_TICKS(300));

  // Шаг 2: Включение чипа (ENABLE = 1, RDSMODE = 1)
  // КРИТИЧЕСКИ: Удерживаем биты DMUTE и DSMUTE в 0. Звук железно закрыт!
  // si4703_read_registers();
  registers[0x02] = 0x4001;

  registers[0x04] = (1 << 12) | 0x0800;  // RDS = 1, DEEMPHASIS = 50 мкс

  registers[0x05] = 0x0010 | 0x1000;  // Сетка 100 кГц + Чувствительность SEEKTH_MID
  registers[0x05] &= ~(0x000F);       // Жестко зануляем младшие 4 бита громкости (Громкость = 0)

  // Предрассчитываем стартовый канал для 99.6 МГц (9960) прямо внутри инита!
  // Это не даст чипу включить базовые 87.5 МГц ни на секунду.
  int start_channel = (9960 - _freqLow) / _freqSteps;
  registers[0x03]   = (start_channel & 0x03FF);  // Записываем канал в буфер

  si4703_write_registers();
  delay_ms(100);

  return 1;
}

// Смена частоты (Точная Си-копия оригинального метода SI4703::setFrequency)
void si4703_set_frequency(uint16_t newF) {
  if (newF < _freqLow) newF = _freqLow;
  if (newF > 10800) newF = 10800;

  si4703_read_registers();

  int channel = (newF - _freqLow) / _freqSteps;

  registers[0x03] &= 0xFC00;
  registers[0x03] |= (channel & 0x03FF);
  registers[0x03] |= (1 << TUNE);

  si4703_write_registers();

  // Перестройка частоты происходит в абсолютной тишине
  si4703_wait_end();

  // И ТОЛЬКО ТЕПЕРЬ, когда частота 99.6 МГц полностью залочена, открываем аудиовыход
  si4703_read_registers();
  registers[0x02] |= (1 << DMUTE);
  registers[0x02] |= (1 << DSMUTE);
  si4703_write_registers();
}

// Чтение реальной частоты (Точная Си-копия оригинального метода SI4703::getFrequency)
uint16_t si4703_get_frequency(void) {
  si4703_read_registers();
  int      channel  = registers[0x0B] & 0x03FF;
  uint16_t freq_raw = (channel * _freqSteps) + _freqLow;
  return freq_raw / 10;
}

// Изменение уровня звука
void si4703_set_volume(uint8_t volume) {
  if (volume > 15) volume = 15;
  si4703_read_registers();
  registers[0x05] &= ~(0x000F);
  registers[0x05] |= volume;
  si4703_write_registers();
}

uint16_t si4703_get_chip_id(void) {
  si4703_read_registers();
  return registers[0x01];
}

uint8_t si4703_get_rssi(void) {
  si4703_read_registers();
  return (uint8_t)(registers[0x0A] & 0x00FF);
}

uint8_t si4703_is_stereo(void) {
  si4703_read_registers();
  if (registers[0x0A] & 0x0100) return 1;
  return 0;
}

// Полная 100% копия оригинальной логики инициализации Arduino-библиотеки на чистом Си
uint8_t si4703_init1(void) {
  gpio_init_type gpio_init_struct;

  // Включаем тактование портов контроллера Artery
  crm_periph_clock_enable(SI4703_CRM_GPIO_CLK, TRUE);
  crm_periph_clock_enable(CRM_I2C1_PERIPH_CLOCK, TRUE);

  // --- Эквивалент блока: if (_sdaPin >= 0) { pinMode(_sdaPin, OUTPUT); digitalWrite(_sdaPin, LOW); delay(5); } ---
  // Переводим пин SDIO (SDA) в режим обычного выхода GPIO и прижимаем к земле
  gpio_default_para_init(&gpio_init_struct);
  gpio_init_struct.gpio_mode           = GPIO_MODE_OUTPUT;
  gpio_init_struct.gpio_out_type       = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init_struct.gpio_pins           = SI4703_PIN_SDIO;
  gpio_init(SI4703_GPIO_PORT, &gpio_init_struct);
  gpio_bits_write(SI4703_GPIO_PORT, SI4703_PIN_SDIO, FALSE);  // SDIO = LOW
  delay_ms(5);

  // --- Эквивалент RADIO::init() (Импульс сброса) ---
  // 1. pinMode(_resetPin, OUTPUT); digitalWrite(_resetPin, LOW); delay(5);
  gpio_init_struct.gpio_pins = SI4703_PIN_RST;
  gpio_init(SI4703_GPIO_PORT, &gpio_init_struct);
  gpio_bits_write(SI4703_GPIO_PORT, SI4703_PIN_RST, FALSE);  // RST = LOW (Чип в сбросе)
  delay_ms(5);

  // 2. digitalWrite(_resetPin, HIGH); delay(5);
  gpio_bits_write(SI4703_GPIO_PORT, SI4703_PIN_RST, TRUE);  // RST = HIGH (Вывели из сброса)
  delay_ms(5);

  // --- Эквивалент _i2cPort->begin(); ---
  // Теперь, когда чип аппаратно зафиксировал режим 2-wire, отдаем пины под аппаратный I2C1
  gpio_init_struct.gpio_mode     = GPIO_MODE_MUX;
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_OPEN_DRAIN;
  gpio_init_struct.gpio_pins     = SI4703_PIN_SCLK | SI4703_PIN_SDIO;
  gpio_init(SI4703_GPIO_PORT, &gpio_init_struct);

  i2c_init(I2C1, 0, 100000);
  i2c_enable(I2C1, TRUE);

  // Очищаем наш ОЗУ-массив перед работой
  for (int i = 0; i < 16; i++) {
    registers[i] = 0x0000;
  }

  // --- Эквивалент _readRegisters(); ---
  si4703_read_registers();  // Вычитываем текущую заводскую карту регистров

  // --- Эквивалент registers[0x07] = 0x8100; _saveRegisters(); ---
  registers[0x07] = 0x8100;  // Активируем встроенный кварцевый генератор XOSCEN
  si4703_write_registers();  // Отправляем изменения в чип

  // --- Эквивалент delay(500); ---
  delay_ms(500);  // Ждем строго полсекунды, пока часовой кварц на модуле стабилизируется

  // Возвращаем TRUE (1), как оригинальный метод при успешном завершении шагов
  return 1;
}

// Полная Си-копия оригинального метода SI4703::setBand(RADIO_BAND_FM) с GitHub
void si4703_set_band_fm(void) {
  si4703_read_registers();

  // 1. Конфигурация POWERCFG (0x02) -> Включаем ИС (0x0001) + verbose RDSMODE (0x4000)
  registers[0x02] = 0x4001;

  // Отключаем MUTE и SOFTMUTE (выставляем биты DMUTE и DSMUTE в 1)
  registers[0x02] |= (1 << 14);  // DMUTE
  registers[0x02] |= (1 << 15);  // DSMUTE

  // 2. Конфигурация SYSCONFIG1 (0x04) -> Включаем RDS (1 << 12) + предыскажения 50мкс (0x0800)
  registers[0x04] |= (1 << 12);  // RDS = 1
  registers[0x04] |= 0x0800;     // DEEMPHASIS50 = 1 (Европа/РФ)

  // 3. Конфигурация SYSCONFIG2 (0x05) -> Сетка 100 кГц (0x0010) + Громкость = 1
  // По умолчанию на GitHub громкость инициализируется на уровень 1
  registers[0x05] &= ~(0x0030);  // Очищаем биты FMSPACE
  registers[0x05] |= 0x0010;     // FMSPACE_100

  registers[0x05] &= ~(0x000F);  // Очищаем биты VOLUME
  registers[0x05] |= 0x0001;     // Громкость = 1

  // 4. Дополнительные параметры автопоиска с GitHub
  registers[0x05] |= 0x1000;     // SEEKTH_MID (0x1000)
  registers[0x06] &= ~(0x00F0);  // Очищаем SKSNR
  registers[0x06] |= 0x0030;     // SKSNR_MID (0x0030)

  // Сохраняем все регистры в чип
  si4703_write_registers();

  // Оригинальная задержка с GitHub: время на включение аналогового тракта
  delay_ms(110);
}