# Имя бинарника
TARGET = main

# Инструменты сборки
CC = arm-none-eabi-gcc
CP = arm-none-eabi-objcopy
SZ = arm-none-eabi-size

# Пути к OpenOCD верхнего уровня
OPENOCD          = ../at32openocd/bin/openocd.exe
OPENOCD_SCRIPTS  = ../at32openocd/scripts

# Путь к библиотекам верхнего уровня
LIB_PATH         = ../at_lib

# Настройки архитектуры Cortex-M4 Hard FPU
MCU = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16

# Флаги компиляции под 403A
CFLAGS = $(MCU) -Wall -Og -g -gdwarf-2 -DAT32F403ACGU7

# Пути заголовочных файлов
INCLUDES = \
-Isrc \
-Iu8g2x \
-I$(LIB_PATH)/bsp \
-I$(LIB_PATH)/cmsis \
-I$(LIB_PATH)/drivers/inc \
-I$(LIB_PATH)/rtos/inc \
-I$(LIB_PATH)/rtos/portable/GCC/ARM_CM4F

# Исходники графической библиотеки U8g2 (ДОБАВЛЕНО)
U8G2_SOURCES = \
u8g2x/lcd_draw_u8g2.c \
u8g2x/u8g2_font.c \
u8g2x/u8fonts/terminus_24b_cyr.c \
u8g2x/u8fonts/terminus_24n_cyr.c

# Исходники приложения
LOCAL_SOURCES = \
src/main.c \
src/common.c \
src/st7789.c \
src/si4703.c

# Исходники BSP 403A (Включая системные вызовы и дисплей)
BSP_SOURCES = \
$(LIB_PATH)/bsp/at32f403a_407_clock.c \
$(LIB_PATH)/bsp/syscalls.c

# Исходники SPL 403A (Добавлен драйвер SPI)
# Исходники SPL 403A (Добавили dma.c)
LIB_SOURCES = \
$(LIB_PATH)/cmsis/system_at32f403a_407.c \
$(LIB_PATH)/drivers/src/at32f403a_407_gpio.c \
$(LIB_PATH)/drivers/src/at32f403a_407_crm.c \
$(LIB_PATH)/drivers/src/at32f403a_407_spi.c \
$(LIB_PATH)/drivers/src/at32f403a_407_dma.c \
$(LIB_PATH)/drivers/src/at32f403a_407_i2c.c \
$(LIB_PATH)/drivers/src/at32f403a_407_pwc.c \
$(LIB_PATH)/drivers/src/at32f403a_407_misc.c \
$(LIB_PATH)/drivers/src/at32f403a_407_rtc.c \
$(LIB_PATH)/drivers/src/at32f403a_407_bpr.c

# Исходники FreeRTOS
RTOS_SOURCES = \
$(LIB_PATH)/rtos/src/tasks.c \
$(LIB_PATH)/rtos/src/list.c \
$(LIB_PATH)/rtos/src/queue.c \
$(LIB_PATH)/rtos/src/timers.c \
$(LIB_PATH)/rtos/portable/GCC/ARM_CM4F/port.c \
$(LIB_PATH)/rtos/portable/MemMang/heap_4.c

# Стартап 403A
ASM_SOURCES = $(LIB_PATH)/bsp/startup_at32f403a_407.s

# Линкер под 256КБ Flash / 96КБ SRAM
LDSCRIPT = $(LIB_PATH)/bsp/AT32F403AxC_FLASH.ld

# Флаги линкера (Добавлен флаг для скрытия варнинга RWX-секций)
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) -Wl,-Map=build/$(TARGET).map,--gc-sections,--no-warn-rwx-segments

# Формирование списков объектов (ДОБАВЛЕНЫ U8G2_SOURCES)
OBJECTS = $(addprefix build/, $(notdir $(LOCAL_SOURCES:.c=.o)))
OBJECTS += $(addprefix build/, $(notdir $(BSP_SOURCES:.c=.o)))
OBJECTS += $(addprefix build/, $(notdir $(LIB_SOURCES:.c=.o)))
OBJECTS += $(addprefix build/, $(notdir $(RTOS_SOURCES:.c=.o)))
OBJECTS += $(addprefix build/, $(notdir $(U8G2_SOURCES:.c=.o)))
OBJECTS += $(addprefix build/, $(notdir $(ASM_SOURCES:.s=.o)))

# Правила сборки
.PHONY: all clean flash

all: build/$(TARGET).elf build/$(TARGET).bin

# Компиляция src/
build/%.o: src/%.c | build
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@

# Компиляция bsp/
build/%.o: $(LIB_PATH)/bsp/%.c | build
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@

build/%.o: $(LIB_PATH)/bsp/%.s | build
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@

# Компиляция system_at32f403a_407.c
build/system_at32f403a_407.o: $(LIB_PATH)/cmsis/system_at32f403a_407.c | build
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@

# Компиляция драйверов SPL периферии Artery
build/at32f403a_407_%.o: $(LIB_PATH)/drivers/src/at32f403a_407_%.c | build
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@

# Компиляция FreeRTOS ядра и порта
build/%.o: $(LIB_PATH)/rtos/src/%.c | build
	$(CC) -c $(CFLAGS) $(INCLUDES) -Isrc $< -o $@

build/%.o: $(LIB_PATH)/rtos/portable/GCC/ARM_CM4F/%.c | build
	$(CC) -c $(CFLAGS) $(INCLUDES) -Isrc $< -o $@

build/%.o: $(LIB_PATH)/rtos/portable/MemMang/%.c | build
	$(CC) -c $(CFLAGS) $(INCLUDES) -Isrc $< -o $@

# Правило компиляции ядра u8g2x и шрифтов (ДОБАВЛЕНО)
build/%.o: u8g2x/%.c | build
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@

build/%.o: u8g2x/u8fonts/%.c | build
	$(CC) -c $(CFLAGS) -include u8g2.h $(INCLUDES) $< -o $@

# Линковка ELF
build/$(TARGET).elf: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

# Создание бинарника
build/$(TARGET).bin: build/$(TARGET).elf
	$(CP) -O binary $< $@

build:
	mkdir -p build

clean:
	rm -rf build

# Прошивка
flash: build/$(TARGET).elf
	$(OPENOCD) -s $(OPENOCD_SCRIPTS) -f interface/jlink.cfg -c "transport select swd" -f target/at32f403axx.cfg -c "program build/$(TARGET).elf verify" -c "reset run" -c "exit"
