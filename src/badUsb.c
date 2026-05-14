#include <string.h>
#include "badUsb.h"
#include "timebase.h"

#define BADUSB_LINE_BUF_SZ			2048
#define BADUSB_PRELOAD_BUF_SZ			128
#define BADUSB_KEY_DELAY_MS			12

struct BadUsbState {
	struct BadUsbStatus status;
	BadUsbStatusF statusF;
	BadUsbWaitButtonF waitButtonF;
	void *userData;
	uint32_t defaultDelay;
	uint32_t defaultStringDelay;
	uint32_t nextStringDelay;
	uint8_t heldMods;
	uint8_t heldKeys[6];
};

struct BadUsbKey {
	const char *name;
	uint8_t usage;
};

struct BadUsbModifier {
	const char *name;
	uint8_t mask;
};

struct BadUsbScratch {
	char line[BADUSB_LINE_BUF_SZ];
	char prevLine[BADUSB_LINE_BUF_SZ];
	char preloadBuf[BADUSB_PRELOAD_BUF_SZ];
};

static struct BadUsbScratch mScratch;

static const struct BadUsbKey mSpecialKeys[] = {
	{"DOWNARROW", 0x51}, {"DOWN", 0x51}, {"LEFTARROW", 0x50}, {"LEFT", 0x50},
	{"RIGHTARROW", 0x4f}, {"RIGHT", 0x4f}, {"UPARROW", 0x52}, {"UP", 0x52},
	{"ENTER", 0x28}, {"RETURN", 0x28}, {"DELETE", 0x4c}, {"DEL", 0x4c},
	{"BACKSPACE", 0x2a}, {"BKSP", 0x2a}, {"END", 0x4d},
	{"HOME", 0x4a}, {"ESCAPE", 0x29}, {"ESC", 0x29}, {"INSERT", 0x49},
	{"INS", 0x49}, {"PAGEUP", 0x4b}, {"PGUP", 0x4b}, {"PAGEDOWN", 0x4e},
	{"PGDN", 0x4e}, {"CAPSLOCK", 0x39}, {"CAPS", 0x39}, {"NUMLOCK", 0x53},
	{"SCROLLLOCK", 0x47}, {"PRINTSCREEN", 0x46}, {"BREAK", 0x48}, {"PAUSE", 0x48},
	{"SPACE", 0x2c}, {"TAB", 0x2b}, {"MENU", 0x65}, {"APP", 0x65},
};

static const struct BadUsbModifier mModifiers[] = {
	{"CTRL", USB_HID_MOD_LCTRL}, {"CONTROL", USB_HID_MOD_LCTRL}, {"LCTRL", USB_HID_MOD_LCTRL}, {"LEFTCTRL", USB_HID_MOD_LCTRL},
	{"RCTRL", USB_HID_MOD_RCTRL}, {"RIGHTCTRL", USB_HID_MOD_RCTRL},
	{"SHIFT", USB_HID_MOD_LSHIFT}, {"LSHIFT", USB_HID_MOD_LSHIFT}, {"LEFTSHIFT", USB_HID_MOD_LSHIFT},
	{"RSHIFT", USB_HID_MOD_RSHIFT}, {"RIGHTSHIFT", USB_HID_MOD_RSHIFT},
	{"ALT", USB_HID_MOD_LALT}, {"LALT", USB_HID_MOD_LALT}, {"LEFTALT", USB_HID_MOD_LALT}, {"OPTION", USB_HID_MOD_LALT},
	{"RALT", USB_HID_MOD_RALT}, {"RIGHTALT", USB_HID_MOD_RALT},
	{"GUI", USB_HID_MOD_LGUI}, {"WINDOWS", USB_HID_MOD_LGUI}, {"COMMAND", USB_HID_MOD_LGUI}, {"CMD", USB_HID_MOD_LGUI},
	{"LGUI", USB_HID_MOD_LGUI}, {"LEFTGUI", USB_HID_MOD_LGUI},
	{"RGUI", USB_HID_MOD_RGUI}, {"RIGHTGUI", USB_HID_MOD_RGUI},
};

static char badUsbPrvUpper(char c)
{
	if (c >= 'a' && c <= 'z')
		return c - 'a' + 'A';
	return c;
}

