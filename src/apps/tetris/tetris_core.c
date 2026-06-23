/*
 * Native DC32 falling-block engine derived from NullpoMino commit
 * 4de098dd0b48d991247313d8dba30b9721e6f9d9.
 * Copyright (c) 2010, NullNoname. BSD 3-Clause terms are retained in
 * third_party/nullpomino/LICENSE-NULLNONAME.
 */
#include "apps/tetris/tetris_core.h"

#include <string.h>

static const int16_t mMarathonGravity[20] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 465, 731, 1280, 1707, -1, -1, -1,
};

static const int16_t mMarathonDenominator[20] = {
	63, 50, 39, 30, 22, 16, 12, 8, 6, 4, 3, 2, 1, 256, 256, 256, 256, 256, 256, 256,
};

static const int8_t mNormalLeft[4][4][2] = {
	{{1,0},{1,-1},{0,2},{1,2}},
	{{1,0},{1,1},{0,-2},{1,-2}},
	{{-1,0},{-1,-1},{0,2},{-1,2}},
	{{-1,0},{-1,1},{0,-2},{-1,-2}},
};

static const int8_t mNormalRight[4][4][2] = {
	{{-1,0},{-1,-1},{0,2},{-1,2}},
	{{1,0},{1,1},{0,-2},{1,-2}},
	{{1,0},{1,-1},{0,2},{1,2}},
	{{-1,0},{-1,1},{0,-2},{-1,-2}},
};

static const int8_t mILeft[4][4][2] = {
	{{-1,0},{2,0},{-1,-2},{2,1}},
	{{2,0},{-1,0},{2,-1},{-1,2}},
	{{1,0},{-2,0},{1,2},{-2,-1}},
	{{-2,0},{1,0},{-2,1},{1,-2}},
};

static const int8_t mIRight[4][4][2] = {
	{{-2,0},{1,0},{-2,1},{1,-2}},
	{{-1,0},{2,0},{-1,-2},{2,1}},
	{{2,0},{-1,0},{2,-1},{-1,2}},
	{{1,0},{-2,0},{1,2},{-2,-1}},
};

static void tetrisHold(struct TetrisGame *game);

static uint32_t tetrisRandom(struct TetrisGame *game)
{
	game->rng = game->rng * 1664525u + 1013904223u;
	return game->rng;
}

static uint32_t tetrisRange(struct TetrisGame *game, uint32_t limit)
{
	return limit ? tetrisRandom(game) % limit : 0u;
}

static int8_t tetrisOffsetX(const struct TetrisGame *game, uint8_t piece, uint8_t direction)
{
	if (game->rule != tetrisRuleGet(TetrisRuleNintendoR))
		return 0;
	if ((piece == TetrisPieceI || piece == TetrisPieceZ || piece == TetrisPieceS) &&
			(direction & 3u) == 3u)
		return 1;
	return 0;
}

static int8_t tetrisOffsetY(const struct TetrisGame *game, uint8_t piece, uint8_t direction)
{
	if (game->rule != tetrisRuleGet(TetrisRuleNintendoR))
		return 0;
	if ((piece == TetrisPieceI || piece == TetrisPieceZ || piece == TetrisPieceS) &&
			(direction & 3u) == 0u)
		return 1;
	return 0;
}

static int8_t tetrisSpawnOffsetX(const struct TetrisGame *game, uint8_t piece)
{
	if (game->rule != tetrisRuleGet(TetrisRuleNintendoR))
		return 0;
	return piece == TetrisPieceI || piece == TetrisPieceO ? 0 : 1;
}

bool tetrisGamePieceBlock(const struct TetrisGame *game, uint8_t piece,
	uint8_t direction, uint8_t block, int8_t *x, int8_t *y)
{
	int8_t cellX, cellY;

	if (!game || !tetrisPieceCell(piece, direction, block, &cellX, &cellY))
		return false;
	cellX += tetrisOffsetX(game, piece, direction);
	cellY += tetrisOffsetY(game, piece, direction);
	if (x) *x = cellX;
	if (y) *y = cellY;
	return true;
}

static bool tetrisOccupied(const struct TetrisGame *game, int8_t x, int8_t y)
{
	if (x < 0 || x >= TETRIS_FIELD_WIDTH || y >= TETRIS_FIELD_HEIGHT)
		return true;
	if (y < -(int8_t)game->rule->hiddenHeight)
		return true;
	return game->board[y + TETRIS_MAX_HIDDEN][x] != 0;
}

