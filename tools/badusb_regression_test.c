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
static uint32_t mUsbBegins, mReports, mSawStart, mSawDone, mSawScriptError, mWaitButtonCalls;

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
		mSawScriptError++;
		fprintf(stderr, "script error line %u: %s\n", (unsigned)status->errorLine, status->error);
		return false;
	}
	return true;
}

static bool badusbWaitButton(void *userData, const struct BadUsbStatus *status)
{
	(void)userData;
	(void)status;
	mWaitButtonCalls++;
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

static bool runScriptBufferExpectEx(const char *name, const char *data, uint32_t size, enum BadUsbResult expected, bool expectWait)
{
	struct FatfsFil fil = {.data = data, .size = size, .pos = 0};
	struct BadUsbPreload preload;
	enum BadUsbResult ret;

	mNow = 0;
	mUsbBegins = 0;
	mReports = 0;
	mSawStart = 0;
	mSawDone = 0;
	mSawScriptError = 0;
	mWaitButtonCalls = 0;
	ret = badUsbPreloadFile(&fil, badusbStatus, NULL, &preload);
	if (ret != BadUsbResultDone) {
		if (ret != expected) {
			fprintf(stderr, "FAIL: %s preload returned %d, expected %d\n", name, ret, expected);
			return false;
		}
		if (mUsbBegins) {
			fprintf(stderr, "FAIL: %s preload touched USB\n", name);
			return false;
		}
		printf("PASS: %s preload returned expected %d\n", name, ret);
		return true;
	}
	if (mUsbBegins) {
		fprintf(stderr, "FAIL: %s preload touched USB\n", name);
		return false;
	}
	if (!usbHidBegin(&preload.info)) {
		fprintf(stderr, "FAIL: %s USB begin failed\n", name);
		return false;
	}
	usbHidSetReportsEnabled(true);
	mSawStart++;
	ret = badUsbRunPreparedFile(&fil, &preload, badusbStatus, badusbWaitButton, NULL);
	usbHidReleaseAll();
	usbHidSetReportsEnabled(false);
	usbHidEnd();
	if (ret != expected) {
		fprintf(stderr, "FAIL: %s returned %d, expected %d\n", name, ret, expected);
		return false;
	}
	if (!mUsbBegins || !mSawStart) {
		fprintf(stderr, "FAIL: %s did not reach USB start/done states\n", name);
		return false;
	}
	if (expected == BadUsbResultDone && !mSawDone) {
		fprintf(stderr, "FAIL: %s did not reach done state\n", name);
		return false;
	}
	if (expected == BadUsbResultDecodeError && !mSawScriptError) {
		fprintf(stderr, "FAIL: %s did not report script error state\n", name);
		return false;
	}
	if (expectWait && !mWaitButtonCalls) {
		fprintf(stderr, "FAIL: %s did not call wait-for-button callback\n", name);
		return false;
	}
	printf("PASS: %s result=%d usbBegins=%u reports=%u\n", name, ret, (unsigned)mUsbBegins, (unsigned)mReports);
	return true;
}

static bool runScriptBufferExpect(const char *name, const char *data, uint32_t size, enum BadUsbResult expected)
{
	return runScriptBufferExpectEx(name, data, size, expected, false);
}

static bool runScriptBuffer(const char *name, const char *data, uint32_t size)
{
	return runScriptBufferExpect(name, data, size, BadUsbResultDone);
}

static bool runLongFinalLineTest(void)
{
	static const char prefix[] = "STRING ";
	uint32_t size = BADUSB_LINE_BUF_SZ + sizeof(prefix) + 8;
	char *script = malloc(size);
	bool ok;

	if (!script) {
		fprintf(stderr, "FAIL: allocate long final line script\n");
		return false;
	}
	memcpy(script, prefix, sizeof(prefix) - 1);
	memset(script + sizeof(prefix) - 1, 'a', size - sizeof(prefix));
	script[size - 1] = 0;
	ok = runScriptBufferExpect("final line too long", script, size - 1, BadUsbResultDecodeError);
	free(script);
	return ok;
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

	ok = runScriptBufferExpect("empty script", "", 0, BadUsbResultDone) && ok;
	ok = runScriptBufferExpect("newline EOF", "REM ok\n", 7, BadUsbResultDone) && ok;
	ok = runScriptBufferExpect("no trailing newline EOF", "STRING hello", 12, BadUsbResultDone) && ok;
	ok = runScriptBufferExpect("malformed final line EOF", "DELAY nope", 10, BadUsbResultDecodeError) && ok;
	ok = runScriptBufferExpectEx("wait for button", "WAIT_FOR_BUTTON_PRESS\nSTRING ok\n", 32, BadUsbResultDone, true) && ok;
	ok = runLongFinalLineTest() && ok;
	ok = runScriptBuffer("embedded long-line script", longLineScript, sizeof(longLineScript) - 1) && ok;
	ok = runScriptFile("C:/Users/mrnic/Desktop/badusb/Hacker_Typer.txt") && ok;
	ok = runScriptFile("C:/Users/mrnic/Desktop/badusb/RickRoll_YT_Win.txt") && ok;
	ok = runScriptFile("C:/Users/mrnic/Desktop/badusb/GoodUSB.txt") && ok;
	ok = runScriptFile("C:/Users/mrnic/Desktop/badusb/Char_Test.txt") && ok;
	return ok ? 0 : 1;
}
