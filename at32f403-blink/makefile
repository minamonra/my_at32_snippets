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
-I$(LIB_PATH)/bsp \
-I$(LIB_PATH)/cmsis \
-I$(LIB_PATH)/drivers/inc

# Исходники приложения
LOCAL_SOURCES = \
src/main.c

# Исходники BSP 403A
BSP_SOURCES = \
$(LIB_PATH)/bsp/at32f403a_407_clock.c

# Исходники SPL 403A
LIB_SOURCES = \
$(LIB_PATH)/cmsis/system_at32f403a_407.c \
$(LIB_PATH)/drivers/src/at32f403a_407_gpio.c \
$(LIB_PATH)/drivers/src/at32f403a_407_crm.c \
$(LIB_PATH)/drivers/src/at32f403a_407_pwc.c \
$(LIB_PATH)/drivers/src/at32f403a_407_misc.c \
$(LIB_PATH)/drivers/src/at32f403a_407_rtc.c \
$(LIB_PATH)/drivers/src/at32f403a_407_bpr.c

# Стартап 403A
ASM_SOURCES = $(LIB_PATH)/bsp/startup_at32f403a_407.s

# Линкер под 256КБ Flash / 96КБ SRAM
LDSCRIPT = $(LIB_PATH)/bsp/AT32F403AxC_FLASH.ld

# Флаги линкера
LDFLAGS = $(MCU) -specs=nano.specs -specs=nosys.specs -T$(LDSCRIPT) -Wl,-Map=build/$(TARGET).map,--gc-sections

# Формирование списков объектов
OBJECTS = $(addprefix build/, $(notdir $(LOCAL_SOURCES:.c=.o)))
OBJECTS += $(addprefix build/, $(notdir $(BSP_SOURCES:.c=.o)))
OBJECTS += $(addprefix build/, $(notdir $(LIB_SOURCES:.c=.o)))
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

# Компиляция system_at32f403a_407.c (Исправлено под новую серию)
build/system_at32f403a_407.o: $(LIB_PATH)/cmsis/system_at32f403a_407.c | build
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@

# Компиляция драйверов
build/%.o: $(LIB_PATH)/drivers/src/%.c | build
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@

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

# Прошивка (Исправлено под таргет at32f403axx.cfg)
flash: build/$(TARGET).elf
	$(OPENOCD) -s $(OPENOCD_SCRIPTS) -f interface/jlink.cfg -c "transport select swd" -f target/at32f403axx.cfg -c "program build/$(TARGET).elf verify" -c "reset run" -c "exit"