static void tetrisPieceBounds(const struct TetrisGame *game, uint8_t piece, uint8_t direction,
	int8_t *minX, int8_t *maxX, int8_t *minY, int8_t *maxY)
{
	int8_t x, y;

	*minX = 127;
	*minY = 127;
	*maxX = -127;
	*maxY = -127;
	for (uint8_t i = 0; i < 4; i++) {
		tetrisPieceCell(piece, direction, i, &x, &y);
		x += tetrisOffsetX(game, piece, direction);
		y += tetrisOffsetY(game, piece, direction);
		if (x < *minX) *minX = x;
		if (x > *maxX) *maxX = x;
		if (y < *minY) *minY = y;
		if (y > *maxY) *maxY = y;
	}
}

bool tetrisGameCollides(const struct TetrisGame *game, int8_t x, int8_t y,
	uint8_t piece, uint8_t direction)
{
	int8_t cellX, cellY;

	if (!game || piece >= TetrisPieceCount)
		return true;
	for (uint8_t i = 0; i < 4; i++) {
		tetrisGamePieceBlock(game, piece, direction, i, &cellX, &cellY);
		cellX += x;
		cellY += y;
		if (tetrisOccupied(game, cellX, cellY))
			return true;
	}
	return false;
}

int8_t tetrisGameGhostY(const struct TetrisGame *game)
{
	int8_t y;

	if (!game || !game->active)
		return 0;
	y = game->activeY;
	while (!tetrisGameCollides(game, game->activeX, y + 1,
			game->activePiece, game->activeDirection))
		y++;
	return y;
}

static void tetrisShuffleBag(struct TetrisGame *game)
{
	do {
		for (uint8_t i = 0; i < TetrisPieceCount; i++)
			game->bag[i] = i;
		for (uint8_t i = TetrisPieceCount; i > 1; i--) {
			uint8_t other = (uint8_t)tetrisRange(game, i);
			uint8_t tmp = game->bag[i - 1u];

			game->bag[i - 1u] = game->bag[other];
			game->bag[other] = tmp;
		}
	} while (game->firstBag && game->rule->firstBagNoSzo &&
		(game->bag[0] == TetrisPieceO || game->bag[0] == TetrisPieceZ ||
		 game->bag[0] == TetrisPieceS));
	game->firstBag = false;
	game->bagPosition = 0;
}

static uint8_t tetrisRandomPiece(struct TetrisGame *game)
{
	if (game->rule->bagRandomizer) {
		if (game->bagPosition >= TetrisPieceCount)
			tetrisShuffleBag(game);
		return game->bag[game->bagPosition++];
	}
	{
		uint8_t id = (uint8_t)tetrisRange(game, TetrisPieceCount + 1u);

		if (id == game->previousRandom || id == TetrisPieceCount)
			id = (uint8_t)tetrisRange(game, TetrisPieceCount);
		game->previousRandom = id;
		return id;
	}
}

static void tetrisFillQueue(struct TetrisGame *game)
{
	while (game->queueCount < TETRIS_NEXT_MAX)
		game->next[game->queueCount++] = tetrisRandomPiece(game);
}

static uint8_t tetrisPopQueue(struct TetrisGame *game)
{
	uint8_t piece;

	tetrisFillQueue(game);
	piece = game->next[0];
	for (uint8_t i = 1; i < game->queueCount; i++)
		game->next[i - 1u] = game->next[i];
	game->queueCount--;
	tetrisFillQueue(game);
	return piece;
}

static void tetrisSetMarathonSpeed(struct TetrisGame *game)
{
	uint16_t level = game->stats.level;

	if (level >= 20u)
		level = 19u;
	game->speed.gravity = mMarathonGravity[level];
	game->speed.denominator = mMarathonDenominator[level];
}

static void tetrisSetModeSpeed(struct TetrisGame *game)
{
	if (game->mode == tetrisModeGet(TetrisModeMarathon)) {
		game->speed.are = 24;
		game->speed.areLine = 24;
		game->speed.lineDelay = 40;
		game->speed.lockDelay = 30;
		game->speed.das = 14;
		tetrisSetMarathonSpeed(game);
	}
	else {
		game->speed.gravity = 4;
		game->speed.denominator = 256;
		game->speed.are = 0;
		game->speed.areLine = 0;
		game->speed.lineDelay = 0;
		game->speed.lockDelay = 30;
		game->speed.das = 14;
	}
	if (game->rule == tetrisRuleGet(TetrisRuleStandardFastB)) {
		game->speed.are = 0;
		game->speed.areLine = 0;
		if (game->speed.das > game->rule->maxDas)
			game->speed.das = game->rule->maxDas;
	}
}

