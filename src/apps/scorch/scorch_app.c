#include "apps/scorch/scorch_port.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "apps/port/port_runtime.h"
#include "dcAppDraw.h"
#include "dispDefcon.h"
#include "fonts.h"
#include "gb.h"
#include "ui.h"
#include "xscorch_assets.h"

#ifndef DCAPP_RUNTIME_ID
#error "DCAPP_RUNTIME_ID must be provided by the app target"
#endif

const struct DcAppImageHeader dcAppImageHeader __attribute__((section(".dcapp_header"), used, aligned(256))) = {
	.magic = DCAPP_MAGIC,
	.headerSize = DCAPP_HEADER_SIZE,
	.abiVersion = DCAPP_ABI_VERSION,
	.runtime = DCAPP_RUNTIME_ID,
	.flags = DCAPP_IMAGE_FLAG_LARGE_XIP,
	.loadAddr = 0x10100000u,
	.appRamStart = 0x2005F000u,
	.appRamSize = 0x00014000u,
};

#define SCORCH_SCREEN_W DC32_PORT_SCREEN_W
#define SCORCH_SCREEN_H DC32_PORT_SCREEN_H
#define SCORCH_ASSET_PATH "/APPS/scorch-xscorch.pak"
#define SCORCH_SAVE_MAGIC 0x524f4353u
#define SCORCH_SAVE_VERSION 1u
#define SCORCH_MAX_TANKS 4u
#define SCORCH_TERRAIN_W 320u
#define SCORCH_MAX_WEAPONS 64u
#define SCORCH_GRAVITY 0.145f
#define SCORCH_PI 3.14159265f

enum ScorchMode {
	ScorchModeAim,
	ScorchModeProjectile,
	ScorchModeRoundOver,
};

struct ScorchTank {
	int16_t x;
	int16_t y;
	int16_t angle;
	int16_t power;
	int16_t health;
	int32_t cash;
	uint8_t inventory[SCORCH_MAX_WEAPONS];
	uint8_t weapon;
	bool alive;
	bool human;
};

struct ScorchProjectile {
	float x;
	float y;
	float vx;
	float vy;
	uint8_t owner;
	uint8_t weapon;
	bool active;
};

struct ScorchSave {
	uint32_t magic;
	uint16_t version;
	uint16_t size;
	uint32_t highRound;
	int32_t bestCash;
};

struct ScorchGame {
	const struct DcAppHostApi *host;
	const struct DcAppRunArgs *args;
	struct DcAppDrawCtx draw;
	uint8_t *frame;
	uint16_t terrain[SCORCH_TERRAIN_W];
	struct ScorchTank tanks[SCORCH_MAX_TANKS];
	struct ScorchProjectile projectile;
	enum ScorchMode mode;
	uint32_t rng;
	uint32_t round;
	uint32_t highRound;
	int32_t bestCash;
	int16_t wind;
	uint8_t current;
	uint8_t aiDelay;
	uint8_t roundDelay;
	uint8_t bannerFrames;
	bool running;
	char banner[42];
};

static volatile bool mScorchAbort;

static uint16_t scorchRgb(uint32_t r, uint32_t g, uint32_t b)
{
	return dcAppDrawRgb565(r, g, b);
}

static uint32_t scorchRand(struct ScorchGame *game)
{
	game->rng = game->rng * 1664525u + 1013904223u;
	return game->rng;
}

static int scorchRandRange(struct ScorchGame *game, int minValue, int maxValue)
{
	uint32_t span = (uint32_t)(maxValue - minValue + 1);

	if (maxValue <= minValue)
		return minValue;
	return minValue + (int)(scorchRand(game) % span);
}

static uint32_t scorchTextWidth(const char *text, enum Font font)
{
	uint32_t width = 0;

	if (!text)
		return 0;
	while (*text) {
		struct FontGlyphInfo glyph;

		if (fontGetGlyphInfo(&glyph, font, (unsigned char)*text))
			width += glyph.width + 1u;
		text++;
	}
	return width ? width - 1u : 0;
}

