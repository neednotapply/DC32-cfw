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
static uint32_t mUsbBegins, mUsbEnds, mUsbDrops, mReports, mMouseReports, mConsumerReports, mGamepadReports, mReportEnables, mReportDisables, mReleaseAlls, mSawStart, mSawDone, mSawError, mEofReads;
static bool mUsbStarted, mReportsEnabled;

static void resetHarness(void)
{
	mNow = 0;
	mUsbBegins = 0;
	mUsbEnds = 0;
	mUsbDrops = 0;
	mReports = 0;
	mMouseReports = 0;
	mConsumerReports = 0;
	mGamepadReports = 0;
	mReportEnables = 0;
	mReportDisables = 0;
	mReleaseAlls = 0;
	mSawStart = 0;
	mSawDone = 0;
	mSawError = 0;
	mEofReads = 0;
	mUsbStarted = false;
	mReportsEnabled = false;
}

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
	mUsbStarted = true;
	mReportsEnabled = false;
	return true;
}

bool usbHidBeginReportSet(const struct UsbHidDeviceInfo *info, enum UsbHidReportSet reportSet)
{
	(void)reportSet;
	return usbHidBegin(info);
}

void usbHidTask(void)
{
}

bool usbHidReady(void)
{
	return mUsbStarted;
}

bool usbHidStarted(void)
{
	return mUsbStarted;
}

void usbHidSetReportsEnabled(bool enabled)
{
	if (enabled && !mReportsEnabled)
		mReportEnables++;
	if (!enabled && mReportsEnabled)
		mReportDisables++;
	mReportsEnabled = enabled;
}

bool usbHidReportsEnabled(void)
{
	return mReportsEnabled;
}

bool usbHidKeyboardReport(uint8_t modifiers, const uint8_t keys[6])
{
	(void)modifiers;
	(void)keys;
	if (!mReportsEnabled)
		return false;
	mReports++;
	return true;
}

bool usbHidMouseReport(uint8_t buttons, int8_t x, int8_t y, int8_t wheel, int8_t pan)
{
	(void)buttons;
	(void)x;
	(void)y;
	(void)wheel;
	(void)pan;
	if (!mReportsEnabled)
		return false;
	mMouseReports++;
	return true;
}

bool usbHidConsumerReport(uint16_t usage)
{
	(void)usage;
	if (!mReportsEnabled)
		return false;
	mConsumerReports++;
	return true;
}

bool usbHidGamepadReport(int8_t x, int8_t y, int8_t z, int8_t rz, int8_t rx, int8_t ry, uint8_t hat, uint32_t buttons)
{
	(void)x;
	(void)y;
	(void)z;
	(void)rz;
	(void)rx;
	(void)ry;
	(void)hat;
	(void)buttons;
	if (!mReportsEnabled)
		return false;
	mGamepadReports++;
	return true;
}

void usbHidReleaseAll(void)
{
	mReleaseAlls++;
}

void usbHidEnd(void)
{
	mUsbEnds++;
	mUsbStarted = false;
	mReportsEnabled = false;
}

void usbHidDropNow(void)
{
	mUsbDrops++;
	mUsbStarted = false;
	mReportsEnabled = false;
}