static bool tetrisSpawn(struct TetrisGame *game, uint8_t piece)
{
	int8_t minX, maxX, minY, maxY;
	int8_t width;

	game->activePiece = piece;
	game->activeDirection = game->rule->spawnDirection;
	tetrisPieceBounds(game, piece, game->activeDirection, &minX, &maxX, &minY, &maxY);
	width = maxX - minX + 1;
	game->activeX = (int8_t)(-1 + (TETRIS_FIELD_WIDTH - width + 1) / 2 +
		tetrisSpawnOffsetX(game, piece));
	game->activeY = game->rule->enterAboveField ? (int8_t)(-1 - maxY) : (int8_t)-minY;
	game->active = true;
	game->holdUsed = false;
	game->lockDelay = 0;
	game->lockResets = 0;
	game->gravityCounter = 0;
	game->stateFrame = 0;
	game->lastAction = TetrisLastNone;
	game->kicked = false;
	if (tetrisGameCollides(game, game->activeX, game->activeY,
			game->activePiece, game->activeDirection)) {
		game->active = false;
		game->state = TetrisStateGameOver;
		game->stateFrame = 0;
		game->events |= TetrisEventGameOver;
		return false;
	}
	return true;
}

static bool tetrisSpawnNext(struct TetrisGame *game)
{
	bool spawned = tetrisSpawn(game, tetrisPopQueue(game));
	int8_t initialRotate = game->initialRotate;
	bool initialHold = game->initialHold;

	game->initialRotate = 0;
	game->initialHold = false;
	if (!spawned)
		return false;
	if (initialHold && game->rule->holdInitial && game->rule->hold) {
		tetrisHold(game);
		if (!game->active)
			return false;
	}
	if (initialRotate && game->rule->rotateInitial)
		(void)tetrisGameTryRotate(game, initialRotate);
	game->lockResets = 0;
	return true;
}

static bool tetrisMove(struct TetrisGame *game, int8_t direction)
{
	bool onGroundBefore;

	if (!direction || tetrisGameCollides(game, game->activeX + direction,
			game->activeY, game->activePiece, game->activeDirection))
		return false;
	onGroundBefore = tetrisGameCollides(game, game->activeX, game->activeY + 1,
		game->activePiece, game->activeDirection);
	game->activeX += direction;
	game->lastAction = TetrisLastMove;
	game->events |= TetrisEventMove;
	if (onGroundBefore && game->rule->lockResetMove &&
			game->lockResets < game->rule->lockResetLimit) {
		game->lockDelay = 0;
		game->lockResets++;
	}
	return true;
}

bool tetrisGameTryRotate(struct TetrisGame *game, int8_t direction)
{
	uint8_t oldDirection;
	uint8_t newDirection;
	const int8_t (*kicks)[4][2];
	bool onGroundBefore;

	if (!game || !game->active || (!game->rule->reverseRotate && direction < 0))
		return false;
	oldDirection = game->activeDirection;
	newDirection = (uint8_t)((oldDirection + (direction > 0 ? 1 : 3)) & 3u);
	onGroundBefore = tetrisGameCollides(game, game->activeX, game->activeY + 1,
		game->activePiece, game->activeDirection);
	if (!tetrisGameCollides(game, game->activeX, game->activeY,
			game->activePiece, newDirection)) {
		game->activeDirection = newDirection;
		game->lastAction = TetrisLastRotate;
		game->kicked = false;
		game->events |= TetrisEventRotate;
	}
	else {
		if (!game->rule->wallkick)
			return false;
		if (game->activePiece == TetrisPieceI)
			kicks = direction > 0 ? mIRight : mILeft;
		else
			kicks = direction > 0 ? mNormalRight : mNormalLeft;
		for (uint8_t i = 0; i < 4; i++) {
			int8_t x = game->activeX + kicks[oldDirection][i][0];
			int8_t y = game->activeY + kicks[oldDirection][i][1];

			if (!tetrisGameCollides(game, x, y, game->activePiece, newDirection)) {
				game->activeX = x;
				game->activeY = y;
				game->activeDirection = newDirection;
				game->lastAction = TetrisLastRotate;
				game->kicked = true;
				game->events |= TetrisEventRotate;
				break;
			}
		}
		if (game->activeDirection != newDirection)
			return false;
	}
	if (onGroundBefore && game->rule->lockResetRotate &&
			game->lockResets < game->rule->lockResetLimit) {
		game->lockDelay = 0;
		game->lockResets++;
	}
	return true;
}

