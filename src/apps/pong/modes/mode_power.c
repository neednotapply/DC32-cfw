#include "apps/pong/modes/pong_mode_common.h"

enum PowerPongType {
	PowerPongEnlarge,
	PowerPongShrink,
	PowerPongSpeed,
	PowerPongSlow,
	PowerPongMulti,
};

#define POWER_NEXT_SPAWN 0
#define POWER_P0_TIMER 1
#define POWER_P1_TIMER 2

static void powerSetPaddleHeight(struct PongPaddle *paddle, int16_t height)
{
	int32_t center = paddle->rect.y + paddle->rect.h / 2;

	paddle->rect.h = height;
	paddle->maxPos = 232 - height;
	paddle->rect.y = (int16_t)pongCoreClamp(center - height / 2,
		paddle->minPos, paddle->maxPos);
}

static void powerInit(struct PongGameState *state)
{
	pongModeAddClassicPaddles(state, 52, 6);
	pongCoreAddBall(state, 160, 110, 4, 2, 8);
	pongCoreAddBall(state, 160, 132, -4, -2, 8);
	state->balls[1].active = false;
	state->modeData[POWER_NEXT_SPAWN] = 180;
}

static void powerSpawn(struct PongGameState *state)
{
	struct PongPowerup *power = &state->powerups[0];

	power->rect.x = 155;
	power->rect.y = (int16_t)(42 + pongCoreRand(state) % 148u);
	power->rect.w = 10;
	power->rect.h = 10;
	power->type = (uint8_t)(pongCoreRand(state) % 5u);
	power->active = true;
	state->modeData[POWER_NEXT_SPAWN] = (int32_t)state->frame + 240;
}

static void powerScaleBalls(struct PongGameState *state, int32_t numerator,
	int32_t denominator)
{
	for (uint_fast8_t i = 0; i < state->ballCount; i++) {
		struct PongBall *ball = &state->balls[i];
		int32_t vx = ball->vel.x * numerator / denominator;
		int32_t vy = ball->vel.y * numerator / denominator;
		int32_t minimum = 2 << PONG_FP_SHIFT;
		int32_t maximum = 8 << PONG_FP_SHIFT;

		if (!ball->active)
			continue;
		if (pongCoreAbs(vx) < minimum)
			vx = vx < 0 ? -minimum : minimum;
		if (pongCoreAbs(vx) > maximum)
			vx = vx < 0 ? -maximum : maximum;
		if (pongCoreAbs(vy) > maximum)
			vy = vy < 0 ? -maximum : maximum;
		ball->vel.x = vx;
		ball->vel.y = vy;
	}
}

static void powerApply(struct PongGameState *state, uint8_t type, int8_t owner,
	struct PongAudio *audio)
{
	uint8_t player = owner == 1 ? 1u : 0u;
	uint8_t opponent = player ^ 1u;

	switch (type) {
	case PowerPongEnlarge:
		powerSetPaddleHeight(&state->paddles[player], 74);
		state->modeData[player ? POWER_P1_TIMER : POWER_P0_TIMER] =
			(int32_t)state->frame + 240;
		break;
	case PowerPongShrink:
		powerSetPaddleHeight(&state->paddles[opponent], 32);
		state->modeData[opponent ? POWER_P1_TIMER : POWER_P0_TIMER] =
			(int32_t)state->frame + 240;
		break;
	case PowerPongSpeed:
		powerScaleBalls(state, 5, 4);
		break;
	case PowerPongSlow:
		powerScaleBalls(state, 3, 4);
		break;
	case PowerPongMulti:
		pongCoreServeBall(state, &state->balls[1], 160, 132, 4, player ? -1 : 1);
		break;
	default:
		break;
	}
	audio->beep(audio->context, 1050u, 90u);
}

static void powerUpdateTimers(struct PongGameState *state)
{
	for (uint_fast8_t player = 0; player < 2; player++) {
		uint_fast8_t timer = player ? POWER_P1_TIMER : POWER_P0_TIMER;

		if (state->modeData[timer] &&
				state->frame >= (uint32_t)state->modeData[timer]) {
			powerSetPaddleHeight(&state->paddles[player], 52);
			state->modeData[timer] = 0;
		}
	}
}

static void powerUpdate(struct PongGameState *state, const struct PongInput *input,
	struct PongAudio *audio)
{
	struct PongPowerup *power = &state->powerups[0];

	pongModeResetMatchIfConfirmed(state, input, powerInit);
	if (!state->roundOver && !power->active &&
			state->frame >= (uint32_t)state->modeData[POWER_NEXT_SPAWN])
		powerSpawn(state);
	pongModeUpdateHorizontalMatch(state, input, audio, 4, 0, 1, 12);
	if (!state->roundOver && power->active)
		for (uint_fast8_t i = 0; i < state->ballCount; i++) {
			struct PongRect ballRect;

			if (!state->balls[i].active)
				continue;
			ballRect = pongCoreBallRect(&state->balls[i]);
			if (pongCoreRectOverlap(&ballRect, &power->rect)) {
				power->active = false;
				powerApply(state, power->type, state->balls[i].lastPlayer, audio);
				break;
			}
		}
	powerUpdateTimers(state);
	state->frame++;
}

static void powerDraw(const struct PongGameState *state, struct PongRenderer *renderer)
{
	static const char *const labels[] = {"E", "S", "F", "L", "M"};
	const struct PongPowerup *power = &state->powerups[0];

	renderer->clear(renderer->context);
	pongModeDrawDivider(renderer);
	pongModeDrawScores(state, renderer, 2);
	if (power->active) {
		pongModeSetColor(renderer, PongColorAccent);
		renderer->draw_rect(renderer->context, power->rect.x, power->rect.y,
			power->rect.w, power->rect.h);
		renderer->draw_text(renderer->context, power->rect.x + 1,
			power->rect.y - 12, labels[power->type]);
	}
	pongModeDrawEntities(state, renderer);
	pongModeDrawMessage(state, renderer);
}

const struct PongMode pongModePower = {
	"POWER PONG",
	"5 POWERUPS / MULTIBALL",
	powerInit,
	powerUpdate,
	powerDraw,
};
