#include <string.h>
#include "abcPlayer.h"
#include "audioPwm.h"
#include "memMap.h"
#include "timebase.h"

#define ABC_BUF_SIZE 4096u
#define ABC_BUF ((char*)(((uint8_t*)CART_RAM_ADDR_IN_RAM) + QSPI_RAM_SIZE_MAX / 2))
#define ABC_ACCIDENTAL_UNSET 127
#define ABC_MAX_EVENTS 65535u

struct AbcFraction {
	uint32_t num;
	uint32_t den;
};

struct AbcPlayer {
	char *buf;
	const char *p;
	const char *end;
	const char *body;
	const char *repeatStart;
	struct MusicPlayerStatus status;
	MusicPlayerControlF controlF;
	void *userData;
	uint64_t deadline;
	uint32_t clockRemainder;
	uint32_t maxProgress;
	uint32_t bpm;
	struct AbcFraction unit;
	struct AbcFraction beat;
	int8_t keyAcc[7];
	int8_t measureAcc[7][11];
	uint8_t meterNum;
	uint8_t meterDen;
	char voices[8][16];
	uint8_t voiceCount;
	uint8_t selectedVoice;
	uint8_t currentVoice;
	uint8_t tupletRemain;
	uint8_t tupletNum;
	uint8_t tupletDen;
	uint8_t nextBrokenNum;
	uint8_t nextBrokenDen;
	uint32_t currentFreq;
	uint32_t events;
	bool toneOn;
	bool repeatUsed;
	bool tied;
	enum MusicPlayerResult error;
};

static bool abcPrvSpace(char c)
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static char abcPrvLower(char c)
{
	return c >= 'A' && c <= 'Z' ? c + 'a' - 'A' : c;
}

static uint32_t abcPrvNum(const char **pP)
{
	const char *p = *pP;
	uint32_t value = 0;

	while (*p >= '0' && *p <= '9') {
		if (value > 1000000u)
			return UINT32_MAX;
		value = value * 10u + (uint32_t)(*p++ - '0');
	}
	*pP = p;
	return value;
}

static uint32_t abcPrvGcd(uint32_t a, uint32_t b)
{
	while (b) {
		uint32_t t = a % b;
		a = b;
		b = t;
	}
	return a ? a : 1u;
}

static bool abcPrvFraction(const char **pP, struct AbcFraction *value, bool requireSlash)
{
	const char *p = *pP;
	uint32_t num = abcPrvNum(&p), den = 1u, slashCount = 0;

	if (num == UINT32_MAX)
		return false;
	if (*p == '/') {
		while (*p == '/') {
			slashCount++;
			p++;
		}
		den = abcPrvNum(&p);
		if (den == UINT32_MAX)
			return false;
		if (!den)
			den = 1u << slashCount;
		else if (slashCount > 1u)
			den <<= slashCount - 1u;
	}
	else if (requireSlash)
		return false;
	if (!num)
		num = 1u;
	if (!den)
		return false;
	value->num = num;
	value->den = den;
	*pP = p;
	return true;
}

static void abcPrvResetMeasure(struct AbcPlayer *player)
{
	memset(player->measureAcc, ABC_ACCIDENTAL_UNSET, sizeof(player->measureAcc));
}

static bool abcPrvStartsNoCase(const char *p, const char *word)
{
	while (*word)
		if (abcPrvLower(*p++) != abcPrvLower(*word++))
			return false;
	return true;
}

static bool abcPrvAtLineStart(const struct AbcPlayer *player, const char *p)
{
	while (p > player->buf && (p[-1] == ' ' || p[-1] == '\t')) p--;
	return p == player->buf || p[-1] == '\r' || p[-1] == '\n';
}

