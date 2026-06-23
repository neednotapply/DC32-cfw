#include "apps/pong/core/pong_core.h"

#include <string.h>

int32_t pongCoreAbs(int32_t value)
{
	return value < 0 ? -value : value;
}

int32_t pongCoreClamp(int32_t value, int32_t minimum, int32_t maximum)
{
	if (value < minimum)
		return minimum;
	if (value > maximum)
		return maximum;
	return value;
}

void pongCoreReset(struct PongGameState *state, uint32_t seed)
{
	memset(state, 0, sizeof(*state));
	state->rng = seed ? seed : 0x504f4e47u;
	state->winner = 0xffu;
}

uint32_t pongCoreRand(struct PongGameState *state)
{
	state->rng = state->rng * 1664525u + 1013904223u;
	return state->rng;
}

bool pongCoreRectOverlap(const struct PongRect *a, const struct PongRect *b)
{
	return a->x < b->x + b->w && a->x + a->w > b->x &&
		a->y < b->y + b->h && a->y + a->h > b->y;
}

int pongCoreAddPaddle(struct PongGameState *state, enum PongEdge edge, uint8_t player,
	int16_t x, int16_t y, int16_t w, int16_t h, int16_t speed,
	int16_t minPos, int16_t maxPos)
{
	struct PongPaddle *paddle;
	int index;

	if (state->paddleCount >= PONG_MAX_PADDLES)
		return -1;
	index = state->paddleCount++;
	paddle = &state->paddles[index];
	paddle->rect.x = x;
	paddle->rect.y = y;
	paddle->rect.w = w;
	paddle->rect.h = h;
	paddle->speed = speed;
	paddle->minPos = minPos;
	paddle->maxPos = maxPos;
	paddle->player = player;
	paddle->edge = (uint8_t)edge;
	paddle->active = true;
	return index;
}

int pongCoreAddBall(struct PongGameState *state, int16_t x, int16_t y,
	int16_t vx, int16_t vy, int16_t size)
{
	struct PongBall *ball;
	int index;

	if (state->ballCount >= PONG_MAX_BALLS)
		return -1;
	index = state->ballCount++;
	ball = &state->balls[index];
	ball->pos.x = (int32_t)x << PONG_FP_SHIFT;
	ball->pos.y = (int32_t)y << PONG_FP_SHIFT;
	ball->prev = ball->pos;
	ball->vel.x = (int32_t)vx << PONG_FP_SHIFT;
	ball->vel.y = (int32_t)vy << PONG_FP_SHIFT;
	ball->size = size;
	ball->lastPlayer = -1;
	ball->active = true;
	return index;
}

void pongCoreServeBall(struct PongGameState *state, struct PongBall *ball,
	int16_t x, int16_t y, int16_t speed, int8_t horizontalDirection)
{
	int32_t vertical = (int32_t)(pongCoreRand(state) % 5u) - 2;

	if (!vertical)
		vertical = (pongCoreRand(state) & 1u) ? 1 : -1;
	ball->pos.x = (int32_t)x << PONG_FP_SHIFT;
	ball->pos.y = (int32_t)y << PONG_FP_SHIFT;
	ball->prev = ball->pos;
	ball->vel.x = (int32_t)(horizontalDirection < 0 ? -speed : speed) * PONG_FP_ONE;
	ball->vel.y = vertical * PONG_FP_ONE;
	ball->lastPlayer = -1;
	ball->active = true;
}

void pongCoreMovePaddle(struct PongPaddle *paddle, int8_t axis)
{
	int32_t next;

	if (!paddle->active || !axis)
		return;
	if (paddle->edge == PongEdgeLeft || paddle->edge == PongEdgeRight) {
		next = paddle->rect.y + axis * paddle->speed;
		paddle->rect.y = (int16_t)pongCoreClamp(next, paddle->minPos, paddle->maxPos);
	}
	else {
		next = paddle->rect.x + axis * paddle->speed;
		paddle->rect.x = (int16_t)pongCoreClamp(next, paddle->minPos, paddle->maxPos);
	}
}

void pongCoreMoveBall(struct PongBall *ball)
{
	if (!ball->active)
		return;
	ball->prev = ball->pos;
	ball->pos.x += ball->vel.x;
	ball->pos.y += ball->vel.y;
}

struct PongRect pongCoreBallRect(const struct PongBall *ball)
{
	struct PongRect rect;

	rect.x = (int16_t)((ball->pos.x >> PONG_FP_SHIFT) - ball->size / 2);
	rect.y = (int16_t)((ball->pos.y >> PONG_FP_SHIFT) - ball->size / 2);
	rect.w = ball->size;
	rect.h = ball->size;
	return rect;
}

static int32_t pongCoreBallCenterAlong(const struct PongBall *ball, bool vertical)
{
	return vertical ? ball->pos.y >> PONG_FP_SHIFT : ball->pos.x >> PONG_FP_SHIFT;
}

