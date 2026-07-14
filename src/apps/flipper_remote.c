#include "apps/flipper_remote.h"
#include "apps/flipper_usb_host.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "dispDefcon.h"
#include "fonts.h"
#include "gb.h"
#include "raster.h"
#include "timebase.h"
#include "ui.h"
#include "usbCdc.h"

#define FRC_MAGIC "FRC1"
#define FRC_VERSION 1u
#define FRC_HEADER_SIZE 20u
#define FRC_MAX_PAYLOAD 1100u
#define FRC_FRAME_BYTES 1024u
#define FRC_FRAME_PAYLOAD (FRC_FRAME_BYTES + 1u)
#define FRC_STATUS_MAX 63u
#define FRC_HELLO_PERIOD_TICKS TICKS_PER_SECOND
#define FRC_LOCAL_EXIT_TICKS TICKS_PER_SECOND
#define FRC_LONG_TICKS (TICKS_PER_SECOND * 350u / 1000u)
#define FRC_REPEAT_TICKS (TICKS_PER_SECOND * 150u / 1000u)

enum FrcPacketType {
	FrcPacketHello = 1,
	FrcPacketReady = 2,
	FrcPacketFrame = 3,
	FrcPacketInput = 4,
	FrcPacketUnlock = 5,
	FrcPacketStatus = 6,
	FrcPacketStream = 7,
};

enum FlipperInputKey {
	FlipperInputUp = 0,
	FlipperInputDown,
	FlipperInputRight,
	FlipperInputLeft,
	FlipperInputOk,
	FlipperInputBack,
};

enum FlipperInputType {
	FlipperInputPress = 0,
	FlipperInputRelease,
	FlipperInputShort,
	FlipperInputLong,
	FlipperInputRepeat,
};

struct FrcStream {
	uint8_t header[FRC_HEADER_SIZE];
	uint32_t headerUsed;
	uint32_t payloadLength;
	uint32_t payloadUsed;
	uint32_t sequence;
	uint32_t crc;
	uint8_t type;
	bool receivingPayload;
};

struct FlipperRemote {
	const struct DcAppHostApi *host;
	struct Canvas canvas;
	struct FrcStream stream;
	uint8_t payload[FRC_MAX_PAYLOAD];
	uint8_t frame[FRC_FRAME_BYTES];
	uint_fast16_t previousKeys;
	uint64_t centerDownAt;
	uint64_t lastHelloAt;
	uint64_t keyDownAt[6];
	uint64_t nextRepeatAt[6];
	uint32_t outgoingSequence;
	bool keyLong[6];
	bool bridgeReady;
	bool haveFrame;
	bool streamPaused;
	bool statusDirty;
	uint8_t orientation;
	char status[FRC_STATUS_MAX + 1u];
	char statusState[32];
	char statusDetail[FRC_STATUS_MAX + 1u];
};

static volatile bool mFlipperRemoteAbort;

void flipperRemoteAbort(void)
{
	mFlipperRemoteAbort = true;
}

static uint32_t frcReadLe32(const uint8_t *src)
{
	return (uint32_t)src[0] | ((uint32_t)src[1] << 8) |
		((uint32_t)src[2] << 16) | ((uint32_t)src[3] << 24);
}

static void frcWriteLe32(uint8_t *dst, uint32_t value)
{
	dst[0] = (uint8_t)value;
	dst[1] = (uint8_t)(value >> 8);
	dst[2] = (uint8_t)(value >> 16);
	dst[3] = (uint8_t)(value >> 24);
}

static uint32_t frcCrc32(const uint8_t *data, uint32_t size)
{
	uint32_t crc = 0xffffffffu;
	uint32_t original = size;

	while (size--) {
		crc ^= *data++;
		for (uint_fast8_t bit = 0; bit < 8u; bit++)
			crc = (crc >> 1) ^ (0xedb88320u & (uint32_t)-(int32_t)(crc & 1u));
	}
	return original ? ~crc : 0u;
}