static void tetrisHold(struct TetrisGame *game)
{
	uint8_t next;

	if (!game->rule->hold || game->holdUsed || !game->active)
		return;
	if (game->holdPiece == TetrisPieceNone) {
		game->holdPiece = game->activePiece;
		next = tetrisPopQueue(game);
	}
	else {
		next = game->holdPiece;
		game->holdPiece = game->activePiece;
	}
	game->active = false;
	if (tetrisSpawn(game, next)) {
		game->holdUsed = true;
		game->events |= TetrisEventHold;
	}
}

static bool tetrisRotationBlocked(const struct TetrisGame *game, int8_t direction)
{
	uint8_t newDirection = (uint8_t)((game->activeDirection +
		(direction > 0 ? 1 : 3)) & 3u);

	return tetrisGameCollides(game, game->activeX, game->activeY,
		game->activePiece, newDirection);
}

static void tetrisDetectSpin(struct TetrisGame *game, bool *spin, bool *mini)
{
	static const int8_t corners[4][2] = {{0,0},{2,0},{0,2},{2,2}};
	uint8_t count = 0;

	*spin = false;
	*mini = false;
	if (!game->mode->spinEnabled || game->activePiece != TetrisPieceT ||
			game->lastAction != TetrisLastRotate)
		return;
	*mini = tetrisRotationBlocked(game, -1) && tetrisRotationBlocked(game, 1);
	for (uint8_t i = 0; i < 4; i++)
		if (tetrisOccupied(game, game->activeX + corners[i][0],
				game->activeY + corners[i][1]))
			count++;
	*spin = count >= 3u;
	if (!*spin)
		*mini = false;
}

static uint8_t tetrisMarkLines(struct TetrisGame *game)
{
	uint8_t count = 0;

	memset(game->clearRows, 0, sizeof(game->clearRows));
	for (int8_t y = 0; y < TETRIS_FIELD_HEIGHT; y++) {
		bool full = true;

		for (int8_t x = 0; x < TETRIS_FIELD_WIDTH; x++)
			if (!game->board[y + TETRIS_MAX_HIDDEN][x]) {
				full = false;
				break;
			}
		if (full) {
			game->clearRows[y + TETRIS_MAX_HIDDEN] = true;
			count++;
		}
	}
	return count;
}

static bool tetrisEmptyAfterClear(const struct TetrisGame *game)
{
	for (uint8_t row = 0; row < TETRIS_FIELD_ROWS; row++) {
		if (game->clearRows[row])
			continue;
		for (uint8_t x = 0; x < TETRIS_FIELD_WIDTH; x++)
			if (game->board[row][x])
				return false;
	}
	return true;
}

static uint32_t tetrisScoreEvent(struct TetrisGame *game, uint8_t lines,
	bool spin, bool mini, bool allClear)
{
	uint32_t points = 0;
	uint32_t level = game->mode == tetrisModeGet(TetrisModeMarathon) ?
		game->stats.level + 1u : 1u;

	if (!game->mode->scoreEnabled)
		return 0;
	if (spin) {
		if (!lines)
			points = (mini ? 100u : 400u) * level;
		else if (lines == 1u)
			points = (mini ? (game->stats.b2b ? 300u : 200u) :
				(game->stats.b2b ? 1200u : 800u)) * level;
		else if (lines == 2u)
			points = (game->stats.b2b ? 1800u : 1200u) * level;
		else
			points = (game->stats.b2b ? 2400u : 1600u) * level;
	}
	else if (lines == 1u)
		points = 100u * level;
	else if (lines == 2u)
		points = 300u * level;
	else if (lines == 3u)
		points = 500u * level;
	else if (lines >= 4u)
		points = (game->stats.b2b ? 1200u : 800u) * level;
	if (game->mode->comboEnabled && lines && game->stats.combo)
		points += (game->stats.combo - 1u) * 50u * level;
	if (allClear)
		points += (game->mode == tetrisModeGet(TetrisModeUltra) ? 3000u : 1800u) * level;
	return points;
}

