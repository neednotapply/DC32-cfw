#include "apps/raspyjack_remote.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "dispDefcon.h"
#include "fonts.h"
#include "gb.h"
#include "picojpeg.h"
#include "raster.h"
#include "timebase.h"
#include "toolWorkspace.h"
#include "ui.h"
#include "usbCdc.h"

#define RJ_MAGIC "RJC2"
#define RJ_VERSION 1u
#define RJ_HEADER_SIZE 20u
#define RJ_MAX_FRAME_BYTES (64u * 1024u)
#define RJ_SOURCE_W 128u
#define RJ_SOURCE_H 128u
#define RJ_SOURCE_BYTES (RJ_SOURCE_W * RJ_SOURCE_H * sizeof(uint16_t))
#define RJ_STATUS_MAX 63u
#define RJ_HELLO_PERIOD_TICKS TICKS_PER_SECOND
#define RJ_LOCAL_EXIT_TICKS TICKS_PER_SECOND
#define RJ_CAP_FRAME_ACK 0x80000000u
#define RJ_CAP_RGB565 0x40000000u
#define RJ_CAP_TILES 0x20000000u
#define RJ_TILE_SIZE 8u
#define RJ_RGB565_HEADER_SIZE 4u
#define RJ_TILES_HEADER_SIZE 7u
#define RJ_TILE_DATA_BYTES (RJ_TILE_SIZE * RJ_TILE_SIZE * sizeof(uint16_t))

enum RjPacketType {
	RjPacketHello = 1,
	RjPacketReady = 2,
	RjPacketFrame = 3,
	RjPacketInput = 4,
	RjPacketStatus = 5,
	RjPacketFrameAck = 6,
	RjPacketFrameRgb565 = 7,
	RjPacketFrameTiles = 8,
};

enum RjButton {
	RjButtonUp = 1,
	RjButtonDown,
	RjButtonLeft,
	RjButtonRight,
	RjButtonOk,
	RjButtonKey1,
	RjButtonKey2,
	RjButtonKey3,
};

struct RjStream {
	uint8_t header[RJ_HEADER_SIZE];
	uint32_t headerUsed;
	uint32_t payloadLength;
	uint32_t payloadUsed;
	uint32_t sequence;
	uint32_t crc;
	uint8_t type;
	bool receivingPayload;
};

struct RjApp {
	const struct DcAppHostApi *host;
	struct Canvas canvas;
	struct RjStream stream;
	uint8_t *jpeg;
	uint16_t *stage;
	uint_fast16_t previousKeys;
	uint64_t centerDownAt;
	uint64_t lastHelloAt;
	uint32_t outgoingSequence;
	uint32_t lastFrameSequence;
	uint32_t pendingAckSequence;
	uint16_t sourceW;
	uint16_t sourceH;
	uint16_t presentedW;
	uint16_t presentedH;
	bool haveFrameSequence;
	bool bridgeReady;
	bool haveFrame;
	bool stagePresented;
	bool pendingAck;
	bool pendingAckSuccess;
	bool workspaceCart;
	bool workspaceWram;
	bool statusDirty;
	char status[RJ_STATUS_MAX + 1u];
	char statusState[32];
	char statusDetail[RJ_STATUS_MAX + 1u];
};

struct RjJpegReader {
	const uint8_t *jpeg;
	uint32_t offset;
	uint32_t size;
};

static volatile bool mRaspyJackAbort;

void raspyJackRemoteAbort(void)
{
	mRaspyJackAbort = true;
}

static uint32_t rjReadLe32(const uint8_t *src)
{
	return (uint32_t)src[0] | ((uint32_t)src[1] << 8) |
		((uint32_t)src[2] << 16) | ((uint32_t)src[3] << 24);
}

static uint16_t rjReadLe16(const uint8_t *src)
{
	return (uint16_t)src[0] | ((uint16_t)src[1] << 8);
}

static void rjWriteLe32(uint8_t *dst, uint32_t value)
{
	dst[0] = (uint8_t)value;
	dst[1] = (uint8_t)(value >> 8);
	dst[2] = (uint8_t)(value >> 16);
	dst[3] = (uint8_t)(value >> 24);
}

static uint32_t rjCrc32(const uint8_t *data, uint32_t size)
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

static void rjText(const struct RjApp *app, int32_t x, int32_t y, const char *text, enum Font font, uint16_t color)
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