static void scorchDrawText(struct DcAppDrawCtx *draw, int32_t x, int32_t y, const char *text, enum Font font, uint16_t color)
{
	if (!draw || !text)
		return;
	while (*text) {
		struct FontGlyphInfo glyph;

		if (fontGetGlyphInfo(&glyph, font, (unsigned char)*text)) {
			for (uint_fast8_t row = 0; row < glyph.height; row++)
				for (uint_fast8_t col = 0; col < glyph.width; col++)
					if (fontGetGlyphPixel(&glyph, row, col))
						dcAppDrawPixel(draw, x + (int32_t)col, y + (int32_t)row, color);
			x += glyph.width + 1;
		}
		text++;
	}
}

static void scorchDrawCentered(struct DcAppDrawCtx *draw, int32_t y, const char *text, enum Font font, uint16_t color)
{
	int32_t x = (int32_t)((SCORCH_SCREEN_W - scorchTextWidth(text, font)) / 2u);

	scorchDrawText(draw, x, y, text, font, color);
}

static void scorchBanner(struct ScorchGame *game, const char *text)
{
	if (!game)
		return;
	snprintf(game->banner, sizeof(game->banner), "%s", text ? text : "");
	game->bannerFrames = 60;
}

static int scorchFindWeapon(const char *ident)
{
	for (uint32_t i = 0; i < XSCORCH_WEAPON_COUNT && i < SCORCH_MAX_WEAPONS; i++)
		if (!strcmp(xscorchWeapons[i].ident, ident) || !strcmp(xscorchWeapons[i].name, ident))
			return (int)i;
	return 1;
}

static bool scorchWeaponUsable(uint32_t index)
{
	const struct XscorchWeaponInfo *weapon;

	if (!index || index >= XSCORCH_WEAPON_COUNT || index >= SCORCH_MAX_WEAPONS)
		return false;
	weapon = &xscorchWeapons[index];
	return !weapon->useless && !weapon->indirect && weapon->radius > 0;
}

static uint8_t scorchNextWeapon(uint8_t current)
{
	for (uint32_t step = 1; step < XSCORCH_WEAPON_COUNT && step < SCORCH_MAX_WEAPONS; step++) {
		uint32_t index = (current + step) % XSCORCH_WEAPON_COUNT;

		if (scorchWeaponUsable(index))
			return (uint8_t)index;
	}
	return current;
}

static uint8_t scorchPrevWeapon(uint8_t current)
{
	for (uint32_t step = 1; step < XSCORCH_WEAPON_COUNT && step < SCORCH_MAX_WEAPONS; step++) {
		uint32_t index = (current + XSCORCH_WEAPON_COUNT - step) % XSCORCH_WEAPON_COUNT;

		if (scorchWeaponUsable(index))
			return (uint8_t)index;
	}
	return current;
}

static bool scorchCanUseWeapon(const struct ScorchTank *tank, uint8_t weapon)
{
	if (!tank || !scorchWeaponUsable(weapon))
		return false;
	if (xscorchWeapons[weapon].price <= 0)
		return true;
	return tank->inventory[weapon] > 0;
}

static void scorchGrantStartingInventory(struct ScorchTank *tank)
{
	int missile = scorchFindWeapon("Missile");
	int babyNuke = scorchFindWeapon("BabyNuke");
	int mirv = scorchFindWeapon("MIRV");

	memset(tank->inventory, 0, sizeof(tank->inventory));
	if (missile > 0 && missile < (int)SCORCH_MAX_WEAPONS)
		tank->inventory[missile] = 2;
	if (babyNuke > 0 && babyNuke < (int)SCORCH_MAX_WEAPONS)
		tank->inventory[babyNuke] = 1;
	if (mirv > 0 && mirv < (int)SCORCH_MAX_WEAPONS)
		tank->inventory[mirv] = 1;
	tank->weapon = 1;
}

static void scorchSaveGame(struct ScorchGame *game)
{
	struct ScorchSave save;

	if (!game || !game->args || !game->args->vol)
		return;
	memset(&save, 0, sizeof(save));
	save.magic = SCORCH_SAVE_MAGIC;
	save.version = SCORCH_SAVE_VERSION;
	save.size = sizeof(save);
	save.highRound = game->highRound;
	save.bestCash = game->bestCash;
	(void)dc32PortSaveWrite(game->args->vol, "scorch", &save, sizeof(save));
}

