#include "apps/pong/modes/pong_mode_common.h"

static void classicInit(struct PongGameState *state)
{
	pongModeAddClassicPaddles(state, 60, 6);
	pongCoreAddBall(state, 160, 120, 3, 2, 8);
}

static void classicUpdate(struct PongGameState *state, const struct PongInput *input,
	struct PongAudio *audio)
{
	pongModeResetMatchIfConfirmed(state, input, classicInit);
	pongModeUpdateHorizontalMatch(state, input, audio, 3, 0, 1, 9);
	state->frame++;
}

static void classicDraw(const struct PongGameState *state, struct PongRenderer *renderer)
{
	renderer->clear(renderer->context);
	pongModeDrawDivider(renderer);
	pongModeDrawScores(state, renderer, 2);
	pongModeDrawEntities(state, renderer);
	pongModeDrawMessage(state, renderer);
}

const struct PongMode pongModeClassic = {
	"CLASSIC PONG",
	"2 PADDLES / FIRST TO 9",
	classicInit,
	classicUpdate,
	classicDraw,
};