static uint32_t rjTextWidth(const char *text, enum Font font)
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

static void rjCentered(const struct RjApp *app, int32_t y, const char *text, enum Font font, uint16_t color)
{
	uint32_t width = rjTextWidth(text, font);
	int32_t x = (int32_t)(app->canvas.w > width ? (app->canvas.w - width) / 2u : 0u);
	rjText(app, x, y, text, font, color);
}

static void rjSetStatus(struct RjApp *app, const char *status)
{
	uint32_t length = status ? (uint32_t)strlen(status) : 0u;
	if (length > RJ_STATUS_MAX)
		length = RJ_STATUS_MAX;
	if (strlen(app->status) == length && !memcmp(app->status, status, length))
		return;
	if (length)
		memcpy(app->status, status, length);
	app->status[length] = 0;
}

static void rjSetStatusView(struct RjApp *app, const char *state, const char *detail)
{
	if (!strcmp(app->statusState, state) && !strcmp(app->statusDetail, detail))
		return;
	strncpy(app->statusState, state, sizeof(app->statusState) - 1u);
	app->statusState[sizeof(app->statusState) - 1u] = 0;
	strncpy(app->statusDetail, detail, sizeof(app->statusDetail) - 1u);
	app->statusDetail[sizeof(app->statusDetail) - 1u] = 0;
	app->statusDirty = true;
}

static void rjPresentStatus(const struct RjApp *app)
{
	dispPrvFrameCtrWait();
	dispPrvWaitForScanoutStart();
	rasterClear(&app->canvas, rasterRgb565(0, 0, 0));
	rjCentered(app, 34, "RASPYJACK REMOTE", FontLarge, rasterRgb565(75, 255, 145));
	rjCentered(app, 92, app->statusState, FontMedium, rasterRgb565(225, 245, 230));
	if (app->statusDetail[0])
		rjCentered(app, 122, app->statusDetail, FontSmall, rasterRgb565(155, 190, 165));
	rjCentered(app, 206, "HOLD CENTER TO EXIT", FontSmall, rasterRgb565(100, 145, 115));
}

static unsigned char rjJpegNeedBytes(unsigned char *buf, unsigned char bufSize,
	unsigned char *bytesReadP, void *userData)
{
	struct RjJpegReader *reader = userData;
	uint32_t remaining = reader->size - reader->offset;
	uint32_t take = remaining < bufSize ? remaining : bufSize;

	if (take)
		memcpy(buf, reader->jpeg + reader->offset, take);
	reader->offset += take;
	*bytesReadP = (unsigned char)take;
	return 0;
}

static bool rjDecodeToStage(struct RjApp *app, uint32_t size)
{
	struct RjJpegReader reader = {.jpeg = app->jpeg, .size = size};
	pjpeg_image_info_t info;
	uint32_t total;

	memset(&info, 0, sizeof(info));
	if (pjpeg_decode_init(&info, rjJpegNeedBytes, &reader, 0) || info.m_width <= 0 || info.m_height <= 0 ||
		(uint32_t)info.m_width > RJ_SOURCE_W || (uint32_t)info.m_height > RJ_SOURCE_H)
		return false;
	total = (uint32_t)info.m_MCUSPerRow * (uint32_t)info.m_MCUSPerCol;
	if (!total || size < 4u)
		return false;
	for (uint32_t index = 0; index < total; index++) {
		uint32_t mcuX = (index % (uint32_t)info.m_MCUSPerRow) * (uint32_t)info.m_MCUWidth;
		uint32_t mcuY = (index / (uint32_t)info.m_MCUSPerRow) * (uint32_t)info.m_MCUHeight;
		uint32_t blocksPerRow = (uint32_t)info.m_MCUWidth / 8u;

		if (pjpeg_decode_mcu())
			return false;
		for (uint32_t y = 0; y < (uint32_t)info.m_MCUHeight; y++) {
			uint32_t srcY = mcuY + y;
			if (srcY >= (uint32_t)info.m_height)
				continue;
			for (uint32_t x = 0; x < (uint32_t)info.m_MCUWidth; x++) {
				uint32_t srcX = mcuX + x;
				uint32_t block = (y / 8u) * blocksPerRow + x / 8u;
				uint32_t offset = block * 64u + (y & 7u) * 8u + (x & 7u);
				uint8_t r = info.m_pMCUBufR[offset];
				uint8_t g = info.m_comps == 1 ? r : info.m_pMCUBufG[offset];
				uint8_t b = info.m_comps == 1 ? r : info.m_pMCUBufB[offset];

				if (srcX < (uint32_t)info.m_width)
					app->stage[srcY * RJ_SOURCE_W + srcX] = rasterRgb565(r, g, b);
			}
		}
		if (!(index & 7u) && app->host->ledsTick)
			app->host->ledsTick();
	}
	app->sourceW = (uint16_t)info.m_width;
	app->sourceH = (uint16_t)info.m_height;
	return true;
}