static bool abcPrvKey(struct AbcPlayer *player, const char *p, const char *end)
{
	static const int8_t naturalFifths[7] = {0, 2, 4, -1, 1, 3, 5}; /* C D E F G A B */
	static const uint8_t sharpOrder[7] = {3, 0, 4, 1, 5, 2, 6};
	static const uint8_t flatOrder[7] = {6, 2, 5, 1, 4, 0, 3};
	int note, fifths, modeOffset = 0, i;

	while (p < end && (*p == ' ' || *p == '\t'))
		p++;
	if (p >= end)
		return false;
	switch (abcPrvLower(*p++)) {
		case 'c': note = 0; break;
		case 'd': note = 1; break;
		case 'e': note = 2; break;
		case 'f': note = 3; break;
		case 'g': note = 4; break;
		case 'a': note = 5; break;
		case 'b': note = 6; break;
		default: return false;
	}
	fifths = naturalFifths[note];
	if (p < end && (*p == '#' || *p == 'b'))
		fifths += *p++ == '#' ? 7 : -7;
	while (p < end && (*p == ' ' || *p == '\t'))
		p++;
	if (abcPrvStartsNoCase(p, "min") || abcPrvStartsNoCase(p, "aeo"))
		modeOffset = -3;
	else if (abcPrvStartsNoCase(p, "mix"))
		modeOffset = -1;
	else if (abcPrvStartsNoCase(p, "dor"))
		modeOffset = -2;
	else if (abcPrvStartsNoCase(p, "phr"))
		modeOffset = -4;
	else if (abcPrvStartsNoCase(p, "loc"))
		modeOffset = -5;
	else if (abcPrvStartsNoCase(p, "lyd"))
		modeOffset = 1;
	fifths += modeOffset;
	if (fifths < -7 || fifths > 7)
		return false;
	memset(player->keyAcc, 0, sizeof(player->keyAcc));
	for (i = 0; i < fifths; i++)
		player->keyAcc[sharpOrder[i]] = 1;
	for (i = 0; i > fifths; i--)
		player->keyAcc[flatOrder[-i]] = -1;
	abcPrvResetMeasure(player);
	return true;
}

static const char *abcPrvLineEnd(const char *p, const char *end)
{
	while (p < end && *p != '\r' && *p != '\n')
		p++;
	return p;
}

static bool abcPrvVoiceId(const char *p, const char *end, char out[16])
{
	uint8_t n = 0;
	while (p < end && (*p == ' ' || *p == '\t')) p++;
	while (p < end && !abcPrvSpace(*p) && *p != ']' && n + 1u < 16u) out[n++] = *p++;
	out[n] = 0;
	return n != 0;
}

static bool abcPrvAddVoice(struct AbcPlayer *player, const char *p, const char *end)
{
	char id[16]; uint8_t i;
	if (!abcPrvVoiceId(p, end, id)) return false;
	for (i = 0; i < player->voiceCount; i++) if (!strcmp(player->voices[i], id)) return true;
	if (player->voiceCount >= 8) return false;
	strcpy(player->voices[player->voiceCount++], id);
	return true;
}

static uint8_t abcPrvFindVoice(const struct AbcPlayer *player, const char *p, const char *end)
{
	char id[16]; uint8_t i;
	if (!abcPrvVoiceId(p, end, id)) return 0xff;
	for (i = 0; i < player->voiceCount; i++) if (!strcmp(player->voices[i], id)) return i;
	return 0xff;
}

static enum MusicPlayerResult abcPrvPreflight(struct AbcPlayer *player)
{
	const char *p = player->buf;
	uint_fast8_t tunes = 0;

	while (p < player->end) {
		const char *lineEnd = abcPrvLineEnd(p, player->end);
		if (lineEnd - p >= 2 && p[1] == ':') {
			if (p[0] == 'X') tunes++;
			else if (p[0] == 'V' && !abcPrvAddVoice(player, p + 2, lineEnd))
				return MusicPlayerResultUnsupported;
		}
		while (p < lineEnd) {
			if (lineEnd - p >= 4 && p[0] == '[' && p[1] == 'V' && p[2] == ':') {
				const char *close = memchr(p + 3, ']', (size_t)(lineEnd - p - 3));
				if (!close)
					return MusicPlayerResultDecodeError;
				if (!abcPrvAddVoice(player, p + 3, close))
					return MusicPlayerResultUnsupported;
				p = close;
			}
			p++;
		}
		while (p < player->end && (*p == '\r' || *p == '\n')) p++;
	}
	return tunes == 1u ? MusicPlayerResultDone :
		tunes > 1u ? MusicPlayerResultUnsupported : MusicPlayerResultDecodeError;
}