static bool badUsbPrvEq(const char *a, const char *b)
{
	while (*a && *b) {
		if (badUsbPrvUpper(*a++) != badUsbPrvUpper(*b++))
			return false;
	}
	return !*a && !*b;
}

static bool badUsbPrvStarts(const char *str, const char *prefix)
{
	while (*prefix)
		if (badUsbPrvUpper(*str++) != badUsbPrvUpper(*prefix++))
			return false;
	return true;
}

static char *badUsbPrvTrim(char *str)
{
	char *end;

	while (*str == ' ' || *str == '\t')
		str++;
	end = str + strlen(str);
	while (end > str && (end[-1] == ' ' || end[-1] == '\t'))
		*--end = 0;
	return str;
}

static bool badUsbPrvReadLine(struct FatfsFil *fil, char *buf, uint32_t bufSz, uint32_t *bytesReadP, bool *truncatedP)
{
	uint32_t pos = 0;

	*truncatedP = false;
	while (1) {
		char ch;
		uint32_t n;

		if (!fatfsFileRead(fil, &ch, 1, &n))
			return false;
		if (!n) {
			buf[pos] = 0;
			return pos || *truncatedP;
		}
		(*bytesReadP)++;
		if (ch == '\n') {
			buf[pos] = 0;
			return true;
		}
		if (ch == '\r')
			continue;
		if (pos + 1 < bufSz)
			buf[pos++] = ch;
		else
			*truncatedP = true;
	}
}

static bool badUsbPrvParseU32(const char **strP, uint32_t *valP)
{
	const char *str = *strP;
	uint32_t val = 0;

	if (*str < '0' || *str > '9')
		return false;
	while (*str >= '0' && *str <= '9') {
		val = val * 10 + *str++ - '0';
	}
	*valP = val;
	*strP = str;
	return true;
}

static bool badUsbPrvAtEnd(const char *str)
{
	while (*str == ' ' || *str == '\t')
		str++;
	return !*str;
}

static bool badUsbPrvParseHex4(const char *str, uint16_t *valP)
{
	uint32_t val = 0, i;

	for (i = 0; i < 4; i++) {
		char c = badUsbPrvUpper(str[i]);

		val <<= 4;
		if (c >= '0' && c <= '9')
			val += c - '0';
		else if (c >= 'A' && c <= 'F')
			val += c - 'A' + 10;
		else
			return false;
	}
	*valP = val;
	return true;
}

static void badUsbPrvSetState(struct BadUsbState *st, enum BadUsbWorkerState state, const char *msg)
{
	st->status.state = state;
	st->status.message = msg;
}

static void badUsbPrvCopy(char *dst, uint32_t dstLen, const char *src)
{
	if (!dstLen)
		return;
	while (--dstLen && *src)
		*dst++ = *src++;
	*dst = 0;
}

static bool badUsbPrvPoll(struct BadUsbState *st, const char *msg)
{
	if (msg)
		st->status.message = msg;
	usbHidTask();
	return !st->statusF || st->statusF(st->userData, &st->status);
}

static bool badUsbPrvFail(struct BadUsbState *st, const char *msg)
{
	badUsbPrvCopy(st->status.error, sizeof(st->status.error), msg);
	st->status.errorLine = st->status.lineNo;
	st->status.state = BadUsbStateScriptError;
	(void)badUsbPrvPoll(st, msg);
	return false;
}

static bool badUsbPrvDelay(struct BadUsbState *st, uint32_t msec, const char *msg)
{
	uint64_t end = getTime() + (uint64_t)msec * (TICKS_PER_SECOND / 1000);
	enum BadUsbWorkerState prevState = st->status.state;

	if (msec >= 1000)
		badUsbPrvSetState(st, BadUsbStateDelay, msg);
	while (getTime() < end) {
		uint64_t now = getTime();

		if (msec >= 1000)
			st->status.delayRemainSec = (uint32_t)((end - now + TICKS_PER_SECOND - 1) / TICKS_PER_SECOND);
		if (!badUsbPrvPoll(st, msg))
			return false;
	}
	st->status.delayRemainSec = 0;
	if (msec >= 1000)
		st->status.state = prevState;
	return true;
}