static void rjPresentStage(struct RjApp *app)
{
	struct RasterRect fit = rasterFit(app->sourceW, app->sourceH, app->canvas.w, app->canvas.h);
	uint16_t *dst = app->canvas.framebuffer;
	uint32_t srcY = 0u;
	uint32_t yError = 0u;
	bool fullPresent = !app->stagePresented || app->presentedW != app->sourceW || app->presentedH != app->sourceH;

	dispPrvWaitForScanoutStart();
	if (fullPresent)
		rasterClear(&app->canvas, rasterRgb565(0, 0, 0));
	for (uint32_t y = 0; y < fit.h; y++) {
		uint32_t srcX = 0u;
		uint32_t xError = 0u;
		int32_t dstIndex;
		int32_t dstStride;

		if (app->canvas.rotated) {
			dstIndex = (int32_t)(fit.x * app->canvas.h + (app->canvas.h - 1u - (fit.y + y)));
			dstStride = (int32_t)app->canvas.h;
			if (app->canvas.flipped) {
				dstIndex = (int32_t)((app->canvas.w - 1u - fit.x) * app->canvas.h + fit.y + y);
				dstStride = -(int32_t)app->canvas.h;
			}
		}
		else {
			dstIndex = (int32_t)((fit.y + y) * app->canvas.w + fit.x);
			dstStride = 1;
			if (app->canvas.flipped) {
				dstIndex = (int32_t)((app->canvas.h - 1u - (fit.y + y)) * app->canvas.w + app->canvas.w - 1u - fit.x);
				dstStride = -1;
			}
		}
		for (uint32_t x = 0; x < fit.w; x++) {
			uint16_t color = app->stage[srcY * RJ_SOURCE_W + srcX];
			if (fullPresent || dst[(uint32_t)dstIndex] != color)
				dst[(uint32_t)dstIndex] = color;
			dstIndex += dstStride;
			xError += app->sourceW;
			if (xError >= fit.w) {
				xError -= fit.w;
				srcX++;
			}
		}
		yError += app->sourceH;
		if (yError >= fit.h) {
			yError -= fit.h;
			srcY++;
		}
	}
	app->stagePresented = true;
	app->presentedW = app->sourceW;
	app->presentedH = app->sourceH;
}

static bool rjRenderFrame(struct RjApp *app, uint32_t sequence, uint32_t size)
{
	if (size < 4u || app->jpeg[0] != 0xffu || app->jpeg[1] != 0xd8u ||
		app->jpeg[size - 2u] != 0xffu || app->jpeg[size - 1u] != 0xd9u ||
		!rjDecodeToStage(app, size))
		return false;
	rjPresentStage(app);
	app->lastFrameSequence = sequence;
	app->haveFrameSequence = true;
	app->haveFrame = true;
	return true;
}

static bool rjRenderRgb565Frame(struct RjApp *app, uint32_t sequence, uint32_t size)
{
	if (size != RJ_RGB565_HEADER_SIZE + RJ_SOURCE_BYTES ||
		rjReadLe16(app->jpeg) != RJ_SOURCE_W || rjReadLe16(app->jpeg + 2u) != RJ_SOURCE_H)
		return false;
	memcpy(app->stage, app->jpeg + RJ_RGB565_HEADER_SIZE, RJ_SOURCE_BYTES);
	app->sourceW = RJ_SOURCE_W;
	app->sourceH = RJ_SOURCE_H;
	rjPresentStage(app);
	app->lastFrameSequence = sequence;
	app->haveFrameSequence = true;
	app->haveFrame = true;
	return true;
}