static bool abcPrvTempo(struct AbcPlayer *player, const char *p, const char *end)
{
	struct AbcFraction beat = {1u, 4u};
	uint32_t bpm;

	while (p < end && (*p == ' ' || *p == '\t'))
		p++;
	if (p < end && *p == '"') {
		p++;
		while (p < end && *p != '"')
			p++;
		if (p < end)
			p++;
	}
	while (p < end && (*p == ' ' || *p == '\t'))
		p++;
	if (memchr(p, '=', (size_t)(end - p))) {
		if (!abcPrvFraction(&p, &beat, true))
			return false;
		while (p < end && (*p == ' ' || *p == '\t'))
			p++;
		if (p >= end || *p++ != '=')
			return false;
	}
	bpm = abcPrvNum(&p);
	if (!bpm || bpm == UINT32_MAX || bpm > 2000u)
		return false;
	player->beat = beat;
	player->bpm = bpm;
	player->status.bpm = bpm;
	return true;
}

static bool abcPrvHeader(struct AbcPlayer *player)
{
	const char *p = player->buf;
	bool haveX = false, haveK = false, haveL = false;

	player->meterNum = 4;
	player->meterDen = 4;
	player->unit.num = 1;
	player->unit.den = 8;
	player->beat.num = 1;
	player->beat.den = 4;
	player->bpm = 120;
	player->status.bpm = 120;
	player->status.trackCount = player->voiceCount ? player->voiceCount : 1;
	player->status.track = 0;
	player->selectedVoice = 0;
	player->currentVoice = 0;
	while (p < player->end) {
		const char *line = p, *lineEnd = abcPrvLineEnd(p, player->end), *v;
		char field = lineEnd - line >= 2 && line[1] == ':' ? line[0] : 0;

		p = lineEnd;
		while (p < player->end && (*p == '\r' || *p == '\n'))
			p++;
		if (!field || line[0] == '%')
			continue;
		v = line + 2;
		if (field == 'X') {
			if (haveX)
				return false;
			haveX = true;
		}
		else if (field == 'M') {
			struct AbcFraction meter;
			if (*v == 'C') {
				player->meterNum = *++v == '|' ? 2 : 4;
				player->meterDen = *v == '|' ? 2 : 4;
			}
			else if (!abcPrvFraction(&v, &meter, true) || meter.num > 255u || meter.den > 255u)
				return false;
			else {
				player->meterNum = (uint8_t)meter.num;
				player->meterDen = (uint8_t)meter.den;
			}
		}
		else if (field == 'L') {
			if (!abcPrvFraction(&v, &player->unit, true))
				return false;
			haveL = true;
		}
		else if (field == 'Q') {
			if (!abcPrvTempo(player, v, lineEnd))
				return false;
		}
		else if (field == 'V') {
			uint8_t voice = abcPrvFindVoice(player, v, lineEnd);
			if (voice == 0xff) return false;
			player->currentVoice = voice;
		}
		else if (field == 'K') {
			if (haveK || !haveX || !abcPrvKey(player, v, lineEnd))
				return false;
			haveK = true;
			player->body = p;
			break;
		}
	}
	if (!haveL && (uint32_t)player->meterNum * 4u < (uint32_t)player->meterDen * 3u)
		player->unit.den = 16u;
	return haveX && haveK && player->body < player->end;
}

static uint32_t abcPrvFreq(int note)
{
	static const uint16_t octave4[12] = {262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494};
	uint32_t freq;
	int octave = note / 12;
	int pitch = note % 12;

	if (pitch < 0) {
		pitch += 12;
		octave--;
	}
	if (octave < 0 || octave > 10)
		return 0;
	freq = octave4[pitch];
	while (octave > 4) { freq *= 2u; octave--; }
	while (octave < 4) { freq = (freq + 1u) / 2u; octave++; }
	return freq;
}