static bool badUsbPrvKeyInHeld(const uint8_t keys[6], uint8_t usage)
{
	uint_fast8_t i;

	for (i = 0; i < 6; i++)
		if (keys[i] == usage)
			return true;
	return false;
}

static bool badUsbPrvSetKey(uint8_t keys[6], uint8_t usage, bool hold)
{
	uint_fast8_t i;

	if (!usage)
		return true;
	if (hold) {
		if (badUsbPrvKeyInHeld(keys, usage))
			return true;
		for (i = 0; i < 6; i++) {
			if (!keys[i]) {
				keys[i] = usage;
				return true;
			}
		}
		return false;
	}
	for (i = 0; i < 6; i++)
		if (keys[i] == usage)
			keys[i] = 0;
	return true;
}

static uint8_t badUsbPrvModifier(const char *tok)
{
	uint_fast8_t i;

	for (i = 0; i < sizeof(mModifiers) / sizeof(*mModifiers); i++)
		if (badUsbPrvEq(tok, mModifiers[i].name))
			return mModifiers[i].mask;
	return 0;
}

static bool badUsbPrvAsciiKey(char ch, uint8_t *usageP, uint8_t *modsP)
{
	*modsP = 0;
	if (ch >= 'a' && ch <= 'z') {
		*usageP = 0x04 + ch - 'a';
		return true;
	}
	if (ch >= 'A' && ch <= 'Z') {
		*usageP = 0x04 + ch - 'A';
		*modsP = USB_HID_MOD_LSHIFT;
		return true;
	}
	if (ch >= '1' && ch <= '9') {
		*usageP = 0x1e + ch - '1';
		return true;
	}
	if (ch == '0') {
		*usageP = 0x27;
		return true;
	}
	switch (ch) {
		case ' ': *usageP = 0x2c; return true;
		case '\t': *usageP = 0x2b; return true;
		case '\n': *usageP = 0x28; return true;
		case '!': *usageP = 0x1e; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '@': *usageP = 0x1f; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '#': *usageP = 0x20; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '$': *usageP = 0x21; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '%': *usageP = 0x22; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '^': *usageP = 0x23; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '&': *usageP = 0x24; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '*': *usageP = 0x25; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '(': *usageP = 0x26; *modsP = USB_HID_MOD_LSHIFT; return true;
		case ')': *usageP = 0x27; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '-': *usageP = 0x2d; return true;
		case '_': *usageP = 0x2d; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '=': *usageP = 0x2e; return true;
		case '+': *usageP = 0x2e; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '[': *usageP = 0x2f; return true;
		case '{': *usageP = 0x2f; *modsP = USB_HID_MOD_LSHIFT; return true;
		case ']': *usageP = 0x30; return true;
		case '}': *usageP = 0x30; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '\\': *usageP = 0x31; return true;
		case '|': *usageP = 0x31; *modsP = USB_HID_MOD_LSHIFT; return true;
		case ';': *usageP = 0x33; return true;
		case ':': *usageP = 0x33; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '\'': *usageP = 0x34; return true;
		case '"': *usageP = 0x34; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '`': *usageP = 0x35; return true;
		case '~': *usageP = 0x35; *modsP = USB_HID_MOD_LSHIFT; return true;
		case ',': *usageP = 0x36; return true;
		case '<': *usageP = 0x36; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '.': *usageP = 0x37; return true;
		case '>': *usageP = 0x37; *modsP = USB_HID_MOD_LSHIFT; return true;
		case '/': *usageP = 0x38; return true;
		case '?': *usageP = 0x38; *modsP = USB_HID_MOD_LSHIFT; return true;
		default: return false;
	}
}

static bool badUsbPrvNamedKey(const char *name, uint8_t *usageP)
{
	uint_fast8_t i;

	for (i = 0; i < sizeof(mSpecialKeys) / sizeof(*mSpecialKeys); i++) {
		if (badUsbPrvEq(name, mSpecialKeys[i].name)) {
			*usageP = mSpecialKeys[i].usage;
			return true;
		}
	}
	if ((name[0] == 'F' || name[0] == 'f') && name[1] >= '1' && name[1] <= '9') {
		uint32_t f = 0;
		const char *p = name + 1;

		while (*p >= '0' && *p <= '9')
			f = f * 10 + *p++ - '0';
		if (!*p && f >= 1 && f <= 24) {
			*usageP = f <= 12 ? 0x3a + f - 1 : 0x68 + f - 13;
			return true;
		}
	}
	if (name[0] && !name[1]) {
		uint8_t mods;
		return badUsbPrvAsciiKey(name[0], usageP, &mods);
	}
	return false;
}