bool fatfsFileRead(struct FatfsFil* fil, void *buf, uint32_t num, uint32_t *numReadP)
{
	uint32_t remain = fil->size - fil->pos;
	uint32_t n = remain < num ? remain : num;

	if (num && fil->pos >= fil->size)
		mEofReads++;
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
		mSawError++;
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
	struct BadUsbPreload preload;
	enum BadUsbResult ret;

	resetHarness();
	ret = badUsbPreloadFile(&fil, badusbStatus, NULL, &preload);
	if (ret != BadUsbResultDone) {
		fprintf(stderr, "FAIL: %s preload returned %d\n", name, ret);
		return false;
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
	usbHidSetReportsEnabled(false);
	usbHidEnd();
	if (ret != BadUsbResultDone) {
		fprintf(stderr, "FAIL: %s returned %d\n", name, ret);
		return false;
	}
	if (!mUsbBegins || !mSawStart || mSawDone) {
		fprintf(stderr, "FAIL: %s did not use EOF return state cleanly\n", name);
		return false;
	}
	if (mReportEnables != 1 || mReportDisables != 1 || mReleaseAlls || mUsbEnds != 1 || mReportsEnabled || mUsbStarted) {
		fprintf(stderr, "FAIL: %s did not clean up HID once (en=%u dis=%u release=%u ends=%u)\n", name, (unsigned)mReportEnables, (unsigned)mReportDisables, (unsigned)mReleaseAlls, (unsigned)mUsbEnds);
		return false;
	}
	printf("PASS: %s usbBegins=%u usbEnds=%u reports=%u\n", name, (unsigned)mUsbBegins, (unsigned)mUsbEnds, (unsigned)mReports);
	return true;
}

static bool runUsbFirstBuffer(const char *name, const char *data, uint32_t size)
{
	struct FatfsFil fil = {.data = data, .size = size, .pos = 0};
	struct UsbHidDeviceInfo info;
	struct BadUsbScratch scratch;
	enum BadUsbResult ret;

	resetHarness();
	if (!badUsbReadDeviceInfoWithScratch(&fil, &info, &scratch, sizeof(scratch))) {
		fprintf(stderr, "FAIL: %s failed to read HID info before USB-first run\n", name);
		return false;
	}
	if (info.vid != 0x1209 || info.pid != 0xdc32 || strcmp(info.manufacturer, "Unit Maker") || strcmp(info.product, "Unit Product")) {
		fprintf(stderr, "FAIL: %s did not parse ID line before USB-first run\n", name);
		return false;
	}
	if (mUsbBegins || mReports) {
		fprintf(stderr, "FAIL: %s touched USB during HID info scan\n", name);
		return false;
	}
	if (!usbHidBegin(&info)) {
		fprintf(stderr, "FAIL: %s USB begin failed\n", name);
		return false;
	}
	usbHidSetReportsEnabled(true);
	mSawStart++;
	ret = badUsbRunPreparedFileWithScratch(&fil, NULL, badusbStatus, badusbWaitButton, NULL, &scratch, sizeof(scratch));
	usbHidDropNow();
	if (ret != BadUsbResultDone || mSawError || mSawDone) {
		fprintf(stderr, "FAIL: %s did not finish USB-first EOF cleanly (ret=%d)\n", name, ret);
		return false;
	}
	if (!mUsbBegins || !mSawStart || mReportEnables != 1 || mReportDisables || mReleaseAlls || mUsbEnds || mUsbDrops != 1 || mReportsEnabled || mUsbStarted) {
		fprintf(stderr, "FAIL: %s did not hard-drop HID once after USB-first EOF (begins=%u en=%u dis=%u release=%u ends=%u drops=%u)\n",
			name, (unsigned)mUsbBegins, (unsigned)mReportEnables, (unsigned)mReportDisables, (unsigned)mReleaseAlls, (unsigned)mUsbEnds, (unsigned)mUsbDrops);
		return false;
	}
	if (mEofReads) {
		fprintf(stderr, "FAIL: %s performed %u extra EOF reads before hard-drop\n", name, (unsigned)mEofReads);
		return false;
	}
	printf("PASS: %s USB-first EOF hard-drop reports=%u\n", name, (unsigned)mReports);
	return true;
}

static bool runMouseMediaBuffer(const char *name, const char *data, uint32_t size, uint32_t expectedMouseReports, uint32_t expectedConsumerReports)
{
	struct FatfsFil fil = {.data = data, .size = size, .pos = 0};
	struct UsbHidDeviceInfo info;
	struct BadUsbScratch scratch;
	enum BadUsbResult ret;

	resetHarness();
	if (!badUsbReadDeviceInfoWithScratch(&fil, &info, &scratch, sizeof(scratch))) {
		fprintf(stderr, "FAIL: %s failed to read HID info\n", name);
		return false;
	}
	if (!usbHidBeginReportSet(&info, UsbHidReportSetKeyboardMouseConsumer)) {
		fprintf(stderr, "FAIL: %s USB begin failed\n", name);
		return false;
	}
	usbHidSetReportsEnabled(true);
	mSawStart++;
	ret = badUsbRunPreparedFileWithScratch(&fil, NULL, badusbStatus, badusbWaitButton, NULL, &scratch, sizeof(scratch));
	usbHidDropNow();
	if (ret != BadUsbResultDone || mSawError) {
		fprintf(stderr, "FAIL: %s did not finish cleanly (ret=%d)\n", name, ret);
		return false;
	}
	if (mMouseReports != expectedMouseReports || mConsumerReports != expectedConsumerReports) {
		fprintf(stderr, "FAIL: %s expected mouse=%u consumer=%u got mouse=%u consumer=%u\n",
			name, (unsigned)expectedMouseReports, (unsigned)expectedConsumerReports,
			(unsigned)mMouseReports, (unsigned)mConsumerReports);
		return false;
	}
	printf("PASS: %s mouse=%u consumer=%u\n", name, (unsigned)mMouseReports, (unsigned)mConsumerReports);
	return true;
}

static bool runDeviceDefaultBuffer(const char *name, const char *data, uint32_t size, bool hasId)
{
	struct FatfsFil fil = {.data = data, .size = size, .pos = 0};
	struct UsbHidDeviceInfo defaults, info;
	struct BadUsbScratch scratch;

	memset(&defaults, 0, sizeof(defaults));
	defaults.vid = 0x1111;
	defaults.pid = 0x2222;
	strcpy(defaults.manufacturer, "Default Maker");
	strcpy(defaults.product, "Default Product");
	resetHarness();
	if (!badUsbReadDeviceInfoWithDefaultScratch(&fil, &defaults, &info, &scratch, sizeof(scratch))) {
		fprintf(stderr, "FAIL: %s failed to read HID info with defaults\n", name);
		return false;
	}
	if (hasId) {
		if (info.vid != 0x1209 || info.pid != 0xdc32 || strcmp(info.manufacturer, "Unit Maker") || strcmp(info.product, "Unit Product")) {
			fprintf(stderr, "FAIL: %s did not let ID override defaults\n", name);
			return false;
		}
	}
	else if (info.vid != defaults.vid || info.pid != defaults.pid || strcmp(info.manufacturer, defaults.manufacturer) || strcmp(info.product, defaults.product)) {
		fprintf(stderr, "FAIL: %s did not keep configured defaults\n", name);
		return false;
	}
	if (mUsbBegins || mReports || mMouseReports || mConsumerReports) {
		fprintf(stderr, "FAIL: %s touched USB during HID info scan\n", name);
		return false;
	}
	printf("PASS: %s default ID precedence\n", name);
	return true;
}

static bool failDuringUsbFirstRunBuffer(const char *name, const char *data, uint32_t size, enum BadUsbResult expected)
{
	struct FatfsFil fil = {.data = data, .size = size, .pos = 0};
	struct UsbHidDeviceInfo info;
	struct BadUsbScratch scratch;
	enum BadUsbResult ret;

	resetHarness();
	if (!badUsbReadDeviceInfoWithScratch(&fil, &info, &scratch, sizeof(scratch))) {
		fprintf(stderr, "FAIL: %s failed to read HID info before USB-first run\n", name);
		return false;
	}
	if (mUsbBegins || mReports) {
		fprintf(stderr, "FAIL: %s touched USB during HID info scan\n", name);
		return false;
	}
	if (!usbHidBegin(&info)) {
		fprintf(stderr, "FAIL: %s USB begin failed\n", name);
		return false;
	}
	usbHidSetReportsEnabled(true);
	mSawStart++;
	ret = badUsbRunPreparedFileWithScratch(&fil, NULL, badusbStatus, badusbWaitButton, NULL, &scratch, sizeof(scratch));
	usbHidDropNow();
	if (ret != expected || !mSawError || mSawDone) {
		fprintf(stderr, "FAIL: %s did not fail during USB-first run cleanly (ret=%d)\n", name, ret);
		return false;
	}
	if (!mUsbBegins || mReportDisables || mReleaseAlls || mUsbEnds || mUsbDrops != 1 || mReportsEnabled || mUsbStarted) {
		fprintf(stderr, "FAIL: %s did not hard-drop HID once after error (begins=%u disables=%u release=%u ends=%u drops=%u)\n", name, (unsigned)mUsbBegins, (unsigned)mReportDisables, (unsigned)mReleaseAlls, (unsigned)mUsbEnds, (unsigned)mUsbDrops);
		return false;
	}
	printf("PASS: %s failed during USB-first run with HID hard-drop\n", name);
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
	static char tooLongLineScript[BADUSB_LINE_BUF_SZ + 32];
	static const char tooLongPrefix[] = "REM ok\n";
	static const char malformedCommandScript[] =
		"GUI r\n"
		"NOT_A_REAL_KEY\n";
	static const char malformedMouseScript[] =
		"MOUSEMOVE 10\n";
	static const char malformedMediaScript[] =
		"MEDIA NO_SUCH_KEY\n";
	static const char mouseMediaScript[] =
		"MOUSEMOVE 300 -260\n"
		"MOUSESCROLL -300\n"
		"LEFTCLICK\n"
		"HOLD LEFTCLICK\n"
		"RELEASE LEFT_CLICK\n"
		"RIGHT_CLICK\n"
		"MEDIA PLAY_PAUSE\n";
	static const char defaultIdScript[] =
		"REM no ID here\n"
		"STRING hi\n";
	static const char usbFirstValidScript[] =
		"ID 1209:DC32 Unit Maker:Unit Product\n"
		"REM valid USB-first EOF path\n"
		"STRING hi\n"
		"ENTER\n";
	static const char usbFirstValidNoFinalNewlineScript[] =
		"ID 1209:DC32 Unit Maker:Unit Product\n"
		"REM valid USB-first EOF path without final newline\n"
		"STRING hi\n"
		"ENTER";
	static char firstLineTooLongScript[BADUSB_LINE_BUF_SZ + 16];
	bool ok = true;

	memcpy(tooLongLineScript, tooLongPrefix, sizeof(tooLongPrefix) - 1);
	memset(tooLongLineScript + sizeof(tooLongPrefix) - 1, 'A', sizeof(tooLongLineScript) - sizeof(tooLongPrefix) - 1);
	tooLongLineScript[sizeof(tooLongLineScript) - 2] = '\n';
	tooLongLineScript[sizeof(tooLongLineScript) - 1] = 0;
	memset(firstLineTooLongScript, 'B', sizeof(firstLineTooLongScript) - 1);
	firstLineTooLongScript[sizeof(firstLineTooLongScript) - 2] = '\n';
	firstLineTooLongScript[sizeof(firstLineTooLongScript) - 1] = 0;
	ok = runScriptBuffer("embedded long-line script", longLineScript, sizeof(longLineScript) - 1) && ok;
	ok = runMouseMediaBuffer("mouse/media script", mouseMediaScript, sizeof(mouseMediaScript) - 1, 12, 2) && ok;
	ok = runDeviceDefaultBuffer("configured default HID info", defaultIdScript, sizeof(defaultIdScript) - 1, false) && ok;
	ok = runDeviceDefaultBuffer("script ID override HID info", usbFirstValidScript, sizeof(usbFirstValidScript) - 1, true) && ok;
	ok = runUsbFirstBuffer("valid USB-first script", usbFirstValidScript, sizeof(usbFirstValidScript) - 1) && ok;
	ok = runUsbFirstBuffer("valid USB-first script without final newline", usbFirstValidNoFinalNewlineScript, sizeof(usbFirstValidNoFinalNewlineScript) - 1) && ok;
	ok = failDuringUsbFirstRunBuffer("overlong line script", tooLongLineScript, sizeof(tooLongLineScript) - 1, BadUsbResultDecodeError) && ok;
	ok = failDuringUsbFirstRunBuffer("overlong first line script", firstLineTooLongScript, sizeof(firstLineTooLongScript) - 1, BadUsbResultDecodeError) && ok;
	ok = failDuringUsbFirstRunBuffer("malformed command script", malformedCommandScript, sizeof(malformedCommandScript) - 1, BadUsbResultDecodeError) && ok;
	ok = failDuringUsbFirstRunBuffer("malformed mouse script", malformedMouseScript, sizeof(malformedMouseScript) - 1, BadUsbResultDecodeError) && ok;
	ok = failDuringUsbFirstRunBuffer("malformed media script", malformedMediaScript, sizeof(malformedMediaScript) - 1, BadUsbResultDecodeError) && ok;
	ok = runScriptFile("C:/Users/mrnic/Desktop/badusb/Hacker_Typer.txt") && ok;
	ok = runScriptFile("C:/Users/mrnic/Desktop/badusb/RickRoll_YT_Win.txt") && ok;
	ok = runScriptFile("C:/Users/mrnic/Desktop/badusb/GoodUSB.txt") && ok;
	ok = runScriptFile("C:/Users/mrnic/Desktop/badusb/Char_Test.txt") && ok;
	return ok ? 0 : 1;
}
