#include <string.h>
#include "timebase.h"
#include "printf.h"
#include "rp2040.h"
#include "oled.h"

#define DISP_DATA_BYTES				(DISP_WIDTH * DISP_HEIGHT * DISP_BPP / 8)

#define SIDE_SET_HAS_ENABLE_BIT				0
#define SIDE_SET_NUM_BITS					1

#define INSTR(_opc, _delay, _sideSet, _misc)			(((_opc) << 13) | ((_sideSet) << (13 - SIDE_SET_NUM_BITS)) | ((_delay) << 8) | (_misc))
#define I_JMP(_delay, _sideSet, _condition, _dst)		INSTR(0b000, _delay, _sideSet, (((_condition) << 5) | (_dst)))
#define I_WAIT(_delay, _sideSet, _polarity, _src, _idx)	INSTR(0b001, _delay, _sideSet, (((_polarity) << 7) | ((_src) << 5) | (_idx)))
#define I_IN(_delay, _sideSet, _src, _nbits)			INSTR(0b010, _delay, _sideSet, (((_src) << 5) | (_nbits)))
#define I_OUT(_delay, _sideSet, _dst, _nbits)			INSTR(0b011, _delay, _sideSet, (((_dst) << 5) | (_nbits)))
#define I_PUSH(_delay, _sideSet, _ifF, _blk)			INSTR(0b100, _delay, _sideSet, (((_ifF) << 6) | ((_blk) << 5)))
#define I_PULL(_delay, _sideSet, _ifF, _blk)			INSTR(0b100, _delay, _sideSet, (0x80 | ((_ifF) << 6) | ((_blk) << 5)))
#define I_MOV(_delay, _sideSet, _dst, _op, _src)		INSTR(0b101, _delay, _sideSet, (((_dst) << 5) | ((_op) << 3) | (_src)))
#define I_IRQ(_delay, _sideSet, _clr, _wait, _idx)		INSTR(0b110, _delay, _sideSet, (((_clr) << 6) | ((_wait) << 5) | (_idx)))
#define I_SET(_delay, _sideSet, _dst, _val)				INSTR(0b111, _delay, _sideSet, (((_dst) << 5) | (_val)))

#define JMP_ALWAYS		0b000	//always
#define JMP_X_ZERO		0b001	//if !X
#define JMP_X_POSTDEC	0b010	//if X--
#define JMP_Y_ZERO		0b011	//if !Y
#define JMP_Y_POSTDEC	0b100	//if Y--
#define JMP_X_NE_Y		0b101	//if X != Y
#define JMP_PIN			0b110	//if: input pin state of (EXECCTRL_JMP_PIN)
#define JMP_OSR_NE		0b111	//if OSR is not empty

#define WAIT_FOR_GPIO	0b00	//absolute gpio address, unrelated ot any configs
#define WAIT_FOR_PIN	0b01	//input pin mapped by PINCTRL_IN_BASE
#define WAIT_FOR_IRQ	0b10	//irq flag selected by index

#define IN_SRC_PINS		0b000
#define IN_SRC_X		0b001
#define IN_SRC_Y		0b010
#define IN_SRC_ZEROES	0b011
#define IN_SRC_ISR		0b110
#define IN_SRC_OSR		0b111

#define OUT_DST_PINS	0b000
#define OUT_DST_X		0b001
#define OUT_DST_Y		0b010
#define OUT_DST_NULL	0b011
#define OUT_DST_PINDIRS	0b100
#define OUT_DST_PC		0b101
#define OUT_DST_ISR		0b110
#define OUT_DST_EXEC	0b111

#define MOV_DST_PINS	0b000
#define MOV_DST_X		0b001
#define MOV_DST_Y		0b010
#define MOV_DST_EXEC	0b100
#define MOV_DST_PC		0b101
#define MOV_DST_ISR		0b110
#define MOV_DST_OSR		0b111

#define MOV_OP_COPY		0b00
#define MOV_OP_INVERT	0b01
#define MOV_OP_BITREV	0b10		//only 32 bits wide

#define MOV_SRC_PINS	0b000
#define MOV_SRC_X		0b001
#define MOV_SRC_Y		0b010
#define MOV_SRC_ZEROES	0b011
#define MOV_SRC_STATUS	0b101		//see EXECCTRL_STATUS_SEL
#define MOV_SRC_ISR		0b110
#define MOV_SRC_OSR		0b111