static void frcText(const struct FlipperRemote *app, int32_t x, int32_t y, const char *text, enum Font font, uint16_t color)
{
	while (text && *text) {
		struct FontGlyphInfo glyph;
		if (fontGetGlyphInfo(&glyph, font, (unsigned char)*text)) {
			for (uint_fast8_t row = 0; row < glyph.height; row++)
				for (uint_fast8_t col = 0; col < glyph.width; col++)
					if (fontGetGlyphPixel(&glyph, row, col))
						rasterPutPixel(&app->canvas, (uint32_t)(x + col), (uint32_t)(y + row), color);
			x += glyph.width + 1;
		}
		text++;
	}
}

static uint32_t frcTextWidth(const char *text, enum Font font)
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

static void frcCentered(const struct FlipperRemote *app, int32_t y, const char *text, enum Font font, uint16_t color)
{
	uint32_t width = frcTextWidth(text, font);
	int32_t x = (int32_t)(app->canvas.w > width ? (app->canvas.w - width) / 2u : 0u);
	frcText(app, x, y, text, font, color);
}

static void frcSetStatusView(struct FlipperRemote *app, const char *state, const char *detail)
{
	if (!strcmp(app->statusState, state) && !strcmp(app->statusDetail, detail))
		return;
	strncpy(app->statusState, state, sizeof(app->statusState) - 1u);
	app->statusState[sizeof(app->statusState) - 1u] = 0;
	strncpy(app->statusDetail, detail, sizeof(app->statusDetail) - 1u);
	app->statusDetail[sizeof(app->statusDetail) - 1u] = 0;
	app->statusDirty = true;
}

static void frcPresentStatus(const struct FlipperRemote *app)
{
	dispPrvFrameCtrWait();
	dispPrvWaitForScanoutStart();
	rasterClear(&app->canvas, rasterRgb565(0, 0, 0));
	frcCentered(app, 34, "FLIPPER REMOTE", FontLarge, rasterRgb565(255, 170, 35));
	frcCentered(app, 92, app->statusState, FontMedium, rasterRgb565(245, 235, 210));
	if (app->statusDetail[0])
		frcCentered(app, 122, app->statusDetail, FontSmall, rasterRgb565(190, 165, 115));
	frcCentered(app, 206, "HOLD FN TO EXIT", FontSmall, rasterRgb565(145, 120, 75));
}

static bool frcSendPacket(struct FlipperRemote *app, uint8_t type, const uint8_t *payload, uint32_t payloadSize)
{
	uint8_t header[FRC_HEADER_SIZE] = {0};
	uint32_t total = FRC_HEADER_SIZE + payloadSize;

	if (payloadSize > 64u || usbCdcWriteAvailable() < total)
		return false;
	memcpy(header, FRC_MAGIC, 4u);
	header[4] = FRC_VERSION;
	header[5] = type;
	frcWriteLe32(header + 8u, ++app->outgoingSequence);
	frcWriteLe32(header + 12u, payloadSize);
	frcWriteLe32(header + 16u, frcCrc32(payload, payloadSize));
	if (usbCdcWrite(header, sizeof(header)) != sizeof(header) ||
		(payloadSize && usbCdcWrite(payload, payloadSize) != payloadSize))
		return false;
	usbCdcFlush();
	return true;
}

static void frcSendHello(struct FlipperRemote *app)
{
	uint8_t payload[8] = {0};

	payload[0] = 128u;
	payload[2] = 64u;
	payload[4] = (uint8_t)app->canvas.w;
	payload[5] = (uint8_t)(app->canvas.w >> 8);
	payload[6] = (uint8_t)app->canvas.h;
	payload[7] = (uint8_t)(app->canvas.h >> 8);
	(void)frcSendPacket(app, FrcPacketHello, payload, sizeof(payload));
}

static void frcSendInput(struct FlipperRemote *app, uint8_t key, uint8_t type)
{
	uint8_t payload[2] = {key, type};
	if (app->bridgeReady)
		(void)frcSendPacket(app, FrcPacketInput, payload, sizeof(payload));
}

