#ifndef __AUDIO_CODEC_H
#define __AUDIO_CODEC_H

#include "at32f403a_407.h"

// Минимальная структура состояния для совместимости с оригинальным audio_class.c
typedef struct {
  uint32_t audio_freq;
  uint8_t  audio_bitw;
  uint32_t spk_freq;
  uint16_t spk_volume;
  uint8_t  spk_mute;
  uint16_t mic_volume;
  uint8_t  mic_mute;
} audio_codec_type;

extern audio_codec_type audio_codec;

// Прототипы функций, которые вызывает оригинальное ядро USB Middleware ArteryTek
error_status audio_codec_init(void);
error_status audio_codec_loop(void);
void         audio_codec_set_spk_freq(uint32_t freq);
void         audio_codec_set_mic_freq(uint32_t freq);
void         audio_codec_spk_alt_setting(uint32_t alt_seting);
void         audio_codec_mic_alt_setting(uint32_t alt_seting);
void         audio_codec_set_spk_volume(uint16_t volume);
void         audio_codec_set_mic_volume(uint16_t volume);
void         audio_codec_set_spk_mute(uint8_t mute);
void         audio_codec_set_mic_mute(uint8_t mute);
uint8_t      audio_codec_spk_feedback(uint8_t* feedback);
void         audio_codec_spk_fifo_write(uint8_t* data, uint32_t len);
uint32_t     audio_codec_mic_get_data(uint8_t* buffer);

#endif /* __AUDIO_CODEC_H */