static void scorchRestoreSave(struct ScorchGame *game)
{
	struct ScorchSave save;

	game->highRound = 0;
	game->bestCash = 0;
	if (!game || !game->args || !game->args->vol ||
			!dc32PortSaveRead(game->args->vol, "scorch", &save, sizeof(save)) ||
			save.magic != SCORCH_SAVE_MAGIC || save.version != SCORCH_SAVE_VERSION ||
			save.size != sizeof(save))
		return;
	game->highRound = save.highRound;
	game->bestCash = save.bestCash;
}

static void scorchGenerateTerrain(struct ScorchGame *game)
{
	int height = scorchRandRange(game, 118, 170);

	for (uint32_t x = 0; x < SCORCH_TERRAIN_W; x++) {
		height += scorchRandRange(game, -5, 5);
		if (height < 96)
			height = 96;
		if (height > 210)
			height = 210;
		game->terrain[x] = (uint16_t)height;
	}
	for (uint32_t pass = 0; pass < 5; pass++) {
		uint16_t prev = game->terrain[0];

		for (uint32_t x = 1; x + 1 < SCORCH_TERRAIN_W; x++) {
			uint16_t next = game->terrain[x + 1];
			game->terrain[x] = (uint16_t)((prev + game->terrain[x] * 2u + next) / 4u);
			prev = game->terrain[x];
		}
	}
}

static uint16_t scorchGroundAt(const struct ScorchGame *game, int x)
{
	if (x < 0)
		x = 0;
	if (x >= (int)SCORCH_TERRAIN_W)
		x = (int)SCORCH_TERRAIN_W - 1;
	return game->terrain[x];
}

static void scorchPlaceTanks(struct ScorchGame *game)
{
	static const int baseX[SCORCH_MAX_TANKS] = {32, 112, 206, 286};

	for (uint32_t i = 0; i < SCORCH_MAX_TANKS; i++) {
		struct ScorchTank *tank = &game->tanks[i];
		int x = baseX[i] + scorchRandRange(game, -10, 10);

		memset(tank, 0, sizeof(*tank));
		tank->x = (int16_t)x;
		tank->y = (int16_t)(scorchGroundAt(game, x) - 5);
		tank->angle = i < 2 ? 45 : 135;
		tank->power = 62;
		tank->health = 100;
		tank->cash = i == 0 ? 25000 : 18000;
		tank->alive = true;
		tank->human = i == 0;
		scorchGrantStartingInventory(tank);
	}
}

static void scorchStartRound(struct ScorchGame *game)
{
	game->round++;
	game->mode = ScorchModeAim;
	game->current = 0;
	game->projectile.active = false;
	game->aiDelay = 45;
	game->roundDelay = 0;
	game->wind = (int16_t)scorchRandRange(game, -20, 20);
	scorchGenerateTerrain(game);
	scorchPlaceTanks(game);
	scorchBanner(game, "ROUND START");
}

static uint8_t scorchAliveCount(const struct ScorchGame *game, uint8_t *last)
{
	uint8_t count = 0;

	for (uint32_t i = 0; i < SCORCH_MAX_TANKS; i++) {
		if (game->tanks[i].alive && game->tanks[i].health > 0) {
			count++;
			if (last)
				*last = (uint8_t)i;
		}
	}
	return count;
}

static void scorchEndRound(struct ScorchGame *game)
{
	uint8_t winner = 0;
	char text[36];

	game->mode = ScorchModeRoundOver;
	game->roundDelay = 45;
	if (scorchAliveCount(game, &winner) == 1) {
		game->tanks[winner].cash += 8000 + (int32_t)game->round * 1500;
		snprintf(text, sizeof(text), game->tanks[winner].human ? "YOU WIN ROUND" : "CPU %u WINS", (unsigned)winner);
	}
	else {
		snprintf(text, sizeof(text), "DRAW");
	}
	if (game->round > game->highRound)
		game->highRound = game->round;
	if (game->tanks[0].cash > game->bestCash)
		game->bestCash = game->tanks[0].cash;
	scorchSaveGame(game);
	scorchBanner(game, text);
}