static bool rjRenderTileFrame(struct RjApp *app, uint32_t sequence, uint32_t size)
{
	uint16_t count;
	uint32_t expected;
	const uint8_t *tile;

	if (size < RJ_TILES_HEADER_SIZE || !app->haveFrame || app->sourceW != RJ_SOURCE_W || app->sourceH != RJ_SOURCE_H ||
		rjReadLe16(app->jpeg) != RJ_SOURCE_W || rjReadLe16(app->jpeg + 2u) != RJ_SOURCE_H ||
		app->jpeg[4] != RJ_TILE_SIZE)
		return false;
	count = rjReadLe16(app->jpeg + 5u);
	expected = RJ_TILES_HEADER_SIZE + (uint32_t)count * (2u + RJ_TILE_DATA_BYTES);
	if (!count || count > (RJ_SOURCE_W / RJ_TILE_SIZE) * (RJ_SOURCE_H / RJ_TILE_SIZE) || size != expected)
		return false;
	tile = app->jpeg + RJ_TILES_HEADER_SIZE;
	for (uint16_t index = 0; index < count; index++, tile += 2u + RJ_TILE_DATA_BYTES)
		if ((uint32_t)tile[0] * RJ_TILE_SIZE >= RJ_SOURCE_W || (uint32_t)tile[1] * RJ_TILE_SIZE >= RJ_SOURCE_H)
			return false;
	tile = app->jpeg + RJ_TILES_HEADER_SIZE;
	for (uint16_t index = 0; index < count; index++, tile += 2u + RJ_TILE_DATA_BYTES) {
		uint32_t baseX = (uint32_t)tile[0] * RJ_TILE_SIZE;
		uint32_t baseY = (uint32_t)tile[1] * RJ_TILE_SIZE;
		for (uint32_t row = 0; row < RJ_TILE_SIZE; row++)
			memcpy(app->stage + (baseY + row) * RJ_SOURCE_W + baseX,
				tile + 2u + row * RJ_TILE_SIZE * sizeof(uint16_t), RJ_TILE_SIZE * sizeof(uint16_t));
	}
	rjPresentStage(app);
	app->lastFrameSequence = sequence;
	app->haveFrameSequence = true;
	app->haveFrame = true;
	return true;
}

static bool rjSequenceNewer(uint32_t value, uint32_t previous)
{
	return (int32_t)(value - previous) > 0;
}

static void rjQueueFrameAck(struct RjApp *app, uint32_t sequence, bool success)
{
	app->pendingAckSequence = sequence;
	app->pendingAckSuccess = success;
	app->pendingAck = true;
}

static void rjHandlePacket(struct RjApp *app)
{
	struct RjStream *stream = &app->stream;

	if (stream->crc != rjCrc32(app->jpeg, stream->payloadLength)) {
		rjSetStatus(app, "Bad packet CRC");
		if (stream->type == RjPacketFrame || stream->type == RjPacketFrameRgb565 || stream->type == RjPacketFrameTiles)
			rjQueueFrameAck(app, stream->sequence, false);
		return;
	}
	if (stream->type == RjPacketReady && stream->payloadLength == 0u) {
		app->bridgeReady = true;
		rjSetStatus(app, "Bridge ready");
	}
	else if (stream->type == RjPacketStatus) {
		uint32_t copy = stream->payloadLength > RJ_STATUS_MAX ? RJ_STATUS_MAX : stream->payloadLength;
		memcpy(app->status, app->jpeg, copy);
		app->status[copy] = 0;
	}
	else if ((stream->type == RjPacketFrame || stream->type == RjPacketFrameRgb565 || stream->type == RjPacketFrameTiles) &&
		(!app->haveFrameSequence || rjSequenceNewer(stream->sequence, app->lastFrameSequence))) {
		bool success = stream->type == RjPacketFrame ? rjRenderFrame(app, stream->sequence, stream->payloadLength) :
			stream->type == RjPacketFrameRgb565 ? rjRenderRgb565Frame(app, stream->sequence, stream->payloadLength) :
			rjRenderTileFrame(app, stream->sequence, stream->payloadLength);
		if (!success)
			rjSetStatus(app, "Frame decode failed");
		rjQueueFrameAck(app, stream->sequence, success);
	}
}

static void rjResetStream(struct RjStream *stream)
{
	memset(stream, 0, sizeof(*stream));
}