static void tetrisUpdateClearStats(struct TetrisGame *game, uint8_t lines,
	bool spin, bool mini)
{
	bool difficult = spin || lines >= 4u;
	bool allClear = lines && tetrisEmptyAfterClear(game);
	uint32_t score;

	if (lines) {
		if (game->mode->comboEnabled) {
			game->stats.combo++;
			if (game->stats.combo > game->stats.maxCombo)
				game->stats.maxCombo = game->stats.combo;
		}
		if (game->mode->b2bEnabled) {
			if (difficult) {
				game->stats.b2bCount++;
				game->stats.b2b = game->stats.b2bCount >= 2u;
			}
			else {
				game->stats.b2b = false;
				game->stats.b2bCount = 0;
			}
		}
		game->stats.lines += lines;
	}
	else {
		game->stats.combo = 0;
	}
	score = tetrisScoreEvent(game, lines, spin, mini, allClear);
	game->stats.lastScore = score;
	game->stats.score += score;
	game->stats.lastClear = lines;
	game->stats.lastTSpin = spin;
	game->stats.lastMini = mini;
	if (spin)
		game->events |= TetrisEventSpin;
}

static void tetrisRemoveLines(struct TetrisGame *game)
{
	int8_t out = TETRIS_FIELD_ROWS - 1;

	for (int8_t row = TETRIS_FIELD_ROWS - 1; row >= 0; row--) {
		if (game->clearRows[row])
			continue;
		if (out != row)
			memcpy(game->board[out], game->board[row], TETRIS_FIELD_WIDTH);
		out--;
	}
	while (out >= 0) {
		memset(game->board[out], 0, TETRIS_FIELD_WIDTH);
		out--;
	}
	memset(game->clearRows, 0, sizeof(game->clearRows));
}

static void tetrisComplete(struct TetrisGame *game)
{
	game->active = false;
	game->state = TetrisStateResult;
	game->stateFrame = 0;
	game->events |= TetrisEventComplete;
}

static bool tetrisModeComplete(const struct TetrisGame *game)
{
	if (game->mode == tetrisModeGet(TetrisModeMarathon))
		return game->stats.lines >= 150u;
	if (game->mode == tetrisModeGet(TetrisModeLineRace))
		return game->stats.lines >= 40u;
	return game->stats.time >= 7200u;
}

static void tetrisBeginAre(struct TetrisGame *game, bool afterLine)
{
	uint8_t delay = afterLine ? game->speed.areLine : game->speed.are;

	if (!afterLine && delay)
		delay += game->rule->lockFlashFrames;
	game->active = false;
	game->state = TetrisStateAre;
	game->stateFrame = 0;
	if (!delay) {
		game->state = TetrisStateMove;
		(void)tetrisSpawnNext(game);
	}
}

static void tetrisLock(struct TetrisGame *game)
{
	bool spin, mini;
	bool anyVisible = false;
	int8_t x, y;

	tetrisDetectSpin(game, &spin, &mini);
	for (uint8_t i = 0; i < 4; i++) {
		tetrisPieceCell(game->activePiece, game->activeDirection, i, &x, &y);
		x += game->activeX + tetrisOffsetX(game, game->activePiece, game->activeDirection);
		y += game->activeY + tetrisOffsetY(game, game->activePiece, game->activeDirection);
		if (y >= 0)
			anyVisible = true;
		if (x >= 0 && x < TETRIS_FIELD_WIDTH &&
				y >= -TETRIS_MAX_HIDDEN && y < TETRIS_FIELD_HEIGHT)
			game->board[y + TETRIS_MAX_HIDDEN][x] = game->activePiece + 1u;
	}
	game->stats.pieces++;
	game->active = false;
	game->events |= TetrisEventLock;
	game->clearCount = tetrisMarkLines(game);
	tetrisUpdateClearStats(game, game->clearCount, spin, mini);
	if (!anyVisible && game->rule->lockoutDeath) {
		game->state = TetrisStateGameOver;
		game->stateFrame = 0;
		game->events |= TetrisEventGameOver;
		return;
	}
	if (game->clearCount) {
		game->state = TetrisStateLineClear;
		game->stateFrame = 0;
		game->events |= TetrisEventClear;
	}
	else {
		tetrisBeginAre(game, false);
	}
}

