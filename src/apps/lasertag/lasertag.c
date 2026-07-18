#include "apps/lasertag/lasertag.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
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
#define LT_SAVE_VERSION 5u
#define LT_SAVE_PATH "/SAVE/PORTS/LASERTAG.DAT"
#define LT_SYNC_PATH "/SAVE/PORTS/LASERTAG.SYNC"
#define LT_EVENTS 32u
#define LT_EVENTS_PER_PAGE 13u
#define LT_SYNC_HEADER_SIZE 28u
#define LT_SYNC_ENTRY_SIZE 8u
#define LT_SYNC_MAX (LT_SYNC_HEADER_SIZE + LT_EVENTS * LT_SYNC_ENTRY_SIZE + 4u)
#define LT_SYNC_TEXT_MAX 512u
#define LT_QR_MAX_VERSION 15
#define LT_QR_BUFFER_SIZE qrcodegen_BUFFER_LEN_FOR_VERSION(LT_QR_MAX_VERSION)
#define LT_FLAG_PROFILE_MASK 0x03u
#define LT_FLAG_SOUND 0x04u
#define LT_SYNC_UNSIGNED 0x80u
#define LT_EVENT_TX 0x01u
#define LT_EVENT_OPENLASIR 0x02u
#define LT_FIRE_MIN_TICKS TICKS_PER_SECOND
#define LT_CONTROL_MIN_TICKS (6ull * TICKS_PER_SECOND)
#define LT_FIRE_WINDOW 30u
#define LT_FIRE_WINDOW_TICKS (60ull * TICKS_PER_SECOND)
#define LT_HIT_LOCKOUT_TICKS (2ull * TICKS_PER_SECOND)
#define LT_ARM_TICKS (2ull * TICKS_PER_SECOND)
#define LT_SYNC_PREFIX "DC32LT2."

enum LtProfile {
	LtProfileCustom,
	LtProfileMjolnirFire,
	LtProfileMjolnirBlink,
	LtProfileDc33Legacy,
};

enum LtView {
	LtViewMonitor,
	LtViewTransmit,
	LtViewHistory,
	LtViewExport,
};

struct LtEvent {
	uint32_t raw;
	uint16_t uptimeSeconds;
	uint8_t flags;
	uint8_t valid;
};

struct LtSave {
	uint32_t magic;
	uint16_t version;
	uint16_t size;
	uint8_t blockId;
	uint8_t deviceId;
	uint8_t mode;
	uint8_t data;
	uint8_t flags;
	uint8_t legacyVector;
	uint8_t reserved[2];
	uint32_t txCount;
	uint32_t rxCount;
	struct LtEvent events[LT_EVENTS];
	uint32_t crc;
};

struct LtApp {
	const struct DcAppHostApi *host;
	const struct DcAppRunArgs *args;
	struct DcAppDrawCtx draw;
	struct Settings baseSettings;
	struct LtSave save;
	enum LtView view;
	uint8_t txRow;
	uint8_t historyOffset;
	uint8_t syncPage;
	uint8_t syncPages;
	uint8_t syncQrSize;
	uint64_t lastTxTime;
	uint64_t fireTimes[LT_FIRE_WINDOW];
	uint8_t fireCount;
	uint64_t cooldownUntil;
	uint64_t armedUntil;
	uint64_t messageUntil;
	bool receiverActive;
	bool saveError;
	bool qrReady;
	bool exported;
	char message[48];
};

static uint8_t mLtFrame[LT_SCREEN_W * LT_SCREEN_H];
static char mLtSyncText[LT_SYNC_TEXT_MAX];
static uint8_t mLtQrTemp[LT_QR_BUFFER_SIZE];
static uint8_t mLtQrCode[LT_QR_BUFFER_SIZE];
static volatile bool mLtAbort;

static const char *const mLtColors[] = {
	"Cyan", "Magenta", "Yellow", "Green", "Red", "Blue", "Orange", "White"
};

