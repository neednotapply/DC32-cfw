#include <string.h>
#include "badUsb.h"
#include "timebase.h"

#define BADUSB_LINE_BUF_SZ			512
#define BADUSB_KEY_DELAY_MS			12
#define BADUSB_ENUM_WAIT_MS			5000
#define BADUSB_MAX_DELAY_MS			300000
#define BADUSB_MAX_REPEAT			1000

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
	uint8_t heldMouseButtons;
};

struct BadUsbKey {
	const char *name;
	uint8_t usage;
};

struct BadUsbMedia {
	const char *name;
	uint16_t usage;
};

static const struct BadUsbKey mSpecialKeys[] = {
	{"DOWNARROW", 0x51}, {"DOWN", 0x51}, {"LEFTARROW", 0x50}, {"LEFT", 0x50},
	{"RIGHTARROW", 0x4f}, {"RIGHT", 0x4f}, {"UPARROW", 0x52}, {"UP", 0x52},
	{"ENTER", 0x28}, {"DELETE", 0x4c}, {"BACKSPACE", 0x2a}, {"END", 0x4d},
	{"HOME", 0x4a}, {"ESCAPE", 0x29}, {"ESC", 0x29}, {"INSERT", 0x49},
	{"PAGEUP", 0x4b}, {"PAGEDOWN", 0x4e}, {"CAPSLOCK", 0x39}, {"NUMLOCK", 0x53},
	{"SCROLLLOCK", 0x47}, {"PRINTSCREEN", 0x46}, {"BREAK", 0x48}, {"PAUSE", 0x48},
	{"SPACE", 0x2c}, {"TAB", 0x2b}, {"MENU", 0x65}, {"APP", 0x65},
};

static const struct BadUsbMedia mMediaKeys[] = {
	{"POWER", 0x0030}, {"REBOOT", 0x0031}, {"SLEEP", 0x0032}, {"LOGOFF", 0x019c},
	{"EXIT", 0x00b4}, {"HOME", 0x0223}, {"BACK", 0x0224}, {"FORWARD", 0x0225},
	{"REFRESH", 0x0227}, {"SNAPSHOT", 0x0065}, {"PLAY", 0x00b0}, {"PAUSE", 0x00b1},
	{"PLAY_PAUSE", 0x00cd}, {"NEXT_TRACK", 0x00b5}, {"PREV_TRACK", 0x00b6},
	{"STOP", 0x00b7}, {"EJECT", 0x00b8}, {"MUTE", 0x00e2}, {"VOLUME_UP", 0x00e9},
	{"VOLUME_DOWN", 0x00ea}, {"FN", 0x029d}, {"BRIGHT_UP", 0x006f}, {"BRIGHT_DOWN", 0x0070},
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
		uint32_t digit = *str++ - '0';

		if (val > (0xfffffffful - digit) / 10)
			return false;
		val = val * 10 + digit;
	}
	*valP = val;
	*strP = str;
	return true;
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
	st->status.message = msg;
	usbHidTask();
	return !st->statusF || st->statusF(st->userData, &st->status);
}

static bool badUsbPrvDelay(struct BadUsbState *st, uint32_t msec, const char *msg)
{
	uint64_t end = getTime() + (uint64_t)msec * (TICKS_PER_SECOND / 1000);

	if (msec > BADUSB_MAX_DELAY_MS)
		return false;

	while (getTime() < end) {
		if (!badUsbPrvPoll(st, msg))
			return false;
	}
	return true;
}