static bool badUsbPrvKeySpec(const char *name, uint8_t *usageP, uint8_t *modsP)
{
	*modsP = 0;
	if (!badUsbPrvNamedKey(name, usageP))
		return false;
	if (name[0] && !name[1])
		(void)badUsbPrvAsciiKey(name[0], usageP, modsP);
	return true;
}

static bool badUsbPrvSendKeyboard(struct BadUsbState *st, uint8_t mods, uint8_t usage)
{
	uint8_t keys[6];

	memcpy(keys, st->heldKeys, sizeof(keys));
	if (!badUsbPrvSetKey(keys, usage, true))
		return false;
	if (!usbHidKeyboardReport(st->heldMods | mods, keys))
		return false;
	if (!badUsbPrvDelay(st, BADUSB_KEY_DELAY_MS, "Sending keys"))
		return false;
	return usbHidKeyboardReport(st->heldMods, st->heldKeys);
}

static bool badUsbPrvSendChar(struct BadUsbState *st, char ch)
{
	uint8_t usage, mods;

	if (!badUsbPrvAsciiKey(ch, &usage, &mods))
		return true;
	if (!badUsbPrvSendKeyboard(st, mods, usage))
		return false;
	if (st->nextStringDelay) {
		if (!badUsbPrvDelay(st, st->nextStringDelay, "String delay"))
			return false;
	}
	else if (st->defaultStringDelay) {
		if (!badUsbPrvDelay(st, st->defaultStringDelay, "String delay"))
			return false;
	}
	return true;
}

static bool badUsbPrvString(struct BadUsbState *st, const char *str, bool enter)
{
	while (*str) {
		if (!badUsbPrvSendChar(st, *str++))
			return false;
	}
	st->nextStringDelay = 0;
	if (enter)
		return badUsbPrvSendKeyboard(st, 0, 0x28);
	return true;
}

static bool badUsbPrvChordParts(struct BadUsbState *st, char *cmd, char *arg)
{
	uint8_t mods = 0, keys[6];
	char *parts[2] = {cmd, arg};
	uint_fast8_t part;

	memcpy(keys, st->heldKeys, sizeof(keys));
	for (part = 0; part < sizeof(parts) / sizeof(*parts); part++) {
		char *tok = parts[part];

		if (!tok)
			continue;
		for (char *p = tok; *p; p++)
			if (*p == '-')
				*p = ' ';
		while (*tok) {
			char *end;
			uint8_t mod;

			while (*tok == ' ' || *tok == '\t')
				tok++;
			if (!*tok)
				break;
			end = tok;
			while (*end && *end != ' ' && *end != '\t')
				end++;
			if (*end)
				*end++ = 0;

			mod = badUsbPrvModifier(tok);
			if (mod)
				mods |= mod;
			else {
				uint8_t usage, asciiMods;

				if (!badUsbPrvKeySpec(tok, &usage, &asciiMods))
					return badUsbPrvFail(st, "Unknown key");
				mods |= asciiMods;
				if (!badUsbPrvSetKey(keys, usage, true))
					return badUsbPrvFail(st, "Too many keys");
			}
			tok = end;
		}
	}

	if (!usbHidKeyboardReport(st->heldMods | mods, keys))
		return false;
	if (!badUsbPrvDelay(st, BADUSB_KEY_DELAY_MS, "Sending keys"))
		return false;
	return usbHidKeyboardReport(st->heldMods, st->heldKeys);
}

static bool badUsbPrvChord(struct BadUsbState *st, char *cmd)
{
	return badUsbPrvChordParts(st, cmd, NULL);
}

