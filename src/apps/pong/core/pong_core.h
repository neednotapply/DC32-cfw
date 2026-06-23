#ifndef DC32_PONG_CORE_H
#define DC32_PONG_CORE_H

#include <stdbool.h>
#include <stdint.h>

#define PONG_SCREEN_W 320
#define PONG_SCREEN_H 240
#define PONG_FP_SHIFT 8
#define PONG_FP_ONE (1 << PONG_FP_SHIFT)

#define PONG_MAX_PLAYERS 4
#define PONG_MAX_PADDLES 8
#define PONG_MAX_BALLS 4
#define PONG_MAX_BRICKS 40
#define PONG_MAX_BASES 4
#define PONG_MAX_POWERUPS 3
#define PONG_MODE_DATA_COUNT 32

enum PongEdge {
	PongEdgeLeft,
	PongEdgeRight,
	PongEdgeTop,
	PongEdgeBottom,
};

struct PongVec {
	int32_t x;
	int32_t y;
};

struct PongRect {
	int16_t x;
	int16_t y;
	int16_t w;
	int16_t h;
};

struct PongPaddle {
	struct PongRect rect;
	int16_t speed;
	int16_t minPos;
	int16_t maxPos;
	uint8_t player;
	uint8_t edge;
	bool active;
};

struct PongBall {
	struct PongVec pos;
	struct PongVec prev;
	struct PongVec vel;
	int16_t size;
	int8_t lastPlayer;
	bool active;
};

struct PongBrick {
	struct PongRect rect;
	uint8_t hits;
	bool active;
};

struct PongBase {
	struct PongRect rect;
	int16_t health;
	uint8_t player;
	bool active;
};

struct PongPowerup {
	struct PongRect rect;
	uint8_t type;
	bool active;
};

struct PongGameState {
	struct PongPaddle paddles[PONG_MAX_PADDLES];
	struct PongBall balls[PONG_MAX_BALLS];
	struct PongBrick bricks[PONG_MAX_BRICKS];
	struct PongBase bases[PONG_MAX_BASES];
	struct PongPowerup powerups[PONG_MAX_POWERUPS];
	int16_t score[PONG_MAX_PLAYERS];
	int16_t lives[PONG_MAX_PLAYERS];
	int32_t modeData[PONG_MODE_DATA_COUNT];
	uint32_t rng;
	uint32_t frame;
	uint8_t paddleCount;
	uint8_t ballCount;
	uint8_t brickCount;
	uint8_t baseCount;
	uint8_t winner;
	bool roundOver;
};

void pongCoreReset(struct PongGameState *state, uint32_t seed);
uint32_t pongCoreRand(struct PongGameState *state);
int32_t pongCoreAbs(int32_t value);
int32_t pongCoreClamp(int32_t value, int32_t minimum, int32_t maximum);
bool pongCoreRectOverlap(const struct PongRect *a, const struct PongRect *b);

int pongCoreAddPaddle(struct PongGameState *state, enum PongEdge edge, uint8_t player,
	int16_t x, int16_t y, int16_t w, int16_t h, int16_t speed,
	int16_t minPos, int16_t maxPos);
int pongCoreAddBall(struct PongGameState *state, int16_t x, int16_t y,
	int16_t vx, int16_t vy, int16_t size);
void pongCoreServeBall(struct PongGameState *state, struct PongBall *ball,
	int16_t x, int16_t y, int16_t speed, int8_t horizontalDirection);
void pongCoreMovePaddle(struct PongPaddle *paddle, int8_t axis);
void pongCoreMoveBall(struct PongBall *ball);
struct PongRect pongCoreBallRect(const struct PongBall *ball);
bool pongCoreCollidePaddle(struct PongBall *ball, const struct PongPaddle *paddle);
bool pongCoreCollideRect(struct PongBall *ball, const struct PongRect *rect, bool destroy);
void pongCoreBounceVerticalWalls(struct PongBall *ball, int16_t top, int16_t bottom);
void pongCoreBounceHorizontalWalls(struct PongBall *ball, int16_t left, int16_t right);
void pongCoreAddScore(struct PongGameState *state, uint8_t player, int16_t amount);
bool pongCoreScoreReached(const struct PongGameState *state, uint8_t player, int16_t target);
bool pongCoreLoseLife(struct PongGameState *state, uint8_t player);

#endif
