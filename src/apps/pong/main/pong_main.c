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
	bool audioEnabled;
	uint8_t volume;
	uint8_t colorTheme;
};

static void pongMainSetRole(struct PongRenderer *renderer, enum PongColorRole role)
{
	renderer->colorRole = (uint8_t)role;
}

static void pongMainNumber(char *buffer, uint32_t size, uint32_t value)
{
	char reverse[10];
	uint32_t count = 0;
	uint32_t out = 0;

	if (!size)
		return;
	if (!value)
		reverse[count++] = '0';
	while (value && count < sizeof(reverse)) {
		reverse[count++] = (char)('0' + value % 10u);
		value /= 10u;
	}
	while (count && out + 1u < size)
		buffer[out++] = reverse[--count];
	buffer[out] = 0;
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
		"AUDIO / VOLUME / COLORS");
	renderer->draw_text(renderer->context, 18, 226, "A SELECT  CENTER EXIT");
}

static const char *pongMainAudioName(const struct PongSettings *settings)
{
	return settings->audioEnabled ? "ON" : "OFF";
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
	static const char *const labels[] = {"IN-GAME AUDIO", "VOLUME", "COLORS", "BACK"};
	struct PongRenderer *renderer = &platform->renderer;
	char volume[8];

	renderer->clear(renderer->context);
	pongMainSetRole(renderer, PongColorNeutral);
	renderer->draw_text(renderer->context, 82, 14, "PONG SETTINGS");
	pongMainSetRole(renderer, PongColorField);
	renderer->draw_line(renderer->context, 38, 34, 282, 34);
	for (uint_fast8_t i = 0; i < 4; i++) {
		int32_t y = 58 + i * 34;

		if (i == selected) {
			pongMainSetRole(renderer, PongColorAccent);
			renderer->draw_rect(renderer->context, 24, y - 3, 6, 14);
			renderer->draw_line(renderer->context, 36, y + 14, 286, y + 14);
		}
		pongMainSetRole(renderer, PongColorNeutral);
		renderer->draw_text(renderer->context, 42, y, labels[i]);
		if (i == 0)
			renderer->draw_text(renderer->context, 220, y, pongMainAudioName(settings));
		else if (i == 1) {
			pongMainNumber(volume, sizeof(volume), settings->volume);
			renderer->draw_text(renderer->context, 220, y, volume);
		}
		else if (i == 2)
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
	if (selected == 0)
		settings->audioEnabled = !settings->audioEnabled;
	else if (selected == 1) {
		int32_t volume = settings->volume + direction;

		if (volume < 0)
			volume = 0;
		if (volume > 15)
			volume = 15;
		settings->volume = (uint8_t)volume;
	}
	else if (selected == 2) {
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
	settings.audioEnabled = true;
	settings.volume = platform.audio.volume;
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
					platform.audio.volume = settings.volume;
					platform.audio.enabled = settings.audioEnabled;
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
					selectedSetting = selectedSetting ? selectedSetting - 1u : 3u;
				else
					selectedSetting = (selectedSetting + 1u) % 4u;
			}
			if (input.pressedX[0])
				pongMainAdjustSettings(&settings, selectedSetting, input.pressedX[0]);
			if (input.confirmPressed) {
				if (selectedSetting == 3)
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