static void frcRenderFrame(struct FlipperRemote *app)
{
	uint32_t viewW = app->orientation < 2u ? 128u : 64u;
	uint32_t viewH = app->orientation < 2u ? 64u : 128u;
	struct RasterRect fit = rasterFit(viewW, viewH, app->canvas.w, app->canvas.h);
	uint16_t foreground = rasterRgb565(255, 180, 50);

	dispPrvWaitForScanoutStart();
	rasterClear(&app->canvas, rasterRgb565(0, 0, 0));
	for (uint32_t y = 0; y < viewH; y++) {
		for (uint32_t x = 0; x < viewW; x++) {
			uint32_t sx;
			uint32_t sy;
			bool on;

			switch (app->orientation) {
			case 1: sx = 127u - x; sy = 63u - y; break;
			case 2: sx = 127u - y; sy = x; break;
			case 3: sx = y; sy = 63u - x; break;
			default: sx = x; sy = y; break;
			}
			on = (app->frame[(sy >> 3u) * 128u + sx] & (1u << (sy & 7u))) != 0u;
			if (on)
				rasterDrawScaledPixel(&app->canvas, &fit, viewW, viewH, x, y, foreground);
		}
	}
	app->haveFrame = true;
}

static void frcHandlePacket(struct FlipperRemote *app)
{
	struct FrcStream *stream = &app->stream;

	if (stream->crc != frcCrc32(app->payload, stream->payloadLength)) {
		frcSetStatusView(app, "Bridge error", "Bad packet CRC");
		return;
	}
	if (stream->type == FrcPacketReady && stream->payloadLength == 0u) {
		app->bridgeReady = true;
		frcSetStatusView(app, "Connected", "Waiting for Flipper");
	}
	else if (stream->type == FrcPacketStatus) {
		uint32_t length = stream->payloadLength > FRC_STATUS_MAX ? FRC_STATUS_MAX : stream->payloadLength;
		memcpy(app->status, app->payload, length);
		app->status[length] = 0;
		frcSetStatusView(app, "Flipper", app->status);
	}
	else if (stream->type == FrcPacketFrame && stream->payloadLength == FRC_FRAME_PAYLOAD && app->payload[0] < 4u) {
		app->orientation = app->payload[0];
		memcpy(app->frame, app->payload + 1u, FRC_FRAME_BYTES);
		if (!app->streamPaused)
			frcRenderFrame(app);
	}
}

static void frcResetStream(struct FrcStream *stream)
{
	memset(stream, 0, sizeof(*stream));
}

static void frcConsumeByte(struct FlipperRemote *app, uint8_t value)
{
	struct FrcStream *stream = &app->stream;

	if (!stream->receivingPayload) {
		stream->header[stream->headerUsed++] = value;
		while (stream->headerUsed >= 4u && memcmp(stream->header, FRC_MAGIC, 4u))
			memmove(stream->header, stream->header + 1u, --stream->headerUsed);
		if (stream->headerUsed != FRC_HEADER_SIZE)
			return;
		if (stream->header[4] != FRC_VERSION) {
			memmove(stream->header, stream->header + 1u, --stream->headerUsed);
			return;
		}
		stream->type = stream->header[5];
		stream->sequence = frcReadLe32(stream->header + 8u);
		stream->payloadLength = frcReadLe32(stream->header + 12u);
		stream->crc = frcReadLe32(stream->header + 16u);
		if (stream->payloadLength > FRC_MAX_PAYLOAD) {
			frcResetStream(stream);
			frcSetStatusView(app, "Bridge error", "Packet too large");
			return;
		}
		stream->payloadUsed = 0;
		stream->receivingPayload = stream->payloadLength != 0u;
		if (!stream->receivingPayload) {
			frcHandlePacket(app);
			frcResetStream(stream);
		}
		return;
	}
	app->payload[stream->payloadUsed++] = value;
	if (stream->payloadUsed == stream->payloadLength) {
		frcHandlePacket(app);
		frcResetStream(stream);
	}
}

static void frcReadUsb(struct FlipperRemote *app)
{
	uint8_t bytes[512];
	while (usbCdcAvailable()) {
		uint32_t got = usbCdcRead(bytes, sizeof(bytes));
		if (!got)
			break;
		for (uint32_t i = 0; i < got; i++)
			frcConsumeByte(app, bytes[i]);
	}
}

