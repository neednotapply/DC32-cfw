#include "apps/pong/modes/pong_mode_common.h"

static void superInit(struct PongGameState *state)
{
	pongModeAddClassicPaddles(state, 36, 7);
	pongCoreAddBall(state, 160, 110, 5, 3, 7);
	pongCoreAddBall(state, 160, 130, -5, -3, 7);
	state->balls[1].active = false;
}

static void superUpdate(struct PongGameState *state, const struct PongInput *input,
	struct PongAudio *audio)
{
	pongModeResetMatchIfConfirmed(state, input, superInit);
	if (!state->roundOver && input->confirmPressed) {
		state->balls[1].active = !state->balls[1].active;
		if (state->balls[1].active)
			pongCoreServeBall(state, &state->balls[1], 160, 132, 5, -1);
		audio->beep(audio->context, state->balls[1].active ? 900u : 450u, 70u);
	}
	pongModeUpdateHorizontalMatch(state, input, audio, 5, 0, 1, 12);
	state->frame++;
}

static void superDraw(const struct PongGameState *state, struct PongRenderer *renderer)
{
	renderer->clear(renderer->context);
	pongModeDrawDivider(renderer);
	pongModeDrawScores(state, renderer, 2);
	pongModeSetColor(renderer, PongColorField);
	renderer->draw_text(renderer->context, 104, 220, "A MULTIBALL");
	pongModeDrawEntities(state, renderer);
	pongModeDrawMessage(state, renderer);
}

const struct PongMode pongModeSuper = {
	"SUPER PONG",
	"FAST / SMALL / MULTIBALL",
	superInit,
	superUpdate,
	superDraw,
};