static bool abcPrvControl(struct AbcPlayer *player, enum MusicPlayerResult *resultP)
{
	enum MusicPlayerControl control = player->controlF ?
		player->controlF(player->userData, &player->status) : MusicPlayerControlNone;
	if (control == MusicPlayerControlTrackPrev || control == MusicPlayerControlTrackNext) {
		if (player->status.trackCount > 1) {
			if (control == MusicPlayerControlTrackPrev)
				player->selectedVoice = (player->selectedVoice + player->status.trackCount - 1u) % player->status.trackCount;
			else
				player->selectedVoice = (player->selectedVoice + 1u) % player->status.trackCount;
			player->status.track = player->selectedVoice;
			player->p = player->body;
			player->currentVoice = player->selectedVoice;
			player->repeatStart = NULL;
			player->repeatUsed = false;
			player->tupletRemain = 0;
			player->nextBrokenNum = 0;
			player->tied = false;
			player->toneOn = false;
			player->deadline = getTime();
			player->status.bytesPlayed = 0;
			audioPwmStop();
		}
		return false;
	}

	if (control == MusicPlayerControlPause) {
		uint64_t pauseStart = getTime();
		player->status.paused = true;
		audioPwmStop();
		while (1) {
			uint64_t poll = getTime() + TICKS_PER_SECOND / 100u;
			control = player->controlF ? player->controlF(player->userData, &player->status) : MusicPlayerControlNone;
			if (control == MusicPlayerControlPause) {
				player->status.paused = false;
				player->deadline += getTime() - pauseStart;
				if (player->toneOn)
					(void)audioPwmTone(player->currentFreq);
				return false;
			}
			if (control == MusicPlayerControlStop || control == MusicPlayerControlPrev ||
				control == MusicPlayerControlNext)
				break;
			while (getTime() < poll);
		}
	}
	if (control == MusicPlayerControlStop)
		*resultP = MusicPlayerResultStopped;
	else if (control == MusicPlayerControlPrev)
		*resultP = MusicPlayerResultPrev;
	else if (control == MusicPlayerControlNext)
		*resultP = MusicPlayerResultNext;
	else
		return false;
	audioPwmStop();
	return true;
}

static bool abcPrvWait(struct AbcPlayer *player, enum MusicPlayerResult *resultP)
{
	while (getTime() < player->deadline) {
		uint64_t poll = getTime() + TICKS_PER_SECOND / 100u;
		if (abcPrvControl(player, resultP))
			return false;
		while (getTime() < player->deadline && getTime() < poll);
	}
	return true;
}

static void abcPrvAdvance(struct AbcPlayer *player, uint64_t usec)
{
	uint64_t scaled = usec * TICKS_PER_SECOND + player->clockRemainder;
	player->deadline += scaled / 1000000u;
	player->clockRemainder = (uint32_t)(scaled % 1000000u);
}

static uint64_t abcPrvDuration(struct AbcPlayer *player, struct AbcFraction length)
{
	uint64_t num = 60000000ull * player->unit.num * length.num * player->beat.den;
	uint64_t den = (uint64_t)player->bpm * player->unit.den * length.den * player->beat.num;
	return den ? (num + den / 2u) / den : 0;
}

static void abcPrvSkipDelimited(struct AbcPlayer *player, char endChar)
{
	player->p++;
	while (player->p < player->end && *player->p != endChar)
		player->p++;
	if (player->p < player->end)
		player->p++;
}

static bool abcPrvSkipFirstEnding(struct AbcPlayer *player)
{
	const char *p = player->p;

	while (p + 1 < player->end && !(p[0] == ':' && p[1] == '|'))
		p++;
	if (p + 1 >= player->end)
		return false;
	p += 2;
	while (p < player->end && abcPrvSpace(*p)) p++;
	if (p + 1 < player->end && p[0] == '[' && p[1] == '2') p += 2;
	player->p = p;
	player->repeatStart = NULL;
	return true;
}

static int abcPrvLetter(char c)
{
	switch (abcPrvLower(c)) {
		case 'c': return 0; case 'd': return 1; case 'e': return 2; case 'f': return 3;
		case 'g': return 4; case 'a': return 5; case 'b': return 6; default: return -1;
	}
}