static bool badUsbPrvHoldRelease(struct BadUsbState *st, char *arg, bool hold)
{
	uint8_t mod = badUsbPrvModifier(arg), usage = 0, asciiMods = 0;

	if (mod) {
		if (hold)
			st->heldMods |= mod;
		else
			st->heldMods &=~ mod;
	}
	else if (badUsbPrvKeySpec(arg, &usage, &asciiMods)) {
		if (hold)
			st->heldMods |= asciiMods;
		else
			st->heldMods &=~ asciiMods;
		if (!badUsbPrvSetKey(st->heldKeys, usage, hold))
			return badUsbPrvFail(st, "Too many held keys");
	}
	else
		return badUsbPrvFail(st, "Unknown key");
	return usbHidKeyboardReport(st->heldMods, st->heldKeys);
}

static uint8_t badUsbPrvNumpadUsage(char digit)
{
	return digit == '0' ? 0x62 : 0x59 + digit - '1';
}

static bool badUsbPrvAltChar(struct BadUsbState *st, uint32_t code)
{
	char digits[12];
	uint32_t i = 0, div = 1000000000;
	bool seen = false;

	while (div) {
		uint32_t d = code / div;
		code %= div;
		if (d || seen || div == 1) {
			digits[i++] = '0' + d;
			seen = true;
		}
		div /= 10;
	}
	digits[i] = 0;
	for (i = 0; digits[i]; i++)
		if (!badUsbPrvSendKeyboard(st, USB_HID_MOD_LALT, badUsbPrvNumpadUsage(digits[i])))
			return false;
	return true;
}

static bool badUsbPrvExecute(struct BadUsbState *st, char *line);

static bool badUsbPrvRepeat(struct BadUsbState *st, char *workLine, const char *prevLine, uint32_t count)
{
	uint32_t i;

	for (i = 0; i < count; i++) {
		badUsbPrvCopy(workLine, BADUSB_LINE_BUF_SZ, prevLine);
		if (!badUsbPrvExecute(st, workLine))
			return false;
	}
	return true;
}

