# sync_at_lib.ps1
# Скрипт полной синхронизации оригинальных файлов из Firmware Library и FreeRTOS-Kernel

$SDK_ROOT = "..\AT32F403A_407_Firmware_Library-master"
$RTOS_ROOT = "..\FreeRTOS-Kernel-main"
$TARGET_DIR = "..\at_lib"

# 1. Очистка старой папки at_lib
if (Test-Path $TARGET_DIR) {
    Write-Host "--- Очистка старой папки $TARGET_DIR ---" -ForegroundColor Yellow
    Remove-Item -Path $TARGET_DIR -Recurse -Force
}

# 2. Создание структуры папок в at_lib
Write-Host "--- Создание структуры папок в at_lib ---" -ForegroundColor Green
New-Item -ItemType Directory -Force -Path "$TARGET_DIR\bsp"
New-Item -ItemType Directory -Force -Path "$TARGET_DIR\cmsis"
New-Item -ItemType Directory -Force -Path "$TARGET_DIR\drivers\inc"
New-Item -ItemType Directory -Force -Path "$TARGET_DIR\drivers\src"
New-Item -ItemType Directory -Force -Path "$TARGET_DIR\usb_drivers\inc"
New-Item -ItemType Directory -Force -Path "$TARGET_DIR\usb_drivers\src"
New-Item -ItemType Directory -Force -Path "$TARGET_DIR\rtos\src"
New-Item -ItemType Directory -Force -Path "$TARGET_DIR\rtos\portable\GCC\ARM_CM4F"

# 3. Копирование файлов CMSIS & Startup
Write-Host "--- Копирование CMSIS & Startup ---" -ForegroundColor Cyan
Copy-Item "$SDK_ROOT\libraries\cmsis\cm4\device_support\at32f403a_407.h" "$TARGET_DIR\cmsis\"
Copy-Item "$SDK_ROOT\libraries\cmsis\cm4\device_support\system_at32f403a_407.c" "$TARGET_DIR\cmsis\"
Copy-Item "$SDK_ROOT\libraries\cmsis\cm4\device_support\system_at32f403a_407.h" "$TARGET_DIR\cmsis\"
Copy-Item "$SDK_ROOT\libraries\cmsis\cm4\device_support\startup\gcc\startup_at32f403a_407.s" "$TARGET_DIR\bsp\"
Copy-Item "$SDK_ROOT\libraries\cmsis\cm4\device_support\startup\gcc\linker\AT32F403AxC_FLASH.ld" "$TARGET_DIR\bsp\"

# Копируем оригинальные файлы core_cm4 и все сопутствующие core_cmInstr/Func из папки шаблонов IAR
Copy-Item "$SDK_ROOT\project\at_start_f403a\templates\iar_v6.10\core_support\*.h" "$TARGET_DIR\cmsis\"

# Дублирование компиляторных хидеров в папку bsp и root проекта для страховки GCC
Copy-Item "$SDK_ROOT\libraries\cmsis\cm4\core_support\cmsis_compiler.h" "$TARGET_DIR\cmsis\" -ErrorAction SilentlyContinue
Copy-Item "$SDK_ROOT\libraries\cmsis\cm4\core_support\cmsis_gcc.h" "$TARGET_DIR\cmsis\" -ErrorAction SilentlyContinue
Copy-Item "$SDK_ROOT\libraries\cmsis\cm4\core_support\cmsis_compiler.h" "$TARGET_DIR\bsp\" -ErrorAction SilentlyContinue
Copy-Item "$SDK_ROOT\libraries\cmsis\cm4\core_support\cmsis_gcc.h" "$TARGET_DIR\bsp\" -ErrorAction SilentlyContinue
Copy-Item "$SDK_ROOT\libraries\cmsis\cm4\core_support\cmsis_compiler.h" ".\src\" -ErrorAction SilentlyContinue
Copy-Item "$SDK_ROOT\libraries\cmsis\cm4\core_support\cmsis_gcc.h" ".\src\" -ErrorAction SilentlyContinue

# 4. Копирование файлов BSP (Тактирование) и шаблона конфигурации периферии
Write-Host "--- Копирование BSP & ---" -ForegroundColor Cyan
Copy-Item "$SDK_ROOT\project\at_start_f403a\templates\src\at32f403a_407_clock.c" "$TARGET_DIR\bsp\"
Copy-Item "$SDK_ROOT\project\at_start_f403a\templates\inc\at32f403a_407_clock.h" "$TARGET_DIR\bsp\"
Copy-Item "$SDK_ROOT\project\at_start_f403a\templates\inc\at32f403a_407_conf.h" "$TARGET_DIR\cmsis\"
Copy-Item "$SDK_ROOT\project\at_start_f403a\templates\inc\at32f403a_407_conf.h" ".\src\"