static bool frcShouldExit(struct FlipperRemote *app, uint_fast16_t keys)
{
	uint64_t now = app->host->getTime();
	bool center = (keys & UI_KEY_BIT_CENTER) != 0u;
	if (center && !app->centerDownAt)
		app->centerDownAt = now;
	if (!center)
		app->centerDownAt = 0u;
	return center && app->centerDownAt && now - app->centerDownAt >= FRC_LOCAL_EXIT_TICKS;
}

static void frcForwardInput(struct FlipperRemote *app, uint_fast16_t keys, uint64_t now)
{
	static const struct { uint_fast16_t local; uint8_t remote; } map[] = {
		{KEY_BIT_UP, FlipperInputUp}, {KEY_BIT_DOWN, FlipperInputDown}, {KEY_BIT_RIGHT, FlipperInputRight},
		{KEY_BIT_LEFT, FlipperInputLeft}, {KEY_BIT_A, FlipperInputOk}, {KEY_BIT_B, FlipperInputBack},
	};

	for (uint_fast8_t i = 0; i < sizeof(map) / sizeof(*map); i++) {
		bool pressed = (keys & map[i].local) != 0u;
		bool wasPressed = (app->previousKeys & map[i].local) != 0u;
		if (pressed && !wasPressed) {
			app->keyDownAt[i] = now;
			app->nextRepeatAt[i] = now + FRC_LONG_TICKS + FRC_REPEAT_TICKS;
			app->keyLong[i] = false;
			frcSendInput(app, map[i].remote, FlipperInputPress);
		}
		else if (!pressed && wasPressed) {
			frcSendInput(app, map[i].remote, FlipperInputRelease);
			if (!app->keyLong[i])
				frcSendInput(app, map[i].remote, FlipperInputShort);
			app->keyDownAt[i] = app->nextRepeatAt[i] = 0u;
		}
		else if (pressed && app->keyDownAt[i]) {
			if (!app->keyLong[i] && now - app->keyDownAt[i] >= FRC_LONG_TICKS) {
				app->keyLong[i] = true;
				frcSendInput(app, map[i].remote, FlipperInputLong);
			}
			if (app->keyLong[i] && now >= app->nextRepeatAt[i]) {
				frcSendInput(app, map[i].remote, FlipperInputRepeat);
				app->nextRepeatAt[i] += FRC_REPEAT_TICKS;
			}
		}
	}
	if ((keys & KEY_BIT_START) && !(app->previousKeys & KEY_BIT_START))
		(void)frcSendPacket(app, FrcPacketUnlock, NULL, 0u);
	if ((keys & KEY_BIT_SEL) && !(app->previousKeys & KEY_BIT_SEL)) {
		uint8_t enabled;
		app->streamPaused = !app->streamPaused;
		enabled = app->streamPaused ? 0u : 1u;
		(void)frcSendPacket(app, FrcPacketStream, &enabled, 1u);
	}
	app->previousKeys = keys;
}

