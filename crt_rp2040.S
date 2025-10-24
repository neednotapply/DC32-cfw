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
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	.word 0
	weakexc SVC
	.word 0
	.word 0
	weakexc PendSV
	weakexc SysTick

	// Chip Level interrupts here
	weakisr TIMER0
	weakisr TIMER1
	weakisr TIMER2
	weakisr TIMER3
	weakisr PWM
	weakisr USB
	weakisr XIP
	weakisr PIO0_0
	weakisr PIO0_1
	weakisr PIO1_0
	weakisr PIO1_1
	weakisr DMA0
	weakisr DMA1
	weakisr IO		//io irq bank0
	weakisr QSPI	//io irq qspi bank
	weakisr SIO0
	weakisr SIO1
	weakisr CLOCKS
	weakisr SPI0
	weakisr SPI1
	weakisr UART0
	weakisr UART1
	weakisr ADC
	weakisr I2C0
	weakisr I2C1
	weakisr RTC

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

.balign 4
9:	//info on layout
	.word __data_start
	.word __data_data
	.word __data_end
	.word __bss_start
	.word __bss_end


3:
//init runtime
	movs	r1, #0
	ldrh	r7, [r1, #0x14]	//function table
	ldrh	r6, [r1, #0x18]	//table lookup
	ldr		r5, =rttab_req_start
	ldr		r4, =rttab_data_start
3:
	ldrh	r1, [r5]
	cmp		r1, #0
	beq		9f
	mov		r0, r7
	blx		r6
	cmp		r0, #0
4:						//initite loop on func not found
	beq		4b
	stmia   r4!, {r0}
	adds	r5, #2
	b		3b

//go
9:
	ldr		r0, =0xE000ED08
	ldr		r1, =__VECTORS
	str		r1, [r0]
	bl		micromain
	//no return
9:
	b		9b

.section .rodata.rttab
	.balign 2
	rttab_req_start:

.section .bss.rttab
	.balign 4
	rttab_data_start:

.macro addrtfunc name, rpname		//3 params max
	.section .rodata.rttab
	.hword \rpname
	.section .bss.rttab
	.globl ___p\name
	___p\name:
	.word 0
	.section .text.rttab
	.globl \name
	\name:
		ldr r3, =___p\name
		ldr	r3, [r3]
		bx  r3
.endm


addrtfunc __clzsi2,			'L' + 256 * '3'
addrtfunc __ctzsi2,			'T' + 256 * '3'
addrtfunc __popcountsi2,	'P' + 256 * '3'
addrtfunc memset,			'M' + 256 * 'S'
addrtfunc memcpy,			'M' + 256 * 'C'



.section .rodata.rttab
	.hword 0		//terminator


	



//safer routines to not worry about irqs. If an irq interrupted a calculation, it would be long finished so we can preserve results
//we do not rely on input regs to stay and thus need not restore them
.section .text.udiv
.globl __udivsi3 
.globl __aeabi_uidivmod
.globl __aeabi_uidiv
__udivsi3:
__aeabi_uidivmod:
__aeabi_uidiv:
	mov		r12, r4
	movs	r4, #0xd0
	lsls	r4, #24
	ldr		r2, [r4, #0x70]
	ldr		r3, [r4, #0x74]
	str		r0, [r4, #0x60]
	str		r1, [r4, #0x64]
	adds	r4, #0x70
	movs	r0, r4
	nop
	nop
	nop
	nop
	nop
	nop
	ldm		r0, {r0, r1}	//get results
	stmia	r4!, {r2, r3}	//restore old ones
	mov		r4, r12
	bx		lr

.section .text.sdiv
.globl __divsi3 
.globl __aeabi_idivmod
.globl __aeabi_idiv
__divsi3:
__aeabi_idivmod:
__aeabi_idiv:
	mov		r12, r4
	movs	r4, #0xd0
	lsls	r4, #24
	ldr		r2, [r4, #0x70]
	ldr		r3, [r4, #0x74]
	str		r0, [r4, #0x60]
	str		r1, [r4, #0x64]
	adds	r4, #0x70
	movs	r0, r4
	nop
	nop
	nop
	nop
	nop
	nop
	ldm		r0, {r0, r1}	//get results
	stmia	r4!, {r2, r3}	//restore old ones
	mov		r4, r12
	bx		lr