#define SET_DST_PINS	0b000
#define SET_DST_X		0b001
#define SET_DST_Y		0b010
#define SET_DST_PINDIRS	0b100

static uint8_t __attribute__((aligned(4))) mFramebuffer[DISP_DATA_BYTES];
static void* mFbPtr = mFramebuffer;
static bool mDispOn = false;

static volatile uint32_t mLinesGo = 0;
static uint32_t mLinesPerFrame;

#define PIN_DISP_nWR		9
#define PIN_DISP_D_nC		10
#define PIN_DISP_nRST		11
#define PIN_DISP_nCS		12
#define PIN_DISP_nRD		13

void __attribute__((used)) PIO0_0_IRQHandler(void)
{
	pio0_hw->irq = 1;
	mLinesGo++;
}

void dispPrvFrameCtrReset(void)
{
	mLinesGo = 0;
}

void dispPrvFrameCtrWait(void)
{
	uint32_t oldIrqSta;
	
	while (mLinesGo < mLinesPerFrame);
	asm volatile("mrs %0, primask\n cpsid i": "=r"(oldIrqSta));
	mLinesGo -= mLinesPerFrame;
	asm volatile("msr primask, %0"::"r"(oldIrqSta));
}

static void oledDelay(void)
{
	//a large delay to make it a very slow clock. we can speed it up AFTER it works
	asm volatile("dsb sy\ndsb sy\ndsb sy\ndsb sy\ndsb sy\ndsb sy\ndsb sy\ndsb sy\n");
	asm volatile("dsb sy\ndsb sy\ndsb sy\ndsb sy\ndsb sy\ndsb sy\ndsb sy\ndsb sy\n");
	asm volatile("dsb sy\ndsb sy\ndsb sy\ndsb sy\ndsb sy\ndsb sy\ndsb sy\ndsb sy\n");
	asm volatile("dsb sy\ndsb sy\ndsb sy\ndsb sy\ndsb sy\ndsb sy\ndsb sy\ndsb sy\n");
}

static uint8_t oledRead(void)		//keep in mind pins are outputs
{
	uint8_t ret;
	
	oledDelay();
	sio_hw->gpio_clr = 1 << PIN_DISP_nRD;
	oledDelay();
	ret = sio_hw->gpio_in;
	sio_hw->gpio_set = 1 << PIN_DISP_nRD;
	oledDelay();
	
	return ret;
}

static void oledWrite(uint8_t val)
{
	oledDelay();
	sio_hw->gpio_set = val;
	sio_hw->gpio_clr = (uint8_t)~val;
	oledDelay();
	sio_hw->gpio_clr = 1 << PIN_DISP_nWR;
	oledDelay();
	sio_hw->gpio_set = 1 << PIN_DISP_nWR;
	oledDelay();
}

static uint8_t oledReadStatus(void)
{
	sio_hw->gpio_clr = 1 << PIN_DISP_D_nC;
	return oledRead();
}

static uint8_t oledReadData(void)		//remember the dummy read requirement (p 13)
{
	sio_hw->gpio_set = 1 << PIN_DISP_D_nC;
	return oledRead();
}

static void oledWriteCmd(uint8_t cmd)
{
	sio_hw->gpio_clr = 1 << PIN_DISP_D_nC;
	oledWrite(cmd);
}

static void oledWriteData(uint8_t data)
{
	sio_hw->gpio_set = 1 << PIN_DISP_D_nC;
	oledWrite(data);
}

static void oledCommand(uint8_t cmd)
{
	oledDelay();
	sio_hw->gpio_clr = 1 << PIN_DISP_nCS;
	oledDelay();
	
	oledWriteCmd(cmd);
	
	sio_hw->gpio_set = 1 << PIN_DISP_nCS;
	oledDelay();
}

static void dispPrvPinsToPio(void)
{
	uint_fast8_t i;
	
	for (i = 0; i < 10; i++) {
		
		if (i != 8)
			iobank0_hw->io[i].ctrl = (iobank0_hw->io[i].ctrl &~ IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS) | IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_PIO0_0;
	}
}

static void dispPrvPinsToSio(void)
{
	uint_fast8_t i;
	
	for (i = 0; i < 10; i++) {
		
		if (i != 8)
			iobank0_hw->io[i].ctrl = (iobank0_hw->io[i].ctrl &~ IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS) | IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0;
	}
}