static bool badUsbPrvExecute(struct BadUsbState *st, char *line)
{
	char *cmd = line, *arg;
	uint32_t val;

	if (st->defaultDelay)
		if (!badUsbPrvDelay(st, st->defaultDelay, "Default delay"))
			return false;

	cmd = badUsbPrvTrim(cmd);
	if (!*cmd)
		return true;
	arg = cmd;
	while (*arg && *arg != ' ' && *arg != '\t')
		arg++;
	if (*arg)
		*arg++ = 0;
	arg = badUsbPrvTrim(arg);

	if (badUsbPrvEq(cmd, "REM"))
		return true;
	if (badUsbPrvEq(cmd, "DELAY")) {
		const char *p = arg;
		if (!badUsbPrvParseU32(&p, &val) || !badUsbPrvAtEnd(p))
			return badUsbPrvFail(st, "Bad DELAY");
		return badUsbPrvDelay(st, val, "Delay");
	}
	if (badUsbPrvEq(cmd, "DEFAULT_DELAY") || badUsbPrvEq(cmd, "DEFAULTDELAY")) {
		const char *p = arg;
		if (!badUsbPrvParseU32(&p, &st->defaultDelay) || !badUsbPrvAtEnd(p))
			return badUsbPrvFail(st, "Bad DEFAULT_DELAY");
		return true;
	}
	if (badUsbPrvEq(cmd, "STRING"))
		return badUsbPrvString(st, arg, false);
	if (badUsbPrvEq(cmd, "STRINGLN"))
		return badUsbPrvString(st, arg, true);
	if (badUsbPrvEq(cmd, "STRING_DELAY") || badUsbPrvEq(cmd, "STRINGDELAY")) {
		const char *p = arg;
		if (!badUsbPrvParseU32(&p, &st->nextStringDelay) || !badUsbPrvAtEnd(p))
			return badUsbPrvFail(st, "Bad STRING_DELAY");
		return true;
	}
	if (badUsbPrvEq(cmd, "DEFAULT_STRING_DELAY") || badUsbPrvEq(cmd, "DEFAULTSTRINGDELAY")) {
		const char *p = arg;
		if (!badUsbPrvParseU32(&p, &st->defaultStringDelay) || !badUsbPrvAtEnd(p))
			return badUsbPrvFail(st, "Bad DEFAULT_STRING_DELAY");
		return true;
	}
	if (badUsbPrvEq(cmd, "HOLD"))
		return badUsbPrvHoldRelease(st, arg, true);
	if (badUsbPrvEq(cmd, "RELEASE"))
		return badUsbPrvHoldRelease(st, arg, false);
	if (badUsbPrvEq(cmd, "ALTCHAR")) {
		const char *p = arg;
		if (!badUsbPrvParseU32(&p, &val) || !badUsbPrvAtEnd(p))
			return badUsbPrvFail(st, "Bad ALTCHAR");
		return badUsbPrvAltChar(st, val);
	}
	if (badUsbPrvEq(cmd, "ALTSTRING") || badUsbPrvEq(cmd, "ALTCODE")) {
		while (*arg)
			if (!badUsbPrvAltChar(st, (uint8_t)*arg++))
				return false;
		return true;
	}
	if (badUsbPrvEq(cmd, "SYSRQ")) {
		char chord[24] = "ALT PRINTSCREEN ";
		badUsbPrvCopy(chord + strlen(chord), sizeof(chord) - strlen(chord), arg);
		return badUsbPrvChord(st, chord);
	}
	if (badUsbPrvEq(cmd, "GLOBE")) {
		char chord[64] = "GUI ";
		badUsbPrvCopy(chord + 4, sizeof(chord) - 4, arg);
		return badUsbPrvChord(st, chord);
	}
	if (badUsbPrvEq(cmd, "WAIT_FOR_BUTTON_PRESS")) {
		if (!st->waitButtonF)
			return true;
		badUsbPrvSetState(st, BadUsbStateWaitForButton, "Waiting for button");
		return st->waitButtonF(st->userData, &st->status);
	}
	if (badUsbPrvEq(cmd, "MEDIA") ||
	    badUsbPrvEq(cmd, "MOUSEMOVE") || badUsbPrvEq(cmd, "MOUSE_MOVE") ||
	    badUsbPrvEq(cmd, "MOUSESCROLL") || badUsbPrvEq(cmd, "MOUSE_SCROLL")) {
		st->status.unsupportedCommands++;
		st->status.lastUnsupportedLine = st->status.lineNo;
		badUsbPrvCopy(st->status.lastUnsupportedCommand, sizeof(st->status.lastUnsupportedCommand), cmd);
		(void)badUsbPrvPoll(st, "Unsupported command skipped");
		return true;
	}
	if (*arg) {
		return badUsbPrvChordParts(st, cmd, arg);
	}
	return badUsbPrvChord(st, cmd);
}

static bool badUsbPrvIsRepeat(char *trimmed, const char **argP)
{
	if (!badUsbPrvStarts(trimmed, "REPEAT"))
		return false;
	if (trimmed[6] != ' ' && trimmed[6] != '\t')
		return false;
	*argP = badUsbPrvTrim(trimmed + 6);
	return true;
}

static bool badUsbPrvFirstTokenEq(const char *line, const char *cmd)
{
	while (*cmd) {
		if (badUsbPrvUpper(*line++) != badUsbPrvUpper(*cmd++))
			return false;
	}
	return *line == 0 || *line == ' ' || *line == '\t';
}

static void badUsbPrvParseIdLine(char *line, struct UsbHidDeviceInfo *info)
{
	char *trimmed = badUsbPrvTrim(line), *p;

	if (!badUsbPrvStarts(trimmed, "ID "))
		return;
	p = badUsbPrvTrim(trimmed + 2);
	if (!badUsbPrvParseHex4(p, &info->vid) || p[4] != ':' || !badUsbPrvParseHex4(p + 5, &info->pid))
		return;
	p += 9;
	while (*p == ' ' || *p == '\t')
		p++;
	if (*p) {
		char *sep = p;

		while (*sep && *sep != ':')
			sep++;
		if (*sep) {
			*sep++ = 0;
			badUsbPrvCopy(info->manufacturer, sizeof(info->manufacturer), badUsbPrvTrim(p));
			badUsbPrvCopy(info->product, sizeof(info->product), badUsbPrvTrim(sep));
		}
	}
}

static enum BadUsbResult badUsbPrvPreload(struct FatfsFil *fil, struct BadUsbState *st, struct UsbHidDeviceInfo *info)
{
	char *firstLine = mScratch.line;
	char *buf = mScratch.preloadBuf;
	uint32_t bytesRead = 0, lineLen = 0, firstLen = 0, lastPollBytes = 0;
	bool inFirstLine = true, firstLineParsed = false;