static void tetrisFallOne(struct TetrisGame *game, bool softDrop)
{
	if (!tetrisGameCollides(game, game->activeX, game->activeY + 1,
			game->activePiece, game->activeDirection)) {
		game->activeY++;
		game->lastAction = TetrisLastFall;
		if (game->rule->lockResetFall)
			game->lockDelay = 0;
		if (softDrop && game->mode->scoreEnabled) {
			game->stats.softDrop++;
			game->stats.score++;
		}
	}
}

static void tetrisUpdateDas(struct TetrisGame *game, const struct TetrisInput *input,
	bool allowMove)
{
	int8_t direction = 0;
	bool pressed = false;

	if (input->left && !input->right) {
		direction = -1;
		pressed = input->leftPressed;
	}
	else if (input->right && !input->left) {
		direction = 1;
		pressed = input->rightPressed;
	}
	if (!game->rule->moveDiagonal && input->down)
		direction = 0;
	if (!direction) {
		game->dasDirection = 0;
		game->dasCount = 0;
		game->dasRepeat = 0;
		return;
	}
	if (direction != game->dasDirection) {
		game->dasDirection = direction;
		game->dasCount = 0;
		game->dasRepeat = 0;
		pressed = true;
	}
	if (pressed) {
		if (allowMove)
			(void)tetrisMove(game, direction);
		if (game->dasCount < game->speed.das)
			game->dasCount++;
		return;
	}
	if (game->dasCount < game->speed.das) {
		game->dasCount++;
		return;
	}
	if (!allowMove)
		return;
	if (game->dasRepeat < game->rule->dasDelay)
		game->dasRepeat++;
	if (game->dasRepeat >= game->rule->dasDelay) {
		game->dasRepeat = 0;
		if (allowMove)
			(void)tetrisMove(game, direction);
	}
}

static void tetrisUpdateMove(struct TetrisGame *game, const struct TetrisInput *input)
{
	bool allowControl;
	bool grounded;
	uint32_t gravity;

	if (!game->active && !tetrisSpawnNext(game))
		return;
	if (input->holdPressed) {
		tetrisHold(game);
		if (!game->active)
			return;
	}
	allowControl = game->stateFrame > 0u || game->rule->moveFirstFrame;
	if (allowControl && input->rotateCwPressed)
		(void)tetrisGameTryRotate(game, 1);
	if (allowControl && input->rotateCcwPressed)
		(void)tetrisGameTryRotate(game, -1);
	tetrisUpdateDas(game, input, allowControl);
	if (allowControl && input->hardDropPressed && game->rule->hardDrop) {
		int8_t ghost = tetrisGameGhostY(game);
		uint8_t fall = (uint8_t)(ghost - game->activeY);

		game->activeY = ghost;
		game->stats.hardDrop += fall;
		if (game->mode->scoreEnabled)
			game->stats.score += fall * 2u;
		game->lastAction = TetrisLastFall;
		if (game->rule->hardDropLocks) {
			tetrisLock(game);
			return;
		}
	}
	gravity = game->speed.gravity < 0 ? (uint32_t)game->speed.denominator :
		(uint32_t)game->speed.gravity;
	if (allowControl && input->down && game->rule->softDropNumerator) {
		if (game->rule == tetrisRuleGet(TetrisRuleStandard))
			gravity *= game->rule->softDropNumerator + 1u;
		else
			gravity += (uint32_t)game->speed.denominator *
				game->rule->softDropNumerator / game->rule->softDropDenominator;
	}
	game->gravityCounter += gravity;
	while (game->speed.gravity < 0 ||
			game->gravityCounter >= (uint32_t)game->speed.denominator) {
		int8_t oldY = game->activeY;

		if (game->speed.gravity >= 0)
			game->gravityCounter -= game->speed.denominator;
		tetrisFallOne(game, allowControl && input->down);
		if (game->activeY == oldY)
			break;
		if (game->speed.gravity < 0)
			continue;
	}
	grounded = tetrisGameCollides(game, game->activeX, game->activeY + 1,
		game->activePiece, game->activeDirection);
	if (grounded && allowControl) {
		if (game->lockDelay < game->speed.lockDelay)
			game->lockDelay++;
		if (!game->speed.lockDelay || game->lockDelay >= game->speed.lockDelay)
			tetrisLock(game);
	}
	else if (!grounded) {
		game->lockDelay = 0;
	}
	if (game->state == TetrisStateMove)
		game->stateFrame++;
}

