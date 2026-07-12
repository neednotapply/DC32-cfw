#include "apps/pong/main/pong_main.h"

#include <stddef.h>

#include "apps/pong/core/pong_core.h"
#include "apps/pong/modes/pong_modes.h"
#include "apps/pong/platform/pong_platform.h"

enum PongMainScreen {
	PongMainModes,
	PongMainPlaying,
};

struct PongSettings {
	uint8_t colorTheme;
};

static void pongMainSetRole(struct PongRenderer *renderer, enum PongColorRole role)
{
	renderer->colorRole = (uint8_t)role;
}

static void pongMainDrawMenu(struct PongPlatform *platform, uint32_t selected)
{
	struct PongRenderer *renderer = &platform->renderer;
	uint32_t itemCount = pongModesCount();

	renderer->clear(renderer->context);
	pongMainSetRole(renderer, PongColorNeutral);
	renderer->draw_text(renderer->context, 96, 10, "PONG ARCADE");
	pongMainSetRole(renderer, PongColorField);
	renderer->draw_line(renderer->context, 44, 30, 276, 30);
	for (uint_fast8_t i = 0; i < itemCount; i++) {
		const char *name = pongModesGet(i)->name;
		int32_t y = 40 + i * 18;

		if (i == selected) {
			pongMainSetRole(renderer, PongColorAccent);
			renderer->draw_rect(renderer->context, 28, y - 3, 6, 14);
			renderer->draw_line(renderer->context, 40, y + 13, 280, y + 13);
		}
		pongMainSetRole(renderer, PongColorNeutral);
		renderer->draw_text(renderer->context, 42, y, name);
	}
	pongMainSetRole(renderer, PongColorField);
	renderer->draw_line(renderer->context, 12, 202, 308, 202);
	pongMainSetRole(renderer, PongColorNeutral);
	renderer->draw_text(renderer->context, 18, 207,
		pongModesGet(selected)->description);
	renderer->draw_text(renderer->context, 18, 226, "A SELECT  FN EXIT");
}

static const char *pongMainThemeName(const struct PongSettings *settings)
{
	static const char *const names[] = {"CLASSIC", "TEAMS", "RAINBOW"};

	return names[settings->colorTheme < PongColorThemeCount ?
		settings->colorTheme : PongColorClassic];
}

static void pongMainAdjustSettings(struct PongSettings *settings, uint32_t selected,
	int8_t direction)
{
	if (selected == 0) {
		int32_t theme = settings->colorTheme + direction;

		if (theme < 0)
			theme = PongColorThemeCount - 1;
		if (theme >= PongColorThemeCount)
			theme = 0;
		settings->colorTheme = (uint8_t)theme;
	}
}

static void pongMainFnSettingValue(void *context, uint8_t index, char *dst,
	uint32_t dstSize)
{
	const struct PongSettings *settings = context;
	const char *value = index == 0 && settings ? pongMainThemeName(settings) : "";
	uint32_t i = 0;

	if (!dstSize)
		return;
	while (value[i] && i + 1u < dstSize) {
		dst[i] = value[i];
		i++;
	}
	dst[i] = 0;
}

static void pongMainFnSettingAdjust(void *context, uint8_t index, int8_t direction)
{
	if (context && index == 0)
		pongMainAdjustSettings(context, index, direction);
}

static const char *const mPongFnSettingLabels[] = {"Colors"};
static const struct UiFnSettings mPongFnSettings = {
	.title = "App Settings",
	.count = sizeof(mPongFnSettingLabels) / sizeof(*mPongFnSettingLabels),
	.labels = mPongFnSettingLabels,
	.value = pongMainFnSettingValue,
	.adjust = pongMainFnSettingAdjust,
};

int pongMainRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	struct PongPlatform platform;
	struct PongGameState state;
	struct PongInput input;
	struct PongSettings settings;
	const struct PongMode *mode = 0;
	enum PongMainScreen screen = PongMainModes;
	uint32_t selectedMode = 0;
	uint32_t uiFrame = 0;
	bool running = true;

	if (!pongDc32PlatformInit(&platform, host, args))
		return -1;
	settings.colorTheme = PongColorClassic;
	if (host->setFnSettings)
		host->setFnSettings(&mPongFnSettings, &settings);
	platform.audio.enabled = false;
	pongCoreReset(&state, platform.timing.ticksMs(platform.timing.context));
	while (running) {
		platform.timing.waitFrame(platform.timing.context);
		running = platform.readInput(platform.context, &input);
		platform.tick(platform.context);
		platform.renderer.frame = uiFrame++;
		platform.renderer.theme = settings.colorTheme;
		if (!running)
			break;
		if (screen == PongMainModes) {
			uint32_t itemCount = pongModesCount();

			if (input.pressedY[0]) {
				if (input.pressedY[0] < 0)
					selectedMode = selectedMode ? selectedMode - 1u : itemCount - 1u;
				else
					selectedMode = (selectedMode + 1u) % itemCount;
			}
			if (input.confirmPressed) {
				mode = pongModesGet(selectedMode);
				pongCoreReset(&state, platform.timing.ticksMs(platform.timing.context) ^
					(selectedMode * 0x9e3779b9u));
				mode->init(&state);
				platform.audio.enabled = true;
				screen = PongMainPlaying;
			}
			if (screen == PongMainModes)
				pongMainDrawMenu(&platform, selectedMode);
		}
		else {
			if (input.backPressed) {
				platform.audio.enabled = false;
				platform.audio.beep(platform.audio.context, 0, 0);
				screen = PongMainModes;
				pongMainDrawMenu(&platform, selectedMode);
			}
			else {
				mode->update(&state, &input, &platform.audio);
				mode->draw(&state, &platform.renderer);
			}
		}
		platform.renderer.present(platform.renderer.context);
	}
	if (host->setFnSettings)
		host->setFnSettings(NULL, NULL);
	platform.shutdown(platform.context);
	return 0;
}