static int flipperRemoteBridgeRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	struct FlipperRemote app;
	struct UsbCdcDeviceInfo info;

	if (!host || !host->displayFb || !host->getTime)
		return 1;
	memset(&app, 0, sizeof(app));
	mFlipperRemoteAbort = false;
	app.host = host;
	if (args && args->canvas)
		app.canvas = *args->canvas;
	if (!app.canvas.framebuffer)
		app.canvas.framebuffer = host->displayFb();
	if (!app.canvas.w)
		app.canvas.w = 320u;
	if (!app.canvas.h)
		app.canvas.h = 240u;
	app.canvas.bpp = 16u;
	if (args && args->rotate)
		app.canvas.flipped = 1u;
	usbCdcDefaultInfo(&info);
	if (!usbCdcBegin(&info))
		return 1;

	while (!mFlipperRemoteAbort) {
		uint_fast16_t keys;
		uint64_t now;

		usbCdcTask();
		if (!usbCdcConnected()) {
			app.bridgeReady = false;
			app.haveFrame = false;
			app.lastHelloAt = 0u;
			frcResetStream(&app.stream);
			frcSetStatusView(&app, "Connect host USB", "Waiting for CDC host");
		}
		else {
			now = host->getTime();
			if (!app.bridgeReady && (!app.lastHelloAt || now - app.lastHelloAt >= FRC_HELLO_PERIOD_TICKS)) {
				frcSendHello(&app);
				app.lastHelloAt = now;
			}
			frcReadUsb(&app);
			if (!app.haveFrame)
				frcSetStatusView(&app, app.bridgeReady ? "Connected" : "USB connected",
					app.bridgeReady ? (app.status[0] ? app.status : "Waiting for Flipper") : "Waiting for bridge");
		}
		if (!app.haveFrame && app.statusDirty) {
			frcPresentStatus(&app);
			app.statusDirty = false;
		}
		keys = host->uiKeysRaw ? host->uiKeysRaw() : 0u;
		if (frcShouldExit(&app, keys))
			break;
		frcForwardInput(&app, keys, host->getTime());
		if (host->ledsTick)
			host->ledsTick();
		if (host->idleWaitMsec)
			host->idleWaitMsec(1u);
		else if (host->delayMsec)
			host->delayMsec(1u);
	}
	usbCdcEnd();
	return 0;
}

enum DirectState { DirectWaitMount, DirectWaitPrompt, DirectWaitRpcEcho, DirectRpc };
struct DirectRemote {
	struct FlipperRemote view;
	enum DirectState state;
	uint8_t rx[2048], tx[2048];
	uint32_t rxUsed, txUsed, commandId;
	uint_fast16_t previousKeys;
	uint64_t downAt[6], nextRepeatAt[6], centerDownAt;
	bool keyLong[6], paused;
};

static uint32_t directVarint(const uint8_t *src, uint32_t size, uint32_t *value)
{
	uint32_t result = 0u;
	for (uint32_t i = 0; i < size && i < 5u; i++) {
		result |= (uint32_t)(src[i] & 0x7fu) << (i * 7u);
		if (!(src[i] & 0x80u)) { *value = result; return i + 1u; }
	}
	return 0u;
}

static uint32_t directPutVarint(uint8_t *dst, uint32_t value)
{
	uint32_t used = 0u;
	do { dst[used] = (uint8_t)(value & 0x7fu); value >>= 7u; if (value) dst[used] |= 0x80u; used++; } while (value);
	return used;
}

static bool directQueue(struct DirectRemote *app, const uint8_t *data, uint32_t size)
{
	if (size > sizeof(app->tx) - app->txUsed)
		return false;
	memcpy(app->tx + app->txUsed, data, size);
	app->txUsed += size;
	return true;
}

static void directSendMain(struct DirectRemote *app, uint32_t tag, const uint8_t *content, uint32_t contentSize)
{
	uint8_t body[128];
	uint32_t used = 0u, id = ++app->commandId;
	used += directPutVarint(body + used, 8u); used += directPutVarint(body + used, id);
	used += directPutVarint(body + used, (tag << 3u) | 2u); used += directPutVarint(body + used, contentSize);
	if (contentSize && used + contentSize <= sizeof(body)) { memcpy(body + used, content, contentSize); used += contentSize; }
	else if (contentSize) return;
	uint8_t prefix[5];
	uint32_t prefixUsed = directPutVarint(prefix, used);
	(void)directQueue(app, prefix, prefixUsed);
	(void)directQueue(app, body, used);
}

static void directSendInput(struct DirectRemote *app, uint8_t key, uint8_t type)
{
	uint8_t content[4] = {8u, key, 16u, type};
	if (app->state == DirectRpc)
		directSendMain(app, 23u, content, sizeof(content));
}