static const uint8_t mLtColorRgb[][3] = {
	{0, 255, 255}, {255, 0, 255}, {255, 255, 0}, {0, 255, 0},
	{255, 0, 0}, {0, 0, 255}, {255, 165, 0}, {255, 255, 255}
};

/* Exact raw values decoded from Defcon33LaserTag.ir. */
static const uint32_t mLtDc33Raw[] = {0x25da0bf4u, 0x24db0bf4u, 0x23dc0bf4u, 0x22dd0bf4u};
static const char *const mLtDc33Names[] = {"L_TAG_1", "L_TAG_2", "L_TAG_3A", "L_TAG_3B"};

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

static uint32_t ltSaveCrc(const struct LtSave *save)
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
	static const char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
	uint32_t in = 0, out = 0;

	while (in < srcSize) {
		uint32_t remain = srcSize - in;
		uint32_t value = (uint32_t)src[in] << 16;

		if (remain > 1u)
			value |= (uint32_t)src[in + 1u] << 8;
		if (remain > 2u)
			value |= src[in + 2u];
		if (out + (remain > 2u ? 4u : remain + 1u) >= dstSize)
			return false;
		dst[out++] = alphabet[(value >> 18) & 63u];
		dst[out++] = alphabet[(value >> 12) & 63u];
		if (remain > 1u)
			dst[out++] = alphabet[(value >> 6) & 63u];
		if (remain > 2u)
			dst[out++] = alphabet[value & 63u];
		in += remain > 2u ? 3u : remain;
	}
	dst[out] = 0;
	return true;
}

static const char *ltModeName(uint8_t mode)
{
	static const char *const names[] = {
		"Fire", "User presence", "Base presence", "User hello", "User reply",
		"Base hello", "Base reply", "Station hello", "Station reply", "Color temporary",
		"Color permanent", "General interact"
	};

	return mode < 12u ? names[mode] : "Reserved";
}

static const char *ltProfileName(const struct LtApp *app)
{
	switch (app->save.flags & LT_FLAG_PROFILE_MASK) {
	case LtProfileMjolnirFire: return "Mj0ln1r fire";
	case LtProfileMjolnirBlink: return "Mj0ln1r blink";
	case LtProfileDc33Legacy: return "DC33 legacy";
	default: return "Custom OpenLASIR";
	}
}

static uint32_t ltOpenLasirRaw(uint8_t block, uint8_t device, uint8_t mode, uint8_t data)
{
	return (uint32_t)block | ((uint32_t)(uint8_t)~block << 8) |
		((uint32_t)device << 16) | ((uint32_t)(mode & 31u) << 24) |
		((uint32_t)(data & 7u) << 29);
}

static uint8_t ltAutoDeviceId(void)
{
	uint64_t uid = flashGetUid();
	uint8_t value = 0x32u;

	for (uint_fast8_t i = 0; i < 8u; i++) {
		value ^= (uint8_t)uid;
		value = (uint8_t)((value << 3) | (value >> 5));
		uid >>= 8;
	}
	return value;
}

