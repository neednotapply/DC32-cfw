#include "apps/pwnagotchi_remote.h"

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

#define PWN_MAGIC "PWN1"
#define PWN_VERSION 1u
#define PWN_HEADER_SIZE 20u
#define PWN_PAYLOAD_MAX 1200u
#define PWN_PAGE_ITEMS 12u
#define PWN_ITEM_CHARS 39u
#define PWN_TITLE_CHARS 31u
#define PWN_DETAIL_CHARS 79u
#define PWN_FACE_W 128u
#define PWN_FACE_H 64u
#define PWN_FACE_BYTES (PWN_FACE_W * PWN_FACE_H / 8u)
#define PWN_HELLO_PERIOD TICKS_PER_SECOND
#define PWN_EXIT_PERIOD TICKS_PER_SECOND

enum PwnPacketType {
	PwnPacketHello = 1,
	PwnPacketReady = 2,
	PwnPacketPage = 3,
	PwnPacketInput = 4,
	PwnPacketStatus = 5,
	PwnPacketFace = 6,
};

enum PwnInput {
	PwnInputUp = 1,
	PwnInputDown,
	PwnInputLeft,
	PwnInputRight,
	PwnInputAccept,
	PwnInputBack,
	PwnInputHome,
	PwnInputPlugins,
};

struct PwnStream {
	uint8_t header[PWN_HEADER_SIZE];
	uint32_t headerUsed;
	uint32_t payloadLength;
	uint32_t payloadUsed;
	uint32_t crc;
	uint8_t type;
	bool receivingPayload;
};

struct PwnPage {
	uint8_t id;
	uint8_t flags;
	uint8_t focus;
	uint8_t count;
	char title[PWN_TITLE_CHARS + 1u];
	char detail[PWN_DETAIL_CHARS + 1u];
	char item[PWN_PAGE_ITEMS][PWN_ITEM_CHARS + 1u];
};

struct PwnApp {
	const struct DcAppHostApi *host;
	struct Canvas canvas;
	struct PwnStream stream;
	struct PwnPage page;
	uint8_t payload[PWN_PAYLOAD_MAX];
	uint8_t face[PWN_FACE_BYTES];
	uint_fast16_t previousKeys;
	uint64_t centerDownAt;
	uint64_t lastHelloAt;
	uint32_t outgoingSequence;
	bool bridgeReady;
	bool pageDirty;
	bool faceValid;
	char status[80];
};

static volatile bool mPwnagotchiAbort;

void pwnagotchiRemoteAbort(void)
{
	mPwnagotchiAbort = true;
}

static uint32_t pwnReadLe32(const uint8_t *src)
{
	return (uint32_t)src[0] | ((uint32_t)src[1] << 8) | ((uint32_t)src[2] << 16) | ((uint32_t)src[3] << 24);
}

static void pwnWriteLe32(uint8_t *dst, uint32_t value)
{
	dst[0] = (uint8_t)value;
	dst[1] = (uint8_t)(value >> 8);
	dst[2] = (uint8_t)(value >> 16);
	dst[3] = (uint8_t)(value >> 24);
}

static uint32_t pwnCrc32(const uint8_t *data, uint32_t size)
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

static void pwnText(const struct PwnApp *app, int32_t x, int32_t y, const char *text, enum Font font, uint16_t color)
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

static uint32_t pwnTextWidth(const char *text, enum Font font)
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

static void pwnCentered(const struct PwnApp *app, int32_t y, const char *text, enum Font font, uint16_t color)
{
	uint32_t width = pwnTextWidth(text, font);
	pwnText(app, (int32_t)(app->canvas.w > width ? (app->canvas.w - width) / 2u : 0u), y, text, font, color);
}

static void pwnFill(const struct PwnApp *app, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t color)
{
	for (uint32_t row = y; row < y + h && row < app->canvas.h; row++)
		for (uint32_t col = x; col < x + w && col < app->canvas.w; col++)
			rasterPutPixel(&app->canvas, col, row, color);
}

