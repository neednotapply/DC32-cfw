APP	= uGB
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
	#aspirational
	CC_FLAGS	= -O3 -mmcu=atmega128 -I/usr/lib/avr/include -DEMBEDDED -D_SIM -ffunction-sections -DAVR_ASM
	LD_FLAGS	= -O3 -mmcu=atmega128 -Wl,--gc-sections
	CC		= avr-gcc
	LD		= avr-gcc
	EXTRA		= avr-size -Ax $(APP) && avr-objcopy -j .text -j .data -O ihex $(APP) $(APP).hex
	EXTRA_OBJS	= main_avr.o gb.avr.o
endif

	
ZWT_ADDR	= 0x20081ff8
CPU			= -mthumb -fsingle-precision-constant -ffast-math -mcpu=cortex-m33 -mfloat-abi=soft
CC_FLAGS	+= -Ofast -Wall -Wextra -Werror $(CPU) -I. -ffixed-r11 -fstack-reuse=all -Wno-array-bounds -Wall -Wno-clobbered
CC_FLAGS	+= -ffunction-sections -fdata-sections -fomit-frame-pointer -Wno-unused-parameter -Wno-unused-function
CC_FLAGS	+= -Wno-unused-but-set-variable -Wno-unused-variable -DZWT_ADDR=$(ZWT_ADDR) -DTICKS_PER_SECOND=125000000U
CC_FLAGS	+= -DNO_SUPPORT_DEBUG_PRINTF -DEMBEDDED
CC_FLAGS	+= -DQUIET_MAPPERS -DFATFS_USE_LFN_SUPPORT=1 -DUSE_WIN_1252_CODE_PAGE
CC_FLAGS	+= -DQSPI_RAM_SIZE_MAX=65536 -DDISP_LCD_DEFCON -DUNSCALED_IMG_ROTATED -DUPSCALER_ROTATES -DUI_ROTATED
CC_FLAGS	+= -DGB_HAVE_EXTRA_IO_REGS
LD_FLAGS	+= -Wl,--gc-sections -Wl,-T linker_rp2350_defcon.lkr -lm $(CPU)
CC			= arm-none-eabi-gcc
LD			= arm-none-eabi-gcc
OBJCOPY		= arm-none-eabi-objcopy
EXTRA_OBJS	= main_rp2350_defcon.o crt_rp2350.o timebase.o printf.o dispDefcon.o utf.o qspi2350.o ui.o settings.o fonts.o sleepDefcon.o sd.o fatfs.o sdHwRP2350.o pioI2C.o pioIrdaSIR.o pioWS2812.o

CC_FLAGS	+= -isysteminc_RP2350

EXTRA_OBJS	+= gbC.o
#EXTRA_OBJS	+= gbAsmM3.o

#ASM video driver is faster on M0, the compiler wins on m3+ since i am too lazy to write an M3 video driver
EXTRA_OBJS	+= videoC.o
#EXTRA_OBJS	+= videoAsmM3.o

EXTRA_OBJS	+= mappersC.o
#EXTRA_OBJS	+= mappersAsmM3.o

TARGET		= $(APP).bin

CMD			= sudo CortexProg info targetsel 0 write $(APP).bin 0x10000000
	

LDFLAGS = $(LD_FLAGS) -Wall -Wextra $(LIBS)
CCFLAGS = $(CC_FLAGS) -Wall -Wextra -DSDL=$(SDL) -DHQX=$(HQX) -Wno-unused-label -Wno-missing-field-initializers

OBJS	= $(EXTRA_OBJS) gb.o mbc.o

ifeq ($(HQX), 1)
	OBJS += hq3x.o
endif

$(APP): $(OBJS)
	$(LD) -o $(APP) $(OBJS) $(LDFLAGS)
	$(EXTRA)


app: $(TARGET)

%.bin: %
	$(OBJCOPY) -O binary $< $@ -j.text -j.rodata -j.data -j.vectors

AVR:	$(APP)
	sudo avrdude -V -p ATmega1284p -c avrisp2 -P usb -U flash:w:$(APP).hex:i

%.o : %.c Makefile
	$(CC) $(CCFLAGS) -c $< -o $@

%.o : %.S Makefile
	$(CC) $(CCFLAGS) -c $< -o $@

clean:
	rm -f $(APP) *.o


flash: $(APP).bin
	$(CMD)

trace: $(APP).bin
	$(CMD) trace $(ZWT_ADDR)