static void rjConsumeByte(struct RjApp *app, uint8_t value)
{
	struct RjStream *stream = &app->stream;

	if (!stream->receivingPayload) {
		stream->header[stream->headerUsed++] = value;
		while (stream->headerUsed >= 4u && memcmp(stream->header, RJ_MAGIC, 4u))
			memmove(stream->header, stream->header + 1u, --stream->headerUsed);
		if (stream->headerUsed != RJ_HEADER_SIZE)
			return;
		if (stream->header[4] != RJ_VERSION) {
			memmove(stream->header, stream->header + 1u, --stream->headerUsed);
			return;
		}
		stream->type = stream->header[5];
		stream->sequence = rjReadLe32(stream->header + 8u);
		stream->payloadLength = rjReadLe32(stream->header + 12u);
		stream->crc = rjReadLe32(stream->header + 16u);
		if (stream->payloadLength > RJ_MAX_FRAME_BYTES) {
			rjResetStream(stream);
			rjSetStatus(app, "Frame too large");
			return;
		}
		stream->payloadUsed = 0;
		stream->receivingPayload = stream->payloadLength != 0u;
		if (!stream->receivingPayload) {
			rjHandlePacket(app);
			rjResetStream(stream);
		}
		return;
	}
	if (stream->payloadUsed >= RJ_MAX_FRAME_BYTES) {
		rjResetStream(stream);
		rjSetStatus(app, "Storage overflow");
		return;
	}
	app->jpeg[stream->payloadUsed++] = value;
	if (stream->payloadUsed == stream->payloadLength) {
		rjHandlePacket(app);
		rjResetStream(stream);
	}
}

static void rjReadUsb(struct RjApp *app)
{
	uint8_t bytes[512];

	while (usbCdcAvailable()) {
		uint32_t got = usbCdcRead(bytes, sizeof(bytes));
		if (!got)
			break;
		for (uint32_t i = 0; i < got; i++)
			rjConsumeByte(app, bytes[i]);
	}
}

static bool rjSendPacket(struct RjApp *app, uint8_t type, const uint8_t *payload, uint32_t payloadSize)
{
	uint8_t header[RJ_HEADER_SIZE] = {0};
	uint32_t total = RJ_HEADER_SIZE + payloadSize;

	if (payloadSize > 64u || usbCdcWriteAvailable() < total)
		return false;
	memcpy(header, RJ_MAGIC, 4u);
	header[4] = RJ_VERSION;
	header[5] = type;
	rjWriteLe32(header + 8u, ++app->outgoingSequence);
	rjWriteLe32(header + 12u, payloadSize);
	rjWriteLe32(header + 16u, rjCrc32(payload, payloadSize));
	if (usbCdcWrite(header, sizeof(header)) != sizeof(header) ||
		(payloadSize && usbCdcWrite(payload, payloadSize) != payloadSize))
		return false;
	usbCdcFlush();
	return true;
}

static void rjSendHello(struct RjApp *app)
{
	uint8_t payload[12] = {0};

	rjWriteLe32(payload, RJ_MAX_FRAME_BYTES);
	payload[4] = (uint8_t)app->canvas.w;
	payload[5] = (uint8_t)(app->canvas.w >> 8);
	payload[6] = (uint8_t)app->canvas.h;
	payload[7] = (uint8_t)(app->canvas.h >> 8);
	rjWriteLe32(payload + 8u, RJ_CAP_FRAME_ACK | RJ_CAP_RGB565 | RJ_CAP_TILES);
	(void)rjSendPacket(app, RjPacketHello, payload, sizeof(payload));
}

static void rjFlushFrameAck(struct RjApp *app)
{
	uint8_t payload[5];

	if (!app->pendingAck)
		return;
	rjWriteLe32(payload, app->pendingAckSequence);
	payload[4] = app->pendingAckSuccess ? 1u : 0u;
	if (rjSendPacket(app, RjPacketFrameAck, payload, sizeof(payload)))
		app->pendingAck = false;
}

static void rjSendInput(struct RjApp *app, uint8_t button, bool pressed)
{
	uint8_t payload[2] = {button, pressed ? 1u : 0u};
	(void)rjSendPacket(app, RjPacketInput, payload, sizeof(payload));
}

static void rjForwardKeys(struct RjApp *app, uint_fast16_t keys)
{
	static const struct { uint_fast16_t key; uint8_t button; } map[] = {
		{KEY_BIT_UP, RjButtonUp}, {KEY_BIT_DOWN, RjButtonDown}, {KEY_BIT_LEFT, RjButtonLeft}, {KEY_BIT_RIGHT, RjButtonRight},
		{KEY_BIT_A, RjButtonOk}, {KEY_BIT_START, RjButtonKey1}, {KEY_BIT_SEL, RjButtonKey2}, {KEY_BIT_B, RjButtonKey3},
	};

	if (app->bridgeReady)
		for (uint_fast8_t i = 0; i < sizeof(map) / sizeof(*map); i++)
			if (((keys ^ app->previousKeys) & map[i].key) != 0u)
				rjSendInput(app, map[i].button, (keys & map[i].key) != 0u);
	app->previousKeys = keys;
}

