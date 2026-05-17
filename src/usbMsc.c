#include <string.h>
#include "tusb.h"
#include "sd.h"
#include "usbDevice.h"
#include "usbMsc.h"

#define USB_MSC_VID		0x1209
#define USB_MSC_PID		0xdc33

static bool mStarted, mWritable, mEjected;
static uint32_t mBlockCount;
static const char *mLastError = "none";
static uint8_t mScratch[SD_BLOCK_SIZE];

static bool usbMscPrvRangeOk(uint32_t lba)
{
	return mStarted && !mEjected && lba < mBlockCount;
}

static int32_t usbMscPrvIoError(const char *err)
{
	mLastError = err;
	return TUD_MSC_RET_ERROR;
}

bool usbMscBegin(bool writable)
{
	struct UsbDeviceInfo info;
	union SdFlags flags;

	mLastError = "none";
	mBlockCount = sdGetNumSecs();
	if (!mBlockCount) {
		mLastError = "SD card is not initialized";
		return false;
	}

	flags.value = sdGetFlags();
	mWritable = writable && !flags.RO;
	mEjected = false;

	memset(&info, 0, sizeof(info));
	info.vid = USB_MSC_VID;
	info.pid = USB_MSC_PID;
	strcpy(info.manufacturer, "DC32");
	strcpy(info.product, "DC32 SD Card");

	if (!usbDeviceBegin(UsbDeviceModeMsc, &info)) {
		mLastError = usbDeviceLastError();
		return false;
	}
	mStarted = true;
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

void usbMscEnd(void)
{
	if (!mStarted)
		return;
	usbDeviceEnd();
	mStarted = false;
	mWritable = false;
	mEjected = false;
	mBlockCount = 0;
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
	if (lun || !mStarted || !mBlockCount || mEjected) {
		(void)tud_msc_set_sense(lun, SCSI_SENSE_NOT_READY, 0x3a, 0x00);
		return false;
	}
	return true;
}

void tud_msc_capacity_cb(uint8_t lun, uint32_t *block_count, uint16_t *block_size)
{
	(void)lun;
	*block_count = mBlockCount;
	*block_size = SD_BLOCK_SIZE;
}

bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject)
{
	(void)lun;
	(void)power_condition;
	if (load_eject && !start)
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
	return mWritable;
}

int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize)
{
	uint8_t *dst = (uint8_t*)buffer;
	uint32_t done = 0;

	if (lun)
		return usbMscPrvIoError("invalid LUN");

	while (done < bufsize) {
		uint32_t chunk;

		if (!usbMscPrvRangeOk(lba) || offset >= SD_BLOCK_SIZE)
			return usbMscPrvIoError("read outside SD card");

		chunk = SD_BLOCK_SIZE - offset;
		if (chunk > bufsize - done)
			chunk = bufsize - done;

		if (!offset && chunk == SD_BLOCK_SIZE) {
			if (!sdSecRead(lba, dst + done))
				return usbMscPrvIoError("SD read failed");
		}
		else {
			if (!sdSecRead(lba, mScratch))
				return usbMscPrvIoError("SD read failed");
			memcpy(dst + done, mScratch + offset, chunk);
		}

		done += chunk;
		lba++;
		offset = 0;
	}

	return (int32_t)done;
}

int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize)
{
	uint32_t done = 0;

	if (lun)
		return usbMscPrvIoError("invalid LUN");
	if (!mWritable)
		return usbMscPrvIoError("SD card is read-only");

	while (done < bufsize) {
		uint32_t chunk;

		if (!usbMscPrvRangeOk(lba) || offset >= SD_BLOCK_SIZE)
			return usbMscPrvIoError("write outside SD card");

		chunk = SD_BLOCK_SIZE - offset;
		if (chunk > bufsize - done)
			chunk = bufsize - done;

		if (!offset && chunk == SD_BLOCK_SIZE) {
			if (!sdSecWrite(lba, buffer + done))
				return usbMscPrvIoError("SD write failed");
		}
		else {
			if (!sdSecRead(lba, mScratch))
				return usbMscPrvIoError("SD read failed");
			memcpy(mScratch + offset, buffer + done, chunk);
			if (!sdSecWrite(lba, mScratch))
				return usbMscPrvIoError("SD write failed");
		}

		done += chunk;
		lba++;
		offset = 0;
	}

	return (int32_t)done;
}

int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void *buffer, uint16_t bufsize)
{
	(void)buffer;
	(void)bufsize;
	(void)scsi_cmd;
	(void)tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
	return TUD_MSC_RET_ERROR;
}
