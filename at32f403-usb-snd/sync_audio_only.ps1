# sync_audio_only.ps1
# Скрипт полной очистки и копирования оригинального класса USB Audio DAC

$SDK_ROOT = "..\AT32F403A_407_Firmware_Library-master"
$RTOS_ROOT = "..\FreeRTOS-Kernel-main"
$TARGET_DIR = "..\at_lib"

# 1. Очистка старой папки at_lib
if (Test-Path $TARGET_DIR) {
    Write-Host "--- Очистка старой папки $TARGET_DIR ---" -ForegroundColor Yellow
    Remove-Item -Path $TARGET_DIR -Recurse -Force
}

# 2. Создание чистой структуры папок в at_lib
Write-Host "--- Создание структуры папок в at_lib ---" -ForegroundColor Green
New-Item -ItemType Directory -Force -Path "$TARGET_DIR\bsp"
New-Item -ItemType Directory -Force -Path "$TARGET_DIR\cmsis"
New-Item -ItemType Directory -Force -Path "$TARGET_DIR\drivers\inc"
New-Item -ItemType Directory -Force -Path "$TARGET_DIR\drivers\src"
New-Item -ItemType Directory -Force -Path "$TARGET_DIR\usb_drivers\inc"
New-Item -ItemType Directory -Force -Path "$TARGET_DIR\usb_drivers\src"
New-Item -ItemType Directory -Force -Path "$TARGET_DIR\rtos\src"
New-Item -ItemType Directory -Force -Path "$TARGET_DIR\rtos\portable\GCC\ARM_CM4F"

# 3. Копирование неизменяемых файлов CMSIS, Linker & Startup
Write-Host "--- Копирование CMSIS & Startup ---" -ForegroundColor Cyan
Copy-Item "$SDK_ROOT\libraries\cmsis\cm4\device_support\at32f403a_407.h" "$TARGET_DIR\cmsis\"
Copy-Item "$SDK_ROOT\libraries\cmsis\cm4\device_support\system_at32f403a_407.c" "$TARGET_DIR\cmsis\"
Copy-Item "$SDK_ROOT\libraries\cmsis\cm4\device_support\system_at32f403a_407.h" "$TARGET_DIR\cmsis\"
Copy-Item "$SDK_ROOT\libraries\cmsis\cm4\device_support\startup\gcc\startup_at32f403a_407.s" "$TARGET_DIR\bsp\"
Copy-Item "$SDK_ROOT\libraries\cmsis\cm4\device_support\startup\gcc\linker\AT32F403AxC_FLASH.ld" "$TARGET_DIR\bsp\"

# Копируем оригинальные файлы заголовков ядра ARM CMSIS из шаблонов IAR
Copy-Item "$SDK_ROOT\project\at_start_f403a\templates\iar_v6.10\core_support\*.h" "$TARGET_DIR\cmsis\"
Copy-Item "$SDK_ROOT\libraries\cmsis\cm4\core_support\cmsis_compiler.h" "$TARGET_DIR\cmsis\" -ErrorAction SilentlyContinue
Copy-Item "$SDK_ROOT\libraries\cmsis\cm4\core_support\cmsis_gcc.h" "$TARGET_DIR\cmsis\" -ErrorAction SilentlyContinue

# 4. Копирование тактирования BSP и заглушки syscalls в at_lib
Write-Host "--- Копирование BSP в at_lib ---" -ForegroundColor Cyan
Copy-Item "$SDK_ROOT\project\at_start_f403a\templates\src\at32f403a_407_clock.c" "$TARGET_DIR\bsp\"
Copy-Item "$SDK_ROOT\project\at_start_f403a\templates\inc\at32f403a_407_clock.h" "$TARGET_DIR\bsp\"

$syscalls_code = @"
void _exit(int status) { while(1); }
int _close(int file) { return -1; }
int _fstat(int file, void *st) { return 0; }
int _isatty(int file) { return 1; }
int _lseek(int file, int ptr, int dir) { return 0; }
int _read(int file, char *ptr, int len) { return 0; }
typedef char * caddr_t;
caddr_t _sbrk(int incr) { extern char end; static char *heap_end; char *prev_heap_end; if (heap_end == 0) { heap_end = &end; } prev_heap_end = heap_end; heap_end += incr; return (caddr_t) prev_heap_end; }
int _write(int file, char *ptr, int len) { return len; }
"@
$syscalls_code | Out-File -FilePath "$TARGET_DIR\bsp\syscalls.c" -Encoding ascii