static void dispPrvDmaStop(void)
{
	//irq
	NVIC_DisableIRQ(PIO0_0_IRQn);
	
	dma_hw->ch[1].al1_ctrl &=~ DMA_CH1_CTRL_TRIG_EN_BITS;	//disable channel 1 and now wait
	
	//we could have gotten unlucky and stopped ch1 while it was busy
	//abort it if so
	if (dma_hw->ch[1].al1_ctrl & DMA_CH0_CTRL_TRIG_BUSY_BITS) {
		
		dma_hw->abort = 1 << 1;
		while (dma_hw->abort & (1 << 1));
	}
	
	//wait for ch0 to finish
	while (dma_hw->ch[0].al1_ctrl & DMA_CH0_CTRL_TRIG_BUSY_BITS);
	while (dma_hw->ch[0].transfer_count);
	
	//we need to give the screen a bit of time too...
	oledDelay();
	
	//exit permanent data mode
	sio_hw->gpio_clr = 1 << PIN_DISP_D_nC;
	
	//deselect
	sio_hw->gpio_set = 1 << PIN_DISP_nCS;
	
	dispPrvPinsToSio();
}

static void dispPrvDmaStart(uint_fast8_t depth)
{
	static const uint8_t winCmds[] = {
		0x21, 0x00, 0x9f,	//set column addrs: 0..159
		0x22, 0x00, 0x4f,	//set row addrs: 0..79
	};
	uint_fast8_t i;
	
	dispPrvPinsToSio();
	
	//set window
	for (i = 0; i < sizeof(winCmds) / sizeof(*winCmds); i++)
		oledCommand(winCmds[i]);
	
	dispPrvPinsToPio();
	
	//permanent data mode
	sio_hw->gpio_set = 1 << PIN_DISP_D_nC;
	
	//select
	sio_hw->gpio_clr = 1 << PIN_DISP_nCS;
	
	dma_hw->ch[0].read_addr = (uintptr_t)mFramebuffer,
	dma_hw->ch[0].write_addr = (uintptr_t)&pio0_hw->txf[0];
	dma_hw->ch[0].transfer_count = DISP_DATA_BYTES / sizeof(uint32_t);
	dma_hw->ch[0].al1_ctrl = (0 << DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB) | DMA_CH0_CTRL_TRIG_INCR_READ_BITS | (DMA_CH0_CTRL_TRIG_DATA_SIZE_VALUE_SIZE_WORD << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB) | (1 << DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB) | DMA_CH0_CTRL_TRIG_EN_BITS;
	
	dma_hw->ch[1].read_addr = (uintptr_t)&mFbPtr;
	dma_hw->ch[1].write_addr = (uintptr_t)&dma_hw->ch[0].read_addr;
	dma_hw->ch[1].transfer_count = 1;
	dma_hw->ch[1].ctrl_trig = (0x3f << DMA_CH0_CTRL_TRIG_TREQ_SEL_LSB) | (DMA_CH0_CTRL_TRIG_DATA_SIZE_VALUE_SIZE_WORD << DMA_CH0_CTRL_TRIG_DATA_SIZE_LSB) | (0 << DMA_CH0_CTRL_TRIG_CHAIN_TO_LSB) | DMA_CH0_CTRL_TRIG_EN_BITS;
	
	//irq
	NVIC_EnableIRQ(PIO0_0_IRQn);
}

