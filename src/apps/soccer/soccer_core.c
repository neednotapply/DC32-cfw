/*
 * Native match model based on the YSoccer 19 constants and state flow.
 * This file is GPL-2.0-only; see third_party/ysoccer19/COPYING.
 */
#include "apps/soccer/soccer_core.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define SOCCER_PLAYER_RADIUS 6.0f
#define SOCCER_POSSESSION_RADIUS 9.0f
#define SOCCER_BALL_RADIUS 4.0f
#define SOCCER_FREE_KICK_DISTANCE (10.0f * ((640.0f - 524.0f) / 12.0f))
#define SOCCER_DIRECT_SHOT_DISTANCE 310.0f
#define SOCCER_PENALTY_AREA_HALF_WIDTH 286.0f
#define SOCCER_PENALTY_AREA_DEPTH 174.0f
#define SOCCER_KEEPER_SMOTHER_RADIUS 11.0f

static uint32_t soccerRandom(struct SoccerMatch *match)
{
	uint32_t x = match->rng ? match->rng : 0x51f15e5du;

	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	match->rng = x;
	return x;
}

static float soccerDistance(float x0, float y0, float x1, float y1)
{
	float dx = x1 - x0;
	float dy = y1 - y0;

	return sqrtf(dx * dx + dy * dy);
}

static float soccerClamp(float value, float low, float high)
{
	if (value < low)
		return low;
	if (value > high)
		return high;
	return value;
}

static void soccerNormalize(float *x, float *y)
{
	float length = sqrtf(*x * *x + *y * *y);

	if (length > 0.001f) {
		*x /= length;
		*y /= length;
	}
}

static bool soccerTeamAttacksDown(const struct SoccerMatch *match, uint8_t team)
{
	return (team != 0u) != match->sidesSwitched;
}

static void soccerPlacePlayer(struct SoccerPlayer *player, uint8_t team, uint8_t number,
	float x, float y, bool attacksDown)
{
	player->team = team;
	player->number = number;
	player->role = number == 0 ? 0 : (number < 5 ? 1 : (number < 9 ? 2 : 3));
	player->x = x;
	player->y = y;
	player->targetX = x;
	player->targetY = y;
	player->speed = number == 0 ? 1.05f : 1.30f + (float)((number * 7u + team * 3u) % 5u) * 0.045f;
	player->animation = 0;
	player->facing = attacksDown ? 2u : 6u;
	player->frameY = 1u;
	player->passing = player->shooting = player->heading = player->tackling =
		player->control = player->finishing = 5u;
}

static void soccerFormationPoint(const struct SoccerMatch *match, uint8_t team,
	uint8_t formation, uint8_t number,
	float *x, float *y)
{
	static const int16_t x442[11] = {0, -360, -120, 120, 360, -360, -120, 120, 360, -150, 150};
	static const int16_t y442[11] = {590, 390, 410, 410, 390, 120, 160, 160, 120, -120, -120};
	static const int16_t x433[11] = {0, -360, -120, 120, 360, -250, 0, 250, -300, 0, 300};
	static const int16_t y433[11] = {590, 390, 410, 410, 390, 150, 190, 150, -100, -140, -100};
	static const int16_t x352[11] = {0, -250, 0, 250, -410, -205, 0, 205, 410, -150, 150};
	static const int16_t y352[11] = {590, 400, 430, 400, 140, 170, 190, 170, 140, -120, -120};
	const int16_t *xs = formation == 1 ? x433 : (formation == 2 ? x352 : x442);
	const int16_t *ys = formation == 1 ? y433 : (formation == 2 ? y352 : y442);
	float side = soccerTeamAttacksDown(match, team) ? -1.0f : 1.0f;

	*x = SOCCER_PITCH_W * 0.5f + xs[number];
	*y = SOCCER_PITCH_H * 0.5f + side * ys[number];
}

static void soccerSetFormationTargets(struct SoccerMatch *match, uint8_t team,
	uint8_t formation)
{
	uint8_t base = team * SOCCER_PLAYERS_PER_TEAM;

	for (uint8_t i = 0; i < SOCCER_PLAYERS_PER_TEAM; i++) {
		struct SoccerPlayer *player = &match->players[base + i];

		soccerFormationPoint(match, team, formation, i,
			&player->targetX, &player->targetY);
	}
}

void soccerCoreSetFormation(struct SoccerMatch *match, uint8_t team, uint8_t formation)
{
	uint8_t base = team * SOCCER_PLAYERS_PER_TEAM;

	match->formation[team] = formation;
	for (uint8_t i = 0; i < SOCCER_PLAYERS_PER_TEAM; i++) {
		float x;
		float y;

		soccerFormationPoint(match, team, formation, i, &x, &y);
		soccerPlacePlayer(&match->players[base + i], team, i, x, y,
			soccerTeamAttacksDown(match, team));
	}
}

void soccerCoreMoveToFormation(struct SoccerMatch *match, uint8_t team, uint8_t formation)
{
	match->formation[team] = formation;
	soccerSetFormationTargets(match, team, formation);
}

static void soccerResetKickoff(struct SoccerMatch *match, uint8_t team)
{
	uint8_t taker = (uint8_t)(team * SOCCER_PLAYERS_PER_TEAM + 9u);
	bool attacksDown = soccerTeamAttacksDown(match, team);

	soccerCoreSetFormation(match, 0, match->formation[0]);
	soccerCoreSetFormation(match, 1, match->formation[1]);
	match->ball.x = SOCCER_PITCH_W * 0.5f;
	match->ball.y = SOCCER_PITCH_H * 0.5f;
	match->ball.z = 0;
	match->ball.vx = 0;
	match->ball.vy = 0;
	match->ball.vz = 0;
	match->ball.spin = 0;
	match->owner = (int8_t)taker;
	match->players[taker].x = match->ball.x;
	match->players[taker].y = match->ball.y + (attacksDown ? -7.0f : 7.0f);
	match->players[taker].facing = attacksDown ? 2u : 6u;
	match->players[taker].frameY = 1u;
	match->lastTouchTeam = team;
	match->restart = SoccerRestartKickoff;
	match->restartTeam = team;
	match->restartPhase = SoccerRestartPhaseReady;
	match->restartTaker = (int8_t)taker;
	match->restartTicks = 0u;
	match->restartNoticeTicks = SOCCER_LOGIC_HZ * 2u;
}

static void soccerBeginGoalRestart(struct SoccerMatch *match, uint8_t scoringTeam)
{
	match->owner = -1;
	match->ball.vx = 0.0f;
	match->ball.vy = 0.0f;
	match->ball.vz = 0.0f;
	match->restart = SoccerRestartKickoff;
	match->restartTeam = 1u - scoringTeam;
	match->restartPhase = SoccerRestartPhaseAnnounce;
	match->restartTaker = -1;
	match->goalTeam = scoringTeam;
	match->breakReason = SoccerBreakGoal;
	match->restartTicks = SOCCER_LOGIC_HZ * 2u;
	match->restartNoticeTicks = SOCCER_LOGIC_HZ * 2u;
}

static void soccerBeginPeriodBreak(struct SoccerMatch *match, uint8_t kickoffTeam,
	enum SoccerBreakReason reason)
{
	match->owner = -1;
	match->ball.x = SOCCER_PITCH_W * 0.5f;
	match->ball.y = SOCCER_PITCH_H * 0.5f;
	match->ball.z = 0.0f;
	match->ball.vx = match->ball.vy = match->ball.vz = 0.0f;
	match->ball.spin = 0.0f;
	match->restart = SoccerRestartKickoff;
	match->restartTeam = kickoffTeam;
	match->restartTaker = -1;
	match->restartPhase = SoccerRestartPhaseAnnounce;
	match->restartTicks = SOCCER_LOGIC_HZ * 2u;
	match->restartNoticeTicks = SOCCER_LOGIC_HZ * 2u;
	match->breakReason = (uint8_t)reason;
}

