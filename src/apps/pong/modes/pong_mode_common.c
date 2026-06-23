#include "apps/pong/modes/pong_mode_common.h"

void pongModeNumber(char *buffer, uint32_t bufferSize, int32_t value)
{
	char reverse[12];
	uint32_t count = 0;
	uint32_t out = 0;
	uint32_t number;

	if (!bufferSize)
		return;
	if (value < 0) {
		buffer[out++] = '-';
		number = (uint32_t)-value;
	}
	else
		number = (uint32_t)value;
	if (!number)
		reverse[count++] = '0';
	while (number && count < sizeof(reverse)) {
		reverse[count++] = (char)('0' + number % 10u);
		number /= 10u;
	}
	while (count && out + 1u < bufferSize)
		buffer[out++] = reverse[--count];
	buffer[out] = 0;
}

void pongModeAddClassicPaddles(struct PongGameState *state, int16_t height, int16_t speed)
{
	pongCoreAddPaddle(state, PongEdgeLeft, 0, 14, (240 - height) / 2, 8, height,
		speed, 8, 232 - height);
	pongCoreAddPaddle(state, PongEdgeRight, 1, 298, (240 - height) / 2, 8, height,
		speed, 8, 232 - height);
}

static const struct PongBall *pongModeNearestBall(const struct PongGameState *state,
	const struct PongPaddle *paddle)
{
	const struct PongBall *best = 0;
	int32_t bestDistance = 0x7fffffff;

	for (uint_fast8_t i = 0; i < state->ballCount; i++) {
		const struct PongBall *ball = &state->balls[i];
		int32_t distance;

		if (!ball->active)
			continue;
		if (paddle->edge == PongEdgeLeft || paddle->edge == PongEdgeRight)
			distance = pongCoreAbs((ball->pos.x >> PONG_FP_SHIFT) -
				(paddle->rect.x + paddle->rect.w / 2));
		else
			distance = pongCoreAbs((ball->pos.y >> PONG_FP_SHIFT) -
				(paddle->rect.y + paddle->rect.h / 2));
		if (distance < bestDistance) {
			bestDistance = distance;
			best = ball;
		}
	}
	return best;
}

void pongModeMoveAiPaddle(struct PongGameState *state, uint8_t paddleIndex,
	const struct PongBall *ball)
{
	struct PongPaddle *paddle;
	int32_t target, center;
	int8_t axis = 0;

	if (paddleIndex >= state->paddleCount)
		return;
	paddle = &state->paddles[paddleIndex];
	if (!ball)
		ball = pongModeNearestBall(state, paddle);
	if (!ball)
		return;
	if (paddle->edge == PongEdgeLeft || paddle->edge == PongEdgeRight) {
		target = ball->pos.y >> PONG_FP_SHIFT;
		center = paddle->rect.y + paddle->rect.h / 2;
	}
	else {
		target = ball->pos.x >> PONG_FP_SHIFT;
		center = paddle->rect.x + paddle->rect.w / 2;
	}
	if (target < center - 3)
		axis = -1;
	else if (target > center + 3)
		axis = 1;
	pongCoreMovePaddle(paddle, axis);
}

void pongModeMoveHumanAndAiVertical(struct PongGameState *state,
	const struct PongInput *input, uint8_t humanPaddle, uint8_t aiPaddle)
{
	if (humanPaddle < state->paddleCount)
		pongCoreMovePaddle(&state->paddles[humanPaddle], input->axisY[0]);
	pongModeMoveAiPaddle(state, aiPaddle, 0);
}

void pongModeSetColor(struct PongRenderer *renderer, enum PongColorRole role)
{
	renderer->colorRole = (uint8_t)role;
}

void pongModeDrawEntities(const struct PongGameState *state, struct PongRenderer *renderer)
{
	for (uint_fast8_t i = 0; i < state->paddleCount; i++) {
		const struct PongPaddle *paddle = &state->paddles[i];

		if (paddle->active) {
			pongModeSetColor(renderer, (enum PongColorRole)(PongColorTeam0 +
				(paddle->player % PONG_MAX_PLAYERS)));
			renderer->draw_rect(renderer->context, paddle->rect.x, paddle->rect.y,
				paddle->rect.w, paddle->rect.h);
		}
	}
	for (uint_fast8_t i = 0; i < state->ballCount; i++) {
		const struct PongBall *ball = &state->balls[i];
		struct PongRect rect;

		if (!ball->active)
			continue;
		rect = pongCoreBallRect(ball);
		pongModeSetColor(renderer, i ? PongColorAccent : PongColorNeutral);
		renderer->draw_rect(renderer->context, rect.x, rect.y, rect.w, rect.h);
	}
}