static void dispPrvProgramPio(uint_fast8_t depth)		//starts the PIO machinery
{
	uint_fast8_t i, nInstrs = 0, startAddr;
	const uint16_t *instrs = NULL;
	
	static const uint16_t code4bpp[] = {
		I_SET(0, 1, SET_DST_X, DISP_DATA_BYTES >> 9),
		I_MOV(0, 1, MOV_DST_ISR, MOV_OP_COPY, MOV_SRC_X),
		I_IN(0, 1, IN_SRC_ZEROES, 9),
		I_MOV(0, 1, MOV_DST_X, MOV_OP_COPY, MOV_SRC_ISR),
		
		//0x04: wait for FR to be low then go hi
		I_WAIT(0, 1, 0, WAIT_FOR_PIN, 0),
		I_WAIT(0, 1, 1, WAIT_FOR_PIN, 0),
		
		//0x06: pull to init buffer
		I_PULL(0, 1, 0, 1),
		
		//0x07: goto 0x09
		I_JMP(0, 1, JMP_ALWAYS, 0x09),
		
		//0x08: out = *data++
		I_OUT(0, 0, OUT_DST_PINS, 8),		//out 8 bits, nRW goes low
		
		//0x09: if (x--) goto 0x08
		I_JMP(0, 1, JMP_X_POSTDEC, 0x08),
		
		//0x0a: irq 0 up
		I_IRQ(0, 1, 0, 0, 0),

		//0x0b: goto 0x00
		I_JMP(0, 1, JMP_ALWAYS, 0x00),
	};
	
	/*
		00 -> 0000
		01 -> 0101
		10 -> 1010
		11 -> 1111
		
		so repeat bottom 2 bits of input
	*/
	static const uint16_t code2bpp[] = {
		
		I_SET(0, 1, SET_DST_X, DISP_DATA_BYTES >> 9),
		I_MOV(0, 1, MOV_DST_ISR, MOV_OP_COPY, MOV_SRC_X),
		I_IN(0, 1, IN_SRC_ZEROES, 9),
		I_MOV(0, 1, MOV_DST_X, MOV_OP_COPY, MOV_SRC_ISR),
		
		//0x04: wait for FR to be low then go hi
		I_WAIT(0, 1, 0, WAIT_FOR_PIN, 0),
		I_WAIT(0, 1, 1, WAIT_FOR_PIN, 0),
		
		//0x06: pull to init buffer
		I_PULL(0, 1, 0, 1),
		
		//0x07: goto 0x13
		I_JMP(0, 1, JMP_ALWAYS, 0x13),
		
		//0x08: dup bottom bits into ISR, then do it again (4 bits in, 8 bits out)
		I_OUT(0, 1, OUT_DST_Y, 2),		//out 2 bits to Y
		I_IN(0, 1, IN_SRC_Y, 2),		//copy to ISR
		I_IN(0, 1, IN_SRC_Y, 2),		//copy to ISR again
		I_OUT(0, 1, OUT_DST_Y, 2),		//out another 2 bits to Y
		I_IN(0, 1, IN_SRC_Y, 2),		//copy to ISR
		I_IN(0, 1, IN_SRC_Y, 2),		//copy to ISR again
		
		//0x0e: ISR now has what we want, but needs the nibble-swapped. first we duplicate the bottom 8 bits
		I_MOV(0, 1, MOV_DST_Y, MOV_OP_COPY, MOV_SRC_ISR),
		I_IN(0, 1, IN_SRC_Y, 8),
		//0x10: then we bitrev it
		I_MOV(0, 1, MOV_DST_ISR, MOV_OP_BITREV, MOV_SRC_ISR),
		//0x11: shift left 4
		I_IN(0, 1, IN_SRC_ZEROES, 4),
		//0x12: bit rev again to output, nRW goes low
		I_MOV(0, 0, MOV_DST_PINS, MOV_OP_BITREV, MOV_DST_ISR),
		
		//0x13: if (x--) goto 0x08
		I_JMP(0, 1, JMP_X_POSTDEC, 0x08),
		
		//0x14: irq 0 up
		I_IRQ(0, 1, 0, 0, 0),

		//0x15: goto 0x00
		I_JMP(0, 1, JMP_ALWAYS, 0x00),
	};
	
	static const uint16_t code1bpp[] = {
		I_SET(0, 1, SET_DST_X, DISP_DATA_BYTES >> 9),
		I_MOV(0, 1, MOV_DST_ISR, MOV_OP_COPY, MOV_SRC_X),
		I_IN(0, 1, IN_SRC_ZEROES, 9),
		I_MOV(0, 1, MOV_DST_X, MOV_OP_COPY, MOV_SRC_ISR),
		
		//0x04: wait for FR to be low then go hi
		I_WAIT(0, 1, 0, WAIT_FOR_PIN, 0),
		I_WAIT(0, 1, 1, WAIT_FOR_PIN, 0),
		
		//0x06: pull to init buffer
		I_PULL(0, 1, 0, 1),
		
		//0x07: goto 0x14
		I_JMP(0, 1, JMP_ALWAYS, 0x15),
		
		//0x08: y = *data++
		I_OUT(0, 1, OUT_DST_Y, 2),		//out 2 bits
		
		//0x09: handle case for "0 0"
		I_JMP(0, 1, JMP_Y_POSTDEC, 0x0c),
		I_MOV(0, 1, MOV_DST_PINS, MOV_OP_COPY, MOV_SRC_ZEROES),				//set pins to 0x00
		I_JMP(0, 0, JMP_ALWAYS, 0x15),										//nRW goes low
		
		//0x0c: handle case for "0 1"
		I_JMP(0, 1, JMP_Y_POSTDEC, 0x10),
		I_SET(0, 1, SET_DST_Y, 0x0f),										//set pins to 0xf0
		I_MOV(0, 1, MOV_DST_PINS, MOV_OP_COPY, MOV_SRC_Y),
		I_JMP(0, 0, JMP_ALWAYS, 0x15),										//nRW goes low
		
		//0x10: handle case for "1 0"
		I_JMP(0, 1, JMP_Y_POSTDEC, 0x14),
		I_SET(0, 1, SET_DST_Y, 0x0f),										//set pins to 0xf0
		I_MOV(0, 1, MOV_DST_PINS, MOV_OP_INVERT, MOV_SRC_Y),
		I_JMP(0, 0, JMP_ALWAYS, 0x15),										//nRW goes low
		
		//0x14: only case "1 1" left
		I_MOV(0, 0, MOV_DST_PINS, MOV_OP_INVERT, MOV_SRC_ZEROES),			//set pins to 0xff, nRW goes low
		
		//0x15: if (x--) goto 0x08
		I_JMP(0, 1, JMP_X_POSTDEC, 0x08),
		
		//0x16: irq 0 up
		I_IRQ(0, 1, 0, 0, 0),

		//0x17: goto 0x00
		I_JMP(0, 1, JMP_ALWAYS, 0),
	};
	
	switch (depth) {
		case 1:
			instrs = code1bpp;
			nInstrs = sizeof(code1bpp) / sizeof(*code1bpp);
			startAddr = 0x00;
			break;
		
		case 2:
			instrs = code2bpp;
			nInstrs = sizeof(code2bpp) / sizeof(*code2bpp);
			startAddr = 0x00;
			break;
		
		case 4:
			instrs = code4bpp;
			nInstrs = sizeof(code4bpp) / sizeof(*code4bpp);
			startAddr = 0x00;
			break;
	}
	
	//load it
	pio0_hw->ctrl = (pio0_hw->ctrl &~ (1 << PIO_CTRL_SM_ENABLE_LSB)) | (1 << PIO_CTRL_SM_RESTART_LSB);	//disable and reset pio0.sm0
	for (i = 0; i < nInstrs; i++)
		pio0_hw->instr_mem[i] = instrs[i];
	
	//goto $startAddr
	pio0_hw->sm[0].instr = I_JMP(0, 1, JMP_ALWAYS, startAddr);
	
	//start PIO
	pio0_hw->ctrl |= (1 << PIO_CTRL_SM_ENABLE_LSB);
}