static void soccerPrepareFreeKickTargets(struct SoccerMatch *match)
{
	uint8_t defendingTeam = 1u - match->restartTeam;
	uint8_t defendingBase = defendingTeam * SOCCER_PLAYERS_PER_TEAM;
	float goalY = soccerTeamAttacksDown(match, match->restartTeam) ?
		SOCCER_PITCH_H : 0.0f;
	float centerX = SOCCER_PITCH_W * 0.5f;
	float leftDistance = soccerDistance(match->ball.x, match->ball.y,
		SOCCER_GOAL_LEFT, goalY);
	float rightDistance = soccerDistance(match->ball.x, match->ball.y,
		SOCCER_GOAL_RIGHT, goalY);
	bool directShot = leftDistance < SOCCER_DIRECT_SHOT_DISTANCE ||
		rightDistance < SOCCER_DIRECT_SHOT_DISTANCE;

	match->freeKickWallMask = 0u;
	match->freeKickWallCount = 0u;
	soccerSetFormationTargets(match, 0u, match->formation[0]);
	soccerSetFormationTargets(match, 1u, match->formation[1]);
	/* As in Team.keepTargetDistanceFrom(), defenders outside the selected wall
	 * remain at least ten yards from the foul position. */
	for (uint8_t local = 1u; local < SOCCER_PLAYERS_PER_TEAM; local++) {
		struct SoccerPlayer *player = &match->players[defendingBase + local];
		float dx = player->targetX - match->ball.x;
		float dy = player->targetY - match->ball.y;
		float distance = sqrtf(dx * dx + dy * dy);

		if (distance < 0.01f) {
			dx = centerX - match->ball.x;
			dy = (soccerTeamAttacksDown(match, defendingTeam) ? 0.0f :
				SOCCER_PITCH_H) - match->ball.y;
			distance = sqrtf(dx * dx + dy * dy);
		}
		if (distance < SOCCER_FREE_KICK_DISTANCE && distance > 0.01f) {
			player->targetX = match->ball.x + dx * SOCCER_FREE_KICK_DISTANCE / distance;
			player->targetY = match->ball.y + dy * SOCCER_FREE_KICK_DISTANCE / distance;
		}
	}
	if (directShot) {
		float relativeX = match->ball.x - centerX;
		float nearPostX = relativeX < 0.0f ? SOCCER_GOAL_LEFT : SOCCER_GOAL_RIGHT;
		float barrierGoalX = centerX + (relativeX < 0.0f ? -35.5f : 35.5f);
		float nearAngle = atan2f(goalY - match->ball.y, nearPostX - match->ball.x);
		float centerAngle = atan2f(goalY - match->ball.y, centerX - match->ball.x);
		float cover = fabsf(nearAngle - centerAngle);
		float step = atan2f(12.0f, SOCCER_FREE_KICK_DISTANCE + SOCCER_PLAYER_RADIUS);
		float dx = barrierGoalX - match->ball.x;
		float dy = goalY - match->ball.y;
		float distance = sqrtf(dx * dx + dy * dy);
		uint8_t count;

		if (cover > 3.14159265f)
			cover = 6.2831853f - cover;
		count = (uint8_t)ceilf(cover / step);
		if (count < 2u) count = 2u;
		if (count > 5u) count = 5u;
		if (distance > 0.01f) {
			float centerWallX;
			float centerWallY;

			dx /= distance;
			dy /= distance;
			centerWallX = match->ball.x + dx *
				(SOCCER_FREE_KICK_DISTANCE + SOCCER_PLAYER_RADIUS);
			centerWallY = match->ball.y + dy *
				(SOCCER_FREE_KICK_DISTANCE + SOCCER_PLAYER_RADIUS);
			for (uint8_t slot = 0u; slot < count; slot++) {
				float offset = ((float)slot - ((float)count - 1.0f) * 0.5f) * 12.0f;
				float wallX = centerWallX - dy * offset;
				float wallY = centerWallY + dx * offset;
				float best = 1000000.0f;
				uint8_t selected = 0u;

				for (uint8_t local = 1u; local < SOCCER_PLAYERS_PER_TEAM; local++) {
					struct SoccerPlayer *candidate =
						&match->players[defendingBase + local];
					float candidateDistance;

					if (match->freeKickWallMask & (1u << local))
						continue;
					candidateDistance = soccerDistance(candidate->x, candidate->y,
						wallX, wallY);
					if (candidateDistance < best) {
						best = candidateDistance;
						selected = local;
					}
				}
				if (selected) {
					struct SoccerPlayer *wallPlayer =
						&match->players[defendingBase + selected];

					wallPlayer->targetX = soccerClamp(wallX, 2.0f,
						SOCCER_PITCH_W - 2.0f);
					wallPlayer->targetY = soccerClamp(wallY, 2.0f,
						SOCCER_PITCH_H - 2.0f);
					match->freeKickWallMask |= (uint16_t)(1u << selected);
					match->freeKickWallCount++;
				}
			}
		}
	}
}

void soccerCoreInit(struct SoccerMatch *match, uint32_t seed, uint8_t matchMinutes,
	uint8_t difficulty, uint8_t weather, bool cpuHome, bool training, uint8_t formation)
{
	memset(match, 0, sizeof(*match));
	match->rng = seed;
	match->difficulty = difficulty;
	match->weather = weather % SoccerWeatherCount;
	match->cpuHome = cpuHome;
	match->training = training;
	match->radar = true;
	match->owner = -1;
	match->controlled = 9;
	match->lastKicker = -1;
	match->restartTaker = -1;
	match->foulPlayer = -1;
	match->foulVictim = -1;
	match->cardPlayer = -1;
	match->halfTicks = (uint32_t)(matchMinutes ? matchMinutes : 1u) * 60u * SOCCER_LOGIC_HZ;
	soccerCoreSetFormation(match, 0, formation);
	soccerCoreSetFormation(match, 1, 0);
	soccerResetKickoff(match, (uint8_t)(soccerRandom(match) & 1u));
	match->event = SoccerEventWhistle;
}

void soccerCoreForceRestart(struct SoccerMatch *match, enum SoccerRestart restart,
	uint8_t team, float x, float y)
{
	match->restart = (uint8_t)restart;
	match->restartTeam = team;
	match->restartPhase = SoccerRestartPhasePositioning;
	match->restartTaker = -1;
	match->restartTicks = SOCCER_LOGIC_HZ * 3u;
	match->restartNoticeTicks = SOCCER_LOGIC_HZ * 2u;
	match->owner = -1;
	match->ball.x = soccerClamp(x, 5.0f, SOCCER_PITCH_W - 5.0f);
	match->ball.y = soccerClamp(y, 5.0f, SOCCER_PITCH_H - 5.0f);
	match->ball.z = 0;
	match->ball.vx = 0;
	match->ball.vy = 0;
	match->ball.vz = 0;
	match->event = SoccerEventWhistle;
	if (restart == SoccerRestartFreeKick)
		soccerPrepareFreeKickTargets(match);
	else {
		match->freeKickWallMask = 0u;
		match->freeKickWallCount = 0u;
	}
}

void soccerCoreStartExtraTime(struct SoccerMatch *match)
{
	match->finished = false;
	match->period = 2u;
	match->ticks = 0u;
	match->periodClockStart = match->clockTicks;
	match->clockRunning = false;
	match->sidesSwitched = false;
	match->bannerTicks = SOCCER_LOGIC_HZ * 2u;
	soccerBeginPeriodBreak(match, (uint8_t)(soccerRandom(match) & 1u),
		SoccerBreakExtraTime);
}

static int8_t soccerNearestPlayer(const struct SoccerMatch *match, uint8_t team,
	float x, float y, bool includeKeeper)
{
	float best = 100000.0f;
	int8_t selected = -1;
	uint8_t base = team * SOCCER_PLAYERS_PER_TEAM;

	for (uint8_t i = includeKeeper ? 0u : 1u; i < SOCCER_PLAYERS_PER_TEAM; i++) {
		const struct SoccerPlayer *player = &match->players[base + i];
		float distance;

		if (player->sentOff)
			continue;
		distance = soccerDistance(player->x, player->y, x, y);
		if (distance < best) {
			best = distance;
			selected = (int8_t)(base + i);
		}
	}
	return selected;
}