void pongModeDrawDivider(struct PongRenderer *renderer)
{
	pongModeSetColor(renderer, PongColorField);
	for (int32_t y = 8; y < PONG_SCREEN_H; y += 14)
		renderer->draw_rect(renderer->context, 159, y, 2, 7);
}

void pongModeDrawScores(const struct PongGameState *state, struct PongRenderer *renderer,
	uint8_t count)
{
	char number[12];

	for (uint_fast8_t i = 0; i < count; i++) {
		int32_t x = count == 2 ? (i ? 238 : 72) : 18 + i * 78;

		pongModeNumber(number, sizeof(number), state->score[i]);
		pongModeSetColor(renderer, (enum PongColorRole)(PongColorTeam0 +
			(i % PONG_MAX_PLAYERS)));
		renderer->draw_text(renderer->context, x, 8, number);
	}
}

void pongModeDrawMessage(const struct PongGameState *state, struct PongRenderer *renderer)
{
	char number[12];

	if (!state->roundOver)
		return;
	pongModeSetColor(renderer, PongColorAccent);
	renderer->draw_line(renderer->context, 70, 92, 250, 92);
	renderer->draw_line(renderer->context, 250, 92, 250, 146);
	renderer->draw_line(renderer->context, 250, 146, 70, 146);
	renderer->draw_line(renderer->context, 70, 146, 70, 92);
	pongModeSetColor(renderer, PongColorNeutral);
	renderer->draw_text(renderer->context, 91, 102, "MATCH OVER");
	renderer->draw_text(renderer->context, 91, 122, "A RESTART");
	pongModeNumber(number, sizeof(number), (int32_t)state->winner + 1);
	renderer->draw_text(renderer->context, 210, 102, number);
}

void pongModeUpdateHorizontalMatch(struct PongGameState *state,
	const struct PongInput *input, struct PongAudio *audio, int16_t serveSpeed,
	uint8_t humanPaddle, uint8_t aiPaddle, int16_t targetScore)
{
	if (state->roundOver)
		return;
	pongModeMoveHumanAndAiVertical(state, input, humanPaddle, aiPaddle);
	for (uint_fast8_t i = 0; i < state->ballCount; i++) {
		struct PongBall *ball = &state->balls[i];
		int32_t x;
		bool hit = false;

		if (!ball->active)
			continue;
		pongCoreMoveBall(ball);
		pongCoreBounceVerticalWalls(ball, 0, PONG_SCREEN_H);
		for (uint_fast8_t p = 0; p < state->paddleCount; p++)
			if (pongCoreCollidePaddle(ball, &state->paddles[p]))
				hit = true;
		if (hit)
			audio->click(audio->context);
		x = ball->pos.x >> PONG_FP_SHIFT;
		if (x < -ball->size) {
			pongCoreAddScore(state, 1, 1);
			audio->score_sound(audio->context);
			pongCoreServeBall(state, ball, 160, 120, serveSpeed, 1);
		}
		else if (x > PONG_SCREEN_W + ball->size) {
			pongCoreAddScore(state, 0, 1);
			audio->score_sound(audio->context);
			pongCoreServeBall(state, ball, 160, 120, serveSpeed, -1);
		}
	}
	if (pongCoreScoreReached(state, 0, targetScore) ||
			pongCoreScoreReached(state, 1, targetScore)) {
		state->roundOver = true;
		state->winner = state->score[0] > state->score[1] ? 0u : 1u;
	}
}

void pongModeResetMatchIfConfirmed(struct PongGameState *state,
	const struct PongInput *input, void (*init)(struct PongGameState *state))
{
	uint32_t rng;

	if (!state->roundOver || !input->confirmPressed)
		return;
	rng = state->rng;
	pongCoreReset(state, rng);
	init(state);
}
