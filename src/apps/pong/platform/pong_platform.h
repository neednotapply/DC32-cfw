#ifndef DC32_PONG_PLATFORM_H
#define DC32_PONG_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

#include "dcApp.h"

#define PONG_INPUT_PLAYERS 4

enum PongColorTheme {
	PongColorClassic,
	PongColorTeams,
	PongColorRainbow,
	PongColorThemeCount,
};

enum PongColorRole {
	PongColorNeutral,
	PongColorField,
	PongColorAccent,
	PongColorTeam0,
	PongColorTeam1,
	PongColorTeam2,
	PongColorTeam3,
};

struct PongInput {
	int8_t axisX[PONG_INPUT_PLAYERS];
	int8_t axisY[PONG_INPUT_PLAYERS];
	int8_t pressedX[PONG_INPUT_PLAYERS];
	int8_t pressedY[PONG_INPUT_PLAYERS];
	bool confirmPressed;
	bool backPressed;
};

struct PongRenderer {
	void *context;
	uint32_t frame;
	uint8_t theme;
	uint8_t colorRole;
	void (*clear)(void *context);
	void (*draw_rect)(void *context, int32_t x, int32_t y, int32_t w, int32_t h);
	void (*draw_line)(void *context, int32_t x1, int32_t y1, int32_t x2, int32_t y2);
	void (*draw_text)(void *context, int32_t x, int32_t y, const char *text);
	void (*present)(void *context);
};

struct PongAudio {
	void *context;
	uint8_t volume;
	bool enabled;
	void (*beep)(void *context, uint32_t frequency, uint32_t durationMs);
	void (*click)(void *context);
	void (*score_sound)(void *context);
};

struct PongTiming {
	void *context;
	void (*waitFrame)(void *context);
	uint32_t (*ticksMs)(void *context);
};

struct PongPlatform {
	struct PongRenderer renderer;
	struct PongAudio audio;
	struct PongTiming timing;
	void *context;
	bool (*readInput)(void *context, struct PongInput *input);
	void (*tick)(void *context);
	void (*shutdown)(void *context);
};

bool pongDc32PlatformInit(struct PongPlatform *platform, const struct DcAppHostApi *host,
	const struct DcAppRunArgs *args);

#endif
