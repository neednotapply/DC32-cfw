#include "apps/pong/modes/pong_mode_common.h"

static void doublesInit(struct PongGameState *state)
{
	pongCoreAddPaddle(state, PongEdgeLeft, 0, 14, 34, 8, 44, 5, 8, 84);
	pongCoreAddPaddle(state, PongEdgeLeft, 0, 14, 162, 8, 44, 5, 128, 188);
	pongCoreAddPaddle(state, PongEdgeRight, 1, 298, 34, 8, 44, 5, 8, 84);
	pongCoreAddPaddle(state, PongEdgeRight, 1, 298, 162, 8, 44, 5, 128, 188);
	pongCoreAddBall(state, 160, 120, 4, 2, 8);
}

static void doublesUpdateBall(struct PongGameState *state, struct PongAudio *audio)
{
	struct PongBall *ball = &state->balls[0];
	int32_t x;

	pongCoreMoveBall(ball);
	pongCoreBounceVerticalWalls(ball, 0, PONG_SCREEN_H);
	for (uint_fast8_t i = 0; i < state->paddleCount; i++)
		if (pongCoreCollidePaddle(ball, &state->paddles[i]))
			audio->click(audio->context);
	x = ball->pos.x >> PONG_FP_SHIFT;
	if (x < -ball->size) {
		pongCoreAddScore(state, 1, 1);
		audio->score_sound(audio->context);
		pongCoreServeBall(state, ball, 160, 120, 4, 1);
	}
	else if (x > PONG_SCREEN_W + ball->size) {
		pongCoreAddScore(state, 0, 1);
		audio->score_sound(audio->context);
		pongCoreServeBall(state, ball, 160, 120, 4, -1);
	}
}

static void doublesUpdate(struct PongGameState *state, const struct PongInput *input,
	struct PongAudio *audio)
{
	pongModeResetMatchIfConfirmed(state, input, doublesInit);
	if (state->roundOver)
		return;
	pongCoreMovePaddle(&state->paddles[0], input->axisY[0]);
	pongCoreMovePaddle(&state->paddles[1], input->axisY[0]);
	pongModeMoveAiPaddle(state, 2, &state->balls[0]);
	pongModeMoveAiPaddle(state, 3, &state->balls[0]);
	doublesUpdateBall(state, audio);
	if (pongCoreScoreReached(state, 0, 9) || pongCoreScoreReached(state, 1, 9)) {
		state->roundOver = true;
		state->winner = state->score[0] > state->score[1] ? 0u : 1u;
	}
	state->frame++;
}

static void doublesDraw(const struct PongGameState *state, struct PongRenderer *renderer)
{
	renderer->clear(renderer->context);
	pongModeDrawDivider(renderer);
	pongModeDrawScores(state, renderer, 2);
	pongModeDrawEntities(state, renderer);
	pongModeDrawMessage(state, renderer);
}

const struct PongMode pongModeDoubles = {
	"PONG DOUBLES",
	"4 PADDLES / TEAM SCORE",
	doublesInit,
	doublesUpdate,
	doublesDraw,
};