# 5. Копирование стандартных драйверов периферии библиотеки SPL в at_lib
Write-Host "--- Копирование Драйверов Периферии в at_lib ---" -ForegroundColor Cyan
Copy-Item "$SDK_ROOT\libraries\drivers\inc\*.h" "$TARGET_DIR\drivers\inc\"
Copy-Item "$SDK_ROOT\libraries\drivers\src\*.c" "$TARGET_DIR\drivers\src\"

# 6. Копирование неизменяемого ядра USB Middleware в at_lib
Write-Host "--- Копирование Ядра USB Middleware в at_lib ---" -ForegroundColor Cyan
Copy-Item "$SDK_ROOT\middlewares\usbd_drivers\inc\*.h" "$TARGET_DIR\usb_drivers\inc\"
Copy-Item "$SDK_ROOT\middlewares\usbd_drivers\src\*.c" "$TARGET_DIR\usb_drivers\src\"

# 7. Копирование неизменяемого USB-класса Audio и дескрипторов в at_lib
Write-Host "--- Копирование Библиотечного класса USB Audio в at_lib ---" -ForegroundColor Cyan
Copy-Item "$SDK_ROOT\middlewares\usbd_class\audio\audio_class.c" "$TARGET_DIR\usb_drivers\src\"
Copy-Item "$SDK_ROOT\middlewares\usbd_class\audio\audio_class.h" "$TARGET_DIR\usb_drivers\inc\"
Copy-Item "$SDK_ROOT\middlewares\usbd_class\audio\audio_desc.c" "$TARGET_DIR\usb_drivers\src\"
Copy-Item "$SDK_ROOT\middlewares\usbd_class\audio\audio_desc.h" "$TARGET_DIR\usb_drivers\inc\"

# 8. Копирование Исходников FreeRTOS Kernel в at_lib
Write-Host "--- Копирование Исходников FreeRTOS Kernel в at_lib ---" -ForegroundColor Cyan
Copy-Item -Path "$RTOS_ROOT\include" -Destination "$TARGET_DIR\rtos\inc" -Recurse -Force
Copy-Item -Path "$RTOS_ROOT\*.c" -Destination "$TARGET_DIR\rtos\src\" -Force
Copy-Item -Path "$RTOS_ROOT\portable\GCC\ARM_CM4F\*" -Destination "$TARGET_DIR\rtos\portable\GCC\ARM_CM4F\" -Force
Copy-Item -Path "$RTOS_ROOT\portable\MemMang\heap_4.c" -Destination "$TARGET_DIR\rtos\src\" -Force

# ==============================================================================
# 9. КОПИРОВАНИЕ ИЗМЕНЯЕМЫХ ФАЙЛОВ ПРИМЕРА СТРОГО ЛОКАЛЬНО В НАШ КАТАЛОГ src/ !
# ==============================================================================
Write-Host "--- ИМПОРТ ИЗМЕНЯЕМЫХ ФАЙЛОВ ПРИМЕРА В ЛОКАЛЬНУЮ ПАПКУ src/ ---" -ForegroundColor Yellow

$AUDIO_EXAMPLE_DIR = "$SDK_ROOT\project\at_start_f403a\examples\usb_device\audio"

# Копируем изменяемые файлы примера (кодек, аппаратные прерывания примера, конфиги)
Copy-Item "$AUDIO_EXAMPLE_DIR\inc\usb_conf.h" ".\src\" -Force
Copy-Item "$AUDIO_EXAMPLE_DIR\inc\audio_codec.h" ".\src\" -Force
Copy-Item "$AUDIO_EXAMPLE_DIR\inc\at32f403a_407_int.h" ".\src\" -Force
Copy-Item "$AUDIO_EXAMPLE_DIR\inc\at32f403a_407_conf.h" ".\src\" -Force
Copy-Item "$AUDIO_EXAMPLE_DIR\src\audio_codec.c" ".\src\" -Force
Copy-Item "$AUDIO_EXAMPLE_DIR\src\at32f403a_407_int.c" ".\src\" -Force

# ПОДТВЕРЖДЕНО КАРТИНКОЙ: Копируем ТОЧНЫЙ родной audio_conf.h из папки класса middlewares\usbd_class\audio ЛОКАЛЬНО В src/
Copy-Item "$SDK_ROOT\middlewares\usbd_class\audio\audio_conf.h" ".\src\" -Force

# Создаем чистый заголовок-заглушку board.h локально в src/
"" | Out-File -FilePath ".\src\at32f403a_407_board.h" -Encoding ascii

Write-Host "====== СИНХРОНИЗАЦИЯ ЧИСТОГО USB AUDIO ЗАВЕРШЕНА НАЧИСТО ======" -ForegroundColor Green
