#include <string.h>
#include "tusb.h"
#include "sd.h"
#include "usbDevice.h"
#include "usbMsc.h"

#define USB_MSC_VID		0x1209
#define USB_MSC_PID		0xdc33
#define USB_MSC_NORMAL_SD_SPEED	500000
#define USB_MSC_FAST_SD_SPEED	12500000

static bool mStarted, mWritable, mEjected, mSpeedLimited;
static uint32_t mBlockCount;
static const char *mLastError = "none";
static const char *mLastOp = "Idle";
static uint32_t mLastLba, mLastBytes;

static bool usbMscPrvRangeOk(uint32_t lba, uint32_t numBlocks)
{
	return mStarted && !mEjected && numBlocks && lba < mBlockCount && numBlocks <= mBlockCount - lba;
}

static int32_t usbMscPrvIoError(const char *err)
{
	mLastError = err;
	return TUD_MSC_RET_ERROR;
}

static void usbMscPrvStatus(const char *op, uint32_t lba, uint32_t bytes)
{
	mLastOp = op;
	mLastLba = lba;
	mLastBytes = bytes;
}

static bool usbMscPrvFallbackSpeed(void)
{
	if (mSpeedLimited)
		return false;
	mSpeedLimited = true;
	sdSetSpeedLimit(USB_MSC_NORMAL_SD_SPEED);
	mLastError = "SD error; using safe speed";
	return true;
}

bool usbMscBegin(bool writable)
{
	struct UsbDeviceInfo info;
	union SdFlags flags;

	mLastError = "none";
	mLastOp = "Starting";
	mLastLba = 0;
	mLastBytes = 0;
	mBlockCount = sdGetNumSecs();
	if (!mBlockCount) {
		mLastError = "SD card is not initialized";
		return false;
	}

	flags.value = sdGetFlags();
	mWritable = writable && !flags.RO;
	mEjected = false;
	mSpeedLimited = false;
	mStarted = true;
	sdSetSpeedLimit(USB_MSC_FAST_SD_SPEED);

	memset(&info, 0, sizeof(info));
	info.vid = USB_MSC_VID;
	info.pid = USB_MSC_PID;
	strcpy(info.manufacturer, "DC32");
	strcpy(info.product, "DC32 SD Card");

	if (!usbDeviceBegin(UsbDeviceModeMsc, &info)) {
		mLastError = usbDeviceLastError();
		sdSetSpeedLimit(USB_MSC_NORMAL_SD_SPEED);
		mStarted = false;
		mWritable = false;
		mBlockCount = 0;
		return false;
	}
	mLastOp = "Ready";
	return true;
}

void usbMscTask(void)
{
	usbDeviceTask();
}

bool usbMscStarted(void)
{
	return mStarted && usbDeviceStarted(UsbDeviceModeMsc);
}

bool usbMscMounted(void)
{
	return usbMscStarted() && usbDeviceMounted();
}

bool usbMscEjected(void)
{
	usbMscTask();
	return mEjected;
}

bool usbMscWritable(void)
{
	return mWritable;
}

const char *usbMscLastError(void)
{
	return mLastError;
}

void usbMscGetStatus(struct UsbMscStatus *status)
{
	if (!status)
		return;
	status->op = mLastOp;
	status->error = mLastError;
	status->lba = mLastLba;
	status->bytes = mLastBytes;
	status->speedLimited = mSpeedLimited;
}

void usbMscEnd(void)
{
	if (!mStarted)
		return;
	usbDeviceEnd();
	sdSetSpeedLimit(USB_MSC_NORMAL_SD_SPEED);
	mStarted = false;
	mWritable = false;
	mEjected = false;
	mSpeedLimited = false;
	mBlockCount = 0;
	mLastOp = "Idle";
}

uint32_t tud_msc_inquiry2_cb(uint8_t lun, scsi_inquiry_resp_t *inquiry_resp, uint32_t bufsize)
{
	const char vid[] = "DC32";
	const char pid[] = "SD Card";
	const char rev[] = "1.0";

	(void)lun;
	(void)bufsize;
	(void)strncpy((char*)inquiry_resp->vendor_id, vid, 8);
	(void)strncpy((char*)inquiry_resp->product_id, pid, 16);
	(void)strncpy((char*)inquiry_resp->product_rev, rev, 4);
	return sizeof(scsi_inquiry_resp_t);
}

bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
	usbMscPrvStatus("Ready?", 0, 0);
	if (lun || !mStarted || !mBlockCount || mEjected) {
		(void)tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3a, 0x00);
		return false;
	}
	return true;
}

void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count, uint16_t *block_size)
{
	(void)lun;
	usbMscPrvStatus("Capacity", 0, 0);
	*block_count = mBlockCount;
	*block_size = SD_BLOCK_SIZE;
}

bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject)
{
	(void)lun;
	(void)power_condition;
	(void)load_eject;
	usbMscPrvStatus(start ? "Start" : "Stop", 0, 0);
	if (start)
		mEjected = false;
	else
		mEjected = true;
	return true;
}

uint8_t tud_msc_get_maxlun_cb(void)
{
	return 0;
}

bool tud_msc_is_writable_cb(uint8_t lun)
{
	(void)lun;
	usbMscPrvStatus("Writable?", 0, 0);
	return mWritable;
}

