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
#include "pioWS2812.h"
#include "qspi.h"
#include "settings.h"
#include "timebase.h"

#define LT_SCREEN_W 320u
#define LT_SCREEN_H 240u
#define LT_SAVE_MAGIC 0x47544c44u
#define LT_SAVE_VERSION 7u
#define LT_SAVE_PATH "/SAVE/PORTS/LASERTAG.DAT"
#define LT_EVENTS 32u
#define LT_FLAG_PROFILE_MASK 0x07u
#define LT_EVENT_TX 0x01u
#define LT_EVENT_OPENLASIR 0x02u
#define LT_FIRE_MIN_TICKS TICKS_PER_SECOND
#define LT_CONTROL_MIN_TICKS (6ull * TICKS_PER_SECOND)
#define LT_FIRE_WINDOW 30u
#define LT_FIRE_WINDOW_TICKS (60ull * TICKS_PER_SECOND)
#define LT_HIT_LOCKOUT_TICKS (2ull * TICKS_PER_SECOND)
#define LT_HIT_SIGNAL_TICKS (3ull * TICKS_PER_SECOND / 4u)
#define LT_SHOT_SIGNAL_TICKS (TICKS_PER_SECOND / 3u)
#define LT_FIRE_BEEP_TICKS (TICKS_PER_SECOND / 14u)
#define LT_HIT_SIREN_TICKS (3ull * TICKS_PER_SECOND / 5u)
#define LT_HIT_SIREN_STEP_TICKS (TICKS_PER_SECOND / 12u)

enum LtProfile {
	LtProfileTeamFire,
	LtProfileCustom,
	LtProfileMjolnirFire,
	LtProfileMjolnirBlink,
	LtProfileDc33Legacy,
};

enum LtView {
	LtViewMonitor,
	LtViewHistory,
	LtViewScoreboard,
};

enum LtSound {
	LtSoundNone,
	LtSoundFire,
	LtSoundHit,
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
	uint8_t team;
	uint8_t reserved;
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
	uint8_t historyOffset;
	uint64_t lastTxTime;
	uint64_t fireTimes[LT_FIRE_WINDOW];
	uint8_t fireCount;
	uint64_t cooldownUntil;
	uint64_t hitSignalUntil;
	uint64_t shotSignalUntil;
	uint8_t hitColor;
	uint64_t soundUntil;
	uint32_t soundFrequency;
	enum LtSound sound;
	uint64_t nextLedSettingsRefresh;
	uint64_t messageUntil;
	bool receiverActive;
	bool saveError;
	char message[48];
};

static uint8_t mLtFrame[LT_SCREEN_W * LT_SCREEN_H];
static volatile bool mLtAbort;

static const uint8_t mLtColorRgb[][3] = {
	{0, 255, 255}, {255, 0, 255}, {255, 255, 0}, {0, 255, 0},
	{255, 0, 0}, {0, 0, 255}, {255, 165, 0}, {255, 255, 255}
};

/* Exact raw values decoded from Defcon33LaserTag.ir. */
static const uint32_t mLtDc33Raw[] = {0x25da0bf4u, 0x24db0bf4u, 0x23dc0bf4u, 0x22dd0bf4u};
static const char *const mLtDc33Names[] = {"L_TAG_1", "L_TAG_2", "L_TAG_3A", "L_TAG_3B"};

/* Named entries from the upstream OpenLASIR allocation table. */
struct LtKnownDevice {
	uint8_t blockId;
	uint8_t deviceId;
	const char *teamName;
};

static const struct LtKnownDevice mLtKnownDevices[] = {
	{1u, 0u, "TamaBadge"},
	{1u, 1u, "irBot"},
	{1u, 2u, "Array BlastIR"},
	{1u, 3u, "Mj0ln1r by Viking"},
	{1u, 4u, "LaserBag by blorfus"},
	{1u, 5u, "TankThing by Caligo"},
	{1u, 6u, "RedGuy by RedGuy"},
	{1u, 7u, "DC32cfwCyan"},
	{1u, 8u, "DC32cfwMag"},
	{1u, 9u, "DC32cfwYello"},
	{1u, 10u, "DC32cfwGreen"},
	{1u, 11u, "DC32cfwRed"},
	{1u, 12u, "DC32cfwBlue"},
	{1u, 13u, "DC32cfwOrang"},
	{1u, 14u, "DC32cfwWhite"},
	{32u, 0u, "CyanDolphin"},
	{32u, 1u, "MagDolphin"},
	{32u, 2u, "YelloDolphin"},
	{32u, 3u, "GreenDolphin"},
	{32u, 4u, "RedDolphin"},
	{32u, 5u, "BlueDolphin"},
	{32u, 6u, "OrangDolphib"},
	{32u, 7u, "WhiteDolphin"},
	{32u, 8u, "MrUnicorn"},
};