static void soccerMovePlayer(struct SoccerPlayer *player, float targetX, float targetY,
	float speed)
{
	float dx = targetX - player->x;
	float dy = targetY - player->y;
	float distance = sqrtf(dx * dx + dy * dy);
	static const uint8_t runFrames[4] = {0u, 1u, 2u, 1u};

	if (distance > 0.01f) {
		float angle = atan2f(dy, dx);
		int32_t direction = (int32_t)lroundf(angle * (4.0f / 3.14159265f));
		player->facing = (uint8_t)(direction & 7);
	}

	if (distance > 0.01f) {
		float step = distance > speed ? speed : distance;

		player->x += dx * step / distance;
		player->y += dy * step / distance;
		player->animation++;
		if (!player->actionTicks)
			player->frameY = runFrames[(player->animation >> 1) & 3u];
	}
	else {
		if (!player->actionTicks)
			player->frameY = 1u;
	}
	player->x = soccerClamp(player->x, 1.0f, SOCCER_PITCH_W - 1.0f);
	player->y = soccerClamp(player->y, -14.0f, SOCCER_PITCH_H + 14.0f);
}

static bool soccerPlayerVisible(const struct SoccerMatch *match, uint8_t index)
{
	const struct SoccerPlayer *player = &match->players[index];
	int32_t cameraX = (int32_t)match->ball.x - 160;
	int32_t cameraY = (int32_t)match->ball.y - 110;
	int32_t screenX;
	int32_t screenY;

	if (cameraX < -96) cameraX = -96;
	if (cameraX > (int32_t)SOCCER_PITCH_W - 320 + 96)
		cameraX = (int32_t)SOCCER_PITCH_W - 320 + 96;
	if (cameraY < -96) cameraY = -96;
	if (cameraY > (int32_t)SOCCER_PITCH_H - 220 + 96)
		cameraY = (int32_t)SOCCER_PITCH_H - 220 + 96;
	screenX = (int32_t)player->x - cameraX;
	screenY = 20 + (int32_t)player->y - cameraY;
	return screenX >= -16 && screenX < 336 && screenY >= 20 && screenY < 240;
}

static int8_t soccerDirectionalSwitch(const struct SoccerMatch *match,
	const struct SoccerInput *input)
{
	const struct SoccerPlayer *origin;
	float directionX = input->x;
	float directionY = input->y;
	float directionLength;
	float bestScore = 100000.0f;
	int8_t selected = -1;

	if ((!input->x && !input->y) || match->controlled < 1 ||
		match->controlled >= SOCCER_PLAYERS_PER_TEAM)
		return -1;
	origin = &match->players[match->controlled];
	directionLength = sqrtf(directionX * directionX + directionY * directionY);
	directionX /= directionLength;
	directionY /= directionLength;
	for (uint8_t i = 1u; i < SOCCER_PLAYERS_PER_TEAM; i++) {
		const struct SoccerPlayer *candidate = &match->players[i];
		float dx;
		float dy;
		float distance;
		float alignment;
		float score;

		if (i == (uint8_t)match->controlled || candidate->sentOff ||
			!soccerPlayerVisible(match, i))
			continue;
		dx = candidate->x - origin->x;
		dy = candidate->y - origin->y;
		distance = sqrtf(dx * dx + dy * dy);
		if (distance < 1.0f)
			continue;
		alignment = (dx * directionX + dy * directionY) / distance;
		if (alignment <= 0.15f)
			continue;
		/* Strongly prefer the intended ray, then the nearest player on it. */
		score = (1.0f - alignment) * 240.0f + distance * 0.08f;
		if (score < bestScore) {
			bestScore = score;
			selected = (int8_t)i;
		}
	}
	return selected;
}

static void soccerUpdateControl(struct SoccerMatch *match, const struct SoccerInput *input)
{
	int8_t nearby[4] = {-1, -1, -1, -1};
	float distances[4] = {100000.0f, 100000.0f, 100000.0f, 100000.0f};
	uint8_t count = 0u;

	if (match->cpuHome)
		return;
	for (uint8_t i = 1u; i < SOCCER_PLAYERS_PER_TEAM; i++) {
		float distance;
		uint8_t slot;

		if (match->players[i].sentOff || !soccerPlayerVisible(match, i))
			continue;
		distance = soccerDistance(match->players[i].x, match->players[i].y,
			match->ball.x, match->ball.y);
		for (slot = 0u; slot < 4u && distance >= distances[slot]; slot++);
		if (slot < 4u) {
			for (uint8_t move = 3u; move > slot; move--) {
				distances[move] = distances[move - 1u];
				nearby[move] = nearby[move - 1u];
			}
			distances[slot] = distance;
			nearby[slot] = (int8_t)i;
		}
	}
	while (count < 4u && nearby[count] >= 0)
		count++;
	if (count && input->switchPressed) {
		if (match->controlled != nearby[0])
			match->controlled = nearby[0];
		else if (input->x || input->y) {
			int8_t directional = soccerDirectionalSwitch(match, input);

			match->controlled = directional >= 0 ? directional : nearby[0];
		}
		else {
			uint8_t next = 0u;

			for (uint8_t i = 0u; i < count; i++)
				if (nearby[i] == match->controlled) {
					next = (i + 1u) % count;
					break;
				}
			match->controlled = nearby[next];
		}
	}
	else if (count && (match->controlled < 0 ||
		match->players[match->controlled].sentOff)) {
		match->controlled = nearby[0];
	}
	if (match->owner >= 0 && match->players[match->owner].team == 0)
		match->controlled = match->owner;
}

static void soccerKick(struct SoccerMatch *match, int8_t owner, int8_t ix, int8_t iy)
{
	struct SoccerPlayer *player = &match->players[owner];
	float dx = ix;
	float dy = iy;
	bool shot = match->charge >= 8u;
	float power;

	if (!shot) {
		float inputX = dx;
		float inputY = dy;
		float bestScore = -100000.0f;
		int8_t mate = -1;
		uint8_t base = player->team * SOCCER_PLAYERS_PER_TEAM;

		if (!ix && !iy)
			inputY = soccerTeamAttacksDown(match, player->team) ? 1.0f : -1.0f;
		soccerNormalize(&inputX, &inputY);
		for (uint8_t i = 1u; i < SOCCER_PLAYERS_PER_TEAM; i++) {
			int8_t candidate = (int8_t)(base + i);
			float tx;
			float ty;
			float distance;
			float alignment;
			float score;

			if (candidate == owner || match->players[candidate].sentOff)
				continue;
			tx = match->players[candidate].x - player->x;
			ty = match->players[candidate].y - player->y;
			distance = sqrtf(tx * tx + ty * ty);
			if (distance < 18.0f || distance > 210.0f)
				continue;
			alignment = (tx * inputX + ty * inputY) / distance;
			score = alignment * 180.0f - distance * 0.20f;
			if (alignment > 0.30f && score > bestScore) {
				bestScore = score;
				mate = candidate;
			}
		}
		if (mate >= 0) {
			dx = match->players[mate].x - player->x;
			dy = match->players[mate].y - player->y;
			power = soccerClamp(soccerDistance(player->x, player->y,
				match->players[mate].x, match->players[mate].y) / 18.0f, 3.0f, 6.2f);
		}
		else
		{
			dx = inputX;
			dy = inputY;
			power = 3.2f + player->passing * 0.10f;
		}
	}
	else
		power = 3.7f + soccerClamp((float)match->charge, 0, 24) * 0.15f +
			player->shooting * 0.06f;

	if (!ix && !iy) {
		if (shot) {
			dx = SOCCER_PITCH_W * 0.5f - player->x;
			dy = soccerTeamAttacksDown(match, player->team) ?
				SOCCER_PITCH_H - player->y : -player->y;
		}
	}
	soccerNormalize(&dx, &dy);
	match->owner = -1;
	match->ball.x = player->x + dx * 7.0f;
	match->ball.y = player->y + dy * 7.0f;
	match->ball.vx = dx * power;
	match->ball.vy = dy * power;
	match->ball.vz = match->charge < 8u ? 0.25f : 0.65f + match->charge * 0.025f;
	/* AI supplies a high-resolution direction (about -100..100), while human
	 * input is -1..1. Spin must use the normalized direction or AI shots curve
	 * roughly one hundred times harder than player shots. */
	match->ball.spin = soccerClamp(dx, -1.0f, 1.0f) *
		(soccerTeamAttacksDown(match, player->team) ? -0.018f : 0.018f);
	match->lastTouchTeam = player->team;
	match->lastKicker = owner;
	match->pickupCooldown = 6u;
	player->frameY = 3u;
	player->actionTicks = 12u;
	if (shot)
		match->shots[player->team]++;
	match->charge = 0;
	match->event = SoccerEventKick;
}