static void tetrisChargeDas(struct TetrisGame *game, const struct TetrisInput *input,
	bool enabled)
{
	if (enabled)
		tetrisUpdateDas(game, input, false);
}

static void tetrisCaptureInitial(struct TetrisGame *game, const struct TetrisInput *input)
{
	if (game->rule->rotateInitial) {
		if (input->rotateCwPressed ||
				(!game->rule->rotateInitialLimit && input->rotateCw))
			game->initialRotate = 1;
		else if (input->rotateCcwPressed ||
				(!game->rule->rotateInitialLimit && input->rotateCcw))
			game->initialRotate = -1;
	}
	if (game->rule->holdInitial &&
			(input->holdPressed || (!game->rule->holdInitialLimit && input->hold)))
		game->initialHold = true;
}

void tetrisGameInit(struct TetrisGame *game, enum TetrisModeId mode,
	enum TetrisRuleId rule, uint32_t seed)
{
	if (!game)
		return;
	memset(game, 0, sizeof(*game));
	game->rule = tetrisRuleGet(rule);
	game->mode = tetrisModeGet(mode);
	game->rng = seed ? seed : 0x4e554c4cu;
	game->holdPiece = TetrisPieceNone;
	game->activePiece = TetrisPieceNone;
	game->previousRandom = TetrisPieceCount;
	game->bagPosition = TetrisPieceCount;
	game->firstBag = true;
	game->state = TetrisStateReady;
	tetrisSetModeSpeed(game);
	tetrisFillQueue(game);
}

uint32_t tetrisGameUpdate(struct TetrisGame *game, const struct TetrisInput *input)
{
	static const struct TetrisInput emptyInput;

	if (!game)
		return 0;
	if (!input)
		input = &emptyInput;
	game->events = 0;
	if (game->state == TetrisStateReady) {
		tetrisCaptureInitial(game, input);
		tetrisChargeDas(game, input, game->rule->chargeDasInReady);
		if (game->stateFrame == 0u || game->stateFrame == 50u)
			game->events |= TetrisEventCountdown;
		if (game->stateFrame >= 100u) {
			game->state = TetrisStateMove;
			game->stateFrame = 0;
			(void)tetrisSpawnNext(game);
		}
		else {
			game->stateFrame++;
		}
		return game->events;
	}
	if (game->state == TetrisStateGameOver || game->state == TetrisStateResult) {
		game->stateFrame++;
		return game->events;
	}

	game->stats.time++;
	if (game->mode == tetrisModeGet(TetrisModeUltra) && game->stats.time >= 7200u) {
		tetrisComplete(game);
		return game->events;
	}

	if (game->state == TetrisStateMove) {
		tetrisUpdateMove(game, input);
	}
	else if (game->state == TetrisStateLineClear) {
		tetrisCaptureInitial(game, input);
		tetrisChargeDas(game, input, game->rule->chargeDasInDelay);
		game->stateFrame++;
		if (game->stateFrame >= game->speed.lineDelay) {
			tetrisRemoveLines(game);
			if (game->mode == tetrisModeGet(TetrisModeMarathon)) {
				uint16_t oldLevel = game->stats.level;

				while (game->stats.level < 19u &&
						game->stats.lines >= (uint32_t)(game->stats.level + 1u) * 10u)
					game->stats.level++;
				if (game->stats.level != oldLevel) {
					tetrisSetMarathonSpeed(game);
					game->events |= TetrisEventLevel;
				}
			}
			if (tetrisModeComplete(game))
				tetrisComplete(game);
			else
				tetrisBeginAre(game, true);
		}
	}
	else if (game->state == TetrisStateAre) {
		uint8_t delay = game->clearCount ? game->speed.areLine : game->speed.are;
		bool lockFlash = !game->clearCount && delay &&
			game->stateFrame < game->rule->lockFlashFrames;

		tetrisCaptureInitial(game, input);
		if (!game->clearCount && delay)
			delay += game->rule->lockFlashFrames;
		tetrisChargeDas(game, input, lockFlash || game->rule->chargeDasInDelay);
		game->stateFrame++;
		if (game->stateFrame >= delay) {
			game->clearCount = 0;
			game->state = TetrisStateMove;
			game->stateFrame = 0;
			(void)tetrisSpawnNext(game);
		}
	}
	return game->events;
}
