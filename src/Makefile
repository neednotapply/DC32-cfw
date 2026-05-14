APP	= DC32-cfw
CC	= gcc
LD	= gcc

BUILD ?= debug
SDL ?= 0
HQX ?= 0

ifeq ($(SDL), 1)
	LIBS		= -lSDL2
endif
TARGET		= $(APP)

ifeq ($(BUILD), avr)
	CC_FLAGS	= -O3 -mmcu=atmega128 -I/usr/lib/avr/include -DEMBEDDED -D_SIM -ffunction-sections -DAVR_ASM
	LD_FLAGS	= -O3 -mmcu=atmega128 -Wl,--gc-sections
	CC		= avr-gcc
	LD		= avr-gcc
	EXTRA		= avr-size -Ax $(APP) && avr-objcopy -j .text -j .data -O ihex $(APP) $(APP).hex
	EXTRA_OBJS	= main_avr.o gb.avr.o
endif

ZWT_ADDR	= 0x20081ff8
CPU		= -mthumb -fsingle-precision-constant -ffast-math -mcpu=cortex-m33 -mfloat-abi=soft
CC_FLAGS	+= -Ofast -Wall -Wextra -Werror $(CPU) -I. -ffixed-r11 -fstack-reuse=all -Wno-array-bounds -Wall -Wno-clobbered
CC_FLAGS	+= -ffunction-sections -fdata-sections -fomit-frame-pointer -Wno-unused-parameter -Wno-unused-function
CC_FLAGS	+= -Wno-unused-but-set-variable -Wno-unused-variable -DZWT_ADDR=$(ZWT_ADDR) -DTICKS_PER_SECOND=125000000U
CC_FLAGS	+= -DNO_SUPPORT_DEBUG_PRINTF -DEMBEDDED
CC_FLAGS	+= -DQUIET_MAPPERS -DFATFS_USE_LFN_SUPPORT=1 -DUSE_WIN_1252_CODE_PAGE
CC_FLAGS	+= -DQSPI_RAM_SIZE_MAX=65536 -DDISP_LCD_DEFCON -DUNSCALED_IMG_ROTATED -DUPSCALER_ROTATES -DUI_ROTATED
CC_FLAGS	+= -DGB_HAVE_EXTRA_IO_REGS
CC_FLAGS	+= -Wno-maybe-uninitialized -fno-builtin-strcmp
LD_FLAGS	+= -Wl,--gc-sections -Wl,-T linker_rp2350_defcon.lkr -lm $(CPU)
CC		= arm-none-eabi-gcc
LD		= arm-none-eabi-gcc
OBJCOPY		= arm-none-eabi-objcopy
PYTHON		?= python3
UF2CONV		= $(PYTHON) ../tools/bin_to_uf2.py
EXTRA_OBJS	= main_rp2350_defcon.o crt_rp2350.o timebase.o printf.o dispDefcon.o utf.o qspi2350.o ui.o settings.o badgeLeds.o bootGuard.o fonts.o sleepDefcon.o sd.o fatfs.o sdHwRP2350.o pioI2C.o pioIrdaSIR.o pioWS2812.o irRemote.o audioPwm.o rtttlPlayer.o usbHid.o badUsb.o toolWorkspace.o stringCompat.o

CC_FLAGS	+= -isysteminc_RP2350

EXTRA_OBJS	+= gbC.o
#EXTRA_OBJS	+= gbAsmM3.o

EXTRA_OBJS	+= videoC.o
#EXTRA_OBJS	+= videoAsmM3.o

EXTRA_OBJS	+= mappersC.o
#EXTRA_OBJS	+= mappersAsmM3.o

TARGET		= $(APP).bin
UF2_TARGET	= $(APP).uf2

CMD		= sudo CortexProg info targetsel 0 write $(APP).bin 0x10000000

LDFLAGS = $(LD_FLAGS) -Wall -Wextra $(LIBS)
CCFLAGS = $(CC_FLAGS) -Wall -Wextra -DSDL=$(SDL) -DHQX=$(HQX) -Wno-unused-label -Wno-missing-field-initializers

OBJS	= $(EXTRA_OBJS) gb.o mbc.o

ifeq ($(HQX), 1)
	OBJS += hq3x.o
endif

ui.o badUsb.o settings.o badgeLeds.o: CCFLAGS += -Os

$(APP): $(OBJS)
	$(LD) -o $(APP) $(OBJS) $(LDFLAGS)
	$(EXTRA)


.PHONY: app AVR clean flash trace uf2

app: $(TARGET) $(UF2_TARGET)

%.bin: %
	$(OBJCOPY) -O binary $< $@ -j.text -j.rodata -j.data -j.vectors

%.uf2: %.bin ../tools/bin_to_uf2.py
	$(UF2CONV) $< $@ --base 0x10000000 --family rp2350-arm-s --abs-block

uf2: $(UF2_TARGET)

AVR:	$(APP)
	sudo avrdude -V -p ATmega1284p -c avrisp2 -P usb -U flash:w:$(APP).hex:i

%.o : %.c Makefile
	$(CC) $(CCFLAGS) -c $< -o $@

%.o : %.S Makefile
	$(CC) $(CCFLAGS) -c $< -o $@

clean:
	rm -f $(APP) $(APP).bin $(UF2_TARGET) *.o

flash: $(APP).bin
	$(CMD)

trace: $(APP).bin
	$(CMD) trace $(ZWT_ADDR)
