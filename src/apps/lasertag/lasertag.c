#include "apps/lasertag/lasertag.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "audioPwm.h"
#include "badgeLeds.h"
#include "dcAppDraw.h"
#include "fatfs.h"
#include "fonts.h"
#include "gb.h"
#include "irRemote.h"
#include "qrcodegen.h"
#include "qspi.h"
#include "settings.h"
#include "timebase.h"

#define LT_SCREEN_W 320u
#define LT_SCREEN_H 240u
#define LT_SAVE_MAGIC 0x47544c44u
#define LT_SAVE_VERSION 4u
#define LT_SAVE_PATH "/SAVE/PORTS/LASERTAG.DAT"
#define LT_SYNC_PATH "/SAVE/PORTS/LASERTAG.SYNC"
#define LT_SYNC_URL_PATH "/SAVE/PORTS/LASERTAG.URL"
#define LT_OPPONENTS 32u
#define LT_FIRE_WINDOW 30u
#define LT_FIRE_MIN_TICKS TICKS_PER_SECOND
#define LT_FIRE_WINDOW_TICKS (60ull * TICKS_PER_SECOND)
#define LT_HIT_LOCKOUT_TICKS (2ull * TICKS_PER_SECOND)
#define LT_DUPLICATE_TICKS (TICKS_PER_SECOND / 4u)
#define LT_SYNC_REMINDER_TICKS (2ull * 60ull * 60ull * TICKS_PER_SECOND)
#define LT_FLAG_DEVICE_AUTO 0x01u
#define LT_FLAG_SOUND 0x02u
#define LT_FLAG_VIBRATION 0x04u
#define LT_FLAG_SYNC_REMINDER 0x08u
#define LT_FLAG_AT_HOME 0x10u
#define LT_FLAG_LED_SHIFT 5u
#define LT_FLAG_LED_MASK 0x60u
#define LT_DEFAULT_FLAGS (LT_FLAG_SOUND | LT_FLAG_SYNC_REMINDER)
#ifndef LT_PROVISIONED_BLOCK_ID
#define LT_PROVISIONED_BLOCK_ID 0u
#endif
#ifndef LT_PROVISIONED_DEVICE_ID
#define LT_PROVISIONED_DEVICE_ID 256u
#endif
#if LT_PROVISIONED_BLOCK_ID > 255u || LT_PROVISIONED_DEVICE_ID > 256u
#error Invalid Laser Tag provisioned identity
#endif
#define LT_SYNC_SCHEMA 2u
#define LT_SYNC_UNSIGNED 0x80u
#define LT_SYNC_HEADER_SIZE 28u
#define LT_SYNC_ENTRY_SIZE 8u
#define LT_SYNC_PAGE_ENTRIES 13u
#define LT_SYNC_BINARY_MAX (LT_SYNC_HEADER_SIZE + LT_OPPONENTS * LT_SYNC_ENTRY_SIZE + 4u)
#define LT_SYNC_TEXT_MAX 512u
#define LT_SYNC_URL_PREFIX_MAX 96u
#define LT_SYNC_PREFIX "DC32LT1."
#define LT_QR_MAX_VERSION 15
#define LT_QR_BUFFER_SIZE qrcodegen_BUFFER_LEN_FOR_VERSION(LT_QR_MAX_VERSION)

struct LtOpponent {
	uint8_t blockId;
	uint8_t deviceId;
	uint8_t color;
	uint8_t valid;
	uint32_t hits;
};

struct LtSave {
	uint32_t magic;
	uint16_t version;
	uint16_t size;
	uint8_t color;
	uint8_t blockId;
	uint8_t deviceId;
	uint8_t flags;
	uint32_t shots;
	uint32_t hits;
	struct LtOpponent opponents[LT_OPPONENTS];
	uint32_t crc;
};

typedef char LtSaveSizeCheck[(sizeof(struct LtSave) == 280u) ? 1 : -1];

enum LtView {
	LtViewPlay,
	LtViewSettings,
	LtViewStats,
	LtViewSync,
	LtViewTutorial,
};

struct LtApp {
	const struct DcAppHostApi *host;
	const struct DcAppRunArgs *args;
	struct DcAppDrawCtx draw;
	struct LtSave save;
	struct Settings baseSettings;
	uint64_t fireTimes[LT_FIRE_WINDOW];
	uint8_t fireCount;
	uint8_t settingsRow;
	uint8_t statsOffset;
	uint8_t effectiveDeviceId;
	enum LtView view;
	uint64_t cooldownUntil;
	uint64_t messageUntil;
	uint64_t toneUntil;
	uint64_t lastSyncTime;
	uint64_t lastHitTime;
	uint32_t lastHitRaw;
	uint8_t lastBlock;
	uint8_t lastDevice;
	uint8_t lastColor;
	bool haveLastHit;
	bool receiverActive;
	bool ledOverride;
	bool saveError;
	bool syncQrReady;
	bool syncExported;
	bool syncUrlConfigured;
	uint8_t syncQrSize;
	uint8_t syncPage;
	uint8_t syncPages;
	char message[48];
};

static uint8_t mLtFrame[LT_SCREEN_W * LT_SCREEN_H];
static char mLtSyncText[LT_SYNC_TEXT_MAX];
static uint8_t mLtQrTemp[LT_QR_BUFFER_SIZE];
static uint8_t mLtQrCode[LT_QR_BUFFER_SIZE];
static volatile bool mLtAbort;

static const char *const mLtColorNames[] = {
	"Cyan", "Magenta", "Yellow", "Green", "Red", "Blue", "Orange", "White"
};

static const uint8_t mLtColors[][3] = {
	{0, 255, 255}, {255, 0, 255}, {255, 255, 0}, {0, 255, 0},
	{255, 0, 0}, {0, 0, 255}, {255, 165, 0}, {255, 255, 255}
};

