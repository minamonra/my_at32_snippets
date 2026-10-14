#ifndef __PCM5102_H__
#define __PCM5102_H__

#include "at32f403a_407.h"

// 512 стерео-фреймов = 512 левых + 512 правых отсчетов = 1024 отсчета (Half-Words) на одну половину буфера
#define AUDIO_CHUNK_SIZE 512

#define AUDIO_HALF_SIZE (AUDIO_CHUNK_SIZE * 2)  // Размер половины буфера в uint16_t (1024)
#define AUDIO_BUF_SIZE  (AUDIO_CHUNK_SIZE * 4)  // Полный размер циклического буфера (2048)

typedef void (*audio_buff_ready_callback_t)(uint16_t* buf);

typedef struct {
  audio_buff_ready_callback_t on_buffer_ready;
} audio_callbacks_t;

uint16_t* pcm5102_get_buffer1(void);
uint16_t* pcm5102_get_buffer2(void);
void      pcm5102_set_callbacks(audio_callbacks_t* cb);

void pcm5102_init(uint32_t sample_rate);
void pcm5102_start(void);
void pcm5102_stop(void);

#endif  // __PCM5102_H__
