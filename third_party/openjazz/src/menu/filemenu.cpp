
/**
 *
 * @file filemenu.cpp
 *
 * Part of the OpenJazz project
 *
 * @par Licence:
 * Copyright (c) 2015-2026 Carsten Teibes
 *
 * OpenJazz is distributed under the terms of
 * the GNU General Public License, version 2.0
 *
 * @par Description:
 * Deals with the running of the menus used save and load a game.
 *
 */

#include "menu.h"

#include "io/controls.h"
#include "io/gfx/font.h"
#include "io/gfx/video.h"
#include "io/sound.h"
#include "jj1/save/jj1save.h"
#include "loop.h"
#include "util.h"
#include <memory>

int FileMenu::run (bool forSaving, bool showCustom) {
	int x, y;
	int chosen = 0;
	std::unique_ptr<JJ1Save> save[4];
	int options = showCustom ? 5 : 4;

	// load save games

	for (int i = 0; i < 4; i++)
		save[i] = JJ1Save::fromSlot(i);

	const char *loadGameOptions[5] = { save[0]->getName(), save[1]->getName(),
		save[2]->getName(), save[3]->getName(),
#ifdef DC32_OPENJAZZ
		"specific level"
#else
		"custom"
#endif
	};

	auto fileChosen = [&]() -> int {
		if (chosen < 4 && !forSaving) {
			if(!save[chosen]->isValid()) {
				playSound(SE::WAIT);
#ifdef DC32_OPENJAZZ
				if (message("EMPTY SAVE SLOT") == E_QUIT)
					return -1;
#endif

				return 0;
			}
		}

		playConfirmSound();
		return 1;
	};

	video.setPalette(menuPalette);

	while (true) {

		if (loop(NORMAL_LOOP) == E_QUIT) return E_QUIT;

		if (menuBackReleased()) return E_RETURN;

		if (controls.release(C_UP)) chosen = (chosen + options - 1) % options;

		if (controls.release(C_DOWN)) chosen = (chosen + 1) % options;

		if (menuConfirmReleased()) {
			int result = fileChosen();
			if (result < 0)
				return E_QUIT;
			if (result)
				return chosen;
		}

		if (controls.getCursor(x, y)) {

			if ((x < 100) && (y >= canvasH - 12) && controls.wasCursorReleased()) return E_RETURN;

			x -= canvasW >> 2;
			y -= (canvasH >> 1) - (options << 3);

			if ((x >= 0) && (x < 256) && (y >= 0) && (y < (options << 4))) {

				chosen = y >> 4;

				if (controls.wasCursorReleased()) {
					int result = fileChosen();
					if (result < 0)
						return E_QUIT;
					if (result)
						return chosen;
				}
			}

		}

		SDL_Delay(T_MENU_FRAME);

		video.clearScreen(0);

		fontmn2->showString(forSaving ? "SAVE GAME" : "LOAD GAME", canvasW >> 1, (canvasH >> 1) - (options << 3) - 32, alignX::Center);

		for (int i = 0; i < options; i++) {
			int optionY = (canvasH >> 1) + (i << 4) - (options << 3);

#ifdef DC32_OPENJAZZ
			if (i == chosen) {
				video.drawRect((canvasW >> 1) - 96, optionY - 2, 192, 14, 79);
				fontmn2->showString(">", (canvasW >> 1) - 92, optionY);
			}
#else
			if (i == chosen) fontmn2->mapPalette(240, 8, 114, 16);
#endif

			fontmn2->showString(loadGameOptions[i], (canvasW >> 1) - 80, optionY);

#ifndef DC32_OPENJAZZ
			if (i == chosen) fontmn2->restorePalette();
#endif

		}

		showEscString();
	}

	return E_NONE;
}