static void scorchNextTurn(struct ScorchGame *game)
{
	if (scorchAliveCount(game, NULL) <= 1) {
		scorchEndRound(game);
		return;
	}
	for (uint32_t i = 0; i < SCORCH_MAX_TANKS; i++) {
		game->current = (uint8_t)((game->current + 1u) % SCORCH_MAX_TANKS);
		if (game->tanks[game->current].alive && game->tanks[game->current].health > 0)
			break;
	}
	game->aiDelay = game->tanks[game->current].human ? 0 : 45;
	game->mode = ScorchModeAim;
}

static void scorchDamageTerrain(struct ScorchGame *game, int cx, int cy, int radius)
{
	int r2 = radius * radius;

	for (int dx = -radius; dx <= radius; dx++) {
		int x = cx + dx;
		int dy;
		int bottom = (int)SCORCH_SCREEN_H - 1;

		if (x < 0 || x >= (int)SCORCH_TERRAIN_W)
			continue;
		dy = (int)sqrtf((float)(r2 - dx * dx));
		if (cy + dy > (int)game->terrain[x])
			game->terrain[x] = (uint16_t)(cy + dy > bottom ? bottom : cy + dy);
	}
	for (uint32_t i = 0; i < SCORCH_MAX_TANKS; i++) {
		struct ScorchTank *tank = &game->tanks[i];

		if (!tank->alive)
			continue;
		tank->y = (int16_t)(scorchGroundAt(game, tank->x) - 5);
		if (tank->y >= (int16_t)(SCORCH_SCREEN_H - 6)) {
			tank->alive = false;
			tank->health = 0;
		}
	}
}

static void scorchDamageTanks(struct ScorchGame *game, int cx, int cy, int radius, int force)
{
	for (uint32_t i = 0; i < SCORCH_MAX_TANKS; i++) {
		struct ScorchTank *tank = &game->tanks[i];
		int dx;
		int dy;
		int dist2;
		int hitRadius = radius + 8;

		if (!tank->alive)
			continue;
		dx = tank->x - cx;
		dy = tank->y - cy;
		dist2 = dx * dx + dy * dy;
		if (dist2 <= hitRadius * hitRadius) {
			int dist = (int)sqrtf((float)dist2);
			int damage = (force / 35) + (radius - dist) * 3;

			if (damage < 5)
				damage = 5;
			tank->health -= (int16_t)damage;
			if (tank->health <= 0) {
				tank->health = 0;
				tank->alive = false;
			}
		}
	}
}

static int scorchScreenRadius(const struct XscorchWeaponInfo *weapon)
{
	int radius = weapon->radius / 6 + 6;

	if (weapon->flags & XSCORCH_WEAPON_FLAG_PLASMA)
		radius += 4;
	if (radius < 5)
		radius = 5;
	if (radius > 42)
		radius = 42;
	return radius;
}

static void scorchExplode(struct ScorchGame *game, int x, int y, uint8_t weaponIndex, uint8_t depth)
{
	const struct XscorchWeaponInfo *weapon = &xscorchWeapons[weaponIndex];
	int radius = scorchScreenRadius(weapon);

	scorchDamageTerrain(game, x, y, radius);
	scorchDamageTanks(game, x, y, radius, weapon->force);
	if (depth || weapon->children <= 0)
		return;
	for (int i = 0; i < weapon->children && i < 8; i++) {
		int ox = scorchRandRange(game, -weapon->scatter * 4 - radius, weapon->scatter * 4 + radius);
		int oy = scorchRandRange(game, -radius, radius / 2);
		uint8_t child = weaponIndex;

		if (weapon->child[0]) {
			int found = scorchFindWeapon(weapon->child);

			if (found > 0)
				child = (uint8_t)found;
		}
		scorchExplode(game, x + ox, y + oy, child, depth + 1u);
	}
}

static void scorchFire(struct ScorchGame *game)
{
	struct ScorchTank *tank = &game->tanks[game->current];
	float angle;
	float speed;

	if (!scorchCanUseWeapon(tank, tank->weapon))
		tank->weapon = 1;
	if (!scorchCanUseWeapon(tank, tank->weapon))
		return;
	if (xscorchWeapons[tank->weapon].price > 0 && tank->inventory[tank->weapon] > 0)
		tank->inventory[tank->weapon]--;
	angle = (float)tank->angle * SCORCH_PI / 180.0f;
	speed = 1.35f + (float)tank->power * 0.045f;
	game->projectile.x = (float)tank->x;
	game->projectile.y = (float)tank->y - 5.0f;
	game->projectile.vx = cosf(angle) * speed;
	game->projectile.vy = -sinf(angle) * speed;
	game->projectile.owner = game->current;
	game->projectile.weapon = tank->weapon;
	game->projectile.active = true;
	game->mode = ScorchModeProjectile;
}

