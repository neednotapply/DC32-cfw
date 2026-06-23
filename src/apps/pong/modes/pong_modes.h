#ifndef DC32_PONG_MODES_H
#define DC32_PONG_MODES_H

#include "apps/pong/core/pong_core.h"
#include "apps/pong/platform/pong_platform.h"

struct PongMode {
	const char *name;
	const char *description;
	void (*init)(struct PongGameState *state);
	void (*update)(struct PongGameState *state, const struct PongInput *input,
		struct PongAudio *audio);
	void (*draw)(const struct PongGameState *state, struct PongRenderer *renderer);
};

extern const struct PongMode pongModeClassic;
extern const struct PongMode pongModeDoubles;
extern const struct PongMode pongModeQuadrapong;
extern const struct PongMode pongModeSuper;
extern const struct PongMode pongModeRebound;
extern const struct PongMode pongModeBreakout;
extern const struct PongMode pongModeWarlords;
extern const struct PongMode pongModePower;

const struct PongMode *pongModesGet(uint32_t index);
uint32_t pongModesCount(void);

#endif
