#include "audio_codec.h"
#include "ring_buffer.h"  // ИСПРАВЛЕНО: подключаем наш потокобезопасный кольцевой буфер
#include <string.h>

// Глобальная ядерная структура состояния оригинального аудио-драйвера ArteryTek
audio_codec_type audio_codec;

// Локальные калбэки для передачи уровня звука на дисплей в ui_task
void (*audio_codec_spk_volume_set)(uint16_t volume) = NULL;
void (*audio_codec_spk_mute_set)(uint8_t mute)      = NULL;

// Заглушка частоты
void audio_codec_set_spk_freq(uint32_t freq) {
  audio_codec.audio_freq = freq;
}
void audio_codec_set_mic_freq(uint32_t freq) { (void)freq; }

// Заглушки альтернат-настроек оригинального класса
void audio_codec_spk_alt_setting(uint32_t alt_seting) { (void)alt_seting; }
void audio_codec_mic_alt_setting(uint32_t alt_seting) { (void)alt_seting; }

// Заглушки управления микрофоном (так как он полностью вырезан)
void     audio_codec_set_mic_mute(uint8_t mute) { (void)mute; }
void     audio_codec_set_mic_volume(uint16_t volume) { (void)volume; }
uint32_t audio_codec_mic_get_data(uint8_t* buffer) {
  (void)buffer;
  return 0;
}

// Обработка Mute динамика от Windows: перенаправляем на наш экран
void audio_codec_set_spk_mute(uint8_t mute) {
  audio_codec.spk_mute = mute;
  if (audio_codec_spk_mute_set != NULL) {
    audio_codec_spk_mute_set(mute);
  }
}

// Обработка громкости динамика от Windows: пересчитываем шаги WM8988 в понятные экрану 0-100%
void audio_codec_set_spk_volume(uint16_t volume) {
  audio_codec.spk_volume = volume;
  if (audio_codec_spk_volume_set != NULL) {
    // В оригинальном примере громкость идет от 0 до 100%, передаем напрямую на экран
    audio_codec_spk_volume_set(volume);
  }
}

// Заглушка расчета фидбэка изохронной точки
uint8_t audio_codec_spk_feedback(uint8_t* feedback) {
  uint32_t feedback_value = 48000;
  feedback_value          = ((feedback_value / 1000) << 14) | ((feedback_value % 1000) << 4);
  feedback[0]             = (uint8_t)(feedback_value);
  feedback[1]             = (uint8_t)(feedback_value >> 8);
  feedback[2]             = (uint8_t)(feedback_value >> 16);
  return 3;
}

// --- НАШ ГЛАВНЫЙ ОПЕРАЦИОННЫЙ ПЕРЕХВАТ ЗВУКА ИЗ USB В ЦАП PCM5102 ---
// Эта функция вызывается оригинальным ядром audio_class.c каждую миллисекунду, когда от ПК прилетает пакет данных
void audio_codec_spk_fifo_write(uint8_t* data, uint32_t len) {
  extern ring_buffer_t audio_ring_buf;
  // Без задержек и копирований в DMA-буферы примера, гоним звук напрямую в наш кольцевой буфер I2S2 шины!
  ring_buffer_write(&audio_ring_buf, data, len);
}

// Заглушка функции модификации частоты по I2C
void audio_codec_modify_freq(uint32_t freq) { (void)freq; }

// Чистая инициализация кодека при старте (оригинальное ядро вызывает её в main.c)
error_status audio_codec_init(void) {
  memset(&audio_codec, 0, sizeof(audio_codec_type));
  audio_codec.audio_freq = 48000;
  audio_codec.audio_bitw = 16;
  return SUCCESS;
}

// Пустой цикл кодека, чтобы не спамить шину I2C в таске main
error_status audio_codec_loop(void) {
  return SUCCESS;
}
