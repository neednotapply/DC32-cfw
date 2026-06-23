#include "apps/pong/modes/pong_mode_common.h"

#define BREAKOUT_COLUMNS 8
#define BREAKOUT_ROWS 4

static void breakoutBuildBricks(struct PongGameState *state)
{
	state->brickCount = BREAKOUT_COLUMNS * BREAKOUT_ROWS;
	for (uint_fast8_t row = 0; row < BREAKOUT_ROWS; row++)
		for (uint_fast8_t column = 0; column < BREAKOUT_COLUMNS; column++) {
			struct PongBrick *brick = &state->bricks[row * BREAKOUT_COLUMNS + column];

			brick->rect.x = 14 + column * 37;
			brick->rect.y = 30 + row * 18;
			brick->rect.w = 34;
			brick->rect.h = 12;
			brick->hits = 1;
			brick->active = true;
		}
}

static void breakoutServe(struct PongGameState *state)
{
	struct PongBall *ball = &state->balls[0];

	ball->pos.x = 160 << PONG_FP_SHIFT;
	ball->pos.y = 180 << PONG_FP_SHIFT;
	ball->prev = ball->pos;
	ball->vel.x = ((pongCoreRand(state) & 1u) ? 3 : -3) * PONG_FP_ONE;
	ball->vel.y = -4 * PONG_FP_ONE;
	ball->active = true;
	ball->lastPlayer = 0;
}

static void breakoutInit(struct PongGameState *state)
{
	pongCoreAddPaddle(state, PongEdgeBottom, 0, 130, 218, 60, 8, 7, 8, 252);
	pongCoreAddBall(state, 160, 180, 3, -4, 7);
	state->lives[0] = 3;
	breakoutBuildBricks(state);
}

static uint8_t breakoutRemaining(const struct PongGameState *state)
{
	uint8_t count = 0;

	for (uint_fast8_t i = 0; i < state->brickCount; i++)
		if (state->bricks[i].active)
			count++;
	return count;
}

static void breakoutUpdate(struct PongGameState *state, const struct PongInput *input,
	struct PongAudio *audio)
{
	struct PongBall *ball;
	int32_t y;

	pongModeResetMatchIfConfirmed(state, input, breakoutInit);
	if (state->roundOver)
		return;
	pongCoreMovePaddle(&state->paddles[0], input->axisX[0]);
	ball = &state->balls[0];
	pongCoreMoveBall(ball);
	pongCoreBounceHorizontalWalls(ball, 0, PONG_SCREEN_W);
	if ((ball->pos.y >> PONG_FP_SHIFT) - ball->size / 2 <= 20 && ball->vel.y < 0)
		ball->vel.y = -ball->vel.y;
	if (pongCoreCollidePaddle(ball, &state->paddles[0]))
		audio->click(audio->context);
	for (uint_fast8_t i = 0; i < state->brickCount; i++) {
		struct PongBrick *brick = &state->bricks[i];

		if (brick->active && pongCoreCollideRect(ball, &brick->rect, true)) {
			brick->active = false;
			pongCoreAddScore(state, 0, 10);
			audio->beep(audio->context, 820u, 35u);
			break;
		}
	}
	if (!breakoutRemaining(state)) {
		pongCoreAddScore(state, 0, 100);
		breakoutBuildBricks(state);
		if (pongCoreAbs(ball->vel.x) < (7 << PONG_FP_SHIFT))
			ball->vel.x += ball->vel.x < 0 ? -PONG_FP_ONE : PONG_FP_ONE;
	}
	y = ball->pos.y >> PONG_FP_SHIFT;
	if (y > PONG_SCREEN_H + ball->size) {
		(void)pongCoreLoseLife(state, 0);
		audio->score_sound(audio->context);
		if (state->lives[0] <= 0) {
			state->roundOver = true;
			state->winner = 0;
		}
		else
			breakoutServe(state);
	}
	state->frame++;
}

static void breakoutDraw(const struct PongGameState *state, struct PongRenderer *renderer)
{
	char value[12];

	renderer->clear(renderer->context);
	for (uint_fast8_t i = 0; i < state->brickCount; i++) {
		const struct PongBrick *brick = &state->bricks[i];

		if (brick->active) {
			pongModeSetColor(renderer, (enum PongColorRole)(PongColorTeam0 +
				(i / BREAKOUT_COLUMNS) % PONG_MAX_PLAYERS));
			renderer->draw_rect(renderer->context, brick->rect.x, brick->rect.y,
				brick->rect.w, brick->rect.h);
		}
	}
	pongModeNumber(value, sizeof(value), state->score[0]);
	pongModeSetColor(renderer, PongColorNeutral);
	renderer->draw_text(renderer->context, 10, 8, value);
	pongModeSetColor(renderer, PongColorField);
	renderer->draw_text(renderer->context, 238, 8, "LIVES");
	pongModeNumber(value, sizeof(value), state->lives[0]);
	renderer->draw_text(renderer->context, 296, 8, value);
	pongModeDrawEntities(state, renderer);
	if (state->roundOver) {
		pongModeSetColor(renderer, PongColorAccent);
		renderer->draw_line(renderer->context, 76, 102, 244, 102);
		renderer->draw_line(renderer->context, 244, 102, 244, 144);
		renderer->draw_line(renderer->context, 244, 144, 76, 144);
		renderer->draw_line(renderer->context, 76, 144, 76, 102);
		pongModeSetColor(renderer, PongColorNeutral);
		renderer->draw_text(renderer->context, 92, 112, "A RESTART");
	}
}

const struct PongMode pongModeBreakout = {
	"BREAKOUT",
	"BRICKS / 3 LIVES",
	breakoutInit,
	breakoutUpdate,
	breakoutDraw,
};