	usbHidDefaultInfo(info);
	st->status.lineNo = 0;
	st->status.lineTotal = 0;
	st->status.bytesRead = 0;
	if (!fatfsFileSeek(fil, 0))
		return BadUsbResultFileError;
	while (1) {
		uint32_t n, i;

		if (!fatfsFileRead(fil, buf, BADUSB_PRELOAD_BUF_SZ, &n))
			return BadUsbResultFileError;
		if (!n)
			break;
		for (i = 0; i < n; i++) {
			char ch = buf[i];

			bytesRead++;
			if (ch == '\r')
				continue;
			if (ch == '\n') {
				st->status.lineTotal++;
				if (inFirstLine && !firstLineParsed) {
					firstLine[firstLen] = 0;
					badUsbPrvParseIdLine(firstLine, info);
					firstLineParsed = true;
				}
				inFirstLine = false;
				lineLen = 0;
				continue;
			}
			if (lineLen + 1 >= BADUSB_LINE_BUF_SZ) {
				st->status.lineNo = st->status.lineTotal + 1;
				st->status.bytesRead = bytesRead;
				(void)badUsbPrvFail(st, "Line too long");
				return BadUsbResultDecodeError;
			}
			if (inFirstLine && firstLen + 1 < BADUSB_LINE_BUF_SZ)
				firstLine[firstLen++] = ch;
			lineLen++;
		}
		st->status.bytesRead = bytesRead;
		if (bytesRead - lastPollBytes >= 512) {
			lastPollBytes = bytesRead;
			if (!badUsbPrvPoll(st, "Preloading"))
				return BadUsbResultCancelled;
		}
	}
	if (lineLen || (bytesRead && inFirstLine)) {
		st->status.lineTotal++;
		if (inFirstLine && !firstLineParsed) {
			firstLine[firstLen] = 0;
			badUsbPrvParseIdLine(firstLine, info);
		}
	}
	st->status.bytesRead = bytesRead;
	if (!badUsbPrvPoll(st, "Preloading"))
		return BadUsbResultCancelled;
	st->status.bytesRead = 0;
	st->status.lineNo = 0;
	return fatfsFileSeek(fil, 0) ? BadUsbResultDone : BadUsbResultFileError;
}

static enum BadUsbResult badUsbPrvRunScript(struct FatfsFil *fil, struct BadUsbState *st)
{
	char *line = mScratch.line, *prevLine = mScratch.prevLine;
	bool truncated;

	prevLine[0] = 0;
	st->status.lineNo = 0;
	st->status.bytesRead = 0;
	st->status.delayRemainSec = 0;
	st->heldMods = 0;
	st->defaultDelay = 0;
	st->defaultStringDelay = 0;
	st->nextStringDelay = 0;
	memset(st->heldKeys, 0, sizeof(st->heldKeys));

	if (!fatfsFileSeek(fil, 0))
		return BadUsbResultFileError;
	while (badUsbPrvReadLine(fil, line, BADUSB_LINE_BUF_SZ, &st->status.bytesRead, &truncated)) {
		char *trimmed;
		const char *repeatArg;

		st->status.lineNo++;
		if (truncated) {
			(void)badUsbPrvFail(st, "Line too long");
			return BadUsbResultDecodeError;
		}
		badUsbPrvSetState(st, BadUsbStateRunning, "Running");
		if (!badUsbPrvPoll(st, "Running"))
			return BadUsbResultCancelled;

		trimmed = badUsbPrvTrim(line);
		if (badUsbPrvStarts(trimmed, "ID ")) {
			if (st->status.lineNo != 1) {
				(void)badUsbPrvFail(st, "ID must be first line");
				return BadUsbResultDecodeError;
			}
			continue;
		}
		if (badUsbPrvIsRepeat(trimmed, &repeatArg)) {
			uint32_t count;

			if (!badUsbPrvParseU32(&repeatArg, &count) || !badUsbPrvAtEnd(repeatArg) || !prevLine[0] || !badUsbPrvRepeat(st, line, prevLine, count)) {
				(void)badUsbPrvFail(st, "Bad REPEAT");
				return BadUsbResultDecodeError;
			}
			continue;
		}
		if (*trimmed && !badUsbPrvFirstTokenEq(trimmed, "REM"))
			badUsbPrvCopy(prevLine, BADUSB_LINE_BUF_SZ, trimmed);
		if (!badUsbPrvExecute(st, trimmed))
			return BadUsbResultDecodeError;
	}

