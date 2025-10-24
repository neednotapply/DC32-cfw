#include "timebase.h"
#include "printf.h"
#include "2350.h"
#include "qspi.h"

#pragma GCC optimize ("Os")
#define __ramcode	__attribute__((section(".fastcode")))

//#define SPEED_TEST_FLASH

#define QSPI_LARGE_ERASE_GRANULARITY		32768

static uint64_t mFlashUid;

static void readTest(const char *lbl, uintptr_t addr, uintptr_t len)
{
	volatile uint32_t *ptr = (volatile uint32_t*)addr;
	uint32_t totalBits = 8 * len;
	uint64_t t, bps;
	
	len /= sizeof(uint32_t);
	t = getTime();
	while (len--)
		(void)*ptr++;
	t = getTime() - t;
	bps = (uint64_t)TICKS_PER_SECOND * totalBits / t;
	
	pr("%s read took 0x%016llx ticks (%llu msec). This is %u.%06u Mbps\n", lbl, t, t * 1000 / TICKS_PER_SECOND, (uint32_t)(bps >> 20), (uint32_t)(bps & ((1 << 20) - 1)));
}

static void __ramcode flashPrvEnterDirectMode(void)
{
	qmi_hw->m[0].rcmd &=~ QMI_M0_RCMD_SUFFIX_BITS;
	(void)*(volatile uint32_t*)0x1c000000;

	//use direct mode
	qmi_hw->direct_csr = (8 << QMI_DIRECT_CSR_CLKDIV_LSB) | QMI_DIRECT_CSR_EN_BITS;
	
	//send exit quad mode command in quad mode (just in case)
	qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS0N_BITS;
	qmi_hw->direct_tx = QMI_DIRECT_TX_NOPUSH_BITS | QMI_DIRECT_TX_OE_BITS | (QMI_DIRECT_TX_IWIDTH_VALUE_Q << QMI_DIRECT_TX_IWIDTH_LSB) | 0xff;
	while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);
	qmi_hw->direct_csr &=~ QMI_DIRECT_CSR_ASSERT_CS0N_BITS;
}

static void __ramcode flashPrvEnterXipMode(void)
{
	uint32_t maxClk = 94000000, rxDelay = TICKS_PER_SECOND / 50000000, divClk;
	uint_fast8_t i;

	divClk = (TICKS_PER_SECOND + maxClk - 1) / maxClk;	//non-integer divisions do not do well with QSPI
	pr("QSPI: using divisor %u for a clock rate of %luHz. RXDELAY = %d\n", divClk, TICKS_PER_SECOND / divClk, rxDelay);
	
	//issue a read with the proper mode bits
	qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS0N_BITS;
	qmi_hw->direct_tx = QMI_DIRECT_TX_NOPUSH_BITS | QMI_DIRECT_TX_OE_BITS | (QMI_DIRECT_TX_IWIDTH_VALUE_S << QMI_DIRECT_TX_IWIDTH_LSB) | 0xEB;
	while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);
	for (i = 0; i < 3; i++) {
		qmi_hw->direct_tx = QMI_DIRECT_TX_NOPUSH_BITS | QMI_DIRECT_TX_OE_BITS | (QMI_DIRECT_TX_IWIDTH_VALUE_Q << QMI_DIRECT_TX_IWIDTH_LSB) | 0x00;
		while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);
	}
	qmi_hw->direct_tx = QMI_DIRECT_TX_NOPUSH_BITS | QMI_DIRECT_TX_OE_BITS | (QMI_DIRECT_TX_IWIDTH_VALUE_Q << QMI_DIRECT_TX_IWIDTH_LSB) | 0x20;
	while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);
	for (i = 0; i < 3; i++) {	//two dummy bytes and one data byte
		qmi_hw->direct_tx = QMI_DIRECT_TX_NOPUSH_BITS | (QMI_DIRECT_TX_IWIDTH_VALUE_Q << QMI_DIRECT_TX_IWIDTH_LSB) | 0x00;
		while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);
	}
	qmi_hw->direct_csr &=~ QMI_DIRECT_CSR_ASSERT_CS0N_BITS;
	

	qmi_hw->m[0].rfmt = (4 << QMI_M0_RFMT_DUMMY_LEN_LSB) | (1 << QMI_M0_RFMT_SUFFIX_LEN_LSB) |
						(QMI_M0_RFMT_DATA_WIDTH_VALUE_Q << QMI_M0_RFMT_DATA_WIDTH_LSB) |
						(QMI_M0_RFMT_DUMMY_WIDTH_VALUE_Q << QMI_M0_RFMT_DUMMY_WIDTH_LSB) |
						(QMI_M0_RFMT_ADDR_WIDTH_VALUE_Q << QMI_M0_RFMT_ADDR_WIDTH_LSB) |
						(QMI_M0_RFMT_SUFFIX_WIDTH_VALUE_Q << QMI_M0_RFMT_SUFFIX_WIDTH_LSB);

	qmi_hw->m[0].rcmd = (0x20 << QMI_M0_RCMD_SUFFIX_LSB);
	qmi_hw->m[0].timing = (2 << QMI_M1_TIMING_COOLDOWN_LSB) | (QMI_M1_TIMING_PAGEBREAK_VALUE_4096 << QMI_M1_TIMING_PAGEBREAK_LSB) | (rxDelay << QMI_M1_TIMING_RXDELAY_LSB) | (divClk << QMI_M1_TIMING_CLKDIV_LSB);

	qmi_hw->direct_csr &=~ QMI_DIRECT_CSR_EN_BITS;
}

