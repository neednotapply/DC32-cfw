#include "apps/pong/modes/pong_mode_common.h"

static void warlordsAddBase(struct PongGameState *state, uint8_t player,
	int16_t x, int16_t y)
{
	struct PongBase *base = &state->bases[state->baseCount++];

	base->rect.x = x;
	base->rect.y = y;
	base->rect.w = 28;
	base->rect.h = 28;
	base->health = 4;
	base->player = player;
	base->active = true;
	state->lives[player] = base->health;
}

static void warlordsInit(struct PongGameState *state)
{
	warlordsAddBase(state, 0, 0, 212);
	warlordsAddBase(state, 1, 292, 212);
	warlordsAddBase(state, 2, 0, 0);
	warlordsAddBase(state, 3, 292, 0);
	pongCoreAddPaddle(state, PongEdgeLeft, 0, 36, 164, 8, 42, 5, 128, 186);
	pongCoreAddPaddle(state, PongEdgeRight, 1, 276, 164, 8, 42, 5, 128, 186);
	pongCoreAddPaddle(state, PongEdgeLeft, 2, 36, 34, 8, 42, 5, 20, 70);
	pongCoreAddPaddle(state, PongEdgeRight, 3, 276, 34, 8, 42, 5, 20, 70);
	pongCoreAddBall(state, 160, 120, 4, 3, 8);
}

static uint8_t warlordsAlive(const struct PongGameState *state)
{
	uint8_t count = 0;

	for (uint_fast8_t i = 0; i < state->baseCount; i++)
		if (state->bases[i].active)
			count++;
	return count;
}

static void warlordsDamage(struct PongGameState *state, struct PongBase *base,
	struct PongAudio *audio)
{
	if (base->health > 0) {
		(void)pongCoreLoseLife(state, base->player);
		base->health = state->lives[base->player];
	}
	audio->score_sound(audio->context);
	if (!base->health) {
		base->active = false;
		state->paddles[base->player].active = false;
	}
	if (warlordsAlive(state) <= 1) {
		state->roundOver = true;
		for (uint_fast8_t i = 0; i < state->baseCount; i++)
			if (state->bases[i].active)
				state->winner = state->bases[i].player;
	}
}

static void warlordsUpdate(struct PongGameState *state, const struct PongInput *input,
	struct PongAudio *audio)
{
	struct PongBall *ball;

	pongModeResetMatchIfConfirmed(state, input, warlordsInit);
	if (state->roundOver)
		return;
	pongCoreMovePaddle(&state->paddles[0], input->axisY[0]);
	for (uint_fast8_t i = 1; i < state->paddleCount; i++)
		pongModeMoveAiPaddle(state, i, 0);
	ball = &state->balls[0];
	pongCoreMoveBall(ball);
	for (uint_fast8_t i = 0; i < state->paddleCount; i++)
		if (pongCoreCollidePaddle(ball, &state->paddles[i]))
			audio->click(audio->context);
	for (uint_fast8_t i = 0; i < state->baseCount; i++) {
		struct PongBase *base = &state->bases[i];

		if (base->active && pongCoreCollideRect(ball, &base->rect, false)) {
			warlordsDamage(state, base, audio);
			break;
		}
	}
	pongCoreBounceHorizontalWalls(ball, 0, PONG_SCREEN_W);
	pongCoreBounceVerticalWalls(ball, 0, PONG_SCREEN_H);
	state->frame++;
}

static void warlordsDraw(const struct PongGameState *state, struct PongRenderer *renderer)
{
	char value[12];

	renderer->clear(renderer->context);
	for (uint_fast8_t i = 0; i < state->baseCount; i++) {
		const struct PongBase *base = &state->bases[i];

		if (base->active) {
			pongModeSetColor(renderer, (enum PongColorRole)(PongColorTeam0 +
				base->player % PONG_MAX_PLAYERS));
			renderer->draw_line(renderer->context, base->rect.x, base->rect.y,
				base->rect.x + base->rect.w, base->rect.y);
			renderer->draw_line(renderer->context, base->rect.x + base->rect.w,
				base->rect.y, base->rect.x + base->rect.w,
				base->rect.y + base->rect.h);
			renderer->draw_line(renderer->context, base->rect.x + base->rect.w,
				base->rect.y + base->rect.h, base->rect.x,
				base->rect.y + base->rect.h);
			renderer->draw_line(renderer->context, base->rect.x,
				base->rect.y + base->rect.h, base->rect.x, base->rect.y);
			pongModeNumber(value, sizeof(value), base->health);
			renderer->draw_text(renderer->context, base->rect.x + 9,
				base->rect.y + 7, value);
		}
	}
	pongModeDrawEntities(state, renderer);
	pongModeDrawMessage(state, renderer);
}

const struct PongMode pongModeWarlords = {
	"WARLORDS-LITE",
	"4 BASES / LAST ALIVE",
	warlordsInit,
	warlordsUpdate,
	warlordsDraw,
};
