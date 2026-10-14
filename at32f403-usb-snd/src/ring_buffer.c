#include "ring_buffer.h"

void ring_buffer_init(ring_buffer_t* rb) {
  rb->head = 0;
  rb->tail = 0;
}

size_t ring_buffer_get_available(const ring_buffer_t* rb) {
  size_t head = rb->head;
  size_t tail = rb->tail;

  if (head >= tail) {
    return (head - tail);
  } else {
    return (RING_BUFFER_SIZE - tail + head);
  }
}

size_t ring_buffer_write(ring_buffer_t* rb, const uint8_t* data, size_t len) {
  size_t head    = rb->head;
  size_t tail    = rb->tail;
  size_t written = 0;

  while (written < len) {
    size_t next_head = (head + 1) % RING_BUFFER_SIZE;

    // Проверка на переполнение буфера
    if (next_head == tail) {
      break;
    }

    rb->storage[head] = data[written];
    head              = next_head;
    written++;
  }

  rb->head = head;  // Атомарное обновление указателя записи
  return written;
}

size_t ring_buffer_read(ring_buffer_t* rb, uint8_t* data, size_t len) {
  size_t head       = rb->head;
  size_t tail       = rb->tail;
  size_t read_bytes = 0;

  while (read_bytes < len) {
    // Проверка, не догнал ли указатель чтения указатель записи
    if (tail == head) {
      break;
    }

    data[read_bytes] = rb->storage[tail];
    tail             = (tail + 1) % RING_BUFFER_SIZE;
    read_bytes++;
  }

  rb->tail = tail;  // Атомарное обновление указателя чтения
  return read_bytes;
}
