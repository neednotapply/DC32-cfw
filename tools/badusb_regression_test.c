#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef TICKS_PER_SECOND
#define TICKS_PER_SECOND 1000U
#endif
#ifndef FATFS_USE_LFN_SUPPORT
#define FATFS_USE_LFN_SUPPORT 1
#endif

#include "../src/badUsb.c"

struct FatfsFil {
	const char *data;
	uint32_t size;
	uint32_t pos;
};

static uint64_t mNow;
static uint32_t mUsbBegins, mReports, mSawStart, mSawDone;

uint64_t getTime(void)
{
	return ++mNow * 25;
}

void usbHidDefaultInfo(struct UsbHidDeviceInfo *info)
{
	memset(info, 0, sizeof(*info));
	info->vid = USB_HID_DEFAULT_VID;
	info->pid = USB_HID_DEFAULT_PID;
	strcpy(info->manufacturer, "test");
	strcpy(info->product, "test HID");
}

bool usbHidPrepare(void)
{
	return true;
}

const char *usbHidLastError(void)
{
	return "test";
}

bool usbHidBegin(const struct UsbHidDeviceInfo *info)
{
	(void)info;
	mUsbBegins++;
	return true;
}

void usbHidTask(void)
{
}

bool usbHidReady(void)
{
	return mUsbBegins != 0;
}

bool usbHidStarted(void)
{
	return mUsbBegins != 0;
}

void usbHidSetReportsEnabled(bool enabled)
{
	(void)enabled;
}

bool usbHidReportsEnabled(void)
{
	return true;
}

bool usbHidKeyboardReport(uint8_t modifiers, const uint8_t keys[6])
{
	(void)modifiers;
	(void)keys;
	mReports++;
	return true;
}

void usbHidReleaseAll(void)
{
}

void usbHidEnd(void)
{
}

bool fatfsFileRead(struct FatfsFil* fil, void *buf, uint32_t num, uint32_t *numReadP)
{
	uint32_t remain = fil->size - fil->pos;
	uint32_t n = remain < num ? remain : num;

	if (buf && n)
		memcpy(buf, fil->data + fil->pos, n);
	fil->pos += n;
	*numReadP = n;
	return true;
}

bool fatfsFileSeek(struct FatfsFil* fil, uint32_t pos)
{
	if (pos > fil->size)
		return false;
	fil->pos = pos;
	return true;
}

uint32_t fatfsFileGetSize(struct FatfsFil* fil)
{
	return fil->size;
}

static bool badusbStatus(void *userData, const struct BadUsbStatus *status)
{
	(void)userData;
	if (status->state == BadUsbStateWillRun)
		mSawStart++;
	if (status->state == BadUsbStateDone)
		mSawDone++;
	if (status->state == BadUsbStateScriptError) {
		fprintf(stderr, "script error line %u: %s\n", (unsigned)status->errorLine, status->error);
		return false;
	}
	return true;
}

static bool badusbWaitButton(void *userData, const struct BadUsbStatus *status)
{
	(void)userData;
	(void)status;
	return true;
}

static char *readWholeFile(const char *path, uint32_t *sizeP)
{
	FILE *f = fopen(path, "rb");
	char *buf;
	long size;

	if (!f)
		return NULL;
	if (fseek(f, 0, SEEK_END) != 0) {
		fclose(f);
		return NULL;
	}
	size = ftell(f);
	if (size < 0) {
		fclose(f);
		return NULL;
	}
	rewind(f);
	buf = malloc((size_t)size + 1);
	if (!buf) {
		fclose(f);
		return NULL;
	}
	if (fread(buf, 1, (size_t)size, f) != (size_t)size) {
		free(buf);
		fclose(f);
		return NULL;
	}
	fclose(f);
	buf[size] = 0;
	*sizeP = (uint32_t)size;
	return buf;
}

static bool runScriptBuffer(const char *name, const char *data, uint32_t size)
{
	struct FatfsFil fil = {.data = data, .size = size, .pos = 0};
	enum BadUsbResult ret;

	mNow = 0;
	mUsbBegins = 0;
	mReports = 0;
	mSawStart = 0;
	mSawDone = 0;
	ret = badUsbRunFile(&fil, badusbStatus, badusbWaitButton, NULL);
	if (ret != BadUsbResultDone) {
		fprintf(stderr, "FAIL: %s returned %d\n", name, ret);
		return false;
	}
	if (!mUsbBegins || !mSawStart || !mSawDone) {
		fprintf(stderr, "FAIL: %s did not reach USB start/done states\n", name);
		return false;
	}
	printf("PASS: %s usbBegins=%u reports=%u\n", name, (unsigned)mUsbBegins, (unsigned)mReports);
	return true;
}

static bool runScriptFile(const char *path)
{
	uint32_t size;
	char *data = readWholeFile(path, &size);
	bool ok;

	if (!data) {
		printf("SKIP: %s\n", path);
		return true;
	}
	ok = runScriptBuffer(path, data, size);
	free(data);
	return ok;
}

int main(void)
{
	static const char longLineScript[] =
		"GUI r\n"
		"DELAY 50\n"
		"STRING "
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
		"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"
		"MEDIA PLAY\n";
	bool ok = true;

	ok = runScriptBuffer("embedded long-line script", longLineScript, sizeof(longLineScript) - 1) && ok;
	ok = runScriptFile("C:/Users/mrnic/Desktop/badusb/Hacker_Typer.txt") && ok;
	ok = runScriptFile("C:/Users/mrnic/Desktop/badusb/RickRoll_YT_Win.txt") && ok;
	ok = runScriptFile("C:/Users/mrnic/Desktop/badusb/GoodUSB.txt") && ok;
	ok = runScriptFile("C:/Users/mrnic/Desktop/badusb/Char_Test.txt") && ok;
	return ok ? 0 : 1;
}