static void soccerGiveYellowCard(struct SoccerMatch *match, int8_t playerIndex)
{
	struct SoccerPlayer *player = &match->players[playerIndex];

	match->cardPlayer = playerIndex;
	player->yellowCards++;
	if (player->yellowCards >= 2u) {
		match->cardType = SoccerCardSecondYellow;
		player->sentOff = true;
	}
	else
		match->cardType = SoccerCardYellow;
	match->event = SoccerEventCard;
}

static void soccerGiveRedCard(struct SoccerMatch *match, int8_t playerIndex)
{
	match->cardPlayer = playerIndex;
	match->cardType = SoccerCardRed;
	match->players[playerIndex].sentOff = true;
	match->event = SoccerEventCard;
}

static void soccerHandleHuman(struct SoccerMatch *match, const struct SoccerInput *input)
{
	struct SoccerPlayer *player;

	if (match->cpuHome || match->controlled < 0)
		return;
	player = &match->players[match->controlled];
	if (input->x || input->y)
		soccerMovePlayer(player, player->x + input->x * player->speed,
			player->y + input->y * player->speed, player->speed);
	if (match->owner == match->controlled) {
		if (input->fire && match->charge < 32u) {
			match->charge++;
			/* YSoccer distinguishes a quick release pass from a held fire shot. */
			if (match->charge == 10u)
				soccerKick(match, match->controlled, input->x, input->y);
		}
		if (input->fireReleased && match->owner == match->controlled)
			soccerKick(match, match->controlled, input->x, input->y);
	}
	else if (input->firePressed && !player->tackleCooldown) {
		float ballDistance = soccerDistance(player->x, player->y, match->ball.x, match->ball.y);

		if (match->owner < 0 && ballDistance < 16.0f && match->ball.z > 6.0f &&
			match->ball.z < 34.0f) {
			float dx = input->x;
			float dy = input->y;

			if (!input->x && !input->y) {
				dx = SOCCER_PITCH_W * 0.5f - player->x;
				dy = soccerTeamAttacksDown(match, player->team) ?
					SOCCER_PITCH_H - player->y : -player->y;
			}
			soccerNormalize(&dx, &dy);
			match->ball.vx = dx * (4.2f + player->heading * 0.12f);
			match->ball.vy = dy * (4.2f + player->heading * 0.12f);
			match->ball.vz = 0.65f;
			match->ball.spin = 0;
			match->pickupCooldown = 5u;
			match->lastTouchTeam = 0u;
			match->lastKicker = match->controlled;
			player->frameY = match->ball.z > 18.0f ? 6u : 5u;
			player->actionTicks = 16u;
			match->event = SoccerEventKick;
			return;
		}
		{
		int8_t victim = soccerNearestPlayer(match, 1, player->x, player->y, false);
		float distance = victim >= 0 ? soccerDistance(player->x, player->y,
			match->players[victim].x, match->players[victim].y) : 999.0f;

		match->event = SoccerEventTackle;
		player->frameY = 4u;
		player->actionTicks = 18u;
		player->tackleCooldown = SOCCER_LOGIC_HZ;
		if (victim >= 0 && distance < 15.0f && match->owner == victim) {
			match->players[victim].tackleCooldown = SOCCER_LOGIC_HZ;
			uint32_t roll = soccerRandom(match) % 100u;
			if (roll < 62u) {
				match->owner = match->controlled;
				match->lastTouchTeam = 0;
			}
			else if (roll > 82u) {
				match->fouls[0]++;
				match->foulPlayer = match->controlled;
				match->foulVictim = victim;
				match->foulReason = SoccerFoulLateTackle;
				match->cardPlayer = -1;
				match->cardType = SoccerCardNone;
				soccerGiveYellowCard(match, match->controlled);
				soccerCoreForceRestart(match, SoccerRestartFreeKick, 1,
					match->players[victim].x, match->players[victim].y);
			}
		}
		}
	}
	if (match->owner < 0 && input->fire && (input->x || input->y)) {
		match->ball.vx += input->x * 0.004f;
		match->ball.vy += input->y * 0.004f;
		match->ball.spin += input->x * 0.0002f;
	}
}

static bool soccerTryAiTackle(struct SoccerMatch *match, uint8_t index, bool chaser)
{
	struct SoccerPlayer *player = &match->players[index];
	struct SoccerPlayer *opponent;
	float dx;
	float dy;
	float distance;
	float lungeSpeed;
	float strength;
	float angleDiff;
	float downProbability;
	float foulProbability;
	uint8_t facingDifference;
	bool wonBall;

	if (!chaser || index % SOCCER_PLAYERS_PER_TEAM == 0u || player->actionTicks ||
		player->tackleCooldown ||
		match->owner < 0 || match->players[match->owner].team == player->team)
		return false;
	opponent = &match->players[match->owner];
	dx = opponent->x - player->x;
	dy = opponent->y - player->y;
	distance = sqrtf(dx * dx + dy * dy);
	/* The compact owner model attaches the ball to the dribbler. Once a defender
	 * is already inside the upstream 12-unit slide threshold, it must still poke
	 * or slide instead of reaching its movement target and standing idle. */
	if (distance > 30.0f)
		return false;
	soccerNormalize(&dx, &dy);
	lungeSpeed = 1.4f * player->speed * (1.0f + 0.02f * player->tackling);
	player->x = soccerClamp(player->x + dx * 10.0f, 1.0f, SOCCER_PITCH_W - 1.0f);
	player->y = soccerClamp(player->y + dy * 10.0f, -14.0f, SOCCER_PITCH_H + 14.0f);
	player->facing = (uint8_t)((int32_t)lroundf(atan2f(dy, dx) *
		(4.0f / 3.14159265f)) & 7);
	player->frameY = 4u;
	player->actionTicks = 18u;
	player->tackleCooldown = SOCCER_LOGIC_HZ;
	opponent->tackleCooldown = SOCCER_LOGIC_HZ;
	match->event = SoccerEventTackle;
	distance = soccerDistance(player->x, player->y, opponent->x, opponent->y);
	if (distance >= 15.0f)
		return true;

	facingDifference = (uint8_t)abs((int)player->facing - (int)opponent->facing);
	if (facingDifference > 4u)
		facingDifference = 8u - facingDifference;
	angleDiff = facingDifference * 45.0f;
	strength = (4.0f + lungeSpeed * 100.0f / 260.0f) / 5.0f;
	downProbability = strength * ((angleDiff < 112.5f ? 0.7f : 0.9f) +
		0.01f * player->tackling - 0.01f * opponent->control);
	if (angleDiff < 67.5f)
		foulProbability = 0.80f;
	else if (angleDiff < 112.5f)
		foulProbability = 0.20f;
	else
		foulProbability = 0.30f;
	wonBall = (soccerRandom(match) % 1000u) <
		(uint32_t)soccerClamp(520.0f + 35.0f * (player->tackling - opponent->control),
			250.0f, 780.0f);
	if (wonBall) {
		match->owner = (int8_t)index;
		match->lastTouchTeam = player->team;
		return true;
	}
	if ((soccerRandom(match) % 1000u) < (uint32_t)(soccerClamp(downProbability,
		0.0f, 1.0f) * 1000.0f)) {
		opponent->frameY = 7u;
		opponent->actionTicks = 24u;
		match->owner = -1;
		match->pickupCooldown = 8u;
		if ((soccerRandom(match) % 1000u) < (uint32_t)(foulProbability * 1000.0f)) {
			match->fouls[player->team]++;
			match->foulPlayer = (int8_t)index;
			match->foulVictim = match->owner >= 0 ? match->owner :
				(int8_t)(opponent - match->players);
			match->foulReason = angleDiff < 67.5f ? SoccerFoulBackTackle :
				(angleDiff < 112.5f ? SoccerFoulSideTackle : SoccerFoulFrontTackle);
			match->cardPlayer = -1;
			match->cardType = SoccerCardNone;
			if (match->foulReason == SoccerFoulBackTackle &&
				(soccerRandom(match) % 100u) < 18u)
				soccerGiveRedCard(match, (int8_t)index);
			else if (foulProbability >= 0.8f && (soccerRandom(match) & 1u))
				soccerGiveYellowCard(match, (int8_t)index);
			soccerCoreForceRestart(match, SoccerRestartFreeKick, opponent->team,
				opponent->x, opponent->y);
		}
	}
	return true;
}