# Создаем чистый syscalls.c (заглушка для Newlib GCC кучи)
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

# 5. Копирование всех драйверов периферии из libraries\drivers
Write-Host "--- Копирование Драйверов Периферии ---" -ForegroundColor Cyan
Copy-Item "$SDK_ROOT\libraries\drivers\inc\*.h" "$TARGET_DIR\drivers\inc\"
Copy-Item "$SDK_ROOT\libraries\drivers\src\*.c" "$TARGET_DIR\drivers\src\"

# 6. Копирование оригинального USB Middleware из middlewares\usbd_drivers
Write-Host "--- Копирование Оригинального USB ядра ---" -ForegroundColor Cyan
Copy-Item "$SDK_ROOT\middlewares\usbd_drivers\inc\*.h" "$TARGET_DIR\usb_drivers\inc\"
Copy-Item "$SDK_ROOT\middlewares\usbd_drivers\src\*.c" "$TARGET_DIR\usb_drivers\src\"

# 7. Копирование ОРИГИНАЛЬНОГО класса audio_hid из middlewares\usbd_class\audio_hid
Write-Host "--- Копирование Оригинального класса Audio-HID из SDK ---" -ForegroundColor Cyan
Copy-Item "$SDK_ROOT\middlewares\usbd_class\audio_hid\*.h" "$TARGET_DIR\usb_drivers\inc\"
Copy-Item "$SDK_ROOT\middlewares\usbd_class\audio_hid\*.c" "$TARGET_DIR\usb_drivers\src\"

# 8. Копирование Исходников FreeRTOS Kernel
Write-Host "--- Копирование Исходников FreeRTOS Kernel ---" -ForegroundColor Cyan
Copy-Item -Path "$RTOS_ROOT\include" -Destination "$TARGET_DIR\rtos\inc" -Recurse -Force
Copy-Item -Path "$RTOS_ROOT\*.c" -Destination "$TARGET_DIR\rtos\src\" -Force
Copy-Item -Path "$RTOS_ROOT\portable\GCC\ARM_CM4F\*" -Destination "$TARGET_DIR\rtos\portable\GCC\ARM_CM4F\" -Force
Copy-Item -Path "$RTOS_ROOT\portable\MemMang\heap_4.c" -Destination "$TARGET_DIR\rtos\src\" -Force

# 9. Копируем оригинальный мост кодека из папки примера composite_audio_hid
Write-Host "--- Копирование Заводского audio_codec.h/c ---" -ForegroundColor Cyan
Copy-Item "$SDK_ROOT\project\at_start_f403a\examples\usb_device\composite_audio_hid\inc\audio_codec.h" "$TARGET_DIR\usb_drivers\inc\"
Copy-Item "$SDK_ROOT\project\at_start_f403a\examples\usb_device\composite_audio_hid\src\audio_codec.c" "$TARGET_DIR\usb_drivers\src\"

# 10. Копируем оригинальный заводской usb_conf.h из папки примера composite_audio_hid
Copy-Item "$SDK_ROOT\project\at_start_f403a\examples\usb_device\composite_audio_hid\inc\usb_conf.h" "$TARGET_DIR\usb_drivers\inc\"
Copy-Item "$SDK_ROOT\project\at_start_f403a\examples\usb_device\composite_audio_hid\inc\usb_conf.h" ".\src\"

# 11. Копируем заводской i2c_application.h/c
Copy-Item "$SDK_ROOT\middlewares\i2c_application_library\*.h" "$TARGET_DIR\usb_drivers\inc\"
Copy-Item "$SDK_ROOT\middlewares\i2c_application_library\*.c" "$TARGET_DIR\usb_drivers\src\"

# 12. АВТОМАТИЧЕСКАЯ ЗАГЛУШКА ДЛЯ ОТСУТСТВУЮЩЕЙ ПЛАТЫ BOARD.H (Исключает ошибку инклуда)
"" | Out-File -FilePath "$TARGET_DIR\bsp\at32f403a_407_board.h" -Encoding ascii
"" | Out-File -FilePath ".\src\at32f403a_407_board.h" -Encoding ascii

Write-Host "====== СИНХРОНИЗАЦИЯ AT_LIB + FREERTOS ЗАВЕРШЕНА НАЧИСТО ======" -ForegroundColor Green