static void scorchStepProjectile(struct ScorchGame *game)
{
	struct ScorchProjectile *p = &game->projectile;

	if (!p->active)
		return;
	for (uint32_t step = 0; step < 3 && p->active; step++) {
		p->x += p->vx;
		p->y += p->vy;
		p->vx += (float)game->wind * 0.0008f;
		p->vy += SCORCH_GRAVITY;
		if (p->x < 0.0f || p->x >= (float)SCORCH_SCREEN_W || p->y >= (float)SCORCH_SCREEN_H) {
			p->active = false;
			scorchNextTurn(game);
			return;
		}
		if (p->y >= (float)scorchGroundAt(game, (int)p->x)) {
			p->active = false;
			scorchExplode(game, (int)p->x, (int)p->y, p->weapon, 0);
			scorchNextTurn(game);
			return;
		}
		for (uint32_t i = 0; i < SCORCH_MAX_TANKS; i++) {
			struct ScorchTank *tank = &game->tanks[i];
			int dx = (int)p->x - tank->x;
			int dy = (int)p->y - tank->y;

			if (tank->alive && dx * dx + dy * dy < 36) {
				p->active = false;
				scorchExplode(game, (int)p->x, (int)p->y, p->weapon, 0);
				scorchNextTurn(game);
				return;
			}
		}
	}
}

static void scorchBuySelected(struct ScorchGame *game, struct ScorchTank *tank)
{
	const struct XscorchWeaponInfo *weapon;

	if (!tank || tank->weapon >= XSCORCH_WEAPON_COUNT || tank->weapon >= SCORCH_MAX_WEAPONS)
		return;
	weapon = &xscorchWeapons[tank->weapon];
	if (weapon->price <= 0 || tank->cash < weapon->price)
		return;
	tank->cash -= weapon->price;
	tank->inventory[tank->weapon] += weapon->bundle > 0 ? (uint8_t)weapon->bundle : 1u;
	scorchBanner(game, "BOUGHT");
}

static uint8_t scorchNearestTarget(const struct ScorchGame *game, uint8_t shooter)
{
	uint8_t best = 0;
	int bestDist = 10000;

	for (uint32_t i = 0; i < SCORCH_MAX_TANKS; i++) {
		int dist;

		if (i == shooter || !game->tanks[i].alive)
			continue;
		dist = abs(game->tanks[i].x - game->tanks[shooter].x);
		if (dist < bestDist) {
			bestDist = dist;
			best = (uint8_t)i;
		}
	}
	return best;
}

static void scorchAiTurn(struct ScorchGame *game)
{
	struct ScorchTank *tank = &game->tanks[game->current];
	struct ScorchTank *target;
	uint8_t targetIndex;
	int dx;

	if (game->aiDelay > 0) {
		game->aiDelay--;
		return;
	}
	targetIndex = scorchNearestTarget(game, game->current);
	target = &game->tanks[targetIndex];
	dx = target->x - tank->x;
	tank->angle = (int16_t)(dx >= 0 ? scorchRandRange(game, 36, 58) : scorchRandRange(game, 122, 144));
	tank->power = (int16_t)(40 + abs(dx) / 4 + scorchRandRange(game, -8, 12));
	if (tank->power < 28)
		tank->power = 28;
	if (tank->power > 100)
		tank->power = 100;
	if (tank->cash > 10000 && scorchRandRange(game, 0, 3) == 0) {
		uint8_t old = tank->weapon;

		tank->weapon = scorchNextWeapon(tank->weapon);
		scorchBuySelected(game, tank);
		if (!scorchCanUseWeapon(tank, tank->weapon))
			tank->weapon = old;
	}
	scorchFire(game);
}

