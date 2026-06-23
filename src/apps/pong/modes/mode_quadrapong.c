#include "apps/pong/modes/pong_mode_common.h"

static void quadInit(struct PongGameState *state)
{
	pongCoreAddPaddle(state, PongEdgeLeft, 0, 8, 94, 8, 52, 5, 20, 168);
	pongCoreAddPaddle(state, PongEdgeRight, 1, 304, 94, 8, 52, 5, 20, 168);
	pongCoreAddPaddle(state, PongEdgeTop, 2, 134, 8, 52, 8, 5, 20, 248);
	pongCoreAddPaddle(state, PongEdgeBottom, 3, 134, 224, 52, 8, 6, 20, 248);
	pongCoreAddBall(state, 160, 120, 3, 3, 8);
	for (uint_fast8_t i = 0; i < 4; i++)
		state->lives[i] = 5;
}

static uint8_t quadAlive(const struct PongGameState *state)
{
	uint8_t count = 0;

	for (uint_fast8_t i = 0; i < 4; i++)
		if (state->lives[i] > 0)
			count++;
	return count;
}

static void quadLoseLife(struct PongGameState *state, uint8_t player,
	struct PongAudio *audio)
{
	(void)pongCoreLoseLife(state, player);
	state->paddles[player].active = state->lives[player] > 0;
	audio->score_sound(audio->context);
	if (quadAlive(state) <= 1) {
		state->roundOver = true;
		for (uint_fast8_t i = 0; i < 4; i++)
			if (state->lives[i] > 0)
				state->winner = i;
	}
	pongCoreServeBall(state, &state->balls[0], 160, 120, 3,
		(pongCoreRand(state) & 1u) ? 1 : -1);
	state->balls[0].vel.y = ((pongCoreRand(state) & 1u) ? 3 : -3) * PONG_FP_ONE;
}

static void quadUpdate(struct PongGameState *state, const struct PongInput *input,
	struct PongAudio *audio)
{
	struct PongBall *ball;
	int32_t x, y;

	pongModeResetMatchIfConfirmed(state, input, quadInit);
	if (state->roundOver)
		return;
	pongCoreMovePaddle(&state->paddles[3], input->axisX[0]);
	pongModeMoveAiPaddle(state, 0, 0);
	pongModeMoveAiPaddle(state, 1, 0);
	pongModeMoveAiPaddle(state, 2, 0);
	ball = &state->balls[0];
	pongCoreMoveBall(ball);
	for (uint_fast8_t i = 0; i < state->paddleCount; i++)
		if (pongCoreCollidePaddle(ball, &state->paddles[i]))
			audio->click(audio->context);
	x = ball->pos.x >> PONG_FP_SHIFT;
	y = ball->pos.y >> PONG_FP_SHIFT;
	if (x < -ball->size)
		quadLoseLife(state, 0, audio);
	else if (x > PONG_SCREEN_W + ball->size)
		quadLoseLife(state, 1, audio);
	else if (y < -ball->size)
		quadLoseLife(state, 2, audio);
	else if (y > PONG_SCREEN_H + ball->size)
		quadLoseLife(state, 3, audio);
	state->frame++;
}

static void quadDraw(const struct PongGameState *state, struct PongRenderer *renderer)
{
	char value[12];

	renderer->clear(renderer->context);
	for (uint_fast8_t i = 0; i < 4; i++) {
		pongModeNumber(value, sizeof(value), state->lives[i]);
		pongModeSetColor(renderer, (enum PongColorRole)(PongColorTeam0 + i));
		renderer->draw_text(renderer->context, 18 + i * 78, 110, value);
	}
	pongModeDrawEntities(state, renderer);
	pongModeDrawMessage(state, renderer);
}

const struct PongMode pongModeQuadrapong = {
	"QUADRAPONG",
	"4 EDGES / 5 LIVES EACH",
	quadInit,
	quadUpdate,
	quadDraw,
};
