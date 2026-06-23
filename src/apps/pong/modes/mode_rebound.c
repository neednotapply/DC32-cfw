#include "apps/pong/modes/pong_mode_common.h"

static const struct PongRect mReboundNet = {157, 132, 6, 92};

static void reboundServe(struct PongGameState *state, int8_t side)
{
	struct PongBall *ball = &state->balls[0];

	ball->pos.x = (int32_t)(side < 0 ? 82 : 238) << PONG_FP_SHIFT;
	ball->pos.y = 74 << PONG_FP_SHIFT;
	ball->prev = ball->pos;
	ball->vel.x = (int32_t)(side < 0 ? 3 : -3) * PONG_FP_ONE;
	ball->vel.y = -4 * PONG_FP_ONE;
	ball->active = true;
	ball->lastPlayer = -1;
}

static void reboundInit(struct PongGameState *state)
{
	pongCoreAddPaddle(state, PongEdgeBottom, 0, 48, 216, 58, 8, 6, 8, 94);
	pongCoreAddPaddle(state, PongEdgeBottom, 1, 214, 216, 58, 8, 5, 168, 254);
	pongCoreAddBall(state, 82, 74, 3, -4, 8);
}

static void reboundUpdate(struct PongGameState *state, const struct PongInput *input,
	struct PongAudio *audio)
{
	struct PongBall *ball;
	int32_t x, y;

	pongModeResetMatchIfConfirmed(state, input, reboundInit);
	if (state->roundOver)
		return;
	pongCoreMovePaddle(&state->paddles[0], input->axisX[0]);
	pongModeMoveAiPaddle(state, 1, 0);
	ball = &state->balls[0];
	ball->vel.y += 42;
	if (ball->vel.y > (7 << PONG_FP_SHIFT))
		ball->vel.y = 7 << PONG_FP_SHIFT;
	pongCoreMoveBall(ball);
	pongCoreBounceHorizontalWalls(ball, 0, PONG_SCREEN_W);
	if ((ball->pos.y >> PONG_FP_SHIFT) < ball->size / 2 && ball->vel.y < 0)
		ball->vel.y = -ball->vel.y;
	if (pongCoreCollideRect(ball, &mReboundNet, false))
		audio->click(audio->context);
	for (uint_fast8_t i = 0; i < state->paddleCount; i++)
		if (pongCoreCollidePaddle(ball, &state->paddles[i]))
			audio->click(audio->context);
	x = ball->pos.x >> PONG_FP_SHIFT;
	y = ball->pos.y >> PONG_FP_SHIFT;
	if (y > PONG_SCREEN_H + ball->size) {
		uint8_t scorer = x < 160 ? 1u : 0u;

		pongCoreAddScore(state, scorer, 1);
		audio->score_sound(audio->context);
		reboundServe(state, scorer ? -1 : 1);
	}
	if (pongCoreScoreReached(state, 0, 9) || pongCoreScoreReached(state, 1, 9)) {
		state->roundOver = true;
		state->winner = state->score[0] > state->score[1] ? 0u : 1u;
	}
	state->frame++;
}

static void reboundDraw(const struct PongGameState *state, struct PongRenderer *renderer)
{
	renderer->clear(renderer->context);
	pongModeSetColor(renderer, PongColorField);
	renderer->draw_rect(renderer->context, mReboundNet.x, mReboundNet.y,
		mReboundNet.w, mReboundNet.h);
	renderer->draw_line(renderer->context, 0, 225, 319, 225);
	pongModeDrawScores(state, renderer, 2);
	pongModeDrawEntities(state, renderer);
	pongModeDrawMessage(state, renderer);
}

const struct PongMode pongModeRebound = {
	"REBOUND",
	"ARCADE VOLLEYBALL",
	reboundInit,
	reboundUpdate,
	reboundDraw,
};
