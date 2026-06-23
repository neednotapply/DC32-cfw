#include "apps/tetris/tetris_core.h"

/*
 * Rules and piece data are adapted from NullpoMino commit
 * 4de098dd0b48d991247313d8dba30b9721e6f9d9.
 * Copyright (c) 2010, NullNoname. BSD 3-Clause terms are retained in
 * third_party/nullpomino/LICENSE-NULLNONAME.
 */

static const struct TetrisRule mRules[TetrisRuleCount] = {
	[TetrisRuleStandard] = {
		.name = "STANDARD",
		.hiddenHeight = 3,
		.nextDisplay = 6,
		.spawnDirection = 0,
		.dasDelay = 1,
		.lockResetLimit = 15,
		.softDropNumerator = 20,
		.softDropDenominator = 1,
		.maxDas = 0xff,
		.lockFlashFrames = 0,
		.bagRandomizer = true,
		.firstBagNoSzo = true,
		.hold = true,
		.ghost = true,
		.hardDrop = true,
		.hardDropLocks = true,
		.reverseRotate = true,
		.wallkick = true,
		.lockResetMove = true,
		.lockResetRotate = true,
		.lockResetFall = true,
		.moveFirstFrame = true,
		.moveDiagonal = true,
		.rotateInitial = true,
		.holdInitial = true,
		.rotateInitialLimit = false,
		.holdInitialLimit = false,
		.enterAboveField = true,
		.lockoutDeath = true,
		.chargeDasInReady = true,
		.chargeDasInDelay = true,
	},
	[TetrisRuleStandardFastB] = {
		.name = "STANDARD-FAST-B",
		.hiddenHeight = 3,
		.nextDisplay = 6,
		.spawnDirection = 0,
		.dasDelay = 1,
		.lockResetLimit = 15,
		.softDropNumerator = 1,
		.softDropDenominator = 1,
		.maxDas = 8,
		.lockFlashFrames = 0,
		.bagRandomizer = true,
		.firstBagNoSzo = true,
		.hold = true,
		.ghost = true,
		.hardDrop = true,
		.hardDropLocks = true,
		.reverseRotate = true,
		.wallkick = true,
		.lockResetMove = true,
		.lockResetRotate = true,
		.lockResetFall = true,
		.moveFirstFrame = true,
		.moveDiagonal = true,
		.rotateInitial = true,
		.holdInitial = true,
		.rotateInitialLimit = true,
		.holdInitialLimit = true,
		.enterAboveField = true,
		.lockoutDeath = true,
		.chargeDasInReady = true,
		.chargeDasInDelay = true,
	},
	[TetrisRuleNintendoR] = {
		.name = "NINTENDO-R",
		.hiddenHeight = 3,
		.nextDisplay = 1,
		.spawnDirection = 2,
		.dasDelay = 6,
		.lockResetLimit = 15,
		.softDropNumerator = 1,
		.softDropDenominator = 2,
		.maxDas = 0xff,
		.lockFlashFrames = 2,
		.bagRandomizer = false,
		.firstBagNoSzo = false,
		.hold = false,
		.ghost = false,
		.hardDrop = false,
		.hardDropLocks = false,
		.reverseRotate = true,
		.wallkick = false,
		.lockResetMove = false,
		.lockResetRotate = false,
		.lockResetFall = true,
		.moveFirstFrame = false,
		.moveDiagonal = false,
		.rotateInitial = false,
		.holdInitial = false,
		.rotateInitialLimit = false,
		.holdInitialLimit = false,
		.enterAboveField = false,
		.lockoutDeath = true,
		.chargeDasInReady = true,
		.chargeDasInDelay = false,
	},
};

static const struct TetrisMode mModes[TetrisModeCount] = {
	[TetrisModeMarathon] = {
		.name = "MARATHON",
		.description = "150 LINES / LEVEL SPEED",
		.goal = 150,
		.scoreEnabled = true,
		.spinEnabled = true,
		.b2bEnabled = true,
		.comboEnabled = true,
	},
	[TetrisModeLineRace] = {
		.name = "40 LINE RACE",
		.description = "CLEAR 40 LINES FAST",
		.goal = 40,
		.scoreEnabled = false,
		.spinEnabled = false,
		.b2bEnabled = false,
		.comboEnabled = false,
	},
	[TetrisModeUltra] = {
		.name = "ULTRA 2 MIN",
		.description = "SCORE FOR 2 MINUTES",
		.goal = 7200,
		.scoreEnabled = true,
		.spinEnabled = true,
		.b2bEnabled = true,
		.comboEnabled = true,
	},
};

static const int8_t mPieceX[TetrisPieceCount][4][4] = {
	{{0,1,2,3},{2,2,2,2},{3,2,1,0},{1,1,1,1}},
	{{2,2,1,0},{2,1,1,1},{0,0,1,2},{0,1,1,1}},
	{{0,1,1,0},{1,1,0,0},{1,0,0,1},{0,0,1,1}},
	{{0,1,1,2},{2,2,1,1},{2,1,1,0},{0,0,1,1}},
	{{1,0,1,2},{2,1,1,1},{1,2,1,0},{0,1,1,1}},
	{{0,0,1,2},{2,1,1,1},{2,2,1,0},{0,1,1,1}},
	{{2,1,1,0},{2,2,1,1},{0,1,1,2},{0,0,1,1}},
};

static const int8_t mPieceY[TetrisPieceCount][4][4] = {
	{{1,1,1,1},{0,1,2,3},{2,2,2,2},{3,2,1,0}},
	{{0,1,1,1},{2,2,1,0},{2,1,1,1},{0,0,1,2}},
	{{0,0,1,1},{0,1,1,0},{1,1,0,0},{1,0,0,1}},
	{{0,0,1,1},{0,1,1,2},{2,2,1,1},{2,1,1,0}},
	{{0,1,1,1},{1,0,1,2},{2,1,1,1},{1,2,1,0}},
	{{0,1,1,1},{0,0,1,2},{2,1,1,1},{2,2,1,0}},
	{{0,0,1,1},{2,1,1,0},{2,2,1,1},{0,1,1,2}},
};

const struct TetrisRule *tetrisRuleGet(enum TetrisRuleId id)
{
	if ((uint32_t)id >= TetrisRuleCount)
		id = TetrisRuleStandard;
	return &mRules[id];
}

const struct TetrisMode *tetrisModeGet(enum TetrisModeId id)
{
	if ((uint32_t)id >= TetrisModeCount)
		id = TetrisModeMarathon;
	return &mModes[id];
}

const char *tetrisPieceName(uint8_t piece)
{
	static const char *const names[TetrisPieceCount] = {"I", "L", "O", "Z", "T", "J", "S"};

	return piece < TetrisPieceCount ? names[piece] : "-";
}

uint8_t tetrisPieceColor(uint8_t piece)
{
	/* NullpoMino block colors: I cyan, L orange, O yellow, Z red, T purple, J blue, S green. */
	static const uint8_t colors[TetrisPieceCount] = {6, 3, 4, 2, 8, 5, 7};

	return piece < TetrisPieceCount ? colors[piece] : 0;
}

bool tetrisPieceCell(uint8_t piece, uint8_t direction, uint8_t block,
	int8_t *x, int8_t *y)
{
	if (piece >= TetrisPieceCount || block >= 4)
		return false;
	direction &= 3u;
	if (x)
		*x = mPieceX[piece][direction][block];
	if (y)
		*y = mPieceY[piece][direction][block];
	return true;
}
