#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

// Переменная из линкер-скрипта, указывающая на начало кучи
extern int __io_putchar(int ch) __attribute__((weak));

int _close(int file) {
  (void)file;
  return -1;
}

int _fstat(int file, struct stat *st) {
  (void)file;
  st->st_mode = S_IFCHR;
  return 0;
}

int _isatty(int file) {
  (void)file;
  return 1;
}

int _lseek(int file, int ptr, int dir) {
  (void)file;
  (void)ptr;
  (void)dir;
  return 0;
}

int _read(int file, char *ptr, int len) {
  (void)file;
  (void)ptr;
  return len;
}

// Символьный вывод для printf. Сделан weak, чтобы потом переопределить через UART
__attribute__((weak)) int _write(int file, char *ptr, int len) {
  (void)file;
  int DataIdx;
  for (DataIdx = 0; DataIdx < len; DataIdx++) {
    #ifdef __io_putchar
      __io_putchar(*ptr++);
    #else
      (void)ptr;
    #endif
  }
  return len;
}

// Управление динамической памятью (куча для стандартной библиотеки malloc)
extern char _end; // Символ конца секции BSS из линкер-скрипта
char *heap_end = 0;

caddr_t _sbrk(int incr) {
  register char *stack_ptr asm("sp");
  char *prev_heap_end;

  if (heap_end == 0) {
    heap_end = &_end;
  }

  prev_heap_end = heap_end;

  // Защита от переполнения кучи в область стека
  if (heap_end + incr > stack_ptr) {
    errno = ENOMEM;
    return (caddr_t)-1;
  }

  heap_end += incr;
  return (caddr_t)prev_heap_end;
}

int _kill(int pid, int sig) {
  (void)pid;
  (void)sig;
  errno = EINVAL;
  return -1;
}

int _getpid(void) {
  return 1;
}