static void directHandleMessage(struct DirectRemote *app, const uint8_t *data, uint32_t size)
{
	uint32_t offset = 0u, tag = 0u, contentOffset = 0u, contentSize = 0u;
	while (offset < size) {
		uint32_t key, value, used = directVarint(data + offset, size - offset, &key);
		if (!used)
			return;
		offset += used;
		if ((key & 7u) == 0u) { used = directVarint(data + offset, size - offset, &value); if (!used) return; offset += used; continue; }
		if ((key & 7u) != 2u) return;
		used = directVarint(data + offset, size - offset, &value); if (!used || value > size - offset - used) return;
		offset += used; tag = key >> 3u; contentOffset = offset; contentSize = value; offset += value;
	}
	if (tag == 22u && contentSize) {
		uint32_t pos = contentOffset; const uint8_t *frame = NULL; uint32_t frameSize = 0u, orientation = 0u;
		while (pos < contentOffset + contentSize) {
			uint32_t key, value, used = directVarint(data + pos, contentOffset + contentSize - pos, &key); if (!used) return; pos += used;
			if ((key & 7u) == 2u) { used = directVarint(data + pos, contentOffset + contentSize - pos, &value); if (!used || value > contentOffset + contentSize - pos - used) return; pos += used; if ((key >> 3u) == 1u) { frame = data + pos; frameSize = value; } pos += value; }
			else if ((key & 7u) == 0u) { used = directVarint(data + pos, contentOffset + contentSize - pos, &value); if (!used) return; pos += used; if ((key >> 3u) == 2u) orientation = value; }
			else return;
		}
		if (frame && frameSize == FRC_FRAME_BYTES && orientation < 4u && !app->paused) { memcpy(app->view.frame, frame, frameSize); app->view.orientation = (uint8_t)orientation; frcRenderFrame(&app->view); }
	}
}

static void directRead(struct DirectRemote *app)
{
	uint8_t bytes[256];
	while (flipperUsbHostAvailable()) {
		uint32_t got = flipperUsbHostRead(bytes, sizeof(bytes)); if (!got) break;
		if (app->rxUsed + got > sizeof(app->rx)) app->rxUsed = 0u;
		memcpy(app->rx + app->rxUsed, bytes, got); app->rxUsed += got;
	}
	if (app->state == DirectWaitPrompt && app->rxUsed >= 4u) {
		for (uint32_t i = 0; i + 3u < app->rxUsed; i++) if (app->rx[i] == '>' && app->rx[i + 1u] == ':' && app->rx[i + 2u] == ' ') { static const uint8_t cmd[] = "start_rpc_session\r"; (void)directQueue(app, cmd, sizeof(cmd) - 1u); app->state = DirectWaitRpcEcho; app->rxUsed = 0u; break; }
	}
	if (app->state == DirectWaitRpcEcho && app->rxUsed && app->rx[app->rxUsed - 1u] == '\n') { app->state = DirectRpc; app->rxUsed = 0u; directSendMain(app, 39u, NULL, 0u); directSendMain(app, 20u, NULL, 0u); directSendMain(app, 68u, NULL, 0u); }
	if (app->state == DirectRpc) while (app->rxUsed) {
		uint32_t messageSize, prefix = directVarint(app->rx, app->rxUsed, &messageSize);
		if (!prefix || messageSize > sizeof(app->rx)) break;
		if (prefix + messageSize > app->rxUsed) break;
		directHandleMessage(app, app->rx + prefix, messageSize);
		memmove(app->rx, app->rx + prefix + messageSize, app->rxUsed - prefix - messageSize); app->rxUsed -= prefix + messageSize;
	}
}