static bool abcPrvNote(struct AbcPlayer *player, enum MusicPlayerResult *resultP)
{
	static const uint8_t semitone[7] = {0, 2, 4, 5, 7, 9, 11};
	const char *p = player->p;
	struct AbcFraction length = {1u, 1u};
	int explicitAcc = ABC_ACCIDENTAL_UNSET, letter, octave, note;
	uint32_t freq = 0, g;
	uint64_t duration, playUsec, restUsec;
	bool rest, tie = false;

	if (*p == '^' || *p == '_' || *p == '=') {
		char mark = *p++;
		explicitAcc = mark == '=' ? 0 : mark == '^' ? 1 : -1;
		if (*p == mark) { explicitAcc *= 2; p++; }
	}
	rest = abcPrvLower(*p) == 'z' || abcPrvLower(*p) == 'x';
	letter = abcPrvLetter(*p);
	if (!rest && letter < 0)
		return false;
	octave = *p >= 'a' && *p <= 'g' ? 5 : 4;
	p++;
	while (p < player->end && (*p == '\'' || *p == ',')) {
		octave += *p++ == '\'' ? 1 : -1;
	}
	if (!abcPrvFraction(&p, &length, false))
		return false;
	if (p < player->end && *p == '.') {
		length.num *= 3u;
		length.den *= 2u;
		p++;
	}
	{
		const char *joined = p;
		while (joined < player->end && (abcPrvSpace(*joined) || *joined == '\\')) joined++;
		if (joined < player->end && (*joined == '-' || *joined == '>' || *joined == '<'))
			p = joined;
	}
	if (p < player->end && *p == '-') { tie = true; p++; }
	if (player->nextBrokenNum) {
		length.num *= player->nextBrokenNum;
		length.den *= player->nextBrokenDen;
		player->nextBrokenNum = 0;
	}
	if (p < player->end && (*p == '>' || *p == '<')) {
		char op = *p;
		uint_fast8_t count = 0;
		while (p < player->end && *p == op && count < 3u) { count++; p++; }
		if (p < player->end && *p == op)
			return false;
		if (op == '>') {
			length.num *= (1u << (count + 1u)) - 1u;
			length.den *= 1u << count;
			player->nextBrokenNum = 1u;
			player->nextBrokenDen = 1u << count;
		}
		else {
			length.den *= 1u << count;
			player->nextBrokenNum = (1u << (count + 1u)) - 1u;
			player->nextBrokenDen = 1u << count;
		}
	}
	if (player->tupletRemain) {
		length.num *= player->tupletNum;
		length.den *= player->tupletDen;
		player->tupletRemain--;
	}
	g = abcPrvGcd(length.num, length.den);
	length.num /= g;
	length.den /= g;
	duration = abcPrvDuration(player, length);
	if (!duration || duration > 600000000ull)
		return false;
	if (!rest) {
		int accidental;
		if (octave < 0 || octave > 10)
			return false;
		if (explicitAcc != ABC_ACCIDENTAL_UNSET) {
			player->measureAcc[letter][octave] = (int8_t)explicitAcc;
			accidental = explicitAcc;
		}
		else if (player->measureAcc[letter][octave] != ABC_ACCIDENTAL_UNSET)
			accidental = player->measureAcc[letter][octave];
		else
			accidental = player->keyAcc[letter];
		note = octave * 12 + semitone[letter] + accidental;
		freq = abcPrvFreq(note);
		if (!freq)
			return false;
	}
	if (player->tied && (rest || freq != player->currentFreq))
		return false;
	player->p = p;
	player->events++;
	if (player->events > ABC_MAX_EVENTS) {
		player->error = MusicPlayerResultUnsupported;
		return false;
	}
	player->status.bytesPlayed = (uint32_t)(player->p - player->buf);
	if (player->status.bytesPlayed < player->maxProgress)
		player->status.bytesPlayed = player->maxProgress;
	else
		player->maxProgress = player->status.bytesPlayed;
	if (rest) {
		player->toneOn = false;
		audioPwmStop();
		abcPrvAdvance(player, duration);
		if (!abcPrvWait(player, resultP)) return true;
		player->tied = false;
		return true;
	}
	player->currentFreq = freq;
	player->toneOn = true;
	if (!player->tied)
		(void)audioPwmTone(freq);
	playUsec = tie ? duration : duration * 7u / 8u;
	restUsec = duration - playUsec;
	abcPrvAdvance(player, playUsec);
	if (!abcPrvWait(player, resultP)) return true;
	player->tied = tie;
	if (!tie) {
		player->toneOn = false;
		audioPwmStop();
		if (restUsec) {
			abcPrvAdvance(player, restUsec);
			if (!abcPrvWait(player, resultP)) return true;
		}
	}
	return true;
}