static uint32_t dispPrvAbsi(int32_t i)
{
	return i * (((i >> 31) << 1) + 1);
}

static void __attribute__((noinline)) dispPrvSetFramerate(uint32_t desiredFpsT1000)
{
	static uint32_t fpsM1000native[16];
	uint32_t testDivSetting = 2, rate, div, bestFpsT1000 = 0, bestSetting;

	if (!fpsM1000native[0]) {
		for (rate = 0; rate < 16; rate++) {
			
			uint32_t curSetting = rate * 16 + testDivSetting, fpsT1000;
			uint64_t time;
			
			oledCommand(0xd5);
			oledCommand(curSetting);	//rate div 8
			delayMsec(50);	//give it time to settle
			
			while (!(sio_hw->gpio_in & (1 << 8)));
			while (sio_hw->gpio_in & (1 << 8));
			time = getTime();
			while (!(sio_hw->gpio_in & (1 << 8)));
			while (sio_hw->gpio_in & (1 << 8));
			time = getTime() - time;
			fpsT1000 = TICKS_PER_SECOND * 1000ull / time;
			fpsM1000native[rate] = fpsT1000 << testDivSetting;
		}
	}
	
	//not that many options, try them all to find the best
	for (rate = 0; rate < 16; rate++) {
		for (div = 0; div < 16; div++) {
			
			uint32_t fpsT1000reached = (fpsM1000native[rate] + (1 << (div - 1))) >> div;
			uint32_t setting = rate * 16 + div;;
			
			if (!bestFpsT1000 || dispPrvAbsi(fpsT1000reached - desiredFpsT1000) < dispPrvAbsi(bestFpsT1000 - desiredFpsT1000)) {
				
				bestFpsT1000 = fpsT1000reached;
				bestSetting = setting;
			}
		}
	}
	pr("OLED: auto-picking OLED config %02xh for %u.%03u FPS\n", bestSetting, bestFpsT1000 / 1000, bestFpsT1000 % 1000);
	
	oledCommand(0xd5);
	oledCommand(bestSetting);
}

