#ifndef DC32_PONG_MODE_COMMON_H
#define DC32_PONG_MODE_COMMON_H

#include "apps/pong/modes/pong_modes.h"

void pongModeAddClassicPaddles(struct PongGameState *state, int16_t height, int16_t speed);
void pongModeMoveHumanAndAiVertical(struct PongGameState *state,
	const struct PongInput *input, uint8_t humanPaddle, uint8_t aiPaddle);
void pongModeMoveAiPaddle(struct PongGameState *state, uint8_t paddleIndex,
	const struct PongBall *ball);
void pongModeSetColor(struct PongRenderer *renderer, enum PongColorRole role);
void pongModeDrawEntities(const struct PongGameState *state, struct PongRenderer *renderer);
void pongModeDrawDivider(struct PongRenderer *renderer);
void pongModeDrawScores(const struct PongGameState *state, struct PongRenderer *renderer,
	uint8_t count);
void pongModeDrawMessage(const struct PongGameState *state, struct PongRenderer *renderer);
void pongModeUpdateHorizontalMatch(struct PongGameState *state,
	const struct PongInput *input, struct PongAudio *audio, int16_t serveSpeed,
	uint8_t humanPaddle, uint8_t aiPaddle, int16_t targetScore);
void pongModeResetMatchIfConfirmed(struct PongGameState *state,
	const struct PongInput *input, void (*init)(struct PongGameState *state));
void pongModeNumber(char *buffer, uint32_t bufferSize, int32_t value);

#endif