static uint16_t ltRgb(uint32_t r, uint32_t g, uint32_t b)
{
	return dcAppDrawRgb565(r, g, b);
}

static uint32_t ltCrc32(const void *data, uint32_t size)
{
	const uint8_t *bytes = data;
	uint32_t crc = 0xffffffffu;

	while (size--) {
		crc ^= *bytes++;
		for (uint_fast8_t bit = 0; bit < 8; bit++)
			crc = (crc >> 1) ^ (0xedb88320u & (uint32_t)-(int32_t)(crc & 1u));
	}
	return ~crc;
}

static uint32_t ltBoundSaveCrc(const struct LtSave *save)
{
	uint64_t uid = flashGetUid();

	return ltCrc32(save, sizeof(*save)) ^ ltCrc32(&uid, sizeof(uid));
}

static void ltWrite32(uint8_t *dst, uint32_t value)
{
	dst[0] = (uint8_t)value;
	dst[1] = (uint8_t)(value >> 8);
	dst[2] = (uint8_t)(value >> 16);
	dst[3] = (uint8_t)(value >> 24);
}

static bool ltBase64Url(char *dst, uint32_t dstSize, const uint8_t *src, uint32_t srcSize)
{
	static const char alphabet[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
	uint32_t in = 0, out = 0;

	while (in < srcSize) {
		uint32_t remain = srcSize - in;
		uint32_t value = (uint32_t)src[in] << 16;

		if (remain > 1)
			value |= (uint32_t)src[in + 1] << 8;
		if (remain > 2)
			value |= src[in + 2];
		if (out + (remain > 2 ? 4u : remain + 1u) >= dstSize)
			return false;
		dst[out++] = alphabet[(value >> 18) & 63u];
		dst[out++] = alphabet[(value >> 12) & 63u];
		if (remain > 1)
			dst[out++] = alphabet[(value >> 6) & 63u];
		if (remain > 2)
			dst[out++] = alphabet[value & 63u];
		in += remain > 2 ? 3u : remain;
	}
	dst[out] = 0;
	return true;
}

static uint8_t ltSyncPageCount(const struct LtApp *app)
{
	uint8_t count = 0;

	for (uint_fast8_t i = 0; i < LT_OPPONENTS; i++)
		if (app->save.opponents[i].valid)
			count++;
	return count ? (uint8_t)((count + LT_SYNC_PAGE_ENTRIES - 1u) / LT_SYNC_PAGE_ENTRIES) : 1u;
}

static uint32_t ltBuildSyncBinary(struct LtApp *app, uint8_t page)
{
	uint8_t *binary = mLtQrTemp;
	uint32_t pos = LT_SYNC_HEADER_SIZE;
	uint64_t uid = flashGetUid();
	uint8_t count = 0, seen = 0;
	uint8_t pages = ltSyncPageCount(app);
	uint8_t first = (uint8_t)(page * LT_SYNC_PAGE_ENTRIES);

	if (page >= pages)
		page = 0;

	memset(binary, 0, LT_SYNC_BINARY_MAX);
	memcpy(binary, "DCLT", 4);
	binary[4] = LT_SYNC_SCHEMA;
	binary[5] = app->save.flags | LT_SYNC_UNSIGNED;
	binary[6] = app->save.blockId;
	binary[7] = app->effectiveDeviceId;
	binary[8] = app->save.color;
	binary[10] = page;
	binary[11] = pages;
	ltWrite32(binary + 12, ltCrc32(&uid, sizeof(uid)));
	ltWrite32(binary + 16, app->save.shots);
	ltWrite32(binary + 20, app->save.hits);
	ltWrite32(binary + 24, (uint32_t)(app->host->getTime() / TICKS_PER_SECOND));

	for (uint_fast8_t i = 0; i < LT_OPPONENTS; i++) {
		const struct LtOpponent *opponent = &app->save.opponents[i];

		if (!opponent->valid)
			continue;
		if (seen++ < first)
			continue;
		if (count >= LT_SYNC_PAGE_ENTRIES)
			break;
		binary[pos++] = opponent->blockId;
		binary[pos++] = opponent->deviceId;
		binary[pos++] = opponent->color;
		binary[pos++] = 0;
		ltWrite32(binary + pos, opponent->hits);
		pos += 4;
		count++;
	}
	binary[9] = count;
	ltWrite32(binary + pos, ltCrc32(binary, pos));
	return pos + 4u;
}

static bool ltBuildSyncText(struct LtApp *app, uint8_t page)
{
	uint32_t binarySize = ltBuildSyncBinary(app, page);
	uint32_t prefixSize = sizeof(LT_SYNC_PREFIX) - 1u;

	memcpy(mLtSyncText, LT_SYNC_PREFIX, prefixSize);
	return ltBase64Url(mLtSyncText + prefixSize, sizeof(mLtSyncText) - prefixSize,
		mLtQrTemp, binarySize);
}

static uint8_t ltAutoDeviceId(void)
{
	uint64_t uid = flashGetUid();
	uint8_t value = 0x32;

	for (uint_fast8_t i = 0; i < 8; i++) {
		value ^= (uint8_t)uid;
		value = (uint8_t)((value << 3) | (value >> 5));
		uid >>= 8;
	}
	/* IDs ending in 0x1f collide with standard NEC for one mode-0 color. */
	if ((value & 0x1fu) == 0x1fu)
		value ^= 1u;
	return value;
}

static bool ltLooksLikeNec(uint8_t deviceId, uint8_t color)
{
	return (uint8_t)(deviceId ^ (uint8_t)((color & 7u) << 5)) == 0xffu;
}

static uint8_t ltProvisionedColor(uint8_t deviceId)
{
	uint64_t uid = flashGetUid();
	uint8_t color = (uint8_t)(uid ^ (uid >> 8) ^ (uid >> 24) ^ (uid >> 40)) & 7u;

	while (ltLooksLikeNec(deviceId, color))
		color = (uint8_t)((color + 1u) & 7u);
	return color;
}

static bool ltApplyProvisioning(struct LtApp *app)
{
	uint8_t deviceId = LT_PROVISIONED_DEVICE_ID < 256u ?
		(uint8_t)LT_PROVISIONED_DEVICE_ID : ltAutoDeviceId();
	uint8_t identityFlag = LT_PROVISIONED_DEVICE_ID < 256u ? 0u : LT_FLAG_DEVICE_AUTO;
	uint8_t flags = app->save.flags;
	uint8_t color = app->save.color;

	if (app->save.version < LT_SAVE_VERSION) {
		flags = LT_DEFAULT_FLAGS;
		color = ltProvisionedColor(deviceId);
	}
	flags = (uint8_t)((flags & ~LT_FLAG_DEVICE_AUTO) | identityFlag);
	if (color >= 8u || ltLooksLikeNec(deviceId, color))
		color = ltProvisionedColor(deviceId);
	bool changed = app->save.blockId != (uint8_t)LT_PROVISIONED_BLOCK_ID ||
		app->save.deviceId != deviceId || app->save.color != color ||
		app->save.flags != flags || app->save.version != LT_SAVE_VERSION;

	app->save.blockId = (uint8_t)LT_PROVISIONED_BLOCK_ID;
	app->save.deviceId = deviceId;
	app->save.color = color;
	app->save.flags = flags;
	app->save.version = LT_SAVE_VERSION;
	app->effectiveDeviceId = deviceId;
	return changed;
}

static void ltDefaults(struct LtApp *app)
{
	memset(&app->save, 0, sizeof(app->save));
	app->save.magic = LT_SAVE_MAGIC;
	app->save.version = LT_SAVE_VERSION;
	app->save.size = sizeof(app->save);
	app->save.color = 1u;
	app->save.flags = LT_FLAG_DEVICE_AUTO | LT_DEFAULT_FLAGS;
}

static bool ltEnsureSaveDir(struct FatfsVol *vol)
{
	struct FatfsDir *saveDir, *portsDir = NULL;
	struct FatFileLocator locator;

	if (!vol)
		return false;
	saveDir = fatfsDirOpen(vol, "/SAVE");
	if (!saveDir) {
		if (!fatfsDirCreate(vol, "/SAVE", &locator))
			return false;
		saveDir = fatfsDirOpenWithLocator(vol, &locator);
	}
	if (!saveDir)
		return false;
	portsDir = fatfsDirOpenAt(saveDir, "PORTS");
	if (!portsDir && fatfsDirCreateAt(saveDir, "PORTS", &locator))
		portsDir = fatfsDirOpenWithLocator(vol, &locator);
	(void)fatfsDirClose(saveDir);
	if (!portsDir)
		return false;
	(void)fatfsDirClose(portsDir);
	return true;
}

static bool ltLoad(struct LtApp *app)
{
	struct FatfsFil *file;
	uint32_t got = 0, expected;
	bool crcValid;

	ltDefaults(app);
	if (!app->args || !app->args->vol)
		return false;
	file = fatfsFileOpen(app->args->vol, LT_SAVE_PATH, OPEN_MODE_READ);
	if (!file)
		return true;
	if (!fatfsFileRead(file, &app->save, sizeof(app->save), &got) || got != sizeof(app->save)) {
		(void)fatfsFileClose(file);
		ltDefaults(app);
		return false;
	}
	(void)fatfsFileClose(file);
	expected = app->save.crc;
	app->save.crc = 0;
	crcValid = expected == (app->save.version >= 3u ?
		ltBoundSaveCrc(&app->save) : ltCrc32(&app->save, sizeof(app->save)));
	if (app->save.magic != LT_SAVE_MAGIC ||
		(app->save.version < 1u || app->save.version > LT_SAVE_VERSION) ||
		app->save.size != sizeof(app->save) || !crcValid) {
		ltDefaults(app);
		return false;
	}
	app->save.crc = expected;
	return true;
}

static bool ltSave(struct LtApp *app)
{
	struct FatfsFil *file;
	uint32_t wrote = 0;

	if (!app->args || !app->args->vol || !ltEnsureSaveDir(app->args->vol))
		return false;
	app->save.magic = LT_SAVE_MAGIC;
	app->save.version = LT_SAVE_VERSION;
	app->save.size = sizeof(app->save);
	app->save.crc = 0;
	app->save.crc = ltBoundSaveCrc(&app->save);
	file = fatfsFileOpen(app->args->vol, LT_SAVE_PATH,
		OPEN_MODE_CREATE | OPEN_MODE_WRITE | OPEN_MODE_TRUNCATE);
	if (!file)
		return false;
	if (!fatfsFileWrite(file, &app->save, sizeof(app->save), &wrote) || wrote != sizeof(app->save)) {
		(void)fatfsFileClose(file);
		return false;
	}
	return fatfsFileClose(file);
}

static bool ltWriteSyncFile(struct LtApp *app)
{
	struct FatfsFil *file;
	uint32_t wrote = 0;
	bool ok = true;

	if (!app->args || !app->args->vol || !ltEnsureSaveDir(app->args->vol))
		return false;
	file = fatfsFileOpen(app->args->vol, LT_SYNC_PATH,
		OPEN_MODE_CREATE | OPEN_MODE_WRITE | OPEN_MODE_TRUNCATE);
	if (!file)
		return false;
	for (uint_fast8_t page = 0; page < app->syncPages && ok; page++) {
		uint32_t length;
		static const char newline = '\n';

		ok = ltBuildSyncText(app, page);
		length = (uint32_t)strlen(mLtSyncText);
		ok = ok && fatfsFileWrite(file, mLtSyncText, length, &wrote) && wrote == length;
		ok = ok && fatfsFileWrite(file, &newline, 1, &wrote) && wrote == 1;
	}
	return fatfsFileClose(file) && ok;
}

static uint32_t ltReadSyncUrlPrefix(struct LtApp *app)
{
	struct FatfsFil *file;
	char *prefix = (char *)mLtQrTemp;
	uint32_t got = 0;

	if (!app->args || !app->args->vol)
		return 0;
	file = fatfsFileOpen(app->args->vol, LT_SYNC_URL_PATH, OPEN_MODE_READ);
	if (!file)
		return 0;
	if (!fatfsFileRead(file, prefix, LT_SYNC_URL_PREFIX_MAX - 1u, &got))
		got = 0;
	(void)fatfsFileClose(file);
	while (got && (prefix[got - 1u] == '\r' || prefix[got - 1u] == '\n' ||
		prefix[got - 1u] == ' ' || prefix[got - 1u] == '\t'))
		got--;
	prefix[got] = 0;
	if (strncmp(prefix, "https://", 8) && strncmp(prefix, "http://", 7))
		return 0;
	return got;
}

static void ltPrepareSyncQr(struct LtApp *app)
{
	uint32_t urlSize, payloadSize;

	app->syncQrReady = false;
	app->syncExported = false;
	app->syncUrlConfigured = false;
	app->syncQrSize = 0;
	app->syncPages = ltSyncPageCount(app);
	if (app->syncPage >= app->syncPages)
		app->syncPage = 0;
	app->syncExported = ltWriteSyncFile(app);
	if (!ltBuildSyncText(app, app->syncPage))
		return;
	urlSize = ltReadSyncUrlPrefix(app);
	payloadSize = (uint32_t)strlen(mLtSyncText);
	if (urlSize) {
		if (urlSize + payloadSize >= sizeof(mLtSyncText))
			return;
		memmove(mLtSyncText + urlSize, mLtSyncText, payloadSize + 1u);
		memcpy(mLtSyncText, mLtQrTemp, urlSize);
		app->syncUrlConfigured = true;
	}

	app->syncQrReady = qrcodegen_encodeText(mLtSyncText, mLtQrTemp, mLtQrCode,
		qrcodegen_Ecc_LOW, qrcodegen_VERSION_MIN, LT_QR_MAX_VERSION,
		qrcodegen_Mask_AUTO, false);
	if (app->syncQrReady)
		app->syncQrSize = (uint8_t)qrcodegen_getSize(mLtQrCode);
}

static uint32_t ltTextWidth(const char *text, enum Font font)
{
	uint32_t width = 0;
	while (text && *text) {
		struct FontGlyphInfo glyph;
		if (fontGetGlyphInfo(&glyph, font, (unsigned char)*text))
			width += glyph.width + 1u;
		text++;
	}
	return width ? width - 1u : 0;
}

static void ltText(struct LtApp *app, int32_t x, int32_t y, const char *text,
	enum Font font, uint16_t color)
{
	while (text && *text) {
		struct FontGlyphInfo glyph;
		if (fontGetGlyphInfo(&glyph, font, (unsigned char)*text)) {
			for (uint_fast8_t row = 0; row < glyph.height; row++)
				for (uint_fast8_t col = 0; col < glyph.width; col++)
					if (fontGetGlyphPixel(&glyph, row, col))
						dcAppDrawPixel(&app->draw, x + col, y + row, color);
			x += glyph.width + 1;
		}
		text++;
	}
}

static void ltCentered(struct LtApp *app, int32_t y, const char *text,
	enum Font font, uint16_t color)
{
	uint32_t width = ltTextWidth(text, font);
	ltText(app, (int32_t)((LT_SCREEN_W > width ? LT_SCREEN_W - width : 0) / 2), y, text, font, color);
}

static void ltApplyLedBrightness(struct LtApp *app)
{
	struct Settings settings = app->baseSettings;
	uint8_t level = (uint8_t)((app->save.flags & LT_FLAG_LED_MASK) >> LT_FLAG_LED_SHIFT);

	if (level == 1u)
		settings.ledBrightness = 64u;
	else if (level == 2u)
		settings.ledBrightness = 144u;
	else if (level == 3u)
		settings.ledBrightness = 255u;
	badgeLedsApplySettings(&settings, true);
}

static void ltRestoreLeds(struct LtApp *app)
{
	if (app->ledOverride) {
		ltApplyLedBrightness(app);
		app->ledOverride = false;
	}
}

static void ltBeep(struct LtApp *app, uint32_t frequency, uint32_t durationMsec)
{
	if (!(app->save.flags & LT_FLAG_SOUND))
		return;
	audioPwmSetVolume(AUDIO_PWM_VOLUME_MAX);
	if (audioPwmTone(frequency))
		app->toneUntil = app->host->getTime() +
			(uint64_t)durationMsec * TICKS_PER_SECOND / 1000u;
}

static void ltSetMessage(struct LtApp *app, const char *message, uint32_t msec)
{
	snprintf(app->message, sizeof(app->message), "%s", message);
	app->messageUntil = app->host->getTime() + (uint64_t)msec * TICKS_PER_SECOND / 1000u;
}

static void ltTrackOpponent(struct LtApp *app, const struct IrRemoteOpenLasirFrame *frame)
{
	uint_fast8_t found = LT_OPPONENTS;
	struct LtOpponent opponent;

	for (uint_fast8_t i = 0; i < LT_OPPONENTS; i++)
		if (app->save.opponents[i].valid && app->save.opponents[i].blockId == frame->blockId &&
			app->save.opponents[i].deviceId == frame->deviceId) {
			found = i;
			break;
		}
	if (found < LT_OPPONENTS)
		opponent = app->save.opponents[found];
	else {
		memset(&opponent, 0, sizeof(opponent));
		opponent.blockId = frame->blockId;
		opponent.deviceId = frame->deviceId;
		opponent.valid = 1;
		found = LT_OPPONENTS - 1u;
	}
	opponent.color = frame->data;
	opponent.hits++;
	if (found)
		memmove(&app->save.opponents[1], &app->save.opponents[0], found * sizeof(opponent));
	app->save.opponents[0] = opponent;
}

static void ltStopReceiver(struct LtApp *app)
{
	if (app->receiverActive) {
		irRemoteOpenLasirEndReceive();
		app->receiverActive = false;
	}
}

static void ltStartReceiver(struct LtApp *app)
{
	if (!app->receiverActive)
		app->receiverActive = irRemoteOpenLasirBeginReceive();
}

static void ltReceive(struct LtApp *app, uint64_t now)
{
	struct IrRemoteOpenLasirFrame frame;

	while (app->receiverActive && irRemoteOpenLasirPoll(&frame)) {
		if (frame.mode != 0 || frame.data >= 8)
			continue;
		if (frame.blockId == app->save.blockId && frame.deviceId == app->effectiveDeviceId)
			continue;
		if (app->haveLastHit && frame.raw == app->lastHitRaw && now - app->lastHitTime < LT_DUPLICATE_TICKS)
			continue;
		app->lastHitRaw = frame.raw;
		app->lastHitTime = now;
		app->lastBlock = frame.blockId;
		app->lastDevice = frame.deviceId;
		app->lastColor = frame.data;
		app->haveLastHit = true;
		app->save.hits++;
		ltTrackOpponent(app, &frame);
		app->saveError = !ltSave(app);
		app->cooldownUntil = now + LT_HIT_LOCKOUT_TICKS;
		badgeLedsOverrideRgb(mLtColors[frame.data][0], mLtColors[frame.data][1], mLtColors[frame.data][2]);
		app->ledOverride = true;
		ltBeep(app, 240u, 180u);
		ltSetMessage(app, "TAGGED", 700);
		ltStopReceiver(app);
		break;
	}
}

static void ltExpireShots(struct LtApp *app, uint64_t now)
{
	uint_fast8_t expired = 0;
	while (expired < app->fireCount && now - app->fireTimes[expired] >= LT_FIRE_WINDOW_TICKS)
		expired++;
	if (expired) {
		memmove(app->fireTimes, app->fireTimes + expired,
			(app->fireCount - expired) * sizeof(*app->fireTimes));
		app->fireCount -= expired;
	}
}

static void ltFire(struct LtApp *app, uint64_t now)
{
	ltExpireShots(app, now);
	if (ltLooksLikeNec(app->effectiveDeviceId, app->save.color)) {
		ltSetMessage(app, "CHANGE DEVICE ID OR COLOR", 1200);
		return;
	}
	if (now < app->cooldownUntil) {
		ltSetMessage(app, "DISABLED AFTER HIT", 700);
		return;
	}
	if (app->fireCount && now - app->fireTimes[app->fireCount - 1u] < LT_FIRE_MIN_TICKS) {
		ltSetMessage(app, "WAIT 1 SECOND", 700);
		return;
	}
	if (app->fireCount >= LT_FIRE_WINDOW) {
		ltSetMessage(app, "30 SHOTS / MINUTE", 900);
		return;
	}
	ltStopReceiver(app);
	irRemoteOpenLasirSend(app->save.blockId, app->effectiveDeviceId, app->save.color);
	ltStartReceiver(app);
	app->fireTimes[app->fireCount++] = now;
	app->save.shots++;
	app->saveError = !ltSave(app);
	ltBeep(app, 880u, 90u);
	ltSetMessage(app, "FIRE!", 450);
}

static void ltHeader(struct LtApp *app, const char *title)
{
	dcAppDrawFill(&app->draw, 0, 0, LT_SCREEN_W, 27, ltRgb(10, 39, 58));
	dcAppDrawFill(&app->draw, 0, 26, LT_SCREEN_W, 1, ltRgb(70, 210, 225));
	ltCentered(app, 7, title, FontMedium, ltRgb(240, 250, 255));
}

static void ltDrawPlay(struct LtApp *app, uint64_t now)
{
	char line[96];
	uint8_t activeColor = now < app->cooldownUntil && app->haveLastHit ? app->lastColor : app->save.color;
	uint16_t signalColor = ltRgb(mLtColors[activeColor][0], mLtColors[activeColor][1],
		mLtColors[activeColor][2]);
	bool syncDue = (app->save.flags & LT_FLAG_SYNC_REMINDER) &&
		!(app->save.flags & LT_FLAG_AT_HOME) && now - app->lastSyncTime >= LT_SYNC_REMINDER_TICKS;
	const char *state = now < app->cooldownUntil ?
		(now < app->messageUntil ? app->message : "COOLDOWN") :
		(now < app->messageUntil ? app->message : (syncDue ? "SYNC REMINDER" : "READY"));

	if (now < app->cooldownUntil)
		dcAppDrawClear(&app->draw, ltRgb(mLtColors[activeColor][0] / 7u,
			mLtColors[activeColor][1] / 7u, mLtColors[activeColor][2] / 7u));
	else
		dcAppDrawClear(&app->draw, ltRgb(4, 12, 22));
	ltHeader(app, "OPENLASIR LASER TAG");
	dcAppDrawFill(&app->draw, 18, 42, 284, 55, ltRgb(12, 29, 43));
	dcAppDrawLine(&app->draw, 18, 42, 301, 42, signalColor);
	ltCentered(app, 56, state, FontLarge, signalColor);
	snprintf(line, sizeof(line), "%s  BLOCK %u  DEVICE %u%s", mLtColorNames[app->save.color],
		(unsigned)app->save.blockId, (unsigned)app->effectiveDeviceId,
		(app->save.flags & LT_FLAG_DEVICE_AUTO) ? " AUTO" : "");
	ltCentered(app, 80, line, FontSmall, ltRgb(185, 210, 220));
	dcAppDrawFill(&app->draw, 45, 112, 230, 54, ltRgb(9, 24, 36));
	dcAppDrawLine(&app->draw, 160, 119, 160, 158, ltRgb(52, 120, 145));
	snprintf(line, sizeof(line), "SHOTS  %lu", (unsigned long)app->save.shots);
	ltText(app, 63, 130, line, FontMedium, ltRgb(225, 235, 240));
	snprintf(line, sizeof(line), "HITS  %lu", (unsigned long)app->save.hits);
	ltText(app, 179, 130, line, FontMedium, ltRgb(225, 235, 240));
	if (app->haveLastHit) {
		snprintf(line, sizeof(line), "LAST: %u/%u  %s", (unsigned)app->lastBlock,
			(unsigned)app->lastDevice, mLtColorNames[app->lastColor]);
		ltCentered(app, 179, line, FontMedium, ltRgb(255, 190, 135));
	}
	else
		ltCentered(app, 179, "NO HITS RECEIVED YET", FontMedium, ltRgb(125, 150, 165));
	ltCentered(app, 216,
		"A FIRE   START SYNC   SELECT SETTINGS   DOWN STATS   B BACK",
		FontSmall,
		ltRgb(150, 185, 200));
	if (app->saveError)
		ltCentered(app, 201, "SAVE FAILED", FontSmall, ltRgb(255, 95, 85));
}

static const char *ltLedBrightnessName(const struct LtApp *app)
{
	static const char *const names[] = {"Auto", "Low", "Medium", "High"};
	return names[(app->save.flags & LT_FLAG_LED_MASK) >> LT_FLAG_LED_SHIFT];
}

static void ltDrawSettings(struct LtApp *app)
{
	char values[8][32];
	static const char *const labels[] = {
		"Sound", "Vibration", "My Color", "LED Brightness",
		"Sync Reminder", "At-Home Mode", "Tutorial", "Home"
	};

	dcAppDrawClear(&app->draw, ltRgb(4, 12, 22));
	ltHeader(app, "SETTINGS");
	snprintf(values[0], sizeof(values[0]), "%s", app->save.flags & LT_FLAG_SOUND ? "ON" : "OFF");
	snprintf(values[1], sizeof(values[1]), "Not fitted");
	snprintf(values[2], sizeof(values[2]), "%s", mLtColorNames[app->save.color]);
	snprintf(values[3], sizeof(values[3]), "%s", ltLedBrightnessName(app));
	snprintf(values[4], sizeof(values[4]), "%s",
		app->save.flags & LT_FLAG_SYNC_REMINDER ? "ON" : "OFF");
	snprintf(values[5], sizeof(values[5]), "%s", app->save.flags & LT_FLAG_AT_HOME ? "ON" : "OFF");
	snprintf(values[6], sizeof(values[6]), "Replay");
	values[7][0] = 0;
	for (uint_fast8_t i = 0; i < 8u; i++) {
		int32_t y = 40 + i * 22;
		if (i == app->settingsRow)
			dcAppDrawFill(&app->draw, 22, y - 5, 276, 20, ltRgb(17, 66, 78));
		ltText(app, 34, y, labels[i], FontMedium, ltRgb(225, 240, 245));
		if (values[i][0]) {
			uint32_t width = ltTextWidth(values[i], FontMedium);
			ltText(app, 286 - (int32_t)width, y, values[i], FontMedium,
				i == 1u ? ltRgb(135, 145, 150) : ltRgb(100, 230, 210));
		}
	}
	char identity[48];
	snprintf(identity, sizeof(identity), "IDENTITY %u/%u LOCKED", (unsigned)app->save.blockId,
		(unsigned)app->effectiveDeviceId);
	ltCentered(app, 220, identity, FontSmall,
		app->save.blockId ? ltRgb(145, 175, 190) : ltRgb(255, 175, 100));
}

static void ltDrawTutorial(struct LtApp *app)
{
	dcAppDrawClear(&app->draw, ltRgb(4, 12, 22));
	ltHeader(app, "HOW TO PLAY");
	ltText(app, 28, 48, "1. WEAR THE BADGE DURING THE EVENT", FontMedium, ltRgb(220, 235, 240));
	ltText(app, 28, 78, "2. PRESS A TO FIRE AT ANOTHER BADGE", FontMedium, ltRgb(220, 235, 240));
	ltText(app, 28, 108, "3. TAG UNIQUE PEOPLE FOR BETTER SCORES", FontMedium, ltRgb(220, 235, 240));
	ltText(app, 28, 138, "4. PRESS START TO SHOW THE SYNC QR", FontMedium, ltRgb(220, 235, 240));
	ltText(app, 28, 168, "5. SYNC PERIODICALLY SO HITS ARE COUNTED", FontMedium, ltRgb(220, 235, 240));
	ltCentered(app, 215, "A OR B RETURN TO SETTINGS", FontSmall, ltRgb(145, 175, 190));
}

static void ltDrawStats(struct LtApp *app)
{
	char line[72];
	uint_fast8_t row = 0;

	dcAppDrawClear(&app->draw, ltRgb(4, 12, 22));
	ltHeader(app, "LASER TAG STATISTICS");
	snprintf(line, sizeof(line), "LIFETIME SHOTS: %lu     HITS RECEIVED: %lu",
		(unsigned long)app->save.shots, (unsigned long)app->save.hits);
	ltCentered(app, 39, line, FontMedium, ltRgb(220, 235, 240));
	ltText(app, 25, 68, "RECENT OPPONENT", FontSmall, ltRgb(105, 210, 220));
	ltText(app, 244, 68, "HITS", FontSmall, ltRgb(105, 210, 220));
	for (uint_fast8_t i = app->statsOffset; i < LT_OPPONENTS && row < 7; i++) {
		if (!app->save.opponents[i].valid)
			continue;
		snprintf(line, sizeof(line), "Block %u / Device %u   %s",
			(unsigned)app->save.opponents[i].blockId,
			(unsigned)app->save.opponents[i].deviceId,
			mLtColorNames[app->save.opponents[i].color & 7u]);
		ltText(app, 25, 88 + row * 18, line, FontSmall, ltRgb(205, 220, 228));
		snprintf(line, sizeof(line), "%lu", (unsigned long)app->save.opponents[i].hits);
		ltText(app, 250, 88 + row * 18, line, FontSmall, ltRgb(255, 190, 130));
		row++;
	}
	if (!row)
		ltCentered(app, 115, "NO OPPONENTS RECORDED", FontMedium, ltRgb(125, 150, 165));
	ltCentered(app, 216, "START SYNC QR   UP/DOWN SCROLL   B RETURN", FontSmall,
		ltRgb(145, 175, 190));
}

static void ltDrawSync(struct LtApp *app)
{
	dcAppDrawClear(&app->draw, ltRgb(4, 12, 22));
	ltHeader(app, "LASER TAG SYNC");
	if (app->syncQrReady && app->syncQrSize) {
		uint32_t modules = app->syncQrSize + 8u;
		uint32_t scale = 180u / modules;
		uint32_t total, left, top = 31u;

		if (scale > 3u)
			scale = 3u;
		if (!scale)
			scale = 1u;
		total = modules * scale;
		left = (LT_SCREEN_W - total) / 2u;
		dcAppDrawFill(&app->draw, left, top, total, total, ltRgb(255, 255, 255));
		for (uint_fast8_t y = 0; y < app->syncQrSize; y++)
			for (uint_fast8_t x = 0; x < app->syncQrSize; x++)
				if (qrcodegen_getModule(mLtQrCode, x, y))
					dcAppDrawFill(&app->draw, left + (x + 4u) * scale,
						top + (y + 4u) * scale, scale, scale, ltRgb(0, 0, 0));
		ltCentered(app, 207,
			app->syncUrlConfigured ? "UPLOAD URL CONFIGURED" : "RAW UNSIGNED PAYLOAD",
			FontSmall, app->syncUrlConfigured ? ltRgb(105, 230, 180) : ltRgb(255, 185, 110));
		char footer[72];
		snprintf(footer, sizeof(footer), app->syncExported ?
			"LEFT/RIGHT PAGE %u/%u   A REFRESH   B BACK" :
			"PAGE %u/%u   A RETRY   B BACK   EXPORT FAILED",
			(unsigned)app->syncPage + 1u, (unsigned)app->syncPages);
		ltCentered(app, 220, footer,
			FontSmall, app->syncExported ? ltRgb(150, 185, 200) : ltRgb(255, 95, 85));
	}
	else {
		ltCentered(app, 92, "SYNC PAYLOAD DID NOT FIT", FontLarge, ltRgb(255, 120, 100));
		ltCentered(app, 126, "A RETRY       B BACK", FontMedium, ltRgb(190, 210, 220));
	}
}

static void ltDraw(struct LtApp *app, uint64_t now)
{
	switch (app->view) {
	case LtViewSettings: ltDrawSettings(app); break;
	case LtViewStats: ltDrawStats(app); break;
	case LtViewSync: ltDrawSync(app); break;
	case LtViewTutorial: ltDrawTutorial(app); break;
	default: ltDrawPlay(app, now); break;
	}
}

static void ltChangeSetting(struct LtApp *app, int delta, bool activate)
{
	uint8_t oldFlags = app->save.flags;
	uint8_t oldColor = app->save.color;

	switch (app->settingsRow) {
	case 0:
		app->save.flags ^= LT_FLAG_SOUND;
		break;
	case 1:
		ltSetMessage(app, "DC32 HAS NO VIBRATION MOTOR", 1000);
		return;
	case 2:
		do {
			app->save.color = (uint8_t)((app->save.color + (delta < 0 ? 7u : 1u)) & 7u);
		} while (ltLooksLikeNec(app->effectiveDeviceId, app->save.color));
		break;
	case 3: {
		uint8_t level = (uint8_t)((app->save.flags & LT_FLAG_LED_MASK) >> LT_FLAG_LED_SHIFT);
		level = (uint8_t)((level + (delta < 0 ? 3u : 1u)) & 3u);
		app->save.flags = (uint8_t)((app->save.flags & ~LT_FLAG_LED_MASK) |
			(level << LT_FLAG_LED_SHIFT));
		ltApplyLedBrightness(app);
		break;
	}
	case 4:
		app->save.flags ^= LT_FLAG_SYNC_REMINDER;
		break;
	case 5:
		app->save.flags ^= LT_FLAG_AT_HOME;
		break;
	case 6:
		if (activate)
			app->view = LtViewTutorial;
		return;
	case 7:
		if (activate) {
			app->view = LtViewPlay;
			ltStartReceiver(app);
		}
		return;
	default:
		return;
	}
	if (app->save.flags != oldFlags || app->save.color != oldColor)
		app->saveError = !ltSave(app);
}

static bool ltInput(struct LtApp *app, uint64_t now)
{
	uint_fast16_t pressed = app->draw.pressed;

	switch (app->view) {
	case LtViewPlay:
		if (pressed & KEY_BIT_B)
			return false;
		if (pressed & KEY_BIT_A)
			ltFire(app, now);
		if (pressed & KEY_BIT_SEL) {
			ltStopReceiver(app);
			app->view = LtViewSettings;
		}
		if (pressed & KEY_BIT_START) {
			if (app->save.flags & LT_FLAG_AT_HOME)
				ltSetMessage(app, "SYNC DISABLED IN AT-HOME MODE", 1200);
			else {
				ltStopReceiver(app);
				app->syncPage = 0;
				ltPrepareSyncQr(app);
				app->lastSyncTime = now;
				app->view = LtViewSync;
			}
		}
		if (pressed & KEY_BIT_DOWN) {
			ltStopReceiver(app);
			app->view = LtViewStats;
		}
		break;
	case LtViewSettings:
		if (pressed & KEY_BIT_UP)
			app->settingsRow = (uint8_t)((app->settingsRow + 7u) % 8u);
		if (pressed & KEY_BIT_DOWN)
			app->settingsRow = (uint8_t)((app->settingsRow + 1u) % 8u);
		if (pressed & KEY_BIT_LEFT)
			ltChangeSetting(app, -1, false);
		if (pressed & KEY_BIT_RIGHT)
			ltChangeSetting(app, 1, false);
		if (pressed & KEY_BIT_A)
			ltChangeSetting(app, 1, true);
		if (pressed & (KEY_BIT_B | KEY_BIT_SEL)) {
			app->view = LtViewPlay;
			ltStartReceiver(app);
		}
		break;
	case LtViewStats:
		if ((pressed & KEY_BIT_UP) && app->statsOffset)
			app->statsOffset--;
		if ((pressed & KEY_BIT_DOWN) && app->statsOffset + 7u < LT_OPPONENTS &&
			app->save.opponents[app->statsOffset + 7u].valid)
			app->statsOffset++;
		if ((pressed & KEY_BIT_START) && !(app->save.flags & LT_FLAG_AT_HOME)) {
			ltPrepareSyncQr(app);
			app->lastSyncTime = now;
			app->view = LtViewSync;
		}
		if (pressed & KEY_BIT_B) {
			app->view = LtViewPlay;
			ltStartReceiver(app);
		}
		break;
	case LtViewSync:
		if ((pressed & KEY_BIT_LEFT) && app->syncPage) {
			app->syncPage--;
			ltPrepareSyncQr(app);
		}
		if ((pressed & KEY_BIT_RIGHT) && app->syncPage + 1u < app->syncPages) {
			app->syncPage++;
			ltPrepareSyncQr(app);
		}
		if (pressed & KEY_BIT_A)
			ltPrepareSyncQr(app);
		if (pressed & (KEY_BIT_B | KEY_BIT_START)) {
			app->view = LtViewPlay;
			ltStartReceiver(app);
		}
		break;
	case LtViewTutorial:
		if (pressed & (KEY_BIT_A | KEY_BIT_B | KEY_BIT_SEL))
			app->view = LtViewSettings;
		break;
	}
	return true;
}

int laserTagAppRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	struct LtApp app;
	bool loaded, provisioningChanged;

	if (!host || !args || !args->canvas || !args->vol)
		return -1;
	memset(&app, 0, sizeof(app));
	app.host = host;
	app.args = args;
	mLtAbort = false;
	if (!dcAppDrawInit(&app.draw, host, args, mLtFrame, LT_SCREEN_W, LT_SCREEN_H))
		return -1;
	loaded = ltLoad(&app);
	provisioningChanged = ltApplyProvisioning(&app);
	if (!loaded || provisioningChanged)
		app.saveError = !ltSave(&app);
	settingsGet(&app.baseSettings);
	ltApplyLedBrightness(&app);
	app.lastSyncTime = host->getTime();
	ltStartReceiver(&app);
	dcAppDrawWaitRelease(&app.draw, KEY_BIT_A | KEY_BIT_B | KEY_BIT_START | KEY_BIT_SEL |
		KEY_BIT_UP | KEY_BIT_DOWN | KEY_BIT_LEFT | KEY_BIT_RIGHT | UI_KEY_BIT_CENTER);

	while (!mLtAbort && dcAppDrawFrame(&app.draw, UI_KEY_BIT_CENTER)) {
		uint64_t now = host->getTime();
		if (app.toneUntil && now >= app.toneUntil) {
			audioPwmStop();
			app.toneUntil = 0;
		}

		if (app.cooldownUntil && now >= app.cooldownUntil) {
			app.cooldownUntil = 0;
			ltRestoreLeds(&app);
			ltStartReceiver(&app);
		}
		if (app.view == LtViewPlay && !app.cooldownUntil)
			ltReceive(&app, now);
		if (app.cooldownUntil && app.haveLastHit) {
			badgeLedsOverrideRgb(mLtColors[app.lastColor][0], mLtColors[app.lastColor][1],
				mLtColors[app.lastColor][2]);
			app.ledOverride = true;
		}
		if (!ltInput(&app, now))
			break;
		ltDraw(&app, now);
	}

	ltStopReceiver(&app);
	ltRestoreLeds(&app);
	audioPwmStop();
	badgeLedsApplySettings(&app.baseSettings, true);
	dcAppDrawWaitRelease(&app.draw, KEY_BIT_A | KEY_BIT_B | UI_KEY_BIT_CENTER);
	return 0;
}

void laserTagAppAbort(void)
{
	mLtAbort = true;
	irRemoteOpenLasirEndReceive();
}