static void directForwardKeys(struct DirectRemote *app, uint_fast16_t keys, uint64_t now)
{
	static const struct { uint_fast16_t local; uint8_t remote; } map[] = {{KEY_BIT_UP,0},{KEY_BIT_DOWN,1},{KEY_BIT_RIGHT,2},{KEY_BIT_LEFT,3},{KEY_BIT_A,4},{KEY_BIT_B,5}};
	for (uint_fast8_t i = 0; i < 6u; i++) { bool down = keys & map[i].local, was = app->previousKeys & map[i].local;
		if (down && !was) { app->downAt[i] = now; app->nextRepeatAt[i] = now + FRC_LONG_TICKS + FRC_REPEAT_TICKS; app->keyLong[i] = false; directSendInput(app,map[i].remote,0u); }
		else if (!down && was) { directSendInput(app,map[i].remote,1u); if (!app->keyLong[i]) directSendInput(app,map[i].remote,2u); }
		else if (down && app->downAt[i] && !app->keyLong[i] && now - app->downAt[i] >= FRC_LONG_TICKS) { app->keyLong[i] = true; directSendInput(app,map[i].remote,3u); }
		if (down && app->keyLong[i] && now >= app->nextRepeatAt[i]) { directSendInput(app,map[i].remote,4u); app->nextRepeatAt[i] += FRC_REPEAT_TICKS; }
	}
	if ((keys & KEY_BIT_START) && !(app->previousKeys & KEY_BIT_START) && app->state == DirectRpc) directSendMain(app,67u,NULL,0u);
	if ((keys & KEY_BIT_SEL) && !(app->previousKeys & KEY_BIT_SEL) && app->state == DirectRpc) { app->paused = !app->paused; directSendMain(app, app->paused ? 21u : 20u, NULL, 0u); }
	app->previousKeys = keys;
}

int flipperRemoteRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	struct DirectRemote app; memset(&app, 0, sizeof(app)); mFlipperRemoteAbort = false;
	if (!host || !host->displayFb || !host->getTime) return 1;
	app.view.host = host; if (args && args->canvas) app.view.canvas = *args->canvas;
	if (!app.view.canvas.framebuffer) app.view.canvas.framebuffer = host->displayFb();
	if (!app.view.canvas.w)
		app.view.canvas.w = 320u;
	if (!app.view.canvas.h)
		app.view.canvas.h = 240u;
	app.view.canvas.bpp = 16u;
	if (args && args->rotate) app.view.canvas.flipped = 1u;
	app.state = DirectWaitPrompt;
	frcSetStatusView(&app.view, "Connect Flipper USB", "Host mode; requires 5V VBUS");
	if (!flipperUsbHostBegin()) {
		frcSetStatusView(&app.view, "USB host unavailable", flipperUsbHostLastError());
		frcPresentStatus(&app.view);
		return 1;
	}
	while (!mFlipperRemoteAbort) {
		uint_fast16_t keys; uint64_t now = host->getTime(); flipperUsbHostTask();
		if (!flipperUsbHostMounted()) {
			if (app.state != DirectWaitMount) {
				app.state = DirectWaitMount;
				app.rxUsed = app.txUsed = 0u;
				app.view.haveFrame = false;
				frcSetStatusView(&app.view, "Connect Flipper USB", "Host mode; requires 5V VBUS");
			}
		}
		else { if (app.state == DirectWaitMount) { static const uint8_t cr[] = "\r"; app.state = DirectWaitPrompt; app.rxUsed = 0u; (void)directQueue(&app,cr,1u); frcSetStatusView(&app.view,"Flipper connected","Starting QFlipper RPC"); } directRead(&app); }
		if (app.view.statusDirty) { frcPresentStatus(&app.view); app.view.statusDirty = false; }
		keys = host->uiKeysRaw ? host->uiKeysRaw() : 0u;
		if ((keys & UI_KEY_BIT_CENTER) && !app.centerDownAt)
			app.centerDownAt = now;
		if (!(keys & UI_KEY_BIT_CENTER))
			app.centerDownAt = 0u;
		if (app.centerDownAt && now - app.centerDownAt >= FRC_LOCAL_EXIT_TICKS) break;
		directForwardKeys(&app,keys,now);
		if (app.txUsed) { uint32_t wrote = flipperUsbHostWrite(app.tx,app.txUsed); if (wrote) { memmove(app.tx,app.tx+wrote,app.txUsed-wrote); app.txUsed-=wrote; flipperUsbHostFlush(); } }
		if (host->ledsTick)
			host->ledsTick();
		if (host->idleWaitMsec)
			host->idleWaitMsec(1u);
		else if (host->delayMsec)
			host->delayMsec(1u);
	}
	flipperUsbHostEnd(); return 0;
}