static bool badUsbPrvWaitReady(struct BadUsbState *st)
{
	uint64_t end = getTime() + (uint64_t)BADUSB_ENUM_WAIT_MS * (TICKS_PER_SECOND / 1000);

	while (getTime() < end) {
		if (!badUsbPrvPoll(st, "Waiting for USB"))
			return false;
		if (usbHidReady())
			return true;
	}
	return usbHidReady();
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
	if (badUsbPrvEq(tok, "CTRL") || badUsbPrvEq(tok, "CONTROL"))
		return USB_HID_MOD_LCTRL;
	if (badUsbPrvEq(tok, "SHIFT"))
		return USB_HID_MOD_LSHIFT;
	if (badUsbPrvEq(tok, "ALT"))
		return USB_HID_MOD_LALT;
	if (badUsbPrvEq(tok, "GUI") || badUsbPrvEq(tok, "WINDOWS"))
		return USB_HID_MOD_LGUI;
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
		uint32_t f = name[1] - '0';

		if (name[2] >= '0' && name[2] <= '9')
			f = f * 10 + name[2] - '0';
		if (f >= 1 && f <= 12) {
			*usageP = 0x3a + f - 1;
			return true;
		}
	}
	if (name[0] && !name[1]) {
		uint8_t mods;
		return badUsbPrvAsciiKey(name[0], usageP, &mods);
	}
	return false;
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

static bool badUsbPrvChord(struct BadUsbState *st, char *cmd)
{
	uint8_t mods = 0, keys[6];
	char *tok;

	memcpy(keys, st->heldKeys, sizeof(keys));
	for (tok = cmd; *tok; tok++)
		if (*tok == '-')
			*tok = ' ';

	tok = cmd;
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
			uint8_t usage, asciiMods = 0;

			if (!badUsbPrvNamedKey(tok, &usage))
				return false;
			if (tok[0] && !tok[1])
				(void)badUsbPrvAsciiKey(tok[0], &usage, &asciiMods);
			mods |= asciiMods;
			if (!badUsbPrvSetKey(keys, usage, true))
				return false;
		}
		tok = end;
	}

	if (!usbHidKeyboardReport(st->heldMods | mods, keys))
		return false;
	if (!badUsbPrvDelay(st, BADUSB_KEY_DELAY_MS, "Sending keys"))
		return false;
	return usbHidKeyboardReport(st->heldMods, st->heldKeys);
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
	else if (badUsbPrvNamedKey(arg, &usage)) {
		if (arg[0] && !arg[1])
			(void)badUsbPrvAsciiKey(arg[0], &usage, &asciiMods);
		if (hold)
			st->heldMods |= asciiMods;
		else
			st->heldMods &=~ asciiMods;
		if (!badUsbPrvSetKey(st->heldKeys, usage, hold))
			return false;
	}
	else if (badUsbPrvEq(arg, "LEFTCLICK") || badUsbPrvEq(arg, "LEFT_CLICK")) {
		if (hold)
			st->heldMouseButtons |= 1;
		else
			st->heldMouseButtons &=~ 1;
		return usbHidMouseReport(st->heldMouseButtons, 0, 0, 0);
	}
	else if (badUsbPrvEq(arg, "RIGHTCLICK") || badUsbPrvEq(arg, "RIGHT_CLICK")) {
		if (hold)
			st->heldMouseButtons |= 2;
		else
			st->heldMouseButtons &=~ 2;
		return usbHidMouseReport(st->heldMouseButtons, 0, 0, 0);
	}
	else
		return false;
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

static bool badUsbPrvMedia(struct BadUsbState *st, const char *name)
{
	uint_fast8_t i;

	for (i = 0; i < sizeof(mMediaKeys) / sizeof(*mMediaKeys); i++) {
		if (badUsbPrvEq(name, mMediaKeys[i].name))
			return usbHidConsumerReport(mMediaKeys[i].usage);
	}
	return false;
}