static void scorchHandleInput(struct ScorchGame *game)
{
	struct ScorchTank *tank = &game->tanks[game->current];
	uint_fast16_t pressed = game->draw.pressed;

	if (pressed & UI_KEY_BIT_CENTER) {
		game->running = false;
		return;
	}
	if (game->mode == ScorchModeRoundOver) {
		if (pressed & (KEY_BIT_START | KEY_BIT_A))
			scorchStartRound(game);
		return;
	}
	if (game->mode != ScorchModeAim || !tank->human)
		return;
	if (game->draw.keys & KEY_BIT_LEFT)
		tank->angle = (int16_t)(tank->angle + 1);
	if (game->draw.keys & KEY_BIT_RIGHT)
		tank->angle = (int16_t)(tank->angle - 1);
	if (game->draw.keys & KEY_BIT_UP)
		tank->power = (int16_t)(tank->power + 1);
	if (game->draw.keys & KEY_BIT_DOWN)
		tank->power = (int16_t)(tank->power - 1);
	if (tank->angle < 0)
		tank->angle = 0;
	if (tank->angle > 180)
		tank->angle = 180;
	if (tank->power < 10)
		tank->power = 10;
	if (tank->power > 100)
		tank->power = 100;
	if (pressed & KEY_BIT_A)
		scorchFire(game);
	if (pressed & KEY_BIT_B) {
		tank->weapon = (game->draw.keys & KEY_BIT_UP) ? scorchPrevWeapon(tank->weapon) : scorchNextWeapon(tank->weapon);
		scorchBanner(game, xscorchWeapons[tank->weapon].name);
	}
	if (pressed & KEY_BIT_SEL)
		scorchBuySelected(game, tank);
	if (pressed & KEY_BIT_START)
		scorchStartRound(game);
}

static void scorchDrawTerrain(struct ScorchGame *game)
{
	uint16_t dirt = scorchRgb(98, 72, 36);
	uint16_t grass = scorchRgb(48, 158, 60);

	for (int x = 0; x < (int)SCORCH_TERRAIN_W; x++) {
		int y0 = game->terrain[x];

		dcAppDrawLine(&game->draw, x, y0, x, SCORCH_SCREEN_H - 1, dirt);
		dcAppDrawPixel(&game->draw, x, y0, grass);
		if (y0 + 1 < (int)SCORCH_SCREEN_H)
			dcAppDrawPixel(&game->draw, x, y0 + 1, grass);
	}
}

static void scorchDrawTank(struct ScorchGame *game, const struct ScorchTank *tank, uint8_t index)
{
	static const uint16_t colors[SCORCH_MAX_TANKS] = {0xf800u, 0x07e0u, 0x001fu, 0xffe0u};
	uint16_t body = tank->alive ? colors[index] : scorchRgb(64, 64, 64);
	float angle = (float)tank->angle * SCORCH_PI / 180.0f;
	int tx = tank->x;
	int ty = tank->y;
	int barrelX = tx + (int)(cosf(angle) * 10.0f);
	int barrelY = ty - 3 - (int)(sinf(angle) * 10.0f);

	dcAppDrawFill(&game->draw, tx - 5, ty - 4, 11, 5, body);
	dcAppDrawFill(&game->draw, tx - 3, ty - 7, 7, 4, body);
	dcAppDrawLine(&game->draw, tx, ty - 5, barrelX, barrelY, scorchRgb(232, 232, 220));
	if (index == game->current && game->mode == ScorchModeAim)
		dcAppDrawLine(&game->draw, tx - 7, ty - 10, tx + 7, ty - 10, scorchRgb(255, 255, 255));
	if (!tank->alive)
		dcAppDrawLine(&game->draw, tx - 5, ty - 10, tx + 5, ty, scorchRgb(20, 20, 20));
}

