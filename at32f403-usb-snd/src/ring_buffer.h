#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#include <stdint.h>
#include <stddef.h>

// Размер кольцевого буфера в байтах
// Закладываем 4096 байт — этого с запасом хватит для сглаживания джиттера пакетов ОС
#define RING_BUFFER_SIZE 4096

typedef struct {
  uint8_t         storage[RING_BUFFER_SIZE];
  volatile size_t head;
  volatile size_t tail;
} ring_buffer_t;

// Инициализация буфера
void ring_buffer_init(ring_buffer_t* rb);

// Запись байт в буфер (Вызывается из прерывания USB)
size_t ring_buffer_write(ring_buffer_t* rb, const uint8_t* data, size_t len);

// Чтение байт из буфера (Вызывается из прерывания DMA I2S)
size_t ring_buffer_read(ring_buffer_t* rb, uint8_t* data, size_t len);

// Получить количество доступных данных для чтения
size_t ring_buffer_get_available(const ring_buffer_t* rb);

#endif  // __RING_BUFFER_H__