static bool rjShouldExit(struct RjApp *app, uint_fast16_t keys)
{
	uint64_t now = app->host->getTime();
	bool center = (keys & UI_KEY_BIT_CENTER) != 0u;

	if (center && !app->centerDownAt)
		app->centerDownAt = now;
	if (!center)
		app->centerDownAt = 0;
	return center && app->centerDownAt && now - app->centerDownAt >= RJ_LOCAL_EXIT_TICKS;
}

static bool rjAcquireStorage(struct RjApp *app)
{
	struct ToolWorkspaceSpan cart = {0}, wram = {0};

	app->workspaceCart = toolWorkspaceAcquire(ToolWorkspaceCartRam, ToolWorkspaceOwnerRaspyJack, &cart);
	app->workspaceWram = toolWorkspaceAcquire(ToolWorkspaceWram, ToolWorkspaceOwnerRaspyJack, &wram);
	if (!app->workspaceCart || !app->workspaceWram || cart.size < RJ_MAX_FRAME_BYTES || wram.size < RJ_SOURCE_BYTES) {
		if (app->workspaceWram)
			toolWorkspaceRelease(ToolWorkspaceWram, ToolWorkspaceOwnerRaspyJack);
		if (app->workspaceCart)
			toolWorkspaceRelease(ToolWorkspaceCartRam, ToolWorkspaceOwnerRaspyJack);
		app->workspaceWram = app->workspaceCart = false;
		return false;
	}
	app->jpeg = cart.ptr;
	app->stage = wram.ptr;
	return true;
}

static void rjReleaseStorage(struct RjApp *app)
{
	if (app->workspaceWram)
		toolWorkspaceRelease(ToolWorkspaceWram, ToolWorkspaceOwnerRaspyJack);
	if (app->workspaceCart)
		toolWorkspaceRelease(ToolWorkspaceCartRam, ToolWorkspaceOwnerRaspyJack);
	app->workspaceWram = app->workspaceCart = false;
}

int raspyJackRemoteRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	struct RjApp app;
	struct UsbCdcDeviceInfo info;

	if (!host || !host->displayFb || !host->getTime)
		return 1;
	memset(&app, 0, sizeof(app));
	mRaspyJackAbort = false;
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
	if (!rjAcquireStorage(&app))
		return 1;
	usbCdcDefaultInfo(&info);
	if (!usbCdcBegin(&info)) {
		rjReleaseStorage(&app);
		return 1;
	}

	while (!mRaspyJackAbort) {
		uint_fast16_t keys;
		uint64_t now;

		usbCdcTask();
		if (!usbCdcConnected()) {
			app.bridgeReady = false;
			app.haveFrame = false;
			app.stagePresented = false;
			app.lastHelloAt = 0;
			rjResetStream(&app.stream);
			rjSetStatusView(&app, "Connect Pi USB", "Waiting for CDC host");
		}
		else {
			now = host->getTime();
			if (!app.bridgeReady && (!app.lastHelloAt || now - app.lastHelloAt >= RJ_HELLO_PERIOD_TICKS)) {
				rjSendHello(&app);
				app.lastHelloAt = now;
			}
			rjReadUsb(&app);
			rjFlushFrameAck(&app);
			if (!app.haveFrame)
				rjSetStatusView(&app, app.bridgeReady ? "Connected" : "USB connected",
					app.bridgeReady ? (app.status[0] ? app.status : "Waiting for RaspyJack") : "Waiting for bridge");
		}
		if (!app.haveFrame && app.statusDirty) {
			rjPresentStatus(&app);
			app.statusDirty = false;
		}
		keys = host->uiKeysRaw ? host->uiKeysRaw() : 0u;
		if (rjShouldExit(&app, keys))
			break;
		rjForwardKeys(&app, keys);
		if (host->ledsTick)
			host->ledsTick();
		if (host->delayMsec)
			host->delayMsec(1u);
	}

	usbCdcEnd();
	rjReleaseStorage(&app);
	return 0;
}
