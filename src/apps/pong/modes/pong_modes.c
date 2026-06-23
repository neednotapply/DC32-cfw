#include "apps/pong/modes/pong_modes.h"

static const struct PongMode *const mPongModes[] = {
	&pongModeClassic,
	&pongModeDoubles,
	&pongModeQuadrapong,
	&pongModeSuper,
	&pongModeRebound,
	&pongModeBreakout,
	&pongModeWarlords,
	&pongModePower,
};

const struct PongMode *pongModesGet(uint32_t index)
{
	if (index >= pongModesCount())
		return 0;
	return mPongModes[index];
}

uint32_t pongModesCount(void)
{
	return sizeof(mPongModes) / sizeof(mPongModes[0]);
}
