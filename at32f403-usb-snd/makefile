TARGET = main

CC = arm-none-eabi-gcc
CP = arm-none-eabi-objcopy
SZ = arm-none-eabi-size

OPENOCD          = ../at32openocd/bin/openocd.exe
OPENOCD_SCRIPTS  = ../at32openocd/scripts
LIB_PATH         = ../at_lib

INCLUDES = \
-Isrc \
-I$(LIB_PATH)/cmsis \
-I$(LIB_PATH)/bsp \
-I$(LIB_PATH)/drivers/inc \
-I$(LIB_PATH)/usb_drivers/inc \
-I$(LIB_PATH)/rtos/inc \
-I$(LIB_PATH)/rtos/portable/GCC/ARM_CM4F \
-Iu8g2x

MCU = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
CFLAGS = $(MCU) -Wall -Og -g -gdwarf-2 -DAT32F403ACGU7 $(INCLUDES)

U8G2_SOURCES = \
u8g2x/lcd_draw_u8g2.c \
u8g2x/u8g2_font.c \
u8g2x/u8fonts/terminus_24b_cyr.c \
u8g2x/u8fonts/terminus_24n_cyr.c

# Файлы примера теперь СТРОГО компилируются локально из папки src/
LOCAL_SOURCES = \
src/main.c \
src/common.c \
src/st7789.c \
src/pcm5102.c \
src/ring_buffer.c \
src/ui_task.c \
src/audio_codec.c \
src/at32f403a_407_int.c

BSP_SOURCES = \
$(LIB_PATH)/bsp/at32f403a_407_clock.c \
$(LIB_PATH)/bsp/syscalls.c

# В LIB_SOURCES осталось только чистое, неизменяемое ядро библиотек ArteryTek
# Найдите LIB_SOURCES в makefile и добавьте туда строки audio_class.c и audio_desc.c в самый конец:
LIB_SOURCES = \
$(LIB_PATH)/cmsis/system_at32f403a_407.c \
$(LIB_PATH)/drivers/src/at32f403a_407_gpio.c \
$(LIB_PATH)/drivers/src/at32f403a_407_crm.c \
$(LIB_PATH)/drivers/src/at32f403a_407_spi.c \
$(LIB_PATH)/drivers/src/at32f403a_407_dma.c \
$(LIB_PATH)/drivers/src/at32f403a_407_pwc.c \
$(LIB_PATH)/drivers/src/at32f403a_407_misc.c \
$(LIB_PATH)/drivers/src/at32f403a_407_rtc.c \
$(LIB_PATH)/drivers/src/at32f403a_407_bpr.c \
$(LIB_PATH)/drivers/src/at32f403a_407_usb.c \
$(LIB_PATH)/drivers/src/at32f403a_407_tmr.c \
$(LIB_PATH)/usb_drivers/src/usbd_core.c \
$(LIB_PATH)/usb_drivers/src/usbd_int.c \
$(LIB_PATH)/usb_drivers/src/usbd_sdr.c \
$(LIB_PATH)/usb_drivers/src/audio_class.c \
$(LIB_PATH)/usb_drivers/src/audio_desc.c

RTOS_SOURCES = \
$(LIB_PATH)/rtos/src/tasks.c \
$(LIB_PATH)/rtos/src/list.c \
$(LIB_PATH)/rtos/src/queue.c \
$(LIB_PATH)/rtos/src/timers.c \
$(LIB_PATH)/rtos/portable/GCC/ARM_CM4F/port.c \
$(LIB_PATH)/rtos/src/heap_4.c

ASM_SOURCES = $(LIB_PATH)/bsp/startup_at32f403a_407.s
LDSCRIPT = $(LIB_PATH)/bsp/AT32F403AxC_FLASH.ld
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) -Wl,-Map=build/$(TARGET).map,--gc-sections,--no-warn-rwx-segments

OBJECTS = $(addprefix build/, $(notdir $(LOCAL_SOURCES:.c=.o)))
OBJECTS += $(addprefix build/, $(notdir $(BSP_SOURCES:.c=.o)))
OBJECTS += $(addprefix build/, $(notdir $(LIB_SOURCES:.c=.o)))
OBJECTS += $(addprefix build/, $(notdir $(RTOS_SOURCES:.c=.o)))
OBJECTS += $(addprefix build/, $(notdir $(U8G2_SOURCES:.c=.o)))
OBJECTS += $(addprefix build/, $(notdir $(ASM_SOURCES:.s=.o)))

.PHONY: all clean flash

all: build/$(TARGET).elf build/$(TARGET).bin

build/%.o: src/%.c | build
	$(CC) -c $(CFLAGS) $< -o $@

build/%.o: $(LIB_PATH)/bsp/%.c | build
	$(CC) -c $(CFLAGS) $< -o $@

build/%.o: $(LIB_PATH)/bsp/%.s | build
	$(CC) -c $(CFLAGS) $< -o $@

build/system_at32f403a_407.o: $(LIB_PATH)/cmsis/system_at32f403a_407.c | build
	$(CC) -c $(CFLAGS) $< -o $@

build/at32f403a_407_%.o: $(LIB_PATH)/drivers/src/at32f403a_407_%.c | build
	$(CC) -c $(CFLAGS) $< -o $@

build/usbd_%.o: $(LIB_PATH)/usb_drivers/src/usbd_%.c | build
	$(CC) -c $(CFLAGS) $< -o $@

build/audio_class.o: $(LIB_PATH)/usb_drivers/src/audio_class.c | build
	$(CC) -c $(CFLAGS) $< -o $@

build/%.o: $(LIB_PATH)/rtos/src/%.c | build
	$(CC) -c $(CFLAGS) $< -o $@

build/%.o: $(LIB_PATH)/rtos/portable/GCC/ARM_CM4F/%.c | build
	$(CC) -c $(CFLAGS) $< -o $@

build/%.o: u8g2x/%.c | build
	$(CC) -c $(CFLAGS) $< -o $@

build/%.o: u8g2x/u8fonts/%.c | build
	$(CC) -c -include u8g2.h $(CFLAGS) $< -o $@

build/audio_desc.o: $(LIB_PATH)/usb_drivers/src/audio_desc.c | build
	$(CC) -c $(CFLAGS) $< -o $@

build/$(TARGET).elf: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

build/$(TARGET).bin: build/$(TARGET).elf
	$(CP) -O binary $< $@

build:
	mkdir -p build

clean:
	rm -rf build

flash: build/$(TARGET).elf
	$(OPENOCD) -s $(OPENOCD_SCRIPTS) -f interface/jlink.cfg -c "transport select swd" -f target/at32f403axx.cfg -c "program build/$(TARGET).elf verify" -c "reset run" -c "exit"