	return BadUsbResultDone;
}

bool badUsbReadDeviceInfo(struct FatfsFil *fil, struct UsbHidDeviceInfo *info)
{
	char *line = mScratch.line, *trimmed, *p;
	uint32_t bytesRead = 0;
	bool truncated;

	usbHidDefaultInfo(info);
	if (!fatfsFileSeek(fil, 0))
		return false;
	if (!badUsbPrvReadLine(fil, line, BADUSB_LINE_BUF_SZ, &bytesRead, &truncated) || truncated)
		return fatfsFileSeek(fil, 0);
	trimmed = badUsbPrvTrim(line);
	if (!badUsbPrvStarts(trimmed, "ID "))
		return fatfsFileSeek(fil, 0);
	p = badUsbPrvTrim(trimmed + 2);
	if (!badUsbPrvParseHex4(p, &info->vid) || p[4] != ':' || !badUsbPrvParseHex4(p + 5, &info->pid))
		return fatfsFileSeek(fil, 0);
	p += 9;
	while (*p == ' ' || *p == '\t')
		p++;
	if (*p) {
		char *sep = p;

		while (*sep && *sep != ':')
			sep++;
		if (*sep) {
			*sep++ = 0;
			badUsbPrvCopy(info->manufacturer, sizeof(info->manufacturer), badUsbPrvTrim(p));
			badUsbPrvCopy(info->product, sizeof(info->product), badUsbPrvTrim(sep));
		}
	}
	return fatfsFileSeek(fil, 0);
}

enum BadUsbResult badUsbPreloadFile(struct FatfsFil *fil, BadUsbStatusF statusF, void *userData, struct BadUsbPreload *preload)
{
	struct BadUsbState st;
	enum BadUsbResult ret;

	memset(&st, 0, sizeof(st));
	st.statusF = statusF;
	st.userData = userData;
	st.status.fileSize = fatfsFileGetSize(fil);

	badUsbPrvSetState(&st, BadUsbStateInit, "Preloading");
	memset(preload, 0, sizeof(*preload));
	ret = badUsbPrvPreload(fil, &st, &preload->info);
	preload->fileSize = st.status.fileSize;
	preload->lineTotal = st.status.lineTotal;
	if (ret != BadUsbResultDone) {
		if (ret == BadUsbResultFileError)
			st.status.state = BadUsbStateFileError;
		return ret;
	}
	if (preload->fileSize && preload->lineTotal > preload->fileSize + 1) {
		st.status.lineNo = 0;
		st.status.errorLine = 0;
		badUsbPrvCopy(st.status.error, sizeof(st.status.error), "Status invalid");
		st.status.state = BadUsbStateScriptError;
		(void)badUsbPrvPoll(&st, "Status invalid");
		return BadUsbResultDecodeError;
	}
	return BadUsbResultDone;
}

enum BadUsbResult badUsbRunPreparedFile(struct FatfsFil *fil, const struct BadUsbPreload *preload, BadUsbStatusF statusF, BadUsbWaitButtonF waitButtonF, void *userData)
{
	struct BadUsbState st;
	enum BadUsbResult ret;

	memset(&st, 0, sizeof(st));
	st.statusF = statusF;
	st.waitButtonF = waitButtonF;
	st.userData = userData;
	if (preload) {
		st.status.fileSize = preload->fileSize;
		st.status.lineTotal = preload->lineTotal;
	}
	else {
		st.status.fileSize = fatfsFileGetSize(fil);
	}

	ret = badUsbPrvRunScript(fil, &st);
	if (ret != BadUsbResultDone)
		return ret;
	badUsbPrvSetState(&st, BadUsbStateDone, "Done");
	if (!badUsbPrvPoll(&st, "Done"))
		return BadUsbResultCancelled;
	return BadUsbResultDone;
}