static void ltDefaults(struct LtApp *app)
{
	memset(&app->save, 0, sizeof(app->save));
	app->save.magic = LT_SAVE_MAGIC;
	app->save.version = LT_SAVE_VERSION;
	app->save.size = sizeof(app->save);
	app->save.blockId = 0u;
	app->save.deviceId = ltAutoDeviceId();
	app->save.mode = 0u;
	app->save.data = 1u;
	app->save.flags = LtProfileCustom | LT_FLAG_SOUND;
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

	ltDefaults(app);
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
	if (app->save.magic != LT_SAVE_MAGIC || app->save.version != LT_SAVE_VERSION ||
		app->save.size != sizeof(app->save) || expected != ltSaveCrc(&app->save)) {
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

	if (!ltEnsureSaveDir(app->args->vol))
		return false;
	app->save.magic = LT_SAVE_MAGIC;
	app->save.version = LT_SAVE_VERSION;
	app->save.size = sizeof(app->save);
	app->save.crc = 0;
	app->save.crc = ltSaveCrc(&app->save);
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

static void ltSetMessage(struct LtApp *app, const char *message, uint32_t msec)
{
	snprintf(app->message, sizeof(app->message), "%s", message);
	app->messageUntil = app->host->getTime() + (uint64_t)msec * TICKS_PER_SECOND / 1000u;
}

static void ltStartReceiver(struct LtApp *app)
{
	if (!app->receiverActive)
		app->receiverActive = irRemoteOpenLasirBeginReceive();
}

static void ltStopReceiver(struct LtApp *app)
{
	if (app->receiverActive) {
		irRemoteOpenLasirEndReceive();
		app->receiverActive = false;
	}
}

static void ltLog(struct LtApp *app, uint32_t raw, bool tx, bool openLasir, uint64_t now)
{
	struct LtEvent event;

	memset(&event, 0, sizeof(event));
	event.raw = raw;
	event.uptimeSeconds = (uint16_t)(now / TICKS_PER_SECOND);
	event.flags = (tx ? LT_EVENT_TX : 0u) | (openLasir ? LT_EVENT_OPENLASIR : 0u);
	event.valid = 1u;
	memmove(&app->save.events[1], &app->save.events[0],
		(LT_EVENTS - 1u) * sizeof(app->save.events[0]));
	app->save.events[0] = event;
}

static void ltExpireFireTimes(struct LtApp *app, uint64_t now)
{
	uint_fast8_t expired = 0;

	while (expired < app->fireCount && now - app->fireTimes[expired] >= LT_FIRE_WINDOW_TICKS)
		expired++;
	if (expired) {
		memmove(app->fireTimes, app->fireTimes + expired,
			(app->fireCount - expired) * sizeof(app->fireTimes[0]));
		app->fireCount -= expired;
	}
}

static uint32_t ltSelectedRaw(const struct LtApp *app, bool *openLasir)
{
	switch (app->save.flags & LT_FLAG_PROFILE_MASK) {
	case LtProfileMjolnirFire:
		*openLasir = true;
		return ltOpenLasirRaw(1u, 3u, 0u, app->save.data);
	case LtProfileMjolnirBlink:
		*openLasir = true;
		return ltOpenLasirRaw(1u, 3u, 9u, app->save.data);
	case LtProfileDc33Legacy:
		*openLasir = false;
		return mLtDc33Raw[app->save.legacyVector & 3u];
	default:
		*openLasir = true;
		return ltOpenLasirRaw(app->save.blockId, app->save.deviceId, app->save.mode, app->save.data);
	}
}

static bool ltCanTransmit(struct LtApp *app, uint64_t now, bool openLasir, uint8_t mode)
{
	uint64_t minimum = (openLasir && mode == 0u) ? LT_FIRE_MIN_TICKS : LT_CONTROL_MIN_TICKS;

	if (now < app->cooldownUntil) {
		ltSetMessage(app, "OUTBOUND COOLDOWN", 900u);
		return false;
	}
	if (app->lastTxTime && now - app->lastTxTime < minimum) {
		ltSetMessage(app, "RATE LIMITED", 900u);
		return false;
	}
	if (openLasir && mode == 0u) {
		ltExpireFireTimes(app, now);
		if (app->fireCount >= LT_FIRE_WINDOW) {
			ltSetMessage(app, "30 FIRE / MIN LIMIT", 900u);
			return false;
		}
	}
	return true;
}

static void ltTransmit(struct LtApp *app, uint64_t now)
{
	bool openLasir;
	uint32_t raw = ltSelectedRaw(app, &openLasir);
	uint8_t mode = (uint8_t)(raw >> 24) & 31u;

	if (!ltCanTransmit(app, now, openLasir, mode))
		return;
	irRemoteNecSend(raw);
	app->lastTxTime = now;
	if (openLasir && mode == 0u)
		app->fireTimes[app->fireCount++] = now;
	app->save.txCount++;
	ltLog(app, raw, true, openLasir, now);
	app->saveError = !ltSave(app);
	app->armedUntil = 0;
	ltSetMessage(app, openLasir ? "OPENLASIR SENT" : "LEGACY NEC SENT", 700u);
}

static const char *ltLegacyName(uint32_t raw)
{
	for (uint_fast8_t i = 0; i < 4u; i++)
		if (raw == mLtDc33Raw[i])
			return mLtDc33Names[i];
	return "Unknown legacy NEC";
}

static void ltReceive(struct LtApp *app, uint64_t now)
{
	struct IrRemoteNecFrame frame;

	while (app->receiverActive && irRemoteNecPoll(&frame)) {
		char message[48];
		bool openLasir = frame.kind == IrRemoteNecOpenLasir;

		app->save.rxCount++;
		ltLog(app, frame.raw, false, openLasir, now);
		if (openLasir) {
			snprintf(message, sizeof(message), "%s B%u D%u", ltModeName(frame.mode),
				(unsigned)frame.blockId, (unsigned)frame.deviceId);
			if (frame.mode == 0u) {
				const uint8_t *rgb = mLtColorRgb[frame.data & 7u];
				badgeLedsOverrideRgb(rgb[0], rgb[1], rgb[2]);
				app->cooldownUntil = now + LT_HIT_LOCKOUT_TICKS;
			}
		}
		else
			snprintf(message, sizeof(message), "%s", ltLegacyName(frame.raw));
		app->saveError = !ltSave(app);
		ltSetMessage(app, message, 1300u);
	}
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
	return width ? width - 1u : 0u;
}

static void ltText(struct LtApp *app, int32_t x, int32_t y, const char *text, enum Font font, uint16_t color)
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

static void ltCentered(struct LtApp *app, int32_t y, const char *text, enum Font font, uint16_t color)
{
	uint32_t width = ltTextWidth(text, font);

	ltText(app, (int32_t)((LT_SCREEN_W - width) / 2u), y, text, font, color);
}

static void ltHeader(struct LtApp *app, const char *title)
{
	dcAppDrawFill(&app->draw, 0, 0, LT_SCREEN_W, 27, ltRgb(10, 39, 58));
	dcAppDrawFill(&app->draw, 0, 26, LT_SCREEN_W, 1, ltRgb(70, 210, 225));
	ltCentered(app, 7, title, FontMedium, ltRgb(240, 250, 255));
}

static void ltDescribeRaw(uint32_t raw, bool openLasir, char *line, uint32_t lineSize)
{
	if (openLasir) {
		uint8_t block = (uint8_t)raw;
		uint8_t device = (uint8_t)(raw >> 16);
		uint8_t mode = (uint8_t)(raw >> 24) & 31u;
		uint8_t data = (uint8_t)(raw >> 29);
		snprintf(line, lineSize, "B%u D%u %s %s", (unsigned)block, (unsigned)device,
			ltModeName(mode), mLtColors[data]);
	}
	else
		snprintf(line, lineSize, "%s", ltLegacyName(raw));
}

static void ltDrawMonitor(struct LtApp *app, uint64_t now)
{
	char line[72];
	const char *state = now < app->cooldownUntil ? "HIT COOLDOWN" : "LISTENING";

	dcAppDrawClear(&app->draw, ltRgb(4, 12, 22));
	ltHeader(app, "OPENLASIR LAB");
	ltCentered(app, 45, state, FontLarge, now < app->cooldownUntil ? ltRgb(255, 170, 100) : ltRgb(95, 235, 190));
	snprintf(line, sizeof(line), "PROFILE: %s", ltProfileName(app));
	ltCentered(app, 79, line, FontMedium, ltRgb(210, 230, 238));
	snprintf(line, sizeof(line), "TX %lu   RX %lu", (unsigned long)app->save.txCount,
		(unsigned long)app->save.rxCount);
	ltCentered(app, 107, line, FontMedium, ltRgb(165, 200, 215));
	if (app->save.events[0].valid) {
		struct LtEvent *event = &app->save.events[0];
		ltDescribeRaw(event->raw, event->flags & LT_EVENT_OPENLASIR, line, sizeof(line));
		ltCentered(app, 143, event->flags & LT_EVENT_TX ? "LAST OUTGOING" : "LAST INCOMING",
			FontSmall, ltRgb(105, 210, 220));
		ltCentered(app, 158, line, FontMedium, ltRgb(255, 210, 145));
		snprintf(line, sizeof(line), "RAW %08lX  T+%us", (unsigned long)event->raw,
			(unsigned)event->uptimeSeconds);
		ltCentered(app, 179, line, FontSmall, ltRgb(145, 175, 190));
	}
	else
		ltCentered(app, 155, "WAITING FOR VALID NEC/OPENLASIR", FontMedium, ltRgb(145, 175, 190));
	if (now < app->messageUntil)
		ltCentered(app, 199, app->message, FontSmall, ltRgb(255, 190, 135));
	ltCentered(app, 218, "A TX   DOWN HISTORY   START EXPORT   B BACK", FontSmall, ltRgb(145, 185, 200));
}

static void ltDrawTransmit(struct LtApp *app, uint64_t now)
{
	char values[6][36];
	static const char *const labels[] = {"Profile", "Block", "Device", "Mode", "Team/Data", "Legacy vector"};
	uint8_t profile = app->save.flags & LT_FLAG_PROFILE_MASK;

	dcAppDrawClear(&app->draw, ltRgb(4, 12, 22));
	ltHeader(app, "MANUAL TRANSMIT");
	snprintf(values[0], sizeof(values[0]), "%s", ltProfileName(app));
	snprintf(values[1], sizeof(values[1]), "%u", profile == LtProfileCustom ? (unsigned)app->save.blockId : 1u);
	snprintf(values[2], sizeof(values[2]), "%u", profile == LtProfileCustom ? (unsigned)app->save.deviceId : 3u);
	snprintf(values[3], sizeof(values[3]), "%u %s", profile == LtProfileCustom ? (unsigned)app->save.mode :
		profile == LtProfileMjolnirBlink ? 9u : 0u, profile == LtProfileDc33Legacy ? "(raw)" : "");
	snprintf(values[4], sizeof(values[4]), "%s (%u)", mLtColors[app->save.data & 7u], (unsigned)(app->save.data & 7u));
	snprintf(values[5], sizeof(values[5]), "%s", mLtDc33Names[app->save.legacyVector & 3u]);
	for (uint_fast8_t i = 0; i < 6u; i++) {
		int32_t y = 42 + i * 24;
		if (i == app->txRow)
			dcAppDrawFill(&app->draw, 17, y - 5, 286, 20, ltRgb(17, 66, 78));
		ltText(app, 28, y, labels[i], FontMedium, ltRgb(225, 240, 245));
		ltText(app, 170, y, values[i], FontSmall, ltRgb(100, 230, 210));
	}
	if (app->armedUntil > now)
		ltCentered(app, 194, "PRESS A AGAIN TO SEND", FontMedium, ltRgb(255, 180, 100));
	else if (now < app->messageUntil)
		ltCentered(app, 194, app->message, FontSmall, ltRgb(255, 190, 135));
	ltCentered(app, 218, "UP/DOWN SELECT  LEFT/RIGHT EDIT  A ARM/SEND  B BACK", FontSmall,
		ltRgb(145, 185, 200));
}

static void ltDrawHistory(struct LtApp *app)
{
	char line[72];
	uint_fast8_t row = 0;

	dcAppDrawClear(&app->draw, ltRgb(4, 12, 22));
	ltHeader(app, "PACKET HISTORY");
	for (uint_fast8_t i = app->historyOffset; i < LT_EVENTS && row < 7u; i++) {
		struct LtEvent *event = &app->save.events[i];
		if (!event->valid)
			continue;
		ltDescribeRaw(event->raw, event->flags & LT_EVENT_OPENLASIR, line, sizeof(line));
		ltText(app, 18, 43 + row * 23, event->flags & LT_EVENT_TX ? "TX" : "RX", FontSmall,
			event->flags & LT_EVENT_TX ? ltRgb(120, 220, 170) : ltRgb(120, 190, 240));
		ltText(app, 47, 43 + row * 23, line, FontSmall, ltRgb(215, 228, 235));
		snprintf(line, sizeof(line), "%08lX", (unsigned long)event->raw);
		ltText(app, 215, 56 + row * 23, line, FontSmall, ltRgb(255, 190, 135));
		row++;
	}
	if (!row)
		ltCentered(app, 112, "NO PACKETS LOGGED", FontMedium, ltRgb(145, 175, 190));
	ltCentered(app, 218, "UP/DOWN SCROLL   START EXPORT   B BACK", FontSmall, ltRgb(145, 185, 200));
}

static uint8_t ltSyncPageCount(const struct LtApp *app)
{
	uint8_t count = 0;

	for (uint_fast8_t i = 0; i < LT_EVENTS; i++)
		if (app->save.events[i].valid)
			count++;
	return count ? (uint8_t)((count + LT_EVENTS_PER_PAGE - 1u) / LT_EVENTS_PER_PAGE) : 1u;
}

static uint32_t ltBuildSyncBinary(struct LtApp *app, uint8_t page)
{
	uint8_t *binary = mLtQrTemp;
	uint32_t pos = LT_SYNC_HEADER_SIZE;
	uint8_t seen = 0, count = 0;
	uint8_t pages = ltSyncPageCount(app);
	uint8_t first = (uint8_t)(page * LT_EVENTS_PER_PAGE);
	uint64_t uid = flashGetUid();

	memset(binary, 0, LT_SYNC_MAX);
	memcpy(binary, "DCLT", 4);
	binary[4] = 3u;
	binary[5] = LT_SYNC_UNSIGNED;
	binary[6] = app->save.blockId;
	binary[7] = app->save.deviceId;
	binary[8] = app->save.data;
	binary[10] = page;
	binary[11] = pages;
	ltWrite32(binary + 12, ltCrc32(&uid, sizeof(uid)));
	ltWrite32(binary + 16, app->save.txCount);
	ltWrite32(binary + 20, app->save.rxCount);
	ltWrite32(binary + 24, (uint32_t)(app->host->getTime() / TICKS_PER_SECOND));
	for (uint_fast8_t i = 0; i < LT_EVENTS; i++) {
		struct LtEvent *event = &app->save.events[i];
		if (!event->valid)
			continue;
		if (seen++ < first)
			continue;
		if (count >= LT_EVENTS_PER_PAGE)
			break;
		ltWrite32(binary + pos, event->raw);
		binary[pos + 4u] = (uint8_t)event->uptimeSeconds;
		binary[pos + 5u] = (uint8_t)(event->uptimeSeconds >> 8);
		binary[pos + 6u] = event->flags;
		binary[pos + 7u] = 0u;
		pos += LT_SYNC_ENTRY_SIZE;
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
	return ltBase64Url(mLtSyncText + prefixSize, sizeof(mLtSyncText) - prefixSize, mLtQrTemp, binarySize);
}

static bool ltWriteSyncFile(struct LtApp *app)
{
	struct FatfsFil *file;
	uint32_t wrote = 0;
	bool ok = true;

	if (!ltEnsureSaveDir(app->args->vol))
		return false;
	file = fatfsFileOpen(app->args->vol, LT_SYNC_PATH, OPEN_MODE_CREATE | OPEN_MODE_WRITE | OPEN_MODE_TRUNCATE);
	if (!file)
		return false;
	for (uint_fast8_t page = 0; page < app->syncPages && ok; page++) {
		uint32_t length;
		static const char newline = '\n';
		ok = ltBuildSyncText(app, page);
		length = (uint32_t)strlen(mLtSyncText);
		ok = ok && fatfsFileWrite(file, mLtSyncText, length, &wrote) && wrote == length;
		ok = ok && fatfsFileWrite(file, &newline, 1u, &wrote) && wrote == 1u;
	}
	return fatfsFileClose(file) && ok;
}

static void ltPrepareExport(struct LtApp *app)
{
	app->syncPages = ltSyncPageCount(app);
	if (app->syncPage >= app->syncPages)
		app->syncPage = 0u;
	app->exported = ltWriteSyncFile(app);
	app->qrReady = ltBuildSyncText(app, app->syncPage) && qrcodegen_encodeText(mLtSyncText,
		mLtQrTemp, mLtQrCode, qrcodegen_Ecc_LOW, qrcodegen_VERSION_MIN, LT_QR_MAX_VERSION,
		qrcodegen_Mask_AUTO, false);
	app->syncQrSize = app->qrReady ? (uint8_t)qrcodegen_getSize(mLtQrCode) : 0u;
}

static void ltDrawExport(struct LtApp *app)
{
	dcAppDrawClear(&app->draw, ltRgb(4, 12, 22));
	ltHeader(app, "LOCAL DIAGNOSTIC EXPORT");
	if (app->qrReady && app->syncQrSize) {
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
					dcAppDrawFill(&app->draw, left + (x + 4u) * scale, top + (y + 4u) * scale,
						scale, scale, ltRgb(0, 0, 0));
		ltCentered(app, 207, "UNSIGNED DC32 EXPORT - NOT OFFICIAL SYNC", FontSmall, ltRgb(255, 185, 110));
		if (app->exported) {
			char line[48];
			snprintf(line, sizeof(line), "LEFT/RIGHT PAGE %u/%u  A REFRESH  B BACK",
				(unsigned)app->syncPage + 1u, (unsigned)app->syncPages);
			ltCentered(app, 220, line, FontSmall, ltRgb(150, 185, 200));
		}
		else
			ltCentered(app, 220, "EXPORT FAILED - A RETRY - B BACK", FontSmall, ltRgb(255, 95, 85));
	}
	else {
		ltCentered(app, 100, "EXPORT DID NOT FIT", FontLarge, ltRgb(255, 120, 100));
		ltCentered(app, 130, "A RETRY  B BACK", FontMedium, ltRgb(190, 210, 220));
	}
}

static void ltDraw(struct LtApp *app, uint64_t now)
{
	switch (app->view) {
	case LtViewTransmit: ltDrawTransmit(app, now); break;
	case LtViewHistory: ltDrawHistory(app); break;
	case LtViewExport: ltDrawExport(app); break;
	default: ltDrawMonitor(app, now); break;
	}
}

static void ltChangeTransmitValue(struct LtApp *app, int delta)
{
	uint8_t profile = app->save.flags & LT_FLAG_PROFILE_MASK;

	switch (app->txRow) {
	case 0:
		profile = (uint8_t)((profile + (delta < 0 ? 3u : 1u)) & LT_FLAG_PROFILE_MASK);
		app->save.flags = (app->save.flags & ~LT_FLAG_PROFILE_MASK) | profile;
		break;
	case 1:
		if (profile == LtProfileCustom)
			app->save.blockId = (uint8_t)(app->save.blockId + delta);
		break;
	case 2:
		if (profile == LtProfileCustom)
			app->save.deviceId = (uint8_t)(app->save.deviceId + delta);
		break;
	case 3:
		if (profile == LtProfileCustom)
			app->save.mode = (uint8_t)((app->save.mode + (delta < 0 ? 31u : 1u)) & 31u);
		break;
	case 4:
		app->save.data = (uint8_t)((app->save.data + (delta < 0 ? 7u : 1u)) & 7u);
		break;
	case 5:
		if (profile == LtProfileDc33Legacy)
			app->save.legacyVector = (uint8_t)((app->save.legacyVector + (delta < 0 ? 3u : 1u)) & 3u);
		break;
	default:
		break;
	}
	app->armedUntil = 0;
	app->saveError = !ltSave(app);
}

static void ltOpenExport(struct LtApp *app)
{
	app->syncPage = 0u;
	ltPrepareExport(app);
	app->view = LtViewExport;
}

static bool ltInput(struct LtApp *app, uint64_t now)
{
	uint_fast16_t pressed = app->draw.pressed;

	switch (app->view) {
	case LtViewMonitor:
		if (pressed & KEY_BIT_B)
			return false;
		if (pressed & KEY_BIT_A) {
			app->txRow = 0u;
			app->view = LtViewTransmit;
		}
		if (pressed & KEY_BIT_DOWN)
			app->view = LtViewHistory;
		if (pressed & KEY_BIT_START)
			ltOpenExport(app);
		break;
	case LtViewTransmit:
		if (pressed & KEY_BIT_UP)
			app->txRow = (uint8_t)((app->txRow + 5u) % 6u);
		if (pressed & KEY_BIT_DOWN)
			app->txRow = (uint8_t)((app->txRow + 1u) % 6u);
		if (pressed & KEY_BIT_LEFT)
			ltChangeTransmitValue(app, -1);
		if (pressed & KEY_BIT_RIGHT)
			ltChangeTransmitValue(app, 1);
		if (pressed & KEY_BIT_A) {
			if (app->armedUntil > now)
				ltTransmit(app, now);
			else {
				app->armedUntil = now + LT_ARM_TICKS;
				ltSetMessage(app, "CONFIRM MANUAL SEND", 1800u);
			}
		}
		if (pressed & KEY_BIT_B) {
			app->armedUntil = 0;
			app->view = LtViewMonitor;
		}
		break;
	case LtViewHistory:
		if ((pressed & KEY_BIT_UP) && app->historyOffset)
			app->historyOffset--;
		if ((pressed & KEY_BIT_DOWN) && app->historyOffset + 7u < LT_EVENTS &&
			app->save.events[app->historyOffset + 7u].valid)
			app->historyOffset++;
		if (pressed & KEY_BIT_START)
			ltOpenExport(app);
		if (pressed & KEY_BIT_B)
			app->view = LtViewMonitor;
		break;
	case LtViewExport:
		if ((pressed & KEY_BIT_LEFT) && app->syncPage) {
			app->syncPage--;
			ltPrepareExport(app);
		}
		if ((pressed & KEY_BIT_RIGHT) && app->syncPage + 1u < app->syncPages) {
			app->syncPage++;
			ltPrepareExport(app);
		}
		if (pressed & KEY_BIT_A)
			ltPrepareExport(app);
		if (pressed & (KEY_BIT_B | KEY_BIT_START))
			app->view = LtViewMonitor;
		break;
	}
	return true;
}

int laserTagAppRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	struct LtApp app;
	bool loaded;

	if (!host || !args || !args->canvas || !args->vol)
		return -1;
	memset(&app, 0, sizeof(app));
	app.host = host;
	app.args = args;
	mLtAbort = false;
	if (!dcAppDrawInit(&app.draw, host, args, mLtFrame, LT_SCREEN_W, LT_SCREEN_H))
		return -1;
	loaded = ltLoad(&app);
	if (!loaded)
		app.saveError = !ltSave(&app);
	settingsGet(&app.baseSettings);
	ltStartReceiver(&app);
	dcAppDrawWaitRelease(&app.draw, KEY_BIT_A | KEY_BIT_B | KEY_BIT_START | KEY_BIT_SEL |
		KEY_BIT_UP | KEY_BIT_DOWN | KEY_BIT_LEFT | KEY_BIT_RIGHT | UI_KEY_BIT_CENTER);
	while (!mLtAbort && dcAppDrawFrame(&app.draw, UI_KEY_BIT_CENTER)) {
		uint64_t now = host->getTime();
		if (app.cooldownUntil && now >= app.cooldownUntil) {
			app.cooldownUntil = 0;
			badgeLedsApplySettings(&app.baseSettings, true);
		}
		ltReceive(&app, now);
		if (!ltInput(&app, now))
			break;
		ltDraw(&app, now);
	}
	ltStopReceiver(&app);
	badgeLedsApplySettings(&app.baseSettings, true);
	dcAppDrawWaitRelease(&app.draw, KEY_BIT_A | KEY_BIT_B | UI_KEY_BIT_CENTER);
	return 0;
}

void laserTagAppAbort(void)
{
	mLtAbort = true;
	irRemoteOpenLasirEndReceive();
}