static bool soccerAiKeeper(struct SoccerMatch *match, uint8_t index)
{
	struct SoccerPlayer *keeper = &match->players[index];
	bool attacksDown;
	float goalY;
	float intoPitch;
	float ballDepth;
	float ballVelocity;
	float predictedX;
	float targetX;
	float targetY;
	float advance = 0.0f;
	float angle;
	bool charging = false;

	if (index % SOCCER_PLAYERS_PER_TEAM != 0u)
		return false;
	attacksDown = soccerTeamAttacksDown(match, keeper->team);
	if (match->owner == (int8_t)index) {
		if (!keeper->actionTicks) {
			match->charge = 14u;
			soccerKick(match, (int8_t)index, 0, attacksDown ? 1 : -1);
		}
		return true;
	}
	/* Work in distance from the keeper's own goal so both teams use exactly
	 * the same positioning and anticipation rules. */
	goalY = attacksDown ? 0.0f : SOCCER_PITCH_H;
	intoPitch = attacksDown ? 1.0f : -1.0f;
	ballDepth = (match->ball.y - goalY) * intoPitch;
	ballVelocity = match->ball.vy * intoPitch;
	predictedX = match->ball.x;
	if (match->owner >= 0 && match->owner < SOCCER_PLAYER_COUNT &&
		match->players[match->owner].team != keeper->team) {
		struct SoccerPlayer *carrier = &match->players[match->owner];
		float carrierDepth = (carrier->y - goalY) * intoPitch;

		if (carrierDepth >= 0.0f && carrierDepth <= SOCCER_PENALTY_AREA_DEPTH &&
			fabsf(carrier->x - SOCCER_PITCH_W * 0.5f) <=
			SOCCER_PENALTY_AREA_HALF_WIDTH) {
			targetX = carrier->x;
			targetY = carrier->y;
			charging = true;
		}
	}
	if (charging) {
		struct SoccerPlayer *carrier = &match->players[match->owner];

		soccerMovePlayer(keeper, targetX, targetY, keeper->speed * 1.20f);
		angle = atan2f(carrier->y - keeper->y, carrier->x - keeper->x);
		keeper->facing = (uint8_t)((int32_t)lroundf(angle *
			(4.0f / 3.14159265f)) & 7);
		if (soccerDistance(keeper->x, keeper->y, carrier->x, carrier->y) <=
			SOCCER_KEEPER_SMOTHER_RADIUS) {
			match->owner = (int8_t)index;
			match->lastTouchTeam = keeper->team;
			match->pickupCooldown = 12u;
			keeper->frameY = 9u;
			keeper->actionTicks = 20u;
			match->event = SoccerEventKeeperSave;
		}
		return true;
	}
	if (match->owner < 0 && ballDepth > 0.0f && ballVelocity < -0.05f) {
		float interceptTicks = soccerClamp(ballDepth / -ballVelocity, 0.0f, 40.0f);

		predictedX += match->ball.vx * interceptTicks;
	}
	targetX = soccerClamp(SOCCER_PITCH_W * 0.5f +
		(predictedX - SOCCER_PITCH_W * 0.5f) * 0.42f,
		SOCCER_GOAL_LEFT - 42.0f, SOCCER_GOAL_RIGHT + 42.0f);
	if (ballDepth < SOCCER_PITCH_H * 0.55f)
		advance = soccerClamp((SOCCER_PITCH_H * 0.55f - ballDepth) * 0.12f,
			0.0f, 72.0f);
	targetY = goalY + intoPitch * (8.0f + advance);
	soccerMovePlayer(keeper, targetX, targetY, keeper->speed * 0.78f);
	angle = atan2f(match->ball.y - keeper->y, match->ball.x - keeper->x);
	keeper->facing = (uint8_t)((int32_t)lroundf(angle *
		(4.0f / 3.14159265f)) & 7);
	return true;
}

static void soccerAiPlayer(struct SoccerMatch *match, uint8_t index, bool chaser)
{
	struct SoccerPlayer *player = &match->players[index];
	float targetX = player->targetX;
	float targetY = player->targetY;
	bool attacksDown = soccerTeamAttacksDown(match, player->team);
	float side = attacksDown ? -1.0f : 1.0f;

	if (player->sentOff)
		return;
	if (soccerAiKeeper(match, index))
		return;
	if (soccerTryAiTackle(match, index, chaser))
		return;
	if (match->owner == (int8_t)index) {
		float goalDistance;
		bool shoot;
		bool pass;

		targetX = SOCCER_PITCH_W * 0.5f + ((int32_t)(soccerRandom(match) % 81u) - 40) * 0.10f;
		targetY = attacksDown ? SOCCER_PITCH_H - 4.0f : 4.0f;
		goalDistance = soccerDistance(player->x, player->y, targetX, targetY);
		shoot = goalDistance < 260.0f;
		pass = !shoot && (soccerRandom(match) & 31u) == 0u;
		if (shoot || pass) {
			float dx = targetX - player->x;
			float dy = targetY - player->y;
			match->charge = shoot ? (uint16_t)(10u + soccerRandom(match) % 13u) : 0u;
			soccerNormalize(&dx, &dy);
			soccerKick(match, (int8_t)index, (int8_t)(dx * 100), (int8_t)(dy * 100));
			return;
		}
	}
	else if (chaser || soccerDistance(player->x, player->y, match->ball.x, match->ball.y) < 48.0f) {
		targetX = match->ball.x;
		targetY = match->ball.y;
	}
	else {
		targetY += side * (match->ball.y - SOCCER_PITCH_H * 0.5f) * 0.22f;
		targetX += (match->ball.x - SOCCER_PITCH_W * 0.5f) * 0.12f;
	}
	soccerMovePlayer(player, targetX, targetY,
		player->speed * (0.83f + 0.08f * match->difficulty));
}

static void soccerUpdateAi(struct SoccerMatch *match)
{
	int8_t chaser[2] = {
		soccerNearestPlayer(match, 0, match->ball.x, match->ball.y, false),
		soccerNearestPlayer(match, 1, match->ball.x, match->ball.y, false),
	};

	for (uint8_t i = 0; i < SOCCER_PLAYER_COUNT; i++) {
		if (!match->cpuHome && i == (uint8_t)match->controlled)
			continue;
		soccerAiPlayer(match, i, chaser[match->players[i].team] == (int8_t)i);
	}
}

static void soccerAcquireBall(struct SoccerMatch *match)
{
	int8_t nearest;
	float distance;

	if (match->owner >= 0 || match->pickupCooldown)
		return;
	for (uint8_t team = 0; team < 2u; team++) {
		int8_t keeper = (int8_t)(team * SOCCER_PLAYERS_PER_TEAM);
		float keeperDistance = soccerDistance(match->players[keeper].x,
			match->players[keeper].y, match->ball.x, match->ball.y);

		if (keeperDistance < 18.0f && match->ball.z < 34.0f) {
			match->owner = keeper;
			match->lastTouchTeam = team;
			match->players[keeper].frameY = match->ball.z > 14.0f ? 17u : 9u;
			match->players[keeper].actionTicks = 20u;
			match->event = SoccerEventKeeperSave;
			return;
		}
	}
	if (match->ball.z > 12.0f)
		return;
	nearest = soccerNearestPlayer(match, 0, match->ball.x, match->ball.y, true);
	if (nearest >= 0) {
		distance = soccerDistance(match->players[nearest].x, match->players[nearest].y,
			match->ball.x, match->ball.y);
		if (distance < SOCCER_POSSESSION_RADIUS)
			match->owner = nearest;
	}
	nearest = soccerNearestPlayer(match, 1, match->ball.x, match->ball.y, true);
	if (nearest >= 0) {
		distance = soccerDistance(match->players[nearest].x, match->players[nearest].y,
			match->ball.x, match->ball.y);
		if (distance < SOCCER_POSSESSION_RADIUS &&
			(match->owner < 0 || (soccerRandom(match) & 1u)))
			match->owner = nearest;
	}
	if (match->owner >= 0)
		match->lastTouchTeam = match->players[match->owner].team;
}

static void soccerGoal(struct SoccerMatch *match, uint8_t team)
{
	match->score[team]++;
	if (match->lastKicker >= 0 && match->players[match->lastKicker].team == team)
		match->playerGoals[match->lastKicker]++;
	match->event = SoccerEventGoal;
	match->bannerTicks = SOCCER_LOGIC_HZ * 2u;
	soccerBeginGoalRestart(match, team);
}