static bool abcPrvTuplet(struct AbcPlayer *player)
{
	const char *p = player->p + 1;
	uint32_t count = abcPrvNum(&p), ratio;

	if (count < 2u || count > 9u || player->tupletRemain)
		return false;
	ratio = (count == 3u || count == 6u) ? 2u : 3u;
	player->tupletDen = (uint8_t)count;
	player->tupletNum = (uint8_t)ratio;
	player->tupletRemain = (uint8_t)count;
	if (*p == ':') {
		p++;
		ratio = abcPrvNum(&p);
		if (!ratio || ratio > 9u) return false;
		player->tupletNum = (uint8_t)ratio;
		if (*p == ':') {
			p++;
			ratio = abcPrvNum(&p);
			if (!ratio || ratio > 9u) return false;
			player->tupletRemain = (uint8_t)ratio;
		}
	}
	player->p = p;
	return true;
}

static bool abcPrvInlineField(struct AbcPlayer *player)
{
	const char *end = player->p + 3;
	while (end < player->end && *end != ']') end++;
	if (end >= player->end) return false;
	if (player->p[1] == 'K') {
		if (!abcPrvKey(player, player->p + 3, end)) return false;
	}
	else if (player->p[1] == 'Q') {
		if (!abcPrvTempo(player, player->p + 3, end)) return false;
	}
	else if (player->p[1] == 'V') {
		uint8_t voice = abcPrvFindVoice(player, player->p + 3, end);
		if (voice == 0xff) return false;
		player->currentVoice = voice;
	}
	else
		return false;
	player->p = end + 1;
	return true;
}

static enum MusicPlayerResult abcPrvPlay(struct AbcPlayer *player)
{
	enum MusicPlayerResult result = MusicPlayerResultDone;