static void pwnRenderFace(const struct PwnApp *app)
{
	uint32_t scale = app->canvas.w / PWN_FACE_W;
	uint32_t w, h, x, y;

	if (!scale)
		scale = 1u;
	w = PWN_FACE_W * scale;
	h = PWN_FACE_H * scale;
	if (h > app->canvas.h - 28u) {
		scale = (app->canvas.h - 28u) / PWN_FACE_H;
		if (!scale)
			scale = 1u;
		w = PWN_FACE_W * scale;
		h = PWN_FACE_H * scale;
	}
	x = app->canvas.w > w ? (app->canvas.w - w) / 2u : 0u;
	y = 28u + (app->canvas.h - 28u > h ? (app->canvas.h - 28u - h) / 2u : 0u);
	for (uint32_t srcY = 0; srcY < PWN_FACE_H; srcY++)
		for (uint32_t srcX = 0; srcX < PWN_FACE_W; srcX++) {
			uint32_t bit = srcY * PWN_FACE_W + srcX;
			uint16_t color = (app->face[bit >> 3] & (uint8_t)(0x80u >> (bit & 7u))) ? rasterRgb565(245, 245, 235) : rasterRgb565(20, 25, 22);
			pwnFill(app, x + srcX * scale, y + srcY * scale, scale, scale, color);
		}
}

static void pwnPresent(const struct PwnApp *app)
{
	uint16_t titleColor = rasterRgb565(105, 255, 160);
	uint16_t textColor = rasterRgb565(230, 245, 232);
	uint16_t mutedColor = rasterRgb565(145, 180, 155);

	dispPrvFrameCtrWait();
	dispPrvWaitForScanoutStart();
	rasterClear(&app->canvas, rasterRgb565(8, 13, 10));
	pwnFill(app, 0, 0, app->canvas.w, 23u, rasterRgb565(18, 60, 35));
	pwnText(app, 6, 5, app->page.title[0] ? app->page.title : "PWNAGOTCHI", FontMedium, titleColor);
	if (app->page.id == 0u && app->faceValid) {
		pwnRenderFace(app);
		return;
	}
	if (app->page.detail[0])
		pwnText(app, 7, 29, app->page.detail, FontSmall, mutedColor);
	for (uint32_t i = 0; i < app->page.count && i < PWN_PAGE_ITEMS; i++) {
		uint32_t y = 46u + i * 15u;
		if (i == app->page.focus)
			pwnFill(app, 4, y - 2u, app->canvas.w - 8u, 14u, rasterRgb565(32, 100, 61));
		pwnText(app, 10, (int32_t)y, app->page.item[i], FontSmall, textColor);
	}
	if (!app->page.count && app->status[0])
		pwnCentered(app, 108, app->status, FontMedium, mutedColor);
	pwnCentered(app, 224, "START HOME  SELECT PLUGINS", FontSmall, mutedColor);
}

static void pwnSetStatus(struct PwnApp *app, const uint8_t *value, uint32_t size)
{
	if (size >= sizeof(app->status))
		size = sizeof(app->status) - 1u;
	if (size)
		memcpy(app->status, value, size);
	app->status[size] = 0;
	app->pageDirty = true;
}

static bool pwnCopyString(char *dst, uint32_t dstSize, const uint8_t *src, uint32_t size, uint32_t *offset)
{
	uint32_t start = *offset;
	uint32_t length = 0;

	while (*offset < size && src[*offset]) {
		(*offset)++;
		length++;
	}
	if (*offset == size)
		return false;
	if (length >= dstSize)
		length = dstSize - 1u;
	memcpy(dst, src + start, length);
	dst[length] = 0;
	(*offset)++;
	return true;
}

