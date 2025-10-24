.macro weaksym	name
	.weak \name
	.set \name, DefaultHandler
	.word \name + 1
.endm

.macro weakexc	name
	weaksym \name\()_Handler
.endm

.macro weakisr	name
	weaksym \name\()_IRQHandler
.endm

.syntax unified
.text
.section .text.vec
.globl __VECTORS
__VECTORS:
	.word __stack_top
	.word __entry + 1
	weakexc NMI
	weakexc HardFault
	weakexc MemManage
	weakexc BusFault
	weakexc UsageFault
	weakexc SecureFault
	.word 2							//FORCED_OTA_VERSION @ 0x20 - one byte.
	.word 0
	.word 0
	weakexc SVC
	weakexc DebugMonitor
	.word 0
	weakexc PendSV
	weakexc SysTick


	// Chip Level interrupts here
	weakisr TIMER0_IRQ_0
	weakisr TIMER0_IRQ_1
	weakisr TIMER0_IRQ_2
	weakisr TIMER0_IRQ_3
	weakisr TIMER1_IRQ_0
	weakisr TIMER1_IRQ_1
	weakisr TIMER1_IRQ_2
	weakisr TIMER1_IRQ_3
	weakisr PWM_IRQ_WRAP_0
	weakisr PWM_IRQ_WRAP_1
	weakisr DMA_IRQ_0
	weakisr DMA_IRQ_1
	weakisr DMA_IRQ_2
	weakisr DMA_IRQ_3
	weakisr USBCTRL_IRQ
	weakisr PIO0_IRQ_0
	weakisr PIO0_IRQ_1
	weakisr PIO1_IRQ_0
	weakisr PIO1_IRQ_1
	weakisr PIO2_IRQ_0
	weakisr PIO2_IRQ_1
	weakisr IO_IRQ_BANK0
	weakisr IO_IRQ_BANK0_NS
	weakisr IO_IRQ_QSPI
	weakisr IO_IRQ_QSPI_NS
	weakisr SIO_IRQ_FIFO
	weakisr SIO_IRQ_BELL
	weakisr SIO_IRQ_FIFO_NS
	weakisr SIO_IRQ_BELL_NS
	weakisr SIO_IRQ_MTIMECMP
	weakisr CLOCKS_IRQ
	weakisr SPI0_IRQ
	weakisr SPI1_IRQ
	weakisr UART0_IRQ
	weakisr UART1_IRQ
	weakisr ADC_IRQ_FIFO
	weakisr I2C0_IRQ
	weakisr I2C1_IRQ
	weakisr OTP_IRQ
	weakisr TRNG_IRQ
	weakisr PROC0_IRQ_CTI
	weakisr PROC1_IRQ_CTI
	weakisr PLL_SYS_IRQ
	weakisr PLL_USB_IRQ
	weakisr POWMAN_IRQ_POW
	weakisr POWMAN_IRQ_TIMER
	weakisr SPAREIRQ_IRQ_0
	weakisr SPAREIRQ_IRQ_1
	weakisr SPAREIRQ_IRQ_2
	weakisr SPAREIRQ_IRQ_3
	weakisr SPAREIRQ_IRQ_4
	weakisr SPAREIRQ_IRQ_5

//the binary header for the boot rom to use

//the binary header for the boot rom to use
rp2350_exe_header_start:

#if 0		//RP2350 A0 rom header

	//start magic
	.word 0xffffded3
	
	//next block ofset
	.byte	0x01															//type = "next block offset"
	.byte	2																//size of this entry in words - 2 words (this header and the offset that follows)
	.hword	0																//unused
	.word	0																//the offset (0 for none)
	
	//image type
	.byte	0x02															//type = "image type"
	.byte	1																//size of this entry in words - 1 word (this one)
	.hword	0x0121															//secure EXE for rp2350
	
	//vector table
	.byte 0x03																//type = "vector table type"
	.byte	2																//size of this entry in words - 2 words (this header and the address that follows)
	.hword	0																//unused
	.word	__VECTORS														//the vector table address
	
	//terminator and size
	.byte	0xff															//type = "terminator"
	.byte	(rp2350_exe_header_end - rp2350_exe_header_start - 12) / 4		//size in words of the rest of the data
	.hword	0																//unused
	
	//end magic
	.word	0xab123579

#else	//A1 works

	//start magic
	.word 0xffffded3
	
	//image type
	.byte	0x42															//type = "image type"
	.byte	1																//size of this entry in words - 1 word (this one)
	.hword	0x1021															//secure EXE for rp2350

	//terminator and size
	.byte	0xff															//type = "terminator"
	.hword	(rp2350_exe_header_end - rp2350_exe_header_start - 16) / 4		//size in words of the rest of the data
	.byte	0																//unused
	
	//not sure
	.word 0

	//end magic
	.word	0xab123579
#endif

rp2350_exe_header_end:



.globl DefaultHandler
DefaultHandler:
	wfi
	b		DefaultHandler

__entry:

//init .data and .bss
	adr		r7, 9f
	ldmia	r7!, {r0-r4}
	movs	r6, #0
	
1:
	cmp		r0, r2
	beq		2f
	ldmia	r1!, {r5}
	stmia	r0!, {r5}
	b		1b
	
2:
	cmp		r3, r4
	beq		3f
	stmia	r3!, {r6}
	b		2b

//go
3:
	ldr		r0, =0xE000ED08
	ldr		r1, =__VECTORS
	str		r1, [r0]
	bl		micromain
	//no return
8:
	b		8b

.balign 4
9:	//info on layout
	.word __data_start
	.word __data_data
	.word __data_end
	.word __bss_start
	.word __bss_end