static void soccerCheckBounds(struct SoccerMatch *match)
{
	float goalY = match->ball.y < SOCCER_PITCH_H * 0.5f ? 0.0f : SOCCER_PITCH_H;
	if (fabsf(match->ball.y - goalY) < 7.0f && match->ball.z < 36.0f &&
		(fabsf(match->ball.x - SOCCER_GOAL_LEFT) < 6.0f ||
		 fabsf(match->ball.x - SOCCER_GOAL_RIGHT) < 6.0f)) {
		match->ball.vx = -match->ball.vx * 0.55f;
		match->ball.vy = -match->ball.vy * 0.55f;
		match->ball.spin = 0;
		match->event = SoccerEventPost;
		return;
	}
	if (fabsf(match->ball.y - goalY) < 6.0f && match->ball.z >= 28.0f &&
		match->ball.z <= 38.0f && match->ball.x > SOCCER_GOAL_LEFT &&
		match->ball.x < SOCCER_GOAL_RIGHT) {
		match->ball.vy = -match->ball.vy * 0.5f;
		match->ball.vz = -fabsf(match->ball.vz) * 0.5f;
		match->ball.spin = 0;
		match->event = SoccerEventPost;
		return;
	}
	if (match->ball.y < -SOCCER_BALL_RADIUS) {
		uint8_t defendingTeam = soccerTeamAttacksDown(match, 0u) ? 0u : 1u;
		uint8_t attackingTeam = 1u - defendingTeam;

		if (match->ball.x > SOCCER_GOAL_LEFT && match->ball.x < SOCCER_GOAL_RIGHT &&
			match->ball.z < 33.0f)
			soccerGoal(match, attackingTeam);
		else if (match->lastTouchTeam == attackingTeam)
			soccerCoreForceRestart(match, SoccerRestartGoalKick, defendingTeam,
				SOCCER_PITCH_W * 0.5f, 18.0f);
		else
			soccerCoreForceRestart(match, SoccerRestartCorner, attackingTeam,
				match->ball.x < SOCCER_PITCH_W * 0.5f ? 2.0f : SOCCER_PITCH_W - 2.0f, 2.0f);
	}
	else if (match->ball.y > SOCCER_PITCH_H + SOCCER_BALL_RADIUS) {
		uint8_t defendingTeam = soccerTeamAttacksDown(match, 0u) ? 1u : 0u;
		uint8_t attackingTeam = 1u - defendingTeam;

		if (match->ball.x > SOCCER_GOAL_LEFT && match->ball.x < SOCCER_GOAL_RIGHT &&
			match->ball.z < 33.0f)
			soccerGoal(match, attackingTeam);
		else if (match->lastTouchTeam == attackingTeam)
			soccerCoreForceRestart(match, SoccerRestartGoalKick, defendingTeam,
				SOCCER_PITCH_W * 0.5f, SOCCER_PITCH_H - 18.0f);
		else
			soccerCoreForceRestart(match, SoccerRestartCorner, attackingTeam,
				match->ball.x < SOCCER_PITCH_W * 0.5f ? 2.0f : SOCCER_PITCH_W - 2.0f,
				SOCCER_PITCH_H - 2.0f);
	}
	else if (match->ball.x < -SOCCER_BALL_RADIUS ||
		match->ball.x > SOCCER_PITCH_W + SOCCER_BALL_RADIUS) {
		uint8_t team = 1u - match->lastTouchTeam;
		soccerCoreForceRestart(match, SoccerRestartThrowIn, team,
			match->ball.x < 0 ? 2.0f : SOCCER_PITCH_W - 2.0f, match->ball.y);
	}
}

static void soccerUpdateBall(struct SoccerMatch *match)
{
	if (match->owner >= 0) {
		struct SoccerPlayer *owner = &match->players[match->owner];
		float direction = soccerTeamAttacksDown(match, owner->team) ? 1.0f : -1.0f;

		match->ball.x = owner->x;
		match->ball.y = owner->y + direction * 6.0f;
		match->ball.z = 0;
		match->ball.vx = 0;
		match->ball.vy = 0;
		return;
	}
	for (uint8_t sub = 0; sub < SOCCER_SUBFRAMES; sub++) {
		float friction = match->weather == SoccerWeatherRain ? 0.9980f : 0.9988f;

		match->ball.x += match->ball.vx / SOCCER_SUBFRAMES;
		match->ball.y += match->ball.vy / SOCCER_SUBFRAMES;
		match->ball.z += match->ball.vz / SOCCER_SUBFRAMES;
		match->ball.vz -= 0.08125f / SOCCER_SUBFRAMES;
		match->ball.vx += -match->ball.vy * match->ball.spin / SOCCER_SUBFRAMES;
		match->ball.vy += match->ball.vx * match->ball.spin / SOCCER_SUBFRAMES;
		match->ball.vx *= friction;
		match->ball.vy *= friction;
		match->ball.spin *= 0.999f;
		if (match->ball.z < 0) {
			match->ball.z = 0;
			if (match->ball.vz < -0.08f) {
				match->ball.vz = -match->ball.vz * 0.62f;
				match->event = SoccerEventBounce;
			}
			else
				match->ball.vz = 0;
			match->ball.vx *= 0.985f;
			match->ball.vy *= 0.985f;
		}
	}
	soccerCheckBounds(match);
}

static bool soccerRunRestart(struct SoccerMatch *match, const struct SoccerInput *input)
{
	int8_t taker = match->restartTaker;
	bool cpu;
	bool kick = false;
	bool noticeVisible;
	int8_t x = 0;
	int8_t y = soccerTeamAttacksDown(match, match->restartTeam) ? 1 : -1;

	if (!match->restart)
		return false;
	noticeVisible = match->restartNoticeTicks != 0u;
	if (match->restartNoticeTicks)
		match->restartNoticeTicks--;
	if (match->restartPhase == SoccerRestartPhaseAnnounce) {
		if (match->restartTicks)
			match->restartTicks--;
		if (!match->restartTicks) {
			soccerSetFormationTargets(match, 0u, match->formation[0]);
			soccerSetFormationTargets(match, 1u, match->formation[1]);
			match->ball.x = SOCCER_PITCH_W * 0.5f;
			match->ball.y = SOCCER_PITCH_H * 0.5f;
			match->ball.z = 0.0f;
			match->restartTaker = (int8_t)(match->restartTeam *
				SOCCER_PLAYERS_PER_TEAM + 9u);
			match->restartPhase = SoccerRestartPhasePositioning;
			match->restartTicks = SOCCER_LOGIC_HZ * 5u;
			match->restartNoticeTicks = SOCCER_LOGIC_HZ * 2u;
		}
		return true;
	}
	if (match->restartPhase == SoccerRestartPhasePositioning) {
		bool arrived = taker >= 0;
		bool fullFormation = match->restart == SoccerRestartKickoff ||
			match->restart == SoccerRestartFreeKick;

		if (taker < 0) {
			taker = soccerNearestPlayer(match, match->restartTeam,
				match->ball.x, match->ball.y,
				match->restart == SoccerRestartGoalKick);
			match->restartTaker = taker;
		}
		arrived = taker >= 0;
		for (uint8_t i = 0; i < SOCCER_PLAYER_COUNT; i++) {
			struct SoccerPlayer *player = &match->players[i];
			float targetX = player->targetX;
			float targetY = player->targetY;

			if (i == (uint8_t)taker) {
				targetX = match->ball.x;
				targetY = match->ball.y +
					(soccerTeamAttacksDown(match, match->restartTeam) ? -7.0f : 7.0f);
			}
			soccerMovePlayer(player, targetX, targetY,
				player->speed * (fullFormation ? 1.6f :
				(i == (uint8_t)taker ? 0.85f : 0.45f)));
			if ((fullFormation || i == (uint8_t)taker) && !player->sentOff &&
				soccerDistance(player->x, player->y, targetX, targetY) >= 2.5f)
				arrived = false;
		}
		if (match->restartTicks)
			match->restartTicks--;
		if (arrived) {
			if (taker >= 0) {
				match->owner = taker;
				match->lastTouchTeam = match->restartTeam;
			}
			match->restartPhase = SoccerRestartPhaseReady;
			match->restartTicks = SOCCER_LOGIC_HZ * 3u / 4u;
		}
		return true;
	}
	if (match->restartPhase != SoccerRestartPhaseReady)
		match->restartPhase = SoccerRestartPhaseReady;
	if (match->restart == SoccerRestartFreeKick && match->freeKickWallMask) {
		uint8_t defendingBase = (1u - match->restartTeam) * SOCCER_PLAYERS_PER_TEAM;

		for (uint8_t local = 1u; local < SOCCER_PLAYERS_PER_TEAM; local++)
			if (match->freeKickWallMask & (1u << local)) {
				struct SoccerPlayer *wallPlayer = &match->players[defendingBase + local];
				float angle = atan2f(match->ball.y - wallPlayer->y,
					match->ball.x - wallPlayer->x);

				wallPlayer->facing = (uint8_t)((int32_t)lroundf(angle *
					(4.0f / 3.14159265f)) & 7);
				wallPlayer->frameY = 10u;
			}
	}
	cpu = match->restartTeam != 0u || match->cpuHome;
	if (match->restartTicks)
		match->restartTicks--;
	if (noticeVisible || match->restartTicks)
		return true;
	if (cpu)
		kick = true;
	else if (input && input->firePressed) {
		kick = true;
		if (input->x || input->y) {
			x = input->x;
			y = input->y;
		}
	}
	if (match->restart == SoccerRestartThrowIn) {
		x = match->ball.x < SOCCER_PITCH_W * 0.5f ? 1 : -1;
		y = 0;
	}
	else if (match->restart == SoccerRestartCorner)
		x = match->ball.x < SOCCER_PITCH_W * 0.5f ? 1 : -1;
	if (kick && match->owner >= 0) {
		match->charge = match->restart == SoccerRestartGoalKick ? 12u : 0u;
		soccerKick(match, match->owner, x, y);
		match->restart = SoccerRestartNone;
		match->restartPhase = SoccerRestartPhaseNone;
		match->restartNoticeTicks = 0u;
		match->restartTaker = -1;
		if (match->restart == SoccerRestartNone) {
			match->clockRunning = true;
			match->breakReason = SoccerBreakNone;
		}
	}
	return true;
}

