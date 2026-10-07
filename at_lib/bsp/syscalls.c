void _exit(int status) { while(1); }
int _close(int file) { return -1; }
int _fstat(int file, void *st) { return 0; }
int _isatty(int file) { return 1; }
int _lseek(int file, int ptr, int dir) { return 0; }
int _read(int file, char *ptr, int len) { return 0; }
typedef char * caddr_t;
caddr_t _sbrk(int incr) { extern char end; static char *heap_end; char *prev_heap_end; if (heap_end == 0) { heap_end = &end; } prev_heap_end = heap_end; heap_end += incr; return (caddr_t) prev_heap_end; }
int _write(int file, char *ptr, int len) { return len; }
