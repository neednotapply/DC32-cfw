#ifndef DC32_TETRIS_CORE_H
#define DC32_TETRIS_CORE_H

#include <stdbool.h>
#include <stdint.h>

#define TETRIS_FIELD_WIDTH 10
#define TETRIS_FIELD_HEIGHT 20
#define TETRIS_MAX_HIDDEN 3
#define TETRIS_FIELD_ROWS (TETRIS_FIELD_HEIGHT + TETRIS_MAX_HIDDEN)
#define TETRIS_NEXT_MAX 6

enum TetrisPiece {
	TetrisPieceI,
	TetrisPieceL,
	TetrisPieceO,
	TetrisPieceZ,
	TetrisPieceT,
	TetrisPieceJ,
	TetrisPieceS,
	TetrisPieceCount,
	TetrisPieceNone = 0xff,
};

enum TetrisRuleId {
	TetrisRuleStandard,
	TetrisRuleStandardFastB,
	TetrisRuleNintendoR,
	TetrisRuleCount,
};

enum TetrisModeId {
	TetrisModeMarathon,
	TetrisModeLineRace,
	TetrisModeUltra,
	TetrisModeCount,
};

enum TetrisState {
	TetrisStateReady,
	TetrisStateMove,
	TetrisStateLineClear,
	TetrisStateAre,
	TetrisStateGameOver,
	TetrisStateResult,
};

enum TetrisEvent {
	TetrisEventNone = 0,
	TetrisEventMove = 1u << 0,
	TetrisEventRotate = 1u << 1,
	TetrisEventHold = 1u << 2,
	TetrisEventLock = 1u << 3,
	TetrisEventClear = 1u << 4,
	TetrisEventSpin = 1u << 5,
	TetrisEventLevel = 1u << 6,
	TetrisEventCountdown = 1u << 7,
	TetrisEventGameOver = 1u << 8,
	TetrisEventComplete = 1u << 9,
};

enum TetrisLastAction {
	TetrisLastNone,
	TetrisLastMove,
	TetrisLastRotate,
	TetrisLastFall,
};

struct TetrisRule {
	const char *name;
	uint8_t hiddenHeight;
	uint8_t nextDisplay;
	uint8_t spawnDirection;
	uint8_t dasDelay;
	uint8_t lockResetLimit;
	uint8_t softDropNumerator;
	uint8_t softDropDenominator;
	uint8_t maxDas;
	uint8_t lockFlashFrames;
	bool bagRandomizer;
	bool firstBagNoSzo;
	bool hold;
	bool ghost;
	bool hardDrop;
	bool hardDropLocks;
	bool reverseRotate;
	bool wallkick;
	bool lockResetMove;
	bool lockResetRotate;
	bool lockResetFall;
	bool moveFirstFrame;
	bool moveDiagonal;
	bool rotateInitial;
	bool holdInitial;
	bool rotateInitialLimit;
	bool holdInitialLimit;
	bool enterAboveField;
	bool lockoutDeath;
	bool chargeDasInReady;
	bool chargeDasInDelay;
};

struct TetrisSpeed {
	int16_t gravity;
	int16_t denominator;
	uint8_t are;
	uint8_t areLine;
	uint8_t lineDelay;
	uint8_t lockDelay;
	uint8_t das;
};

struct TetrisMode {
	const char *name;
	const char *description;
	uint16_t goal;
	bool scoreEnabled;
	bool spinEnabled;
	bool b2bEnabled;
	bool comboEnabled;
};

struct TetrisInput {
	bool left;
	bool right;
	bool down;
	bool hardDrop;
	bool rotateCw;
	bool rotateCcw;
	bool hold;
	bool leftPressed;
	bool rightPressed;
	bool rotateCwPressed;
	bool rotateCcwPressed;
	bool holdPressed;
	bool hardDropPressed;
};

struct TetrisStats {
	uint32_t score;
	uint32_t lines;
	uint32_t time;
	uint32_t pieces;
	uint32_t softDrop;
	uint32_t hardDrop;
	uint32_t lastScore;
	uint16_t level;
	uint16_t combo;
	uint16_t maxCombo;
	uint16_t b2bCount;
	bool b2b;
	bool lastTSpin;
	bool lastMini;
	uint8_t lastClear;
};

struct TetrisGame {
	const struct TetrisRule *rule;
	const struct TetrisMode *mode;
	struct TetrisSpeed speed;
	struct TetrisStats stats;
	uint8_t board[TETRIS_FIELD_ROWS][TETRIS_FIELD_WIDTH];
	uint8_t next[TETRIS_NEXT_MAX];
	uint8_t bag[TetrisPieceCount];
	uint8_t activePiece;
	uint8_t activeDirection;
	uint8_t holdPiece;
	uint8_t bagPosition;
	uint8_t queueCount;
	uint8_t clearCount;
	uint8_t state;
	uint8_t lastAction;
	uint8_t dasCount;
	uint8_t dasRepeat;
	uint8_t lockDelay;
	uint8_t lockResets;
	uint8_t previousRandom;
	int8_t dasDirection;
	int8_t initialRotate;
	int8_t activeX;
	int8_t activeY;
	uint16_t stateFrame;
	uint32_t gravityCounter;
	uint32_t rng;
	uint32_t events;
	bool active;
	bool holdUsed;
	bool firstBag;
	bool kicked;
	bool initialHold;
	bool clearRows[TETRIS_FIELD_ROWS];
};

const struct TetrisRule *tetrisRuleGet(enum TetrisRuleId id);
const struct TetrisMode *tetrisModeGet(enum TetrisModeId id);
const char *tetrisPieceName(uint8_t piece);
uint8_t tetrisPieceColor(uint8_t piece);
bool tetrisPieceCell(uint8_t piece, uint8_t direction, uint8_t block,
	int8_t *x, int8_t *y);
bool tetrisGamePieceBlock(const struct TetrisGame *game, uint8_t piece,
	uint8_t direction, uint8_t block, int8_t *x, int8_t *y);
bool tetrisGameCollides(const struct TetrisGame *game, int8_t x, int8_t y,
	uint8_t piece, uint8_t direction);
int8_t tetrisGameGhostY(const struct TetrisGame *game);
bool tetrisGameTryRotate(struct TetrisGame *game, int8_t direction);
void tetrisGameInit(struct TetrisGame *game, enum TetrisModeId mode,
	enum TetrisRuleId rule, uint32_t seed);
uint32_t tetrisGameUpdate(struct TetrisGame *game, const struct TetrisInput *input);

#endif