static bool pwnDecodePage(struct PwnApp *app, const uint8_t *payload, uint32_t size)
{
	struct PwnPage page;
	uint32_t offset = 4u;

	if (size < 6u || payload[3] > PWN_PAGE_ITEMS)
		return false;
	memset(&page, 0, sizeof(page));
	page.id = payload[0];
	page.flags = payload[1];
	page.focus = payload[2] < payload[3] ? payload[2] : 0u;
	page.count = payload[3];
	if (!pwnCopyString(page.title, sizeof(page.title), payload, size, &offset) ||
		!pwnCopyString(page.detail, sizeof(page.detail), payload, size, &offset))
		return false;
	for (uint32_t i = 0; i < page.count; i++)
		if (!pwnCopyString(page.item[i], sizeof(page.item[i]), payload, size, &offset))
			return false;
	app->page = page;
	app->pageDirty = true;
	return true;
}

static void pwnResetStream(struct PwnStream *stream)
{
	memset(stream, 0, sizeof(*stream));
}

static void pwnHandlePacket(struct PwnApp *app)
{
	struct PwnStream *stream = &app->stream;

	if (stream->crc != pwnCrc32(app->payload, stream->payloadLength)) {
		pwnSetStatus(app, (const uint8_t *)"Bad packet CRC", 14u);
		return;
	}
	if (stream->type == PwnPacketReady && stream->payloadLength == 0u) {
		app->bridgeReady = true;
		pwnSetStatus(app, (const uint8_t *)"Connected", 9u);
	}
	else if (stream->type == PwnPacketStatus)
		pwnSetStatus(app, app->payload, stream->payloadLength);
	else if (stream->type == PwnPacketPage && !pwnDecodePage(app, app->payload, stream->payloadLength))
		pwnSetStatus(app, (const uint8_t *)"Bad page", 8u);
	else if (stream->type == PwnPacketFace && stream->payloadLength == PWN_FACE_BYTES) {
		memcpy(app->face, app->payload, PWN_FACE_BYTES);
		app->faceValid = true;
		if (app->page.id == 0u)
			app->pageDirty = true;
	}
}

static void pwnConsumeByte(struct PwnApp *app, uint8_t value)
{
	struct PwnStream *stream = &app->stream;

	if (!stream->receivingPayload) {
		stream->header[stream->headerUsed++] = value;
		while (stream->headerUsed >= 4u && memcmp(stream->header, PWN_MAGIC, 4u))
			memmove(stream->header, stream->header + 1u, --stream->headerUsed);
		if (stream->headerUsed != PWN_HEADER_SIZE)
			return;
		if (stream->header[4] != PWN_VERSION) {
			memmove(stream->header, stream->header + 1u, --stream->headerUsed);
			return;
		}
		stream->type = stream->header[5];
		stream->payloadLength = pwnReadLe32(stream->header + 12u);
		stream->crc = pwnReadLe32(stream->header + 16u);
		if (stream->payloadLength > PWN_PAYLOAD_MAX) {
			pwnResetStream(stream);
			pwnSetStatus(app, (const uint8_t *)"Packet too large", 16u);
			return;
		}
		stream->payloadUsed = 0u;
		stream->receivingPayload = stream->payloadLength != 0u;
		if (!stream->receivingPayload) {
			pwnHandlePacket(app);
			pwnResetStream(stream);
		}
		return;
	}
	app->payload[stream->payloadUsed++] = value;
	if (stream->payloadUsed == stream->payloadLength) {
		pwnHandlePacket(app);
		pwnResetStream(stream);
	}
}

static void pwnReadUsb(struct PwnApp *app)
{
	uint8_t bytes[256];
	while (usbCdcAvailable()) {
		uint32_t got = usbCdcRead(bytes, sizeof(bytes));
		if (!got)
			break;
		for (uint32_t i = 0; i < got; i++)
			pwnConsumeByte(app, bytes[i]);
	}
}