	player->p = player->body;
	player->deadline = getTime();
	while (player->p < player->end) {
		char c = *player->p;
		if (player->voiceCount > 1 && player->currentVoice != player->selectedVoice &&
			!(abcPrvAtLineStart(player, player->p) && player->p + 1 < player->end &&
				player->p[1] == ':' && player->p[0] == 'V')) {
			player->p = abcPrvLineEnd(player->p, player->end);
			continue;
		}

		if (c == '%') { player->p = abcPrvLineEnd(player->p, player->end); continue; }
		if (abcPrvAtLineStart(player, player->p) &&
			player->p + 1 < player->end && player->p[1] == ':') {
			const char *lineEnd = abcPrvLineEnd(player->p, player->end);
			if (c == 'X') { player->error = MusicPlayerResultUnsupported; break; }
			if (c == 'V') {
				uint8_t voice = abcPrvFindVoice(player, player->p + 2, lineEnd);
				if (voice == 0xff) { player->error = MusicPlayerResultDecodeError; break; }
				player->currentVoice = voice;
				player->p = lineEnd;
				continue;
			}
			if (c == 'K' && !abcPrvKey(player, player->p + 2, lineEnd)) break;
			if (c == 'Q' && !abcPrvTempo(player, player->p + 2, lineEnd)) break;
			player->p = lineEnd;
			continue;
		}
		if (abcPrvSpace(c)) { player->p++; continue; }
		if (c == '"') { abcPrvSkipDelimited(player, '"'); continue; }
		if ((c == 'W' || c == 'w') && player->p + 1 < player->end && player->p[1] == ':') {
			player->p = abcPrvLineEnd(player->p, player->end);
			continue;
		}
		if (c == '!' || c == '+') { abcPrvSkipDelimited(player, c); continue; }
		if (c == '{') { abcPrvSkipDelimited(player, '}'); continue; }
		if (c == '&') { player->error = MusicPlayerResultUnsupported; break; }
		if (c == '[') {
			if (player->p + 1 < player->end && player->p[1] == '|') {
				player->p += 2;
				abcPrvResetMeasure(player);
				continue;
			}
			if (player->p + 2 < player->end && player->p[2] == ':' &&
				(player->p[1] == 'K' || player->p[1] == 'Q' || player->p[1] == 'V')) {
				if (!abcPrvInlineField(player)) break;
				continue;
			}
			if (player->p + 1 < player->end && (player->p[1] == '1' || player->p[1] == '2')) {
				if (player->p[1] == '1' && player->repeatUsed && !abcPrvSkipFirstEnding(player)) break;
				else player->p += 2;
				continue;
			}
			player->error = MusicPlayerResultUnsupported;
			break;
		}
		if (c == '|' || c == ':') {
			if (c == '|' && player->p + 1 < player->end && player->p[1] == ':') {
				player->p += 2;
				player->repeatStart = player->p;
				player->repeatUsed = false;
			}
			else if (c == ':' && player->p + 1 < player->end && player->p[1] == '|') {
				player->p += 2;
				if (!player->repeatUsed) {
					player->repeatUsed = true;
					player->p = player->repeatStart ? player->repeatStart : player->body;
				}
				else player->repeatStart = NULL;
			}
			else player->p++;
			abcPrvResetMeasure(player);
			continue;
		}
		if (c == '(') {
			if (player->p + 1 < player->end && player->p[1] >= '2' && player->p[1] <= '9') {
				if (!abcPrvTuplet(player)) break;
			}
			else player->p++;
			continue;
		}
		if (c == ')' || c == '\\' || c == '$' || c == ']' || c == '.' || c == '~' ||
			c == 'H' || c == 'L' || c == 'M' || c == 'O' || c == 'P' || c == 'S' ||
			c == 'T' || c == 'u' || c == 'v' || c == 'y' || c == '@' || c == ';' ||
			c == '}' || c == '1' || c == '2') { player->p++; continue; }
		if (abcPrvLetter(c) >= 0 || abcPrvLower(c) == 'z' || abcPrvLower(c) == 'x' ||
			c == '^' || c == '_' || c == '=') {
			if (!abcPrvNote(player, &result)) break;
			if (result != MusicPlayerResultDone) return result;
			continue;
		}
		player->error = MusicPlayerResultDecodeError;
		break;
	}
	audioPwmStop();
	if (player->error == MusicPlayerResultDone && player->p < player->end)
		player->error = MusicPlayerResultDecodeError;
	if (player->error != MusicPlayerResultDone)
		return player->error;
	if (player->tied || player->tupletRemain || player->nextBrokenNum)
		return MusicPlayerResultDecodeError;
	player->status.bytesPlayed = player->status.fileSize;
	(void)abcPrvControl(player, &result);
	return result;
}

enum MusicPlayerResult abcPlayerPlayFile(struct FatfsFil *fil, MusicPlayerControlF controlF,
	void *userData)
{
	struct AbcPlayer player;
	uint32_t nRead;

	memset(&player, 0, sizeof(player));
	player.buf = ABC_BUF;
	player.status.fileSize = fatfsFileGetSize(fil);
	player.status.trackCount = 1;
	player.controlF = controlF;
	player.userData = userData;
	player.error = MusicPlayerResultDone;
	if (!player.status.fileSize || player.status.fileSize >= ABC_BUF_SIZE)
		return MusicPlayerResultDecodeError;
	if (!fatfsFileRead(fil, player.buf, player.status.fileSize, &nRead) ||
		nRead != player.status.fileSize)
		return MusicPlayerResultFileError;
	player.buf[nRead] = 0;
	player.end = player.buf + nRead;
	player.error = abcPrvPreflight(&player);
	if (player.error != MusicPlayerResultDone)
		return player.error;
	player.error = MusicPlayerResultDone;
	if (!abcPrvHeader(&player))
		return MusicPlayerResultDecodeError;
	return abcPrvPlay(&player);
}