static void soccerUpdateClock(struct SoccerMatch *match)
{
	uint32_t periodLength = match->period >= 2u ? match->halfTicks / 3u :
		match->halfTicks;

	if (match->training || match->finished)
		return;
	match->ticks++;
	if (match->period == 0u && match->ticks >= periodLength) {
		match->period = 1;
		match->ticks = 0;
		match->periodClockStart = match->clockTicks;
		match->clockRunning = false;
		match->sidesSwitched = !match->sidesSwitched;
		match->event = SoccerEventHalfTime;
		match->bannerTicks = SOCCER_LOGIC_HZ * 2u;
		soccerBeginPeriodBreak(match, 1u, SoccerBreakHalfTime);
	}
	else if (match->period == 2u && match->ticks >= periodLength) {
		match->period = 3u;
		match->ticks = 0u;
		match->periodClockStart = match->clockTicks;
		match->clockRunning = false;
		match->sidesSwitched = !match->sidesSwitched;
		match->event = SoccerEventHalfTime;
		match->bannerTicks = SOCCER_LOGIC_HZ * 2u;
		soccerBeginPeriodBreak(match, 1u, SoccerBreakExtraTime);
	}
	else if (match->period >= 1u && match->ticks >= periodLength) {
		match->finished = true;
		match->clockRunning = false;
		match->event = SoccerEventFullTime;
		match->bannerTicks = SOCCER_LOGIC_HZ * 3u;
	}
}

void soccerCoreTick(struct SoccerMatch *match, const struct SoccerInput *input)
{
	match->event = SoccerEventNone;
	if (match->clockRunning && !match->training && !match->finished)
		match->clockTicks++;
	if (match->pickupCooldown)
		match->pickupCooldown--;
	for (uint8_t i = 0; i < SOCCER_PLAYER_COUNT; i++) {
		if (match->players[i].tackleCooldown)
			match->players[i].tackleCooldown--;
		if (match->players[i].actionTicks && --match->players[i].actionTicks == 0u)
			match->players[i].frameY = 1u;
	}
	if (match->finished) {
		if (match->bannerTicks)
			match->bannerTicks--;
		return;
	}
	if (match->bannerTicks)
		match->bannerTicks--;
	if (soccerRunRestart(match, input))
		return;
	soccerUpdateControl(match, input);
	soccerHandleHuman(match, input);
	soccerUpdateAi(match);
	soccerUpdateBall(match);
	if (match->restart)
		return;
	soccerAcquireBall(match);
	if (match->owner >= 0)
		match->possession[match->players[match->owner].team]++;
	soccerUpdateClock(match);
}

void soccerTableApply(struct SoccerTableRow *home, struct SoccerTableRow *away,
	uint8_t homeGoals, uint8_t awayGoals)
{
	home->played++;
	away->played++;
	home->goalsFor += homeGoals;
	home->goalsAgainst += awayGoals;
	away->goalsFor += awayGoals;
	away->goalsAgainst += homeGoals;
	if (homeGoals > awayGoals) {
		home->won++;
		away->lost++;
		home->points += 3;
	}
	else if (awayGoals > homeGoals) {
		away->won++;
		home->lost++;
		away->points += 3;
	}
	else {
		home->drawn++;
		away->drawn++;
		home->points++;
		away->points++;
	}
}

int soccerTableCompare(const struct SoccerTableRow *a, const struct SoccerTableRow *b)
{
	int goalDifferenceA = (int)a->goalsFor - a->goalsAgainst;
	int goalDifferenceB = (int)b->goalsFor - b->goalsAgainst;

	if (a->points != b->points)
		return a->points > b->points ? -1 : 1;
	if (goalDifferenceA != goalDifferenceB)
		return goalDifferenceA > goalDifferenceB ? -1 : 1;
	if (a->goalsFor != b->goalsFor)
		return a->goalsFor > b->goalsFor ? -1 : 1;
	return a->team < b->team ? -1 : (a->team > b->team ? 1 : 0);
}