static bool badUsbPrvMouse(struct BadUsbState *st, char *cmd, char *arg)
{
	if (badUsbPrvEq(cmd, "LEFTCLICK") || badUsbPrvEq(cmd, "LEFT_CLICK"))
		return usbHidMouseReport(st->heldMouseButtons | 1, 0, 0, 0) && badUsbPrvDelay(st, BADUSB_KEY_DELAY_MS, "Mouse") && usbHidMouseReport(st->heldMouseButtons, 0, 0, 0);
	if (badUsbPrvEq(cmd, "RIGHTCLICK") || badUsbPrvEq(cmd, "RIGHT_CLICK"))
		return usbHidMouseReport(st->heldMouseButtons | 2, 0, 0, 0) && badUsbPrvDelay(st, BADUSB_KEY_DELAY_MS, "Mouse") && usbHidMouseReport(st->heldMouseButtons, 0, 0, 0);
	if (badUsbPrvEq(cmd, "MOUSEMOVE") || badUsbPrvEq(cmd, "MOUSE_MOVE")) {
		const char *p = arg;
		uint32_t x, y;
		bool nx = *p == '-';

		if (nx)
			p++;
		if (!badUsbPrvParseU32(&p, &x))
			return false;
		while (*p == ' ' || *p == '\t')
			p++;
		{
			bool ny = *p == '-';
			if (ny)
				p++;
			if (!badUsbPrvParseU32(&p, &y))
				return false;
			return usbHidMouseReport(st->heldMouseButtons, nx ? -(int8_t)x : (int8_t)x, ny ? -(int8_t)y : (int8_t)y, 0);
		}
	}
	if (badUsbPrvEq(cmd, "MOUSESCROLL") || badUsbPrvEq(cmd, "MOUSE_SCROLL")) {
		const char *p = arg;
		uint32_t val;
		bool neg = *p == '-';

		if (neg)
			p++;
		if (!badUsbPrvParseU32(&p, &val))
			return false;
		return usbHidMouseReport(st->heldMouseButtons, 0, 0, neg ? -(int8_t)val : (int8_t)val);
	}
	return false;
}

static bool badUsbPrvExecute(struct BadUsbState *st, char *line);