bool pongCoreCollidePaddle(struct PongBall *ball, const struct PongPaddle *paddle)
{
	struct PongRect ballRect;
	bool vertical;
	int32_t ballCenter, paddleCenter, offset;
	int32_t minimumSpeed = 3 << PONG_FP_SHIFT;

	if (!ball->active || !paddle->active)
		return false;
	ballRect = pongCoreBallRect(ball);
	if (!pongCoreRectOverlap(&ballRect, &paddle->rect))
		return false;

	vertical = paddle->edge == PongEdgeLeft || paddle->edge == PongEdgeRight;
	ballCenter = pongCoreBallCenterAlong(ball, vertical);
	paddleCenter = vertical ? paddle->rect.y + paddle->rect.h / 2 :
		paddle->rect.x + paddle->rect.w / 2;
	offset = ballCenter - paddleCenter;
	if (vertical) {
		int32_t speed = pongCoreAbs(ball->vel.x);

		if (speed < minimumSpeed)
			speed = minimumSpeed;
		ball->vel.x = paddle->edge == PongEdgeLeft ? speed : -speed;
		ball->vel.y += offset * (PONG_FP_ONE / 12);
		ball->pos.x = (int32_t)(paddle->edge == PongEdgeLeft ?
			paddle->rect.x + paddle->rect.w + ball->size / 2 :
			paddle->rect.x - ball->size / 2) << PONG_FP_SHIFT;
	}
	else {
		int32_t speed = pongCoreAbs(ball->vel.y);

		if (speed < minimumSpeed)
			speed = minimumSpeed;
		ball->vel.y = paddle->edge == PongEdgeTop ? speed : -speed;
		ball->vel.x += offset * (PONG_FP_ONE / 12);
		ball->pos.y = (int32_t)(paddle->edge == PongEdgeTop ?
			paddle->rect.y + paddle->rect.h + ball->size / 2 :
			paddle->rect.y - ball->size / 2) << PONG_FP_SHIFT;
	}
	ball->lastPlayer = (int8_t)paddle->player;
	return true;
}

bool pongCoreCollideRect(struct PongBall *ball, const struct PongRect *rect, bool destroy)
{
	struct PongRect ballRect;
	int32_t prevX, prevY;

	(void)destroy;
	if (!ball->active)
		return false;
	ballRect = pongCoreBallRect(ball);
	if (!pongCoreRectOverlap(&ballRect, rect))
		return false;
	prevX = ball->prev.x >> PONG_FP_SHIFT;
	prevY = ball->prev.y >> PONG_FP_SHIFT;
	if (prevY + ball->size / 2 <= rect->y || prevY - ball->size / 2 >= rect->y + rect->h)
		ball->vel.y = -ball->vel.y;
	else if (prevX + ball->size / 2 <= rect->x || prevX - ball->size / 2 >= rect->x + rect->w)
		ball->vel.x = -ball->vel.x;
	else {
		ball->vel.x = -ball->vel.x;
		ball->vel.y = -ball->vel.y;
	}
	ball->pos = ball->prev;
	return true;
}

void pongCoreBounceVerticalWalls(struct PongBall *ball, int16_t top, int16_t bottom)
{
	int32_t y = ball->pos.y >> PONG_FP_SHIFT;
	int32_t half = ball->size / 2;

	if (y - half <= top && ball->vel.y < 0) {
		ball->pos.y = (int32_t)(top + half) << PONG_FP_SHIFT;
		ball->vel.y = -ball->vel.y;
	}
	else if (y + half >= bottom && ball->vel.y > 0) {
		ball->pos.y = (int32_t)(bottom - half) << PONG_FP_SHIFT;
		ball->vel.y = -ball->vel.y;
	}
}

void pongCoreBounceHorizontalWalls(struct PongBall *ball, int16_t left, int16_t right)
{
	int32_t x = ball->pos.x >> PONG_FP_SHIFT;
	int32_t half = ball->size / 2;

	if (x - half <= left && ball->vel.x < 0) {
		ball->pos.x = (int32_t)(left + half) << PONG_FP_SHIFT;
		ball->vel.x = -ball->vel.x;
	}
	else if (x + half >= right && ball->vel.x > 0) {
		ball->pos.x = (int32_t)(right - half) << PONG_FP_SHIFT;
		ball->vel.x = -ball->vel.x;
	}
}

void pongCoreAddScore(struct PongGameState *state, uint8_t player, int16_t amount)
{
	if (player < PONG_MAX_PLAYERS)
		state->score[player] += amount;
}

bool pongCoreScoreReached(const struct PongGameState *state, uint8_t player, int16_t target)
{
	return player < PONG_MAX_PLAYERS && state->score[player] >= target;
}

bool pongCoreLoseLife(struct PongGameState *state, uint8_t player)
{
	if (player >= PONG_MAX_PLAYERS || state->lives[player] <= 0)
		return false;
	state->lives[player]--;
	return state->lives[player] == 0;
}
