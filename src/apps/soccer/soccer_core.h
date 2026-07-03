#ifndef DC32_SOCCER_CORE_H
#define DC32_SOCCER_CORE_H

#include <stdbool.h>
#include <stdint.h>

#define SOCCER_PLAYERS_PER_TEAM 11
#define SOCCER_PLAYER_COUNT 22
#define SOCCER_REPLAY_ENTITIES 23
#define SOCCER_LOGIC_HZ 64u
#define SOCCER_SUBFRAMES 8u
#define SOCCER_PITCH_W 1020.0f
#define SOCCER_PITCH_H 1280.0f
#define SOCCER_GOAL_LEFT 439.0f
#define SOCCER_GOAL_RIGHT 581.0f

enum SoccerWeather {
	SoccerWeatherDry,
	SoccerWeatherRain,
	SoccerWeatherSnow,
	SoccerWeatherFog,
	SoccerWeatherCount,
};

enum SoccerRestart {
	SoccerRestartNone,
	SoccerRestartKickoff,
	SoccerRestartThrowIn,
	SoccerRestartCorner,
	SoccerRestartGoalKick,
	SoccerRestartFreeKick,
};

enum SoccerRestartPhase {
	SoccerRestartPhaseNone,
	SoccerRestartPhaseAnnounce,
	SoccerRestartPhasePositioning,
	SoccerRestartPhaseReady,
};

enum SoccerEvent {
	SoccerEventNone,
	SoccerEventKick,
	SoccerEventTackle,
	SoccerEventWhistle,
	SoccerEventGoal,
	SoccerEventHalfTime,
	SoccerEventFullTime,
	SoccerEventCard,
	SoccerEventPost,
	SoccerEventBounce,
	SoccerEventKeeperSave,
};

enum SoccerFoulReason {
	SoccerFoulNone,
	SoccerFoulLateTackle,
	SoccerFoulBackTackle,
	SoccerFoulSideTackle,
	SoccerFoulFrontTackle,
};

enum SoccerCardType {
	SoccerCardNone,
	SoccerCardYellow,
	SoccerCardSecondYellow,
	SoccerCardRed,
};

enum SoccerBreakReason {
	SoccerBreakNone,
	SoccerBreakGoal,
	SoccerBreakHalfTime,
	SoccerBreakExtraTime,
};

struct SoccerInput {
	int8_t x;
	int8_t y;
	bool fire;
	bool firePressed;
	bool fireReleased;
	bool switchPressed;
};

struct SoccerPlayer {
	float x;
	float y;
	float targetX;
	float targetY;
	float speed;
	uint8_t team;
	uint8_t number;
	uint8_t role;
	uint8_t animation;
	uint8_t facing;
	uint8_t frameY;
	uint8_t actionTicks;
	uint8_t tackleCooldown;
	uint8_t yellowCards;
	uint8_t passing;
	uint8_t shooting;
	uint8_t heading;
	uint8_t tackling;
	uint8_t control;
	uint8_t finishing;
	bool sentOff;
};

struct SoccerBall {
	float x;
	float y;
	float z;
	float vx;
	float vy;
	float vz;
	float spin;
};

struct SoccerMatch {
	struct SoccerPlayer players[SOCCER_PLAYER_COUNT];
	struct SoccerBall ball;
	int8_t owner;
	int8_t controlled;
	int8_t lastKicker;
	uint8_t score[2];
	uint8_t playerGoals[SOCCER_PLAYER_COUNT];
	uint8_t period;
	uint8_t restart;
	uint8_t restartTeam;
	uint8_t restartPhase;
	uint8_t goalTeam;
	int8_t restartTaker;
	int8_t foulPlayer;
	int8_t foulVictim;
	uint8_t foulReason;
	uint8_t freeKickWallCount;
	uint16_t freeKickWallMask;
	int8_t cardPlayer;
	uint8_t cardType;
	uint8_t breakReason;
	uint8_t lastTouchTeam;
	uint8_t weather;
	uint8_t difficulty;
	uint8_t formation[2];
	uint8_t substitutions[2];
	uint8_t pickupCooldown;
	uint16_t charge;
	uint32_t ticks;
	uint32_t clockTicks;
	uint32_t periodClockStart;
	uint32_t halfTicks;
	uint32_t restartTicks;
	uint32_t restartNoticeTicks;
	uint32_t bannerTicks;
	uint32_t rng;
	uint32_t possession[2];
	uint16_t shots[2];
	uint16_t fouls[2];
	bool cpuHome;
	bool training;
	bool finished;
	bool radar;
	bool sidesSwitched;
	bool clockRunning;
	enum SoccerEvent event;
};

struct SoccerTableRow {
	uint16_t team;
	uint8_t played;
	uint8_t won;
	uint8_t drawn;
	uint8_t lost;
	uint8_t goalsFor;
	uint8_t goalsAgainst;
	uint8_t points;
};

void soccerCoreInit(struct SoccerMatch *match, uint32_t seed, uint8_t matchMinutes,
	uint8_t difficulty, uint8_t weather, bool cpuHome, bool training, uint8_t formation);
void soccerCoreTick(struct SoccerMatch *match, const struct SoccerInput *input);
void soccerCoreSetFormation(struct SoccerMatch *match, uint8_t team, uint8_t formation);
void soccerCoreMoveToFormation(struct SoccerMatch *match, uint8_t team, uint8_t formation);
void soccerCoreForceRestart(struct SoccerMatch *match, enum SoccerRestart restart,
	uint8_t team, float x, float y);
void soccerCoreStartExtraTime(struct SoccerMatch *match);
void soccerTableApply(struct SoccerTableRow *home, struct SoccerTableRow *away,
	uint8_t homeGoals, uint8_t awayGoals);
int soccerTableCompare(const struct SoccerTableRow *a, const struct SoccerTableRow *b);
bool soccerCoreSelfTest(void);

#endif