static void scorchDrawHud(struct ScorchGame *game)
{
	const struct ScorchTank *tank = &game->tanks[game->current];
	const struct XscorchWeaponInfo *weapon = &xscorchWeapons[tank->weapon];
	char text[58];
	uint16_t white = scorchRgb(238, 238, 224);
	uint16_t dim = scorchRgb(170, 178, 170);

	snprintf(text, sizeof(text), "SCORCHED EARTH  R%lu  WIND %+d", (unsigned long)game->round, game->wind);
	scorchDrawText(&game->draw, 6, 5, text, FontSmall, white);
	snprintf(text, sizeof(text), "P%u %s  A%d P%d H%d", (unsigned)(game->current + 1u),
		tank->human ? "YOU" : "CPU", tank->angle, tank->power, tank->health);
	scorchDrawText(&game->draw, 6, 220, text, FontSmall, white);
	snprintf(text, sizeof(text), "%s x%u $%ld", weapon->name,
		weapon->price <= 0 ? 99u : (unsigned)tank->inventory[tank->weapon],
		(long)tank->cash);
	scorchDrawText(&game->draw, 150, 220, text, FontSmall, dim);
	if (game->mode == ScorchModeRoundOver)
		scorchDrawCentered(&game->draw, 104, "START NEXT ROUND", FontMedium, white);
	else if (tank->human && game->mode == ScorchModeAim)
		scorchDrawText(&game->draw, 206, 5, "A FIRE  B WEAPON  SEL BUY", FontSmall, dim);
	if (game->bannerFrames > 0) {
		scorchDrawCentered(&game->draw, 24, game->banner, FontMedium, white);
		game->bannerFrames--;
	}
}

static void scorchDraw(struct ScorchGame *game)
{
	dcAppDrawClear(&game->draw, scorchRgb(58, 77, 110));
	scorchDrawTerrain(game);
	for (uint32_t i = 0; i < SCORCH_MAX_TANKS; i++)
		scorchDrawTank(game, &game->tanks[i], (uint8_t)i);
	if (game->projectile.active) {
		int x = (int)game->projectile.x;
		int y = (int)game->projectile.y;

		dcAppDrawFill(&game->draw, x - 1, y - 1, 3, 3, scorchRgb(255, 240, 120));
	}
	scorchDrawHud(game);
	dcAppDrawPresent(&game->draw);
}

static bool scorchAssetsPresent(const struct DcAppRunArgs *args)
{
	struct Dc32PortPak pak;
	bool ok;

	if (!args || !args->vol)
		return false;
	ok = dc32PortOpenAssetPack(args->vol, SCORCH_ASSET_PATH, &pak);
	if (ok)
		dc32PortCloseAssetPack(&pak);
	return ok;
}

int scorchAppRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	struct ScorchGame *game;

	mScorchAbort = false;
	dc32PortHeapInitDefault();
	game = dc32PortCalloc(1, sizeof(*game));
	if (!game)
		return -1;
	game->frame = dc32PortMalloc(SCORCH_SCREEN_W * SCORCH_SCREEN_H);
	if (!game->frame)
		return -1;
	game->host = host;
	game->args = args;
	game->rng = 0x53434f52u;
	if (!dcAppDrawInit(&game->draw, host, args, game->frame, SCORCH_SCREEN_W, SCORCH_SCREEN_H))
		return -1;
	if (!scorchAssetsPresent(args)) {
		dc32PortShowMissingData(host, args, "Scorched Earth assets", SCORCH_ASSET_PATH, game->frame);
		return 0;
	}
	scorchRestoreSave(game);
	scorchStartRound(game);
	dcAppDrawWaitRelease(&game->draw, KEY_BIT_A | KEY_BIT_B | KEY_BIT_START | KEY_BIT_SEL | UI_KEY_BIT_CENTER);
	dispSetFramerate(60);
	game->running = true;
	while (game->running && !mScorchAbort) {
		scorchDraw(game);
		if (!dcAppDrawFrame(&game->draw, UI_KEY_BIT_CENTER))
			break;
		scorchHandleInput(game);
		if (game->mode == ScorchModeProjectile)
			scorchStepProjectile(game);
		else if (game->mode == ScorchModeAim && !game->tanks[game->current].human)
			scorchAiTurn(game);
	}
	scorchSaveGame(game);
	dcAppDrawWaitRelease(&game->draw, UI_KEY_BIT_CENTER);
	return 0;
}

void scorchAppAbort(void)
{
	mScorchAbort = true;
}

int dcAppEntry(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	return scorchAppRun(host, args);
}

void dcAppAbort(void)
{
	scorchAppAbort();
}

void dcAppRefreshDisplayOptions(void)
{
}
