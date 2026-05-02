#include <string.h>
#include "audioPwm.h"
#include "rtttlPlayer.h"
#include "timebase.h"
#include "toolWorkspace.h"

#define RTTTL_BUF_SZ	4096

static char *rtttlPrvBuf(void)
{
	struct ToolWorkspaceSpan span = toolWorkspaceGet(ToolWorkspaceCartRamUpper);

	return (char*)span.ptr;
}

static bool rtttlPrvIsSpace(char c)
{
	return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static char rtttlPrvLower(char c)
{
	if (c >= 'A' && c <= 'Z')
		return c + 'a' - 'A';
	return c;
}

static uint32_t rtttlPrvReadNum(const char **pP)
{
	const char *p = *pP;
	uint32_t val = 0;

	while (*p >= '0' && *p <= '9') {
		val = val * 10 + *p - '0';
		p++;
	}
	*pP = p;
	return val;
}

static void rtttlPrvSkipSpaces(const char **pP)
{
	const char *p = *pP;

	while (rtttlPrvIsSpace(*p))
		p++;
	*pP = p;
}

static bool rtttlPrvHandleControl(MusicPlayerControlF controlF, void *userData, struct MusicPlayerStatus *status, enum MusicPlayerResult *retP)
{
	enum MusicPlayerControl ctl = controlF ? controlF(userData, status) : MusicPlayerControlNone;

	if (ctl == MusicPlayerControlPause) {
		status->paused = true;
		audioPwmStop();
		while (1) {
			ctl = controlF ? controlF(userData, status) : MusicPlayerControlNone;
			if (ctl == MusicPlayerControlPause) {
				status->paused = false;
				return false;
			}
			if (ctl == MusicPlayerControlStop) {
				*retP = MusicPlayerResultStopped;
				return true;
			}
			if (ctl == MusicPlayerControlPrev) {
				*retP = MusicPlayerResultPrev;
				return true;
			}
			if (ctl == MusicPlayerControlNext) {
				*retP = MusicPlayerResultNext;
				return true;
			}
		}
	}
	if (ctl == MusicPlayerControlStop) {
		audioPwmStop();
		*retP = MusicPlayerResultStopped;
		return true;
	}
	if (ctl == MusicPlayerControlPrev) {
		audioPwmStop();
		*retP = MusicPlayerResultPrev;
		return true;
	}
	if (ctl == MusicPlayerControlNext) {
		audioPwmStop();
		*retP = MusicPlayerResultNext;
		return true;
	}
	return false;
}

static bool rtttlPrvDelay(uint32_t msec, MusicPlayerControlF controlF, void *userData, struct MusicPlayerStatus *status, enum MusicPlayerResult *retP)
{
	uint64_t end = getTime() + (uint64_t)msec * (TICKS_PER_SECOND / 1000);

	while (getTime() < end) {
		uint64_t stepEnd = getTime() + TICKS_PER_SECOND / 50;

		if (rtttlPrvHandleControl(controlF, userData, status, retP))
			return true;
		while (getTime() < stepEnd && getTime() < end);
	}
	return false;
}

static uint32_t rtttlPrvFreq(uint_fast8_t note, uint_fast8_t octave)
{
	static const uint16_t octave4[] = {262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494};
	uint32_t freq;

	if (note >= sizeof(octave4) / sizeof(*octave4))
		return 0;

	freq = octave4[note];
	while (octave > 4) {
		freq *= 2;
		octave--;
	}
	while (octave < 4) {
		freq = (freq + 1) / 2;
		octave++;
	}
	return freq;
}

static int_fast8_t rtttlPrvNoteIdx(char note)
{
	switch (rtttlPrvLower(note)) {
		case 'c': return 0;
		case 'd': return 2;
		case 'e': return 4;
		case 'f': return 5;
		case 'g': return 7;
		case 'a': return 9;
		case 'b': return 11;
		default: return -1;
	}
}

enum MusicPlayerResult rtttlPlayerPlayFile(struct FatfsFil *fil, MusicPlayerControlF controlF, void *userData)
{
	struct MusicPlayerStatus status;
	char *buf = rtttlPrvBuf();
	const char *p;
	uint32_t nRead, defaultDur = 4, defaultOct = 6, bpm = 63, wholeNoteMsec;

	memset(&status, 0, sizeof(status));
	status.fileSize = fatfsFileGetSize(fil);
	if (status.fileSize >= RTTTL_BUF_SZ)
		return MusicPlayerResultDecodeError;
	if (!fatfsFileRead(fil, buf, status.fileSize, &nRead) || nRead != status.fileSize)
		return MusicPlayerResultFileError;
	buf[nRead] = 0;

	p = buf;
	while (*p && *p != ':')
		p++;
	if (*p != ':')
		return MusicPlayerResultDecodeError;
	p++;

	while (*p && *p != ':') {
		char key;
		uint32_t val;

		rtttlPrvSkipSpaces(&p);
		key = rtttlPrvLower(*p++);
		if (*p++ != '=')
			return MusicPlayerResultDecodeError;
		val = rtttlPrvReadNum(&p);
		if (key == 'd' && val)
			defaultDur = val;
		else if (key == 'o' && val)
			defaultOct = val;
		else if (key == 'b' && val)
			bpm = val;
		rtttlPrvSkipSpaces(&p);
		if (*p == ',')
			p++;
	}
	if (*p != ':' || !bpm || !defaultDur)
		return MusicPlayerResultDecodeError;
	p++;

	wholeNoteMsec = 240000 / bpm;
	status.sampleRate = bpm;

	while (*p) {
		enum MusicPlayerResult ctlRet;
		uint32_t dur, noteMsec, playMsec, pauseMsec, octave = defaultOct;
		int_fast8_t noteIdx;
		bool dotted = false, pause = false;

		rtttlPrvSkipSpaces(&p);
		status.bytesPlayed = p - buf;
		if (rtttlPrvHandleControl(controlF, userData, &status, &ctlRet))
			return ctlRet;
		if (!*p)
			break;

		dur = rtttlPrvReadNum(&p);
		if (!dur)
			dur = defaultDur;

		if (rtttlPrvLower(*p) == 'p') {
			pause = true;
			noteIdx = -1;
			p++;
		}
		else {
			noteIdx = rtttlPrvNoteIdx(*p++);
			if (noteIdx < 0)
				return MusicPlayerResultDecodeError;
			if (*p == '#') {
				noteIdx++;
				p++;
			}
		}

		if (*p == '.') {
			dotted = true;
			p++;
		}
		if (*p >= '0' && *p <= '9')
			octave = rtttlPrvReadNum(&p);
		if (*p == '.') {
			dotted = true;
			p++;
		}

		noteMsec = wholeNoteMsec / dur;
		if (dotted)
			noteMsec += noteMsec / 2;
		playMsec = noteMsec * 7 / 8;
		pauseMsec = noteMsec - playMsec;

		if (!pause)
			(void)audioPwmTone(rtttlPrvFreq(noteIdx, octave));
		if (rtttlPrvDelay(pause ? noteMsec : playMsec, controlF, userData, &status, &ctlRet))
			return ctlRet;
		audioPwmStop();
		if (!pause && pauseMsec && rtttlPrvDelay(pauseMsec, controlF, userData, &status, &ctlRet))
			return ctlRet;

		rtttlPrvSkipSpaces(&p);
		if (*p == ',')
			p++;
	}

	audioPwmStop();
	return MusicPlayerResultDone;
}