static bool usbMscPrvReadSectors(uint32_t lba, uint8_t *dst, uint32_t numBlocks)
{
	uint32_t i;

	if (!usbMscPrvRangeOk(lba, numBlocks))
		return false;
	if (numBlocks == 1)
		return sdSecRead(lba, dst);
	if (!sdReadStart(lba, numBlocks))
		return false;
	for (i = 0; i < numBlocks; i++) {
		if (!sdReadNext(dst + i * SD_BLOCK_SIZE)) {
			(void)sdReadStop();
			return false;
		}
	}
	return sdReadStop();
}

static bool usbMscPrvWriteSectors(uint32_t lba, const uint8_t *src, uint32_t numBlocks)
{
	uint32_t i;

	if (!usbMscPrvRangeOk(lba, numBlocks))
		return false;
	if (numBlocks == 1)
		return sdSecWrite(lba, src);
	if (!sdWriteStart(lba, numBlocks))
		return false;
	for (i = 0; i < numBlocks; i++) {
		if (!sdWriteNext(src + i * SD_BLOCK_SIZE)) {
			(void)sdWriteStop();
			return false;
		}
	}
	return sdWriteStop();
}

static int32_t usbMscPrvRead10(uint8_t lun, uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize)
{
	uint8_t *dst = (uint8_t*)buffer;
	uint8_t scratch[SD_BLOCK_SIZE] __attribute__((aligned(4)));
	uint32_t done = 0;

	if (lun)
		return usbMscPrvIoError("invalid LUN");
	usbMscPrvStatus("READ10", lba, bufsize);

	while (done < bufsize) {
		uint32_t chunk, blocks;

		if (!usbMscPrvRangeOk(lba, 1) || offset >= SD_BLOCK_SIZE)
			return usbMscPrvIoError("read outside SD card");

		if (!offset && bufsize - done >= SD_BLOCK_SIZE) {
			blocks = (bufsize - done) / SD_BLOCK_SIZE;
			if (!usbMscPrvRangeOk(lba, blocks))
				return usbMscPrvIoError("read outside SD card");
			if (!usbMscPrvReadSectors(lba, dst + done, blocks))
				return usbMscPrvIoError("SD read failed");
			done += blocks * SD_BLOCK_SIZE;
			lba += blocks;
			continue;
		}

		chunk = SD_BLOCK_SIZE - offset;
		if (chunk > bufsize - done)
			chunk = bufsize - done;

		if (!sdSecRead(lba, scratch))
			return usbMscPrvIoError("SD read failed");
		memcpy(dst + done, scratch + offset, chunk);

		done += chunk;
		lba++;
		offset = 0;
	}

	return (int32_t)done;
}

int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize)
{
	int32_t ret = usbMscPrvRead10(lun, lba, offset, buffer, bufsize);

	if (ret < 0 && usbMscPrvFallbackSpeed())
		ret = usbMscPrvRead10(lun, lba, offset, buffer, bufsize);
	return ret;
}

static int32_t usbMscPrvWrite10(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize)
{
	uint8_t scratch[SD_BLOCK_SIZE] __attribute__((aligned(4)));
	uint32_t done = 0;

	if (lun)
		return usbMscPrvIoError("invalid LUN");
	if (!mWritable)
		return usbMscPrvIoError("SD card is read-only");
	usbMscPrvStatus("WRITE10", lba, bufsize);

	while (done < bufsize) {
		uint32_t chunk, blocks;

		if (!usbMscPrvRangeOk(lba, 1) || offset >= SD_BLOCK_SIZE)
			return usbMscPrvIoError("write outside SD card");

		if (!offset && bufsize - done >= SD_BLOCK_SIZE) {
			blocks = (bufsize - done) / SD_BLOCK_SIZE;
			if (!usbMscPrvRangeOk(lba, blocks))
				return usbMscPrvIoError("write outside SD card");
			if (!usbMscPrvWriteSectors(lba, buffer + done, blocks))
				return usbMscPrvIoError("SD write failed");
			done += blocks * SD_BLOCK_SIZE;
			lba += blocks;
			continue;
		}

		chunk = SD_BLOCK_SIZE - offset;
		if (chunk > bufsize - done)
			chunk = bufsize - done;

		if (!sdSecRead(lba, scratch))
			return usbMscPrvIoError("SD read failed");
		memcpy(scratch + offset, buffer + done, chunk);
		if (!sdSecWrite(lba, scratch))
			return usbMscPrvIoError("SD write failed");

		done += chunk;
		lba++;
		offset = 0;
	}

	return (int32_t)done;
}

int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize)
{
	int32_t ret = usbMscPrvWrite10(lun, lba, offset, buffer, bufsize);

	if (ret < 0 && usbMscPrvFallbackSpeed())
		ret = usbMscPrvWrite10(lun, lba, offset, buffer, bufsize);
	return ret;
}

int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void *buffer, uint16_t bufsize)
{
	(void)buffer;
	(void)bufsize;
	(void)scsi_cmd;
	usbMscPrvStatus("SCSI", 0, 0);
	(void)tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
	return TUD_MSC_RET_ERROR;
}