/* DC32 may transmit only from the eight allocations assigned to its teams. */
static const struct LtKnownDevice mLtTeams[] = {
	{1u, 7u, "DC32cfwCyan"},
	{1u, 8u, "DC32cfwMag"},
	{1u, 9u, "DC32cfwYello"},
	{1u, 10u, "DC32cfwGreen"},
	{1u, 11u, "DC32cfwRed"},
	{1u, 12u, "DC32cfwBlue"},
	{1u, 13u, "DC32cfwOrang"},
	{1u, 14u, "DC32cfwWhite"},
};

#define LT_TEAM_COUNT (sizeof(mLtTeams) / sizeof(mLtTeams[0]))
#define LT_LEADERBOARD_QR_SIZE 29u

/* Fixed QR for https://www.dani.pink/lasertag/leaderboard. No runtime QR generator is linked. */
static const char *const mLtLeaderboardQr[LT_LEADERBOARD_QR_SIZE] = {
	"11111110001010101111101111111",
	"10000010101001011000101000001",
	"10111010000010011011101011101",
	"10111010111011110101001011101",
	"10111010010000001011001011101",
	"10000010110111010010101000001",
	"11111110101010101010101111111",
	"00000000011100011001000000000",
	"11111011110101110101010101010",
	"11011101001011001001111110001",
	"10011010101001111110000000000",
	"01111101000010110001110101010",
	"01001010111011110101000001100",
	"00101101010000001011111010001",
	"00010110110110011000110101100",
	"00101001001100111000000110010",
	"11000111101101110101000101100",
	"11101000011010101001111110101",
	"10101111000001010110001100100",
	"10010000010010011010011110010",
	"10110111100011100100111110111",
	"00000000101000001001100011111",
	"11111110110110111001101011100",
	"10000010011100001011100010011",
	"10111010110100000100111110111",
	"10111010100011000010100101111",
	"10111010110001111011111111110",
	"10000010100011001000111101010",
	"11111110100010110100000111100",
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

static uint32_t ltSaveCrc(const struct LtSave *save)
{
	uint64_t uid = flashGetUid();

	return ltCrc32(save, sizeof(*save)) ^ ltCrc32(&uid, sizeof(uid));
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

static const char *ltTeamName(uint8_t blockId, uint8_t deviceId)
{
	for (uint_fast8_t i = 0; i < sizeof(mLtKnownDevices) / sizeof(mLtKnownDevices[0]); i++)
		if (mLtKnownDevices[i].blockId == blockId && mLtKnownDevices[i].deviceId == deviceId)
			return mLtKnownDevices[i].teamName;
	return NULL;
}

static uint8_t ltTeamIndex(const struct LtApp *app)
{
	return app->save.team < LT_TEAM_COUNT ? app->save.team : 0u;
}

static const struct LtKnownDevice *ltTeam(const struct LtApp *app)
{
	return &mLtTeams[ltTeamIndex(app)];
}

static uint32_t ltOpenLasirRaw(uint8_t block, uint8_t device, uint8_t mode, uint8_t data)
{
	return (uint32_t)block | ((uint32_t)(uint8_t)~block << 8) |
		((uint32_t)device << 16) | ((uint32_t)(mode & 31u) << 24) |
		((uint32_t)(data & 7u) << 29);
}

static uint8_t ltRandomTeam(void)
{
	uint64_t uid = flashGetUid();
	uint32_t hash = 0x811c9dc5u;

	for (uint_fast8_t i = 0; i < 8u; i++, uid >>= 8) {
		hash ^= (uint8_t)uid;
		hash *= 0x01000193u;
	}
	return (uint8_t)(hash % LT_TEAM_COUNT);
}

static void ltSetTeam(struct LtApp *app, uint8_t team)
{
	const struct LtKnownDevice *known;

	app->save.team = (uint8_t)(team % LT_TEAM_COUNT);
	known = ltTeam(app);
	app->save.flags = (app->save.flags & ~LT_FLAG_PROFILE_MASK) | LtProfileTeamFire;
	app->save.blockId = known->blockId;
	app->save.deviceId = known->deviceId;
	app->save.mode = 0u;
	app->save.data = app->save.team;
}

static void ltDefaults(struct LtApp *app)
{
	memset(&app->save, 0, sizeof(app->save));
	app->save.magic = LT_SAVE_MAGIC;
	app->save.version = LT_SAVE_VERSION;
	app->save.size = sizeof(app->save);
	app->save.flags = LtProfileTeamFire;
	ltSetTeam(app, ltRandomTeam());
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
	uint16_t version;
	bool changed = false;

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
	version = app->save.version;
	if (app->save.magic != LT_SAVE_MAGIC || (version != 5u && version != 6u && version != LT_SAVE_VERSION) ||
		app->save.size != sizeof(app->save) || expected != ltSaveCrc(&app->save)) {
		ltDefaults(app);
		return false;
	}
	app->save.crc = expected;
	if (version == 5u) {
		ltSetTeam(app, ltRandomTeam());
		changed = true;
	}
	if (version < 7u) {
		/* Earlier RX included all NEC packets; Hits now means accepted OpenLASIR fire packets. */
		app->save.rxCount = 0u;
		changed = true;
	}
	if (app->save.team >= LT_TEAM_COUNT) {
		ltSetTeam(app, ltRandomTeam());
		changed = true;
	}
	if ((app->save.flags & LT_FLAG_PROFILE_MASK) != LtProfileTeamFire) {
		ltSetTeam(app, ltTeamIndex(app));
		changed = true;
	}
	return !changed;
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

static void ltFnSettingValue(void *context, uint8_t index, char *dst, uint32_t dstSize)
{
	struct LtApp *app = context;

	if (!dstSize)
		return;
	if (index == 0u)
		snprintf(dst, dstSize, "%s (D%u)", app ? ltTeam(app)->teamName : "",
			(unsigned)(app ? ltTeam(app)->deviceId : 0u));
}

static void ltFnSettingAdjust(void *context, uint8_t index, int8_t direction)
{
	struct LtApp *app = context;

	if (!app)
		return;
	if (index == 0u && direction) {
		ltSetTeam(app, (uint8_t)(ltTeamIndex(app) +
			(direction < 0 ? LT_TEAM_COUNT - 1u : 1u)));
		ltSetMessage(app, ltTeam(app)->teamName, 1000u);
	}
	else
		return;
	app->saveError = !ltSave(app);
}

static const char *const mLtFnSettingLabels[] = {
	"TEAM",
};
static const struct UiFnSettings mLtFnSettings = {
	.title = "OpenLasir Settings",
	.menuLabel = "OpenLasir Settings",
	.count = sizeof(mLtFnSettingLabels) / sizeof(mLtFnSettingLabels[0]),
	.labels = mLtFnSettingLabels,
	.value = ltFnSettingValue,
	.adjust = ltFnSettingAdjust,
};

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
	const struct LtKnownDevice *team = ltTeam(app);

	*openLasir = true;
	return ltOpenLasirRaw(team->blockId, team->deviceId, 0u, ltTeamIndex(app));
}

static uint8_t ltScaleLed(uint8_t value, uint8_t scale)
{
	return (uint8_t)(((uint32_t)value * scale + 127u) / 255u);
}

static void ltSetLed(uint8_t led, const uint8_t *rgb, uint8_t scale)
{
	ws2812SetRgb(led, ltScaleLed(rgb[0], scale), ltScaleLed(rgb[1], scale),
		ltScaleLed(rgb[2], scale));
}

static uint8_t ltLedSpeedFactor(uint8_t speed)
{
	static const uint8_t factors[] = {0u, 3u, 5u, 7u, 10u, 12u, 15u, 17u, 19u, 22u, 24u};

	return speed >= 1u && speed <= 10u ? factors[speed] : factors[4u];
}

static void ltRenderLeds(struct LtApp *app, uint64_t now)
{
	uint8_t brightness, speedFactor;
	uint64_t dotPeriod, hitAlternateTicks;
	const uint8_t *teamRgb = mLtColorRgb[ltTeamIndex(app)];
	const uint8_t *opponentRgb = mLtColorRgb[app->hitColor & 7u];

	/* The Fn menu pauses this loop; refresh the saved LED settings on return. */
	if (!app->nextLedSettingsRefresh || now >= app->nextLedSettingsRefresh) {
		settingsGet(&app->baseSettings);
		app->nextLedSettingsRefresh = now + TICKS_PER_SECOND / 4u;
	}
	brightness = app->baseSettings.ledBrightness;
	speedFactor = ltLedSpeedFactor(app->baseSettings.ledSpeed);
	dotPeriod = TICKS_PER_SECOND / speedFactor;
	hitAlternateTicks = TICKS_PER_SECOND / speedFactor;
	uint8_t dot = (uint8_t)((now / dotPeriod) % NUM_WS2812s);
	uint8_t neighbor = dot ? dot - 1u : NUM_WS2812s - 1u;
	bool hit = now < app->hitSignalUntil;
	bool cooldown = now < app->cooldownUntil;
	bool shot = now < app->shotSignalUntil;
	bool hitFlashOn = hit && (((app->hitSignalUntil - now) / hitAlternateTicks & 1u) == 0u);

	for (uint8_t led = 0; led < NUM_WS2812s; led++) {
		if (hit)
			ltSetLed(led, opponentRgb, hitFlashOn ? brightness : 0u);
		else if (cooldown) {
			ltSetLed(led, opponentRgb, ltScaleLed(led == dot ? 180u : led == neighbor ? 75u : 0u, brightness));
		}
		else if (shot) {
			uint8_t tail = dot < 2u ? (uint8_t)(dot + NUM_WS2812s - 2u) : dot - 2u;
			ltSetLed(led, teamRgb, ltScaleLed(led == dot ? 255u : led == neighbor ? 100u : led == tail ? 28u : 0u, brightness));
		}
		else
			ltSetLed(led, teamRgb, ltScaleLed(led == dot ? 72u : led == neighbor ? 26u : 6u, brightness));
	}
	badgeLedsGameWrite();
	ws2812refresh();
}

static void ltStartSound(struct LtApp *app, enum LtSound sound, uint64_t now)
{
	if (app->baseSettings.audioMuted)
		return;
	app->sound = sound;
	app->soundFrequency = 0u;
	app->soundUntil = now + (sound == LtSoundHit ? LT_HIT_SIREN_TICKS : LT_FIRE_BEEP_TICKS);
}

static void ltUpdateSound(struct LtApp *app, uint64_t now)
{
	uint32_t frequency;

	if (app->sound == LtSoundNone)
		return;
	if (app->baseSettings.audioMuted || now >= app->soundUntil) {
		audioPwmStop();
		app->sound = LtSoundNone;
		app->soundFrequency = 0u;
		return;
	}
	if (app->sound == LtSoundFire)
		frequency = 1240u;
	else if (now + 2u * LT_HIT_SIREN_STEP_TICKS < app->soundUntil)
		frequency = ((app->soundUntil - now) / LT_HIT_SIREN_STEP_TICKS & 1u) ? 680u : 1120u;
	else
		frequency = 170u;
	if (frequency != app->soundFrequency && audioPwmTone(frequency))
		app->soundFrequency = frequency;
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
	if (openLasir && mode == 0u) {
		app->fireTimes[app->fireCount++] = now;
		app->shotSignalUntil = now + LT_SHOT_SIGNAL_TICKS;
		ltStartSound(app, LtSoundFire, now);
	}
	app->save.txCount++;
	ltLog(app, raw, true, openLasir, now);
	app->saveError = !ltSave(app);
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

		ltLog(app, frame.raw, false, openLasir, now);
		if (openLasir) {
			const char *teamName = ltTeamName(frame.blockId, frame.deviceId);

			if (teamName)
				snprintf(message, sizeof(message), "%s %s", teamName, ltModeName(frame.mode));
			else
				snprintf(message, sizeof(message), "%s B%u D%u", ltModeName(frame.mode),
					(unsigned)frame.blockId, (unsigned)frame.deviceId);
			if (frame.mode == 0u) {
				/* The OpenLasir data bits always carry the shot color, even for unknown IDs. */
				app->save.rxCount++;
				app->hitColor = frame.data & 7u;
				app->hitSignalUntil = now + LT_HIT_SIGNAL_TICKS;
				app->cooldownUntil = now + LT_HIT_LOCKOUT_TICKS;
				ltStartSound(app, LtSoundHit, now);
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

static void ltDescribeDevice(uint8_t block, uint8_t device, char *line, uint32_t lineSize)
{
	const char *teamName = ltTeamName(block, device);

	if (teamName)
		snprintf(line, lineSize, "%s", teamName);
	else
		snprintf(line, lineSize, "B%u D%u", (unsigned)block, (unsigned)device);
}

static void ltDescribeRaw(uint32_t raw, bool openLasir, char *line, uint32_t lineSize)
{
	if (openLasir) {
		uint8_t block = (uint8_t)raw;
		uint8_t device = (uint8_t)(raw >> 16);
		uint8_t mode = (uint8_t)(raw >> 24) & 31u;
		char deviceName[40];

		ltDescribeDevice(block, device, deviceName, sizeof(deviceName));
		snprintf(line, lineSize, "%s %s", deviceName, ltModeName(mode));
	}
	else
		snprintf(line, lineSize, "%s", ltLegacyName(raw));
}

static void ltDescribeActivity(const struct LtEvent *event, char *line, uint32_t lineSize)
{
	bool openLasir = event->flags & LT_EVENT_OPENLASIR;
	bool shot = false;
	char deviceName[40];

	if (openLasir) {
		uint8_t block = (uint8_t)event->raw;
		uint8_t device = (uint8_t)(event->raw >> 16);
		uint8_t mode = (uint8_t)(event->raw >> 24) & 31u;

		ltDescribeDevice(block, device, deviceName, sizeof(deviceName));
		shot = mode == 0u;
		if (event->flags & LT_EVENT_TX)
			snprintf(line, lineSize, "Shots fired by %s", deviceName);
		else if (shot)
			snprintf(line, lineSize, "Hit by %s", deviceName);
		else
			snprintf(line, lineSize, "%s from %s", ltModeName(mode), deviceName);
	}
	else if (event->flags & LT_EVENT_TX)
		snprintf(line, lineSize, "Legacy NEC fired");
	else
		snprintf(line, lineSize, "Legacy NEC received");
}

static void ltDrawMonitor(struct LtApp *app)
{
	char line[72];

	dcAppDrawClear(&app->draw, ltRgb(4, 12, 22));
	ltHeader(app, "OPENLASIR TAG");
	ltCentered(app, 39, "SHOTS FIRED", FontSmall, ltRgb(145, 185, 200));
	snprintf(line, sizeof(line), "%lu", (unsigned long)app->save.txCount);
	ltCentered(app, 52, line, FontLarge, ltRgb(120, 220, 170));
	ltCentered(app, 82, "HITS RECEIVED", FontSmall, ltRgb(145, 185, 200));
	snprintf(line, sizeof(line), "%lu", (unsigned long)app->save.rxCount);
	ltCentered(app, 95, line, FontLarge, ltRgb(120, 190, 240));
	ltCentered(app, 128, "LAST ACTIVITY", FontSmall, ltRgb(105, 210, 220));
	if (app->save.events[0].valid) {
		struct LtEvent *event = &app->save.events[0];
		ltDescribeActivity(event, line, sizeof(line));
		ltCentered(app, 144, line, FontMedium, ltRgb(255, 210, 145));
	}
	else
		ltCentered(app, 144, "NO ACTIVITY YET", FontMedium, ltRgb(145, 175, 190));
	ltCentered(app, 187, "A: FIRE        START: HISTORY", FontSmall, ltRgb(145, 185, 200));
	ltCentered(app, 204, "SELECT: TEAM & SCORES", FontSmall, ltRgb(145, 185, 200));
	ltCentered(app, 221, "FN: OPTIONS / EXIT", FontSmall, ltRgb(145, 185, 200));
}

static void ltDrawHistory(struct LtApp *app)
{
	char line[72];
	uint_fast8_t row = 0;

	dcAppDrawClear(&app->draw, ltRgb(4, 12, 22));
	ltHeader(app, "PERSONAL EVENT LOG");
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
	ltCentered(app, 204, "UP/DOWN: SCROLL", FontSmall, ltRgb(145, 185, 200));
	ltCentered(app, 221, "B OR SELECT: BACK", FontSmall, ltRgb(145, 185, 200));
}

static void ltDrawScoreboard(struct LtApp *app)
{
	char line[56];
	const uint32_t scale = 4u;
	const uint32_t border = 4u;
	const uint32_t modules = LT_LEADERBOARD_QR_SIZE + 2u * border;
	const uint32_t size = modules * scale;
	const uint32_t left = (LT_SCREEN_W - size) / 2u;
	const uint32_t top = 48u;

	dcAppDrawClear(&app->draw, ltRgb(4, 12, 22));
	ltHeader(app, "TEAM & SCOREBOARD");
	snprintf(line, sizeof(line), "TEAM: %s", ltTeam(app)->teamName);
	ltCentered(app, 33, line, FontSmall, ltRgb(210, 230, 238));
	dcAppDrawFill(&app->draw, left, top, size, size, ltRgb(255, 255, 255));
	for (uint_fast8_t y = 0; y < LT_LEADERBOARD_QR_SIZE; y++)
		for (uint_fast8_t x = 0; x < LT_LEADERBOARD_QR_SIZE; x++)
			if (mLtLeaderboardQr[y][x] == '1')
				dcAppDrawFill(&app->draw, left + (x + border) * scale, top + (y + border) * scale,
					scale, scale, ltRgb(0, 0, 0));
	ltCentered(app, 205, "SCAN FOR TEAM SCORES", FontSmall, ltRgb(255, 210, 145));
	ltCentered(app, 218, "dani.pink/lasertag/leaderboard", FontSmall, ltRgb(145, 185, 200));
	ltCentered(app, 231, "B OR SELECT: BACK", FontSmall, ltRgb(145, 185, 200));
}

static void ltDraw(struct LtApp *app)
{
	switch (app->view) {
	case LtViewHistory: ltDrawHistory(app); break;
	case LtViewScoreboard: ltDrawScoreboard(app); break;
	default: ltDrawMonitor(app); break;
	}
}

static bool ltInput(struct LtApp *app, uint64_t now)
{
	uint_fast16_t pressed = app->draw.pressed;

	switch (app->view) {
	case LtViewMonitor:
		if (pressed & KEY_BIT_A)
			ltTransmit(app, now);
		if (pressed & KEY_BIT_START)
			app->view = LtViewHistory;
		if (pressed & KEY_BIT_SEL)
			app->view = LtViewScoreboard;
		break;
	case LtViewHistory:
		if ((pressed & KEY_BIT_UP) && app->historyOffset)
			app->historyOffset--;
		if ((pressed & KEY_BIT_DOWN) && app->historyOffset + 7u < LT_EVENTS &&
			app->save.events[app->historyOffset + 7u].valid)
			app->historyOffset++;
		if (pressed & (KEY_BIT_B | KEY_BIT_SEL))
			app->view = LtViewMonitor;
		break;
	case LtViewScoreboard:
		if (pressed & (KEY_BIT_B | KEY_BIT_SEL))
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
	if (args->toolAction == DcAppToolActionLaserTagSettings) {
		struct UiFnSettings settings = mLtFnSettings;

		settings.directOpen = true;
		if (host->setFnSettings && host->portMenu) {
			host->setFnSettings(&settings, &app);
			(void)host->portMenu(args->canvas);
			host->setFnSettings(NULL, NULL);
		}
		return 0;
	}
	if (host->setFnSettings)
		host->setFnSettings(&mLtFnSettings, &app);
	settingsGet(&app.baseSettings);
	ltStartReceiver(&app);
	ltRenderLeds(&app, host->getTime());
	dcAppDrawWaitRelease(&app.draw, KEY_BIT_A | KEY_BIT_B | KEY_BIT_START | KEY_BIT_SEL |
		KEY_BIT_UP | KEY_BIT_DOWN | KEY_BIT_LEFT | KEY_BIT_RIGHT | UI_KEY_BIT_CENTER);
	while (!mLtAbort && dcAppDrawFrame(&app.draw, UI_KEY_BIT_CENTER)) {
		uint64_t now = host->getTime();
		if (app.draw.returnedFromPortMenu) {
			settingsGet(&app.baseSettings);
			app.nextLedSettingsRefresh = now + TICKS_PER_SECOND / 4u;
		}
		if (app.cooldownUntil && now >= app.cooldownUntil) {
			app.cooldownUntil = 0;
			ltStartReceiver(&app);
		}
		ltReceive(&app, now);
		if (!ltInput(&app, now))
			break;
		ltUpdateSound(&app, now);
		ltRenderLeds(&app, now);
		ltDraw(&app);
	}
	ltStopReceiver(&app);
	audioPwmStop();
	settingsGet(&app.baseSettings);
	badgeLedsApplySettings(&app.baseSettings, true);
	if (host->setFnSettings)
		host->setFnSettings(NULL, NULL);
	dcAppDrawWaitRelease(&app.draw, KEY_BIT_A | KEY_BIT_B | UI_KEY_BIT_CENTER);
	return 0;
}

void laserTagAppAbort(void)
{
	mLtAbort = true;
	irRemoteOpenLasirEndReceive();
}