static void __ramcode flashPrvBusyWait(void)
{
	uint8_t sta;

	do {
		qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS0N_BITS;
		qmi_hw->direct_tx = QMI_DIRECT_TX_NOPUSH_BITS | QMI_DIRECT_TX_OE_BITS | (QMI_DIRECT_TX_IWIDTH_VALUE_S << QMI_DIRECT_TX_IWIDTH_LSB) | 0x05;
		while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);
		qmi_hw->direct_tx = (QMI_DIRECT_TX_IWIDTH_VALUE_S << QMI_DIRECT_TX_IWIDTH_LSB) | 0x00;
		while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);
		qmi_hw->direct_csr &=~ QMI_DIRECT_CSR_ASSERT_CS0N_BITS;
		sta = qmi_hw->direct_rx;
	} while (sta & 0x01);
}

static void __ramcode flashPrvWel(void)
{
	qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS0N_BITS;
	qmi_hw->direct_tx = QMI_DIRECT_TX_NOPUSH_BITS | QMI_DIRECT_TX_OE_BITS | (QMI_DIRECT_TX_IWIDTH_VALUE_S << QMI_DIRECT_TX_IWIDTH_LSB) | 0x06;
	while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);
	qmi_hw->direct_csr &=~ QMI_DIRECT_CSR_ASSERT_CS0N_BITS;
}

static void __ramcode flashPrvReadUid(uint64_t *uidP)
{
	uint32_t cmd = 0x4b;
	uint64_t uid = 0;
	uint_fast8_t i;

	qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS0N_BITS;
	for (i = 0; i < 5; i++, cmd >>= 8) {
		qmi_hw->direct_tx = QMI_DIRECT_TX_NOPUSH_BITS | QMI_DIRECT_TX_OE_BITS | (QMI_DIRECT_TX_IWIDTH_VALUE_S << QMI_DIRECT_TX_IWIDTH_LSB) | cmd;
		while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);
	}
	for (i = 0; i < 8; i++) {
		qmi_hw->direct_tx = (QMI_DIRECT_TX_IWIDTH_VALUE_S << QMI_DIRECT_TX_IWIDTH_LSB) | 0x00;
		while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);
		uid = (uid << 8) + (uint8_t)qmi_hw->direct_rx;
	}
	qmi_hw->direct_csr &=~ QMI_DIRECT_CSR_ASSERT_CS0N_BITS;
	*uidP = uid;
}

static void __ramcode flashPrvProgram(uint32_t addr, const uint8_t *buf, uint32_t len)
{
	uint32_t send = (addr & 0x00ffffff) | 0x02000000;
	uint_fast8_t i;

	flashPrvWel();

	qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS0N_BITS;
	for (i = 0; i < 4; i++, send <<= 8) {

		qmi_hw->direct_tx = QMI_DIRECT_TX_NOPUSH_BITS | QMI_DIRECT_TX_OE_BITS | (QMI_DIRECT_TX_IWIDTH_VALUE_S << QMI_DIRECT_TX_IWIDTH_LSB) | (send >> 24);
		while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);
	}
	while (len--) {

		qmi_hw->direct_tx = QMI_DIRECT_TX_NOPUSH_BITS | QMI_DIRECT_TX_OE_BITS | (QMI_DIRECT_TX_IWIDTH_VALUE_S << QMI_DIRECT_TX_IWIDTH_LSB) | *buf++;
		while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);
	}
	qmi_hw->direct_csr &=~ QMI_DIRECT_CSR_ASSERT_CS0N_BITS;

	flashPrvBusyWait();
}

