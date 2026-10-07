#ifndef SI4703_H
#define SI4703_H

#include <stdint.h>

// Физические пины управления и шины I2C1
#define SI4703_CRM_GPIO_CLK CRM_GPIOB_PERIPH_CLOCK
#define SI4703_GPIO_PORT    GPIOB
#define SI4703_PIN_SCLK     GPIO_PINS_6  // PB6 - I2C1_SCL
#define SI4703_PIN_SDIO     GPIO_PINS_7  // PB7 - I2C1_SDA
#define SI4703_PIN_RST      GPIO_PINS_5  // PB5 - Аппаратный Reset

// I2C адрес чипа (без левого сдвига для Artery = 0x10)
#define SI4703_I2C_ADDR 0x20

// Битовые маски регистра POWERCFG (0x02)
#define DSMUTE  15
#define DMUTE   14
#define SETMONO 13
#define RDSMODE 11
#define SKMODE  10
#define SEEKUP  9
#define SEEK    8

// Битовые маски регистра CHANNEL (0x03)
#define TUNE 15

// Битовые маски регистра STATUSRSSI (0x0A)
#define STC  0x4000  // Seek Tune Complete
#define SFBL 0x2000  // Seek Fail Band Limit

// Прототипы функций интерфейса на Си
uint8_t  si4703_init(void);
void     si4703_set_volume(uint8_t newVolume);
void     si4703_set_frequency(uint16_t newF);
uint16_t si4703_get_frequency(void);
uint8_t  si4703_read_registers(void);
uint16_t si4703_get_chip_id(void);
uint8_t  si4703_get_rssi(void);
uint8_t  si4703_is_stereo(void);
uint8_t  si4703_init1(void);
void     si4703_set_band_fm(void);

#endif  // SI4703_H