static bool badUsbPrvRepeat(struct BadUsbState *st, char *prevLine, uint32_t count)
{
	uint32_t i;

	for (i = 0; i < count; i++) {
		char tmp[BADUSB_LINE_BUF_SZ];

		badUsbPrvCopy(tmp, sizeof(tmp), prevLine);
		if (!badUsbPrvExecute(st, tmp))
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
		return badUsbPrvParseU32(&p, &val) && badUsbPrvDelay(st, val, "Delay");
	}
	if (badUsbPrvEq(cmd, "DEFAULT_DELAY") || badUsbPrvEq(cmd, "DEFAULTDELAY")) {
		const char *p = arg;
		if (!badUsbPrvParseU32(&p, &st->defaultDelay))
			return false;
		return true;
	}
	if (badUsbPrvEq(cmd, "STRING"))
		return badUsbPrvString(st, arg, false);
	if (badUsbPrvEq(cmd, "STRINGLN"))
		return badUsbPrvString(st, arg, true);
	if (badUsbPrvEq(cmd, "STRING_DELAY") || badUsbPrvEq(cmd, "STRINGDELAY")) {
		const char *p = arg;
		return badUsbPrvParseU32(&p, &st->nextStringDelay);
	}
	if (badUsbPrvEq(cmd, "DEFAULT_STRING_DELAY") || badUsbPrvEq(cmd, "DEFAULTSTRINGDELAY")) {
		const char *p = arg;
		return badUsbPrvParseU32(&p, &st->defaultStringDelay);
	}
	if (badUsbPrvEq(cmd, "HOLD"))
		return badUsbPrvHoldRelease(st, arg, true);
	if (badUsbPrvEq(cmd, "RELEASE"))
		return badUsbPrvHoldRelease(st, arg, false);
	if (badUsbPrvEq(cmd, "ALTCHAR")) {
		const char *p = arg;
		return badUsbPrvParseU32(&p, &val) && badUsbPrvAltChar(st, val);
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
	if (badUsbPrvEq(cmd, "MEDIA"))
		return badUsbPrvMedia(st, arg);
	if (badUsbPrvEq(cmd, "GLOBE")) {
		char chord[64] = "GUI ";
		badUsbPrvCopy(chord + 4, sizeof(chord) - 4, arg);
		return badUsbPrvChord(st, chord);
	}
	if (badUsbPrvEq(cmd, "WAIT_FOR_BUTTON_PRESS")) {
		if (!st->waitButtonF)
			return true;
		return st->waitButtonF(st->userData, &st->status);
	}
	if (badUsbPrvMouse(st, cmd, arg))
		return true;
	if (*arg) {
		char chord[BADUSB_LINE_BUF_SZ];

		badUsbPrvCopy(chord, sizeof(chord), cmd);
		if (strlen(chord) + 1 < sizeof(chord)) {
			strcat(chord, " ");
			badUsbPrvCopy(chord + strlen(chord), sizeof(chord) - strlen(chord), arg);
			return badUsbPrvChord(st, chord);
		}
		return false;
	}
	return badUsbPrvChord(st, cmd);
}

bool badUsbReadDeviceInfo(struct FatfsFil *fil, struct UsbHidDeviceInfo *info)
{
	char line[BADUSB_LINE_BUF_SZ], *trimmed, *p;
	uint32_t bytesRead = 0;
	bool truncated;

	usbHidDefaultInfo(info);
	if (!fatfsFileSeek(fil, 0))
		return false;
	if (!badUsbPrvReadLine(fil, line, sizeof(line), &bytesRead, &truncated) || truncated)
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

enum BadUsbResult badUsbRunFile(struct FatfsFil *fil, BadUsbStatusF statusF, BadUsbWaitButtonF waitButtonF, void *userData)
{
	struct BadUsbState st;
	struct UsbHidDeviceInfo info;
	char line[BADUSB_LINE_BUF_SZ], prevLine[BADUSB_LINE_BUF_SZ] = {0};
	bool truncated;

	memset(&st, 0, sizeof(st));
	st.statusF = statusF;
	st.waitButtonF = waitButtonF;
	st.userData = userData;
	st.status.fileSize = fatfsFileGetSize(fil);

	if (!badUsbReadDeviceInfo(fil, &info))
		return BadUsbResultFileError;
	if (!usbHidBegin(&info))
		return BadUsbResultUsbError;
	if (!badUsbPrvWaitReady(&st)) {
		usbHidEnd();
		return BadUsbResultUsbError;
	}

	while (badUsbPrvReadLine(fil, line, sizeof(line), &st.status.bytesRead, &truncated)) {
		char execLine[BADUSB_LINE_BUF_SZ], *trimmed;

		st.status.lineNo++;
		if (truncated) {
			usbHidReleaseAll();
			usbHidEnd();
			return BadUsbResultDecodeError;
		}
		if (!badUsbPrvPoll(&st, "Running")) {
			usbHidReleaseAll();
			usbHidEnd();
			return BadUsbResultCancelled;
		}

		badUsbPrvCopy(execLine, sizeof(execLine), line);
		trimmed = badUsbPrvTrim(execLine);
		if (badUsbPrvStarts(trimmed, "ID "))
			continue;
		if (badUsbPrvStarts(trimmed, "REPEAT ")) {
			const char *p = badUsbPrvTrim(trimmed + 6);
			uint32_t count;

			if (!badUsbPrvParseU32(&p, &count) || count > BADUSB_MAX_REPEAT || !prevLine[0] || !badUsbPrvRepeat(&st, prevLine, count)) {
				usbHidReleaseAll();
				usbHidEnd();
				return BadUsbResultDecodeError;
			}
			continue;
		}
		if (!badUsbPrvExecute(&st, trimmed)) {
			usbHidReleaseAll();
			usbHidEnd();
			return BadUsbResultDecodeError;
		}
		if (*trimmed && !badUsbPrvEq(trimmed, "REM"))
			badUsbPrvCopy(prevLine, sizeof(prevLine), line);
	}

	usbHidReleaseAll();
	usbHidEnd();
	if (!badUsbPrvPoll(&st, "Done"))
		return BadUsbResultCancelled;
	return BadUsbResultDone;
}