static bool pwnSendPacket(struct PwnApp *app, uint8_t type, const uint8_t *payload, uint32_t size)
{
	uint8_t header[PWN_HEADER_SIZE] = {0};
	if (size > 64u || usbCdcWriteAvailable() < PWN_HEADER_SIZE + size)
		return false;
	memcpy(header, PWN_MAGIC, 4u);
	header[4] = PWN_VERSION;
	header[5] = type;
	pwnWriteLe32(header + 8u, ++app->outgoingSequence);
	pwnWriteLe32(header + 12u, size);
	pwnWriteLe32(header + 16u, pwnCrc32(payload, size));
	if (usbCdcWrite(header, sizeof(header)) != sizeof(header) || (size && usbCdcWrite(payload, size) != size))
		return false;
	usbCdcFlush();
	return true;
}

static void pwnSendHello(struct PwnApp *app)
{
	uint8_t payload[4] = {(uint8_t)app->canvas.w, (uint8_t)(app->canvas.w >> 8), (uint8_t)app->canvas.h, (uint8_t)(app->canvas.h >> 8)};
	(void)pwnSendPacket(app, PwnPacketHello, payload, sizeof(payload));
}

static void pwnForwardKeys(struct PwnApp *app, uint_fast16_t keys)
{
	static const struct { uint_fast16_t key; uint8_t input; } map[] = {
		{KEY_BIT_UP, PwnInputUp}, {KEY_BIT_DOWN, PwnInputDown}, {KEY_BIT_LEFT, PwnInputLeft}, {KEY_BIT_RIGHT, PwnInputRight},
		{KEY_BIT_A, PwnInputAccept}, {KEY_BIT_B, PwnInputBack}, {KEY_BIT_START, PwnInputHome}, {KEY_BIT_SEL, PwnInputPlugins},
	};
	if (app->bridgeReady)
		for (uint_fast8_t i = 0; i < sizeof(map) / sizeof(*map); i++)
			if ((keys & map[i].key) && !(app->previousKeys & map[i].key))
				(void)pwnSendPacket(app, PwnPacketInput, &map[i].input, 1u);
	app->previousKeys = keys;
}

static bool pwnShouldExit(struct PwnApp *app, uint_fast16_t keys)
{
	uint64_t now = app->host->getTime();
	bool center = (keys & UI_KEY_BIT_CENTER) != 0u;
	if (center && !app->centerDownAt)
		app->centerDownAt = now;
	if (!center)
		app->centerDownAt = 0u;
	return center && app->centerDownAt && now - app->centerDownAt >= PWN_EXIT_PERIOD;
}

int pwnagotchiRemoteRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	struct PwnApp app;
	struct UsbCdcDeviceInfo info;

	if (!host || !host->displayFb || !host->getTime)
		return 1;
	memset(&app, 0, sizeof(app));
	mPwnagotchiAbort = false;
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
	strcpy(app.page.title, "PWNAGOTCHI");
	strcpy(app.status, "Connect Pi USB");
	app.pageDirty = true;
	usbCdcDefaultInfo(&info);
	strcpy(info.product, "DC32 Pwnagotchi Remote");
	if (!usbCdcBegin(&info))
		return 1;

	while (!mPwnagotchiAbort) {
		uint_fast16_t keys;
		uint64_t now;
		usbCdcTask();
		if (!usbCdcConnected()) {
			app.bridgeReady = false;
			app.faceValid = false;
			app.lastHelloAt = 0u;
			if (strcmp(app.status, "Connect Pi USB"))
				pwnSetStatus(&app, (const uint8_t *)"Connect Pi USB", 14u);
		}
		else {
			now = host->getTime();
			if (!app.bridgeReady && (!app.lastHelloAt || now - app.lastHelloAt >= PWN_HELLO_PERIOD)) {
				pwnSendHello(&app);
				app.lastHelloAt = now;
			}
			pwnReadUsb(&app);
		}
		if (app.pageDirty) {
			pwnPresent(&app);
			app.pageDirty = false;
		}
		keys = host->uiKeysRaw ? host->uiKeysRaw() : 0u;
		if (pwnShouldExit(&app, keys))
			break;
		pwnForwardKeys(&app, keys);
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