static void dispPrvPower(uint8_t depth, uint32_t fpsT1000)	//zero for off
{
	if (depth && !mDispOn) {
		
		uint32_t i;
		
		pr("OLED: configure pins\n");
		for (i = 0; i < 14; i++) {
			
			padsbank0_hw->io[i] = (padsbank0_hw->io[i] &~ (PADS_BANK0_GPIO0_OD_BITS | PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_DRIVE_BITS | PADS_BANK0_GPIO0_PUE_BITS | PADS_BANK0_GPIO0_PDE_BITS | PADS_BANK0_GPIO0_SCHMITT_BITS | PADS_BANK0_GPIO0_SLEWFAST_BITS)) | PADS_BANK0_GPIO0_DRIVE_VALUE_2MA;
			iobank0_hw->io[i].ctrl = (iobank0_hw->io[i].ctrl &~ IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS) | IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0;
		}
		padsbank0_hw->io[8] |= PADS_BANK0_GPIO8_OD_BITS | PADS_BANK0_GPIO8_IE_BITS;		//XXXX: is is important
		sio_hw->gpio_oe_clr = 1 << 8;
		sio_hw->gpio_oe_set = (1 << 14) - 1 - (1 << 8);
			
		pr("OLED :Set pin states\n");
		sio_hw->gpio_set = (1 << PIN_DISP_nWR) | (1 << PIN_DISP_nRD) | (1 << PIN_DISP_nCS) | (1 << PIN_DISP_nRST);
		
		pr("OLED: reset\n");
		sio_hw->gpio_clr = 1 << PIN_DISP_nRST;
		delayMsec(1);
		sio_hw->gpio_set = 1 << PIN_DISP_nRST;
		delayMsec(10);
	
		pr("OLED: init\n");
		static const uint8_t initseq[] = {
			0xfd, 0x12,			//set command lock mode: unlocked
			0xae,				//display: off
			0x20, 0x01,			//set memory addressing mode: vertical
			0x25, 0x01,			//set portrait addressimg mode: portrait
			0xa1,				//set segment remap: SEG0 = col0
			0xa2, 0x00,			//set first row: 0
			0xa4,				//set draw contents selection: GRAM contents
			0xa6,				//set invert: NO
			0xa8, 0x9f,			//set multiplex ratio: 160
	//		0xad, 0x10,			//set IREF: internal
			0xad, 0x00,			//set IREF: external
			0x21, 0x00, 0x9f,	//set column addrs: 0..159
			0x22, 0x00, 0x4f,	//set row addrs: 0..79
			0x81, 0x6f,			//set contrast: 0xdf (internal iref is strong)
			0xbc, 0x1e,			//set pre-charge voltage level: Vcc * 0.5
			0xc8,				//set COM scan direction: ?
			0xd3, 0x00,			//set vertical shift: 0
			0xd5, 0x71,			//set display clock speed and divider ratio: approx 120 FPS
			0xd9, 0x72,			//set phase length: ?
			0xda, 0x12,			//set SEG pins HW config: ?
			0xdb, 0x20,			//set VCOMH: Vcc * 0.8
			0xbf,				//grey scale table: default linear
			
			0xaf,				//display: on
		};
		
		for (i = 0; i < sizeof(initseq); i++)
			oledCommand(initseq[i]);
		
		pr("OLED: setup done\n");
		
		//clear it
		sio_hw->gpio_clr = 1 << PIN_DISP_nCS;
		oledDelay();
		for (i = 0; i < DISP_WIDTH * DISP_HEIGHT * DISP_BPP / 8; i++)
			oledWriteData(0);
		oledDelay();
		sio_hw->gpio_set = 1 << PIN_DISP_nCS;
		oledDelay();
		
		
		//below 80 fps screen feels weird...
		if (fpsT1000 >= 80000)
			mLinesPerFrame = 1;
		else if (fpsT1000 >= 40000)
			mLinesPerFrame = 2;
		else if (fpsT1000 >= 30000)
			mLinesPerFrame = 3;
		
		dispPrvSetFramerate(fpsT1000 * mLinesPerFrame);
		
		pio0_hw->ctrl = (pio0_hw->ctrl &~ (1 << PIO_CTRL_SM_ENABLE_LSB)) | (1 << PIO_CTRL_SM_RESTART_LSB);	//disable and reset pio0.sm0
		pio0_hw->sm[0].clkdiv = (32 << PIO_SM0_CLKDIV_INT_LSB);	//slow, for now
		pio0_hw->sm[0].execctrl = (pio0_hw->sm[0].execctrl &~ PIO_SM0_EXECCTRL_SIDE_EN_BITS) | (SIDE_SET_HAS_ENABLE_BIT ? PIO_SM0_EXECCTRL_SIDE_EN_BITS : 0) | PIO_SM0_EXECCTRL_OUT_STICKY_BITS;
		pio0_hw->sm[0].shiftctrl = (pio0_hw->sm[0].shiftctrl &~ (PIO_SM0_SHIFTCTRL_PULL_THRESH_BITS | PIO_SM0_SHIFTCTRL_OUT_SHIFTDIR_BITS | PIO_SM0_SHIFTCTRL_IN_SHIFTDIR_BITS)) | PIO_SM0_SHIFTCTRL_OUT_SHIFTDIR_BITS | PIO_SM0_SHIFTCTRL_AUTOPULL_BITS;	//shift right, autofill after 8 bits
		pio0_hw->sm[0].pinctrl = (10 << PIO_SM0_PINCTRL_OUT_COUNT_LSB) | (0 << PIO_SM0_PINCTRL_OUT_BASE_LSB);		//0..7 for data, nWR(9) for SET, FR(8) for input
		
		dispPrvPinsToPio();
		
		//set pins 0..9 as outputs
		//move it to X
		pio0_hw->sm[0].instr = I_MOV(0, 1, MOV_DST_OSR, MOV_OP_INVERT, MOV_SRC_ZEROES);
		//set it to pin dirs
		pio0_hw->sm[0].instr = I_OUT(0, 0, OUT_DST_PINDIRS, 10);
		//re-config pins: 0..7 for data, nWR(9) for SET, FR(8) for input
		
		pio0_hw->sm[0].pinctrl = (SIDE_SET_NUM_BITS << PIO_SM0_PINCTRL_SIDESET_COUNT_LSB) |
									(8 << PIO_SM0_PINCTRL_SET_COUNT_LSB) | (0 << PIO_SM0_PINCTRL_SET_BASE_LSB) | 
									(8 << PIO_SM0_PINCTRL_OUT_COUNT_LSB) | (8 << PIO_SM0_PINCTRL_IN_BASE_LSB) |
									(9 << PIO_SM0_PINCTRL_SIDESET_BASE_LSB) | (0 << PIO_SM0_PINCTRL_OUT_BASE_LSB);
	
		//frame irq
		pio0_hw->inte0 = PIO_IRQ0_INTE_SM0_BITS;
	
		//program code
		dispPrvProgramPio(depth);
		dispPrvDmaStart(depth);
		
		pr("display should be up\n");
		
		mDispOn = true;
	}
	else if (!depth && mDispOn) {
		
		uint_fast8_t i;
		
		dispPrvDmaStop();
		oledCommand(0xae);			//display off
		
		mDispOn = false;
		
		for (i = 0; i < 10; i++)
			iobank0_hw->io[i].ctrl = (iobank0_hw->io[i].ctrl &~ IO_BANK0_GPIO0_CTRL_FUNCSEL_BITS) | IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_SIO_0;
		
		sio_hw->gpio_clr = (1 << PIN_DISP_nWR) | (1 << PIN_DISP_nRD) | (1 << PIN_DISP_nCS) | (1 << PIN_DISP_nRST) | ((1 << 10) - 1);
	}
}

void dispOff(void)
{
	dispPrvPower(0, 0);
}

void dispInit(uint32_t desiredFramerate)
{
	pr("oled: requested %u fps\n", desiredFramerate / 1000);
	
	dispPrvPower(4, desiredFramerate);
}

void dispSetContrast(uint_fast8_t val)
{
	//convert 5-bit val to a sane value
	val = 0x30 + ((val & 0x1f) * 0xc0 + 0x10) / 0x1f;
		
	dispPrvDmaStop();
	oledCommand(0x81);
	oledCommand(val);
	dispPrvDmaStart(4);
}

void dispSetBrightness(uint_fast8_t val)
{
	//nothing
}

void* dispGetFb(void)
{
	return mFramebuffer;
}