static void __ramcode flashPrvErz(uint32_t addr, uint8_t cmd)
{
	uint32_t send = (addr & 0x00ffffff) | (((uint32_t)cmd) << 24);
	uint_fast8_t i;

	flashPrvWel();

	qmi_hw->direct_csr |= QMI_DIRECT_CSR_ASSERT_CS0N_BITS;
	for (i = 0; i < 4; i++, send <<= 8) {

		qmi_hw->direct_tx = QMI_DIRECT_TX_NOPUSH_BITS | QMI_DIRECT_TX_OE_BITS | (QMI_DIRECT_TX_IWIDTH_VALUE_S << QMI_DIRECT_TX_IWIDTH_LSB) | (send >> 24);
		while (qmi_hw->direct_csr & QMI_DIRECT_CSR_BUSY_BITS);
	}
	qmi_hw->direct_csr &=~ QMI_DIRECT_CSR_ASSERT_CS0N_BITS;

	flashPrvBusyWait();
}

bool __attribute__((noinline)) __ramcode flashWrite(uint32_t addr, uint32_t erzSz, const void *dataSrc, uint32_t writeSz)	//will erase as needed, sz must be >= 256
{
	const uint8_t *buf = (const uint8_t*)dataSrc;
	uint32_t i, prevPrimask, curAddr, now, largerSize;
	bool ret = true;
	
	addr &= 0x00ffffff;	//24-bit addressing

	if (erzSz % QSPI_ERASE_GRANULARITY)
		return false;
	if (writeSz % QSPI_WRITE_GRANULARITY)
		return false;
	if (erzSz && addr % QSPI_ERASE_GRANULARITY)
		return false;
	if (writeSz && addr % QSPI_WRITE_GRANULARITY)
		return false;

	//we cannot handle an interrupt at this time
	asm volatile("mrs %0, PRIMASK\n cpsid i":"=r"(prevPrimask)::"memory");

	flashPrvEnterDirectMode();
	
	//erase
	for (curAddr = addr, i = 0; i < erzSz; i += now, curAddr += now) {

		uint32_t left = erzSz - i;

		if (curAddr % QSPI_LARGE_ERASE_GRANULARITY || left < QSPI_LARGE_ERASE_GRANULARITY) {

			flashPrvErz(curAddr, 0x20);
			now = QSPI_ERASE_GRANULARITY;
		}
		else {

			flashPrvErz(curAddr, 0x52);
			now = QSPI_LARGE_ERASE_GRANULARITY;
		}
	}

	//write
	for (curAddr = addr, i = 0; i < writeSz; i += QSPI_WRITE_GRANULARITY, curAddr += QSPI_WRITE_GRANULARITY, buf += QSPI_WRITE_GRANULARITY)
		flashPrvProgram(curAddr, buf, QSPI_WRITE_GRANULARITY);

	//flush
	largerSize = erzSz > writeSz ? erzSz : writeSz;
	for (i = 0; i < largerSize; i += 8)
		*(volatile uint32_t*)(0x18000000 + i + addr) = 2;

	flashPrvEnterXipMode();
	
	//now we can take irqs
	asm volatile("msr PRIMASK, %0"::"r"(prevPrimask):"memory");


	return ret;
}


void flashBootInit(void)
{
	uint32_t i, prevPrimask;
	uint64_t uid;

#ifdef SPEED_TEST_FLASH
	pr("flash reads: 0x%08x 0x%08x 0x%08x 0x%08x\n", ((volatile uint32_t*)0x1c000000)[0], ((volatile uint32_t*)0x1c000000)[1], ((volatile uint32_t*)0x1c000000)[2], ((volatile uint32_t*)0x1c000000)[3]);

	for (i = 0; i < 3; i++)	
		readTest("ROM", 0x1c000000, 2 << 20);

#endif

	//we cannot handle an interrupt at this time
	asm volatile("mrs %0, PRIMASK\n cpsid i":"=r"(prevPrimask)::"memory");

	flashPrvEnterDirectMode();
	flashPrvReadUid(&uid);
	flashPrvEnterXipMode();
	
	//now we can take irqs
	asm volatile("msr PRIMASK, %0"::"r"(prevPrimask):"memory");

	mFlashUid = uid;
	pr("Flash uid: 0x%016llx\n", uid);
	
#ifdef SPEED_TEST_FLASH
	pr("flash reads: 0x%08x 0x%08x 0x%08x 0x%08x\n", ((volatile uint32_t*)0x1c000000)[0], ((volatile uint32_t*)0x1c000000)[1], ((volatile uint32_t*)0x1c000000)[2], ((volatile uint32_t*)0x1c000000)[3]);

	for (i = 0; i < 3; i++)	
		readTest("ROM", 0x1c000000, 2 << 20);
#endif
}

uint64_t flashGetUid(void)
{
	return mFlashUid;
}