bool soccerCoreSelfTest(void)
{
	struct SoccerMatch match;
	struct SoccerInput input = {0};

	soccerCoreInit(&match, 0x19dc3225u, 1u, 1u, SoccerWeatherDry, false, false, 0u);
	if (match.halfTicks != 60u * SOCCER_LOGIC_HZ)
		return false;
	soccerResetKickoff(&match, 0u);
	match.restartTicks = 0u;
	match.restartNoticeTicks = 1u;
	memset(&input, 0, sizeof(input));
	input.y = -1;
	input.firePressed = true;
	soccerCoreTick(&match, &input);
	if (match.restartNoticeTicks || match.restart != SoccerRestartKickoff)
		return false;
	input.firePressed = false;
	soccerCoreTick(&match, &input);
	if (match.restart != SoccerRestartKickoff)
		return false;
	input.firePressed = true;
	soccerCoreTick(&match, &input);
	if (match.restart != SoccerRestartNone || match.owner >= 0 ||
		match.event != SoccerEventKick || match.players[9].frameY != 3u)
		return false;

	soccerCoreInit(&match, 0x19dc3225u, 1u, 1u, SoccerWeatherDry, false, false, 0u);
	match.restart = SoccerRestartNone;
	match.restartTicks = 0u;
	match.owner = 9;
	match.players[20].x = match.players[9].x + 8.0f;
	match.players[20].y = match.players[9].y;
	if (!soccerTryAiTackle(&match, 20u, true) ||
		match.players[20].frameY != 4u || !match.players[20].actionTicks ||
		!match.players[20].tackleCooldown || !match.players[9].tackleCooldown)
		return false;

	soccerCoreInit(&match, 0x19dc3225u, 1u, 1u, SoccerWeatherDry, true, false, 0u);
	match.restart = SoccerRestartNone;
	match.restartTicks = 0u;
	match.owner = 20;
	match.charge = 12u;
	soccerKick(&match, 20, 100, 0);
	if (match.ball.vx < 1.0f || fabsf(match.ball.spin) > 0.0181f)
		return false;

	soccerCoreInit(&match, 0x19dc3225u, 1u, 1u, SoccerWeatherDry, true, false, 0u);
	match.restart = SoccerRestartNone;
	match.restartTicks = 0u;
	match.owner = 20;
	match.players[20].x = SOCCER_PITCH_W * 0.5f;
	match.players[20].y = SOCCER_PITCH_H - 120.0f;
	soccerAiPlayer(&match, 20u, true);
	if (match.owner >= 0 || match.ball.vy <= 1.0f || fabsf(match.ball.spin) > 0.0181f)
		return false;

	soccerCoreInit(&match, 0x19dc3225u, 1u, 1u, SoccerWeatherDry, true, false, 0u);
	match.restart = SoccerRestartNone;
	match.restartTicks = 0u;
	match.owner = -1;
	match.ball.x = SOCCER_GOAL_RIGHT + 80.0f;
	match.ball.y = 180.0f;
	match.ball.vx = 1.25f;
	match.ball.vy = -3.0f;
	{
		float oldKeeperX = match.players[SOCCER_PLAYERS_PER_TEAM].x;
		float topKeeperX;
		float topKeeperY;

		soccerAiKeeper(&match, SOCCER_PLAYERS_PER_TEAM);
		if (match.players[SOCCER_PLAYERS_PER_TEAM].x <= oldKeeperX)
			return false;
		topKeeperX = match.players[SOCCER_PLAYERS_PER_TEAM].x;
		topKeeperY = match.players[SOCCER_PLAYERS_PER_TEAM].y;
		soccerCoreInit(&match, 0x19dc3225u, 1u, 1u, SoccerWeatherDry,
			true, false, 0u);
		match.restart = SoccerRestartNone;
		match.owner = -1;
		match.ball.x = SOCCER_GOAL_RIGHT + 80.0f;
		match.ball.y = SOCCER_PITCH_H - 180.0f;
		match.ball.vx = 1.25f;
		match.ball.vy = 3.0f;
		soccerAiKeeper(&match, 0u);
		if (fabsf(match.players[0].x - topKeeperX) > 0.001f ||
			fabsf((SOCCER_PITCH_H - match.players[0].y) - topKeeperY) > 0.001f)
			return false;
	}
	{
		float topKeeperY;

		soccerCoreInit(&match, 0x19dc3225u, 1u, 1u, SoccerWeatherDry,
			true, false, 0u);
		match.restart = SoccerRestartNone;
		match.owner = 9;
		match.players[9].x = SOCCER_PITCH_W * 0.5f;
		match.players[9].y = 100.0f;
		match.ball.x = match.players[9].x;
		match.ball.y = match.players[9].y;
		topKeeperY = match.players[SOCCER_PLAYERS_PER_TEAM].y;
		soccerAiKeeper(&match, SOCCER_PLAYERS_PER_TEAM);
		if (match.players[SOCCER_PLAYERS_PER_TEAM].y <= topKeeperY)
			return false;
		topKeeperY = match.players[SOCCER_PLAYERS_PER_TEAM].y;

		soccerCoreInit(&match, 0x19dc3225u, 1u, 1u, SoccerWeatherDry,
			true, false, 0u);
		match.restart = SoccerRestartNone;
		match.owner = 20;
		match.players[20].x = SOCCER_PITCH_W * 0.5f;
		match.players[20].y = SOCCER_PITCH_H - 100.0f;
		match.ball.x = match.players[20].x;
		match.ball.y = match.players[20].y;
		soccerAiKeeper(&match, 0u);
		if (fabsf((SOCCER_PITCH_H - match.players[0].y) - topKeeperY) > 0.001f)
			return false;

		match.owner = 20;
		match.players[20].x = match.players[0].x + 5.0f;
		match.players[20].y = match.players[0].y - 5.0f;
		match.ball.x = match.players[20].x;
		match.ball.y = match.players[20].y;
		soccerAiKeeper(&match, 0u);
		if (match.owner != 0 || match.event != SoccerEventKeeperSave)
			return false;
	}
	soccerGiveYellowCard(&match, 12);
	soccerGiveYellowCard(&match, 12);
	if (match.cardType != SoccerCardSecondYellow || !match.players[12].sentOff)
		return false;
	match.sidesSwitched = true;
	soccerCoreSetFormation(&match, 0u, 0u);
	if (match.players[0].y >= SOCCER_PITCH_H * 0.5f)
		return false;
	match.owner = -1;
	match.controlled = 1;
	match.ball.x = match.players[1].x;
	match.ball.y = match.players[1].y;
	memset(&input, 0, sizeof(input));
	input.switchPressed = true;
	soccerUpdateControl(&match, &input);
	{
		int8_t firstSwitch = match.controlled;
		soccerUpdateControl(&match, &input);
		if (match.controlled == firstSwitch)
			return false;
	}
	soccerCoreInit(&match, 0x19dc3225u, 1u, 1u, SoccerWeatherDry, false, false, 0u);
	match.restart = SoccerRestartNone;
	match.owner = -1;
	for (uint8_t i = 1u; i < SOCCER_PLAYERS_PER_TEAM; i++)
		match.players[i].sentOff = true;
	match.players[1].sentOff = false;
	match.players[2].sentOff = false;
	match.players[3].sentOff = false;
	match.players[4].sentOff = false;
	match.players[1].x = 500.0f;
	match.players[1].y = 600.0f;
	match.players[2].x = 650.0f;
	match.players[2].y = 600.0f;
	match.players[3].x = 400.0f;
	match.players[3].y = 600.0f;
	match.players[4].x = 100.0f;
	match.players[4].y = 600.0f;
	match.controlled = 3;
	memset(&input, 0, sizeof(input));
	input.switchPressed = true;
	soccerUpdateControl(&match, &input);
	if (match.controlled != 1)
		return false;
	input.x = -1;
	soccerUpdateControl(&match, &input);
	if (match.controlled != 3)
		return false;
	soccerUpdateControl(&match, &input);
	if (match.controlled != 1)
		return false;

	soccerCoreInit(&match, 0x19dc3225u, 1u, 1u, SoccerWeatherDry, false, false, 0u);
	match.restart = SoccerRestartNone;
	match.restartTicks = 0u;
	match.owner = 9;
	match.controlled = 9;
	input.fire = true;
	input.firePressed = true;
	soccerCoreTick(&match, &input);
	input.fire = false;
	input.firePressed = false;
	input.fireReleased = true;
	soccerCoreTick(&match, &input);
	if (match.owner >= 0 || match.event != SoccerEventKick ||
		fabsf(match.ball.vx) + fabsf(match.ball.vy) < 1.0f)
		return false;

	soccerCoreInit(&match, 0x19dc3225u, 1u, 1u, SoccerWeatherDry, false, false, 0u);
	match.restart = SoccerRestartNone;
	match.restartTicks = 0u;
	match.owner = 9;
	match.controlled = 9;
	memset(&input, 0, sizeof(input));
	input.fire = true;
	for (uint8_t tick = 0; tick < 10u; tick++)
		soccerCoreTick(&match, &input);
	if (match.owner >= 0 || match.shots[0] != 1u || match.ball.vy >= -1.0f)
		return false;

	soccerCoreInit(&match, 0x19dc3225u, 1u, 1u, SoccerWeatherDry, false, false, 0u);
	soccerCoreForceRestart(&match, SoccerRestartFreeKick, 0u,
		SOCCER_PITCH_W * 0.5f, 210.0f);
	if (match.restartPhase != SoccerRestartPhasePositioning ||
		match.freeKickWallCount < 2u || match.freeKickWallCount > 5u ||
		!match.freeKickWallMask)
		return false;

	soccerCoreInit(&match, 0x19dc3225u, 1u, 1u, SoccerWeatherDry, false, false, 0u);
	match.restart = SoccerRestartNone;
	match.restartTicks = 0u;
	match.owner = -1;
	match.ball.x = SOCCER_PITCH_W * 0.5f;
	match.ball.y = -SOCCER_BALL_RADIUS - 1.0f;
	match.ball.z = 0.0f;
	match.ball.vy = -1.0f;
	memset(&input, 0, sizeof(input));
	soccerCoreTick(&match, &input);
	return match.score[0] == 1u && match.event == SoccerEventGoal &&
		match.restart == SoccerRestartKickoff &&
		match.restartPhase == SoccerRestartPhaseAnnounce &&
		match.goalTeam == 0u && match.owner < 0 &&
		match.restartTicks == SOCCER_LOGIC_HZ * 2u &&
		match.restartNoticeTicks == SOCCER_LOGIC_HZ * 2u;
}
