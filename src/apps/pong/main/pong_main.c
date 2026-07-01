#include "apps/pong/main/pong_main.h"

#include "apps/pong/core/pong_core.h"
#include "apps/pong/modes/pong_modes.h"
#include "apps/pong/platform/pong_platform.h"

enum PongMainScreen {
	PongMainModes,
	PongMainSettings,
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
	uint32_t itemCount = pongModesCount() + 1u;

	renderer->clear(renderer->context);
	pongMainSetRole(renderer, PongColorNeutral);
	renderer->draw_text(renderer->context, 96, 10, "PONG ARCADE");
	pongMainSetRole(renderer, PongColorField);
	renderer->draw_line(renderer->context, 44, 30, 276, 30);
	for (uint_fast8_t i = 0; i < itemCount; i++) {
		const char *name = i < pongModesCount() ?
			pongModesGet(i)->name : "SETTINGS";
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
		selected < pongModesCount() ? pongModesGet(selected)->description :
		"COLOR THEMES");
	renderer->draw_text(renderer->context, 18, 226, "A SELECT  CENTER EXIT");
}

static const char *pongMainThemeName(const struct PongSettings *settings)
{
	static const char *const names[] = {"CLASSIC", "TEAMS", "RAINBOW"};

	return names[settings->colorTheme < PongColorThemeCount ?
		settings->colorTheme : PongColorClassic];
}

static void pongMainDrawSettings(struct PongPlatform *platform,
	const struct PongSettings *settings, uint32_t selected)
{
	static const char *const labels[] = {"COLORS", "BACK"};
	struct PongRenderer *renderer = &platform->renderer;

	renderer->clear(renderer->context);
	pongMainSetRole(renderer, PongColorNeutral);
	renderer->draw_text(renderer->context, 82, 14, "PONG SETTINGS");
	pongMainSetRole(renderer, PongColorField);
	renderer->draw_line(renderer->context, 38, 34, 282, 34);
	for (uint_fast8_t i = 0; i < 2; i++) {
		int32_t y = 82 + i * 48;

		if (i == selected) {
			pongMainSetRole(renderer, PongColorAccent);
			renderer->draw_rect(renderer->context, 24, y - 3, 6, 14);
			renderer->draw_line(renderer->context, 36, y + 14, 286, y + 14);
		}
		pongMainSetRole(renderer, PongColorNeutral);
		renderer->draw_text(renderer->context, 42, y, labels[i]);
		if (i == 0)
			renderer->draw_text(renderer->context, 192, y, pongMainThemeName(settings));
	}
	pongMainSetRole(renderer, PongColorField);
	renderer->draw_line(renderer->context, 12, 202, 308, 202);
	pongMainSetRole(renderer, PongColorNeutral);
	renderer->draw_text(renderer->context, 18, 214, "LEFT/RIGHT CHANGE");
	renderer->draw_text(renderer->context, 200, 214, "B BACK");
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

int pongMainRun(const struct DcAppHostApi *host, const struct DcAppRunArgs *args)
{
	struct PongPlatform platform;
	struct PongGameState state;
	struct PongInput input;
	struct PongSettings settings;
	const struct PongMode *mode = 0;
	enum PongMainScreen screen = PongMainModes;
	uint32_t selectedMode = 0;
	uint32_t selectedSetting = 0;
	uint32_t uiFrame = 0;
	bool running = true;

	if (!pongDc32PlatformInit(&platform, host, args))
		return -1;
	settings.colorTheme = PongColorClassic;
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
			uint32_t itemCount = pongModesCount() + 1u;

			if (input.pressedY[0]) {
				if (input.pressedY[0] < 0)
					selectedMode = selectedMode ? selectedMode - 1u : itemCount - 1u;
				else
					selectedMode = (selectedMode + 1u) % itemCount;
			}
			if (input.confirmPressed) {
				if (selectedMode == pongModesCount()) {
					screen = PongMainSettings;
				}
				else {
					mode = pongModesGet(selectedMode);
					pongCoreReset(&state, platform.timing.ticksMs(platform.timing.context) ^
						(selectedMode * 0x9e3779b9u));
					mode->init(&state);
					platform.audio.enabled = true;
					screen = PongMainPlaying;
				}
			}
			if (screen == PongMainModes)
				pongMainDrawMenu(&platform, selectedMode);
			else if (screen == PongMainSettings)
				pongMainDrawSettings(&platform, &settings, selectedSetting);
		}
		else if (screen == PongMainSettings) {
			if (input.pressedY[0]) {
				if (input.pressedY[0] < 0)
					selectedSetting = selectedSetting ? selectedSetting - 1u : 1u;
				else
					selectedSetting = (selectedSetting + 1u) % 2u;
			}
			if (input.pressedX[0])
				pongMainAdjustSettings(&settings, selectedSetting, input.pressedX[0]);
			if (input.confirmPressed) {
				if (selectedSetting == 1)
					screen = PongMainModes;
				else
					pongMainAdjustSettings(&settings, selectedSetting, 1);
			}
			if (input.backPressed)
				screen = PongMainModes;
			if (screen == PongMainSettings)
				pongMainDrawSettings(&platform, &settings, selectedSetting);
			else
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
	platform.shutdown(platform.context);
	return 0;
}
