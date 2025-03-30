#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <unistd.h>
#include <ncurses.h>
#include <map>
#include "band.h"
#include "throw.h"
#include "util.h"
#include "ui.h"

Ui::Ui()
{
	// TODO: handle SIGWINCH somehow

	xassert(initscr(), "initscr() call failed");
	xassert(cbreak() != ERR, "cbreak() call failed");
	xassert(noecho() != ERR, "noecho() call failed");
	xassert(nonl() != ERR, "nonl() call failed");
	xassert(keypad(stdscr, TRUE) != ERR, "keypad() call failed");
	xassert(nodelay(stdscr, TRUE) != ERR, "nodelay() call failed");
	xassert((metersWin = newwin(1, COLS, 0, 0)) != nullptr, "newwin() failed");
	xassert((mainWin = newwin(LINES - 1, COLS, 1, 0)) != nullptr, "newwin() failed");
	xassert(scrollok(mainWin, TRUE) != ERR, "scrollok() call failed");
	xassert(start_color() != ERR, "start_color() call failed");
	xassert(init_pair(1, COLOR_WHITE, COLOR_BLUE) != ERR, "init_pair() call failed");
	xassert(wbkgd(metersWin, COLOR_PAIR(1)) != ERR, "wbkgd() call failed");
	print("Press 'h' for help, 'q' to quit");
}

Ui::~Ui()
{
	print("QRT");
	endwin();
}

int Ui::getFd() const
{
	return STDIN_FILENO;
}

void Ui::print(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	char *p;
	xassert(vasprintf(&p, fmt, ap) != -1, "Memory allocation error in vasprintf()");
	va_end(ap);

	const std::string s(p);
	free(p);

	xassert(wprintw(mainWin, "%s\n", s.c_str()) != ERR, "wprintw() call failed");
	maybeRefresh();
}

void Ui::printNoNL(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	char *p;
	xassert(vasprintf(&p, fmt, ap) != -1, "Memory allocation error in vasprintf()");
	va_end(ap);

	const std::string s(p);
	free(p);

	xassert(wprintw(mainWin, "%s", s.c_str()) != ERR, "wprintw() call failed");
	maybeRefresh();
}

void Ui::updateMeters(const std::map<meters::Meter, uint8_t> &meters, const std::optional<uint32_t> &freq, const std::optional<Mode> &mode)
{
	struct Meter {
		std::string name;
		uint8_t raw;
		std::string text;
	};

	static const std::string busyChars("|/-\\");
	std::string prefix(util::format("%c ", busyChars[busyCharIndex++]));
	busyCharIndex %= busyChars.size();

	if(freq && mode) {
		prefix += util::format("%u.%03u kHz %s | ", freq.value() / 1000, freq.value() % 1000, getModeName(mode.value()).c_str());
	}

	std::vector<Meter> translatedMeters;
	unsigned totalLength(prefix.size());
	for(std::map<meters::Meter, uint8_t>::const_iterator i(meters.begin()); i != meters.end(); ++i) {
		Meter m;
		m.name = meters::getName(i->first);
		m.raw  = i->second;
		m.text = meters::getValue(i->first, i->second);
		translatedMeters.push_back(m);

		// name [......] text
		totalLength += m.name.size() + m.text.size() + 4;
	}

	totalLength += 3 * (meters.size() - 1); /* Meters separator: " | " */

	const unsigned cols(COLS);
	xassert(cols > 1, "Screen too narrow");
	const unsigned lineLength(cols - 1);
	const unsigned bargraphLength((lineLength - totalLength) / meters.size());

	std::string metersString(prefix);
	for(std::vector<Meter>::const_iterator i(translatedMeters.begin()); i != translatedMeters.end(); ++i) {
		if(i != translatedMeters.begin()) {
			metersString += " | ";
		}

		std::string bargraph;
		if(bargraphLength > 0) {
			const unsigned on(i->raw * bargraphLength / 256);
			const unsigned off(bargraphLength - on);
			bargraph = std::string(on, '*') + std::string(off, ' ');
		}

		metersString += util::format("%s [%s] %s", i->name.c_str(), bargraph.c_str(), i->text.c_str());
	}

	if(metersString.size() > lineLength) {
		metersString = metersString.substr(0, lineLength);
	}

	xassert(werase(metersWin) != ERR, "werase() call failed");
	xassert(mvwprintw(metersWin, 0, 0, "%s", metersString.c_str()) != ERR, "mvwprintw() call failed");
	xassert(wrefresh(metersWin) != ERR, "wrefresh() call failed");
}

UiEvt Ui::read()
{
	const int ch(getch());
	switch(state) {
		case STATE_CMD:
			return readCmd(ch);

		case STATE_BAND:
			return readBand(ch);

		case STATE_MODE:
			return readMode(ch);

		case STATE_CHECK_CALL:
			return readCheckCall(ch);

		case STATE_LOG:
			return readLog(ch);

		case STATE_SEND_TEXT:
			return readSendText(ch);

		case STATE_NOTE:
			return readNote(ch);
	}

	xthrow("Invalid state %d", state);
	/* NOTREACHED */
	return UiEvt::EVT_NONE;
}

UiEvt Ui::readCmd(int ch)
{
	printKey(ch);
	switch(ch) {
		case 'h':
			help();
			break;

		case 'q':
			return UiEvt::EVT_QUIT;

		case 'n':
			setState(STATE_NOTE);
			break;

		case KEY_RIGHT:
			return UiEvt::EVT_FREQ_UP_VSLOW;

		case KEY_UP:
			return UiEvt::EVT_FREQ_UP_SLOW;

		case KEY_PPAGE:
			return UiEvt::EVT_FREQ_UP_FAST;

		case KEY_LEFT:
			return UiEvt::EVT_FREQ_DOWN_VSLOW;

		case KEY_DOWN:
			return UiEvt::EVT_FREQ_DOWN_SLOW;

		case KEY_NPAGE:
			return UiEvt::EVT_FREQ_DOWN_FAST;

		case '=':
			return UiEvt::EVT_FREQ_RESET;

		case 'b':
			setState(STATE_BAND);
			break;

		case 'm':
			setState(STATE_MODE);
			break;

		case 'v':
			return UiEvt::EVT_SWAP;

		case 'p':
			return UiEvt::EVT_SHOW_PRESETS;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			return (unsigned) ch - '0';

		case 'x':
			return UiEvt::EVT_SHOW_XCHG;

		case 'c':
			setState(STATE_CHECK_CALL);
			break;

		case 'l':
			setState(STATE_LOG);
			return UiEvt::EVT_FREEZE_TIME;

		case 't':
			setState(STATE_SEND_TEXT);
			break;

		case 'u':
			return UiEvt::EVT_WPM_UP;

		case 'd':
			return UiEvt::EVT_WPM_DOWN;

		case 'a':
			return UiEvt::EVT_SEND_ABORT;

		case 'z':
			return UiEvt::EVT_ZERO_IN;

		default:
			print("Invalid key pressed; press 'h' for help, 'q' to quit");
			break;
	}

	return UiEvt::EVT_NONE;
}

UiEvt Ui::readBand(int ch)
{
	printKey(ch);
	setState(STATE_CMD);

	static const std::map<int, std::pair<std::string, Band> > bands = {
	    {'1', {"160 m", BAND_160}},
	    {'2', {"80 m", BAND_80}},
	    {'3', {"40 m", BAND_40}},
	    {'4', {"30 m", BAND_30}},
	    {'5', {"20 m", BAND_20}},
	    {'6', {"17 m", BAND_17}},
	    {'7', {"15 m", BAND_15}},
	    {'8', {"12 m", BAND_12}},
	    {'9', {"10 m", BAND_10}},
	    {'0', {"6 m", BAND_6}},
	    {'g', {"generic", BAND_GEN}},
	    {'m', {"MW", BAND_MW}},
	};

	if(ch == 0x08) {
		print("Band selection abandoned");
		return UiEvt::EVT_NONE;
	}

	const std::map<int, std::pair<std::string, Band> >::const_iterator i(bands.find(ch));
	if(i == bands.end()) {
		print("Unknown key pressed -- band selection abandoned");
		return UiEvt::EVT_NONE;
	}

	print("Selecting %s band", i->second.first.c_str());
	return i->second.second;
}

UiEvt Ui::readMode(int ch)
{
	printKey(ch);
	setState(STATE_CMD);

	// TODO check if LSB / USB works
	// TODO check what's the real difference between modes
	static const std::map<int, std::pair<std::string, Mode> > modes = {
	    {'l', {"LSB", MODE_LSB}},
	    {'u', {"USB", MODE_USB}},
	    {'c', {"CW", MODE_CW_1}},
	    {'d', {"data", MODE_DATA_1}},
	    {'f', {"FM", MODE_FM}},
	    {'a', {"AM", MODE_AM}},
	};

	if(ch == 0x08) {
		print("Mode selection abandoned");
		return UiEvt::EVT_NONE;
	}

	const std::map<int, std::pair<std::string, Mode> >::const_iterator i(modes.find(ch));
	if(i == modes.end()) {
		print("Unknown key pressed -- mode selection abandoned");
		return UiEvt::EVT_NONE;
	}

	print("Selecting %s mode", i->second.first.c_str());
	return i->second.second;
}

UiEvt Ui::readCheckCall(int ch)
{
	if(!handleTextInput(ch)) {
		return UiEvt::EVT_NONE;
	}

	setState(STATE_CMD);
	if(pendingText.empty()) {
		print("Callsign check aborted");
		return UiEvt::EVT_NONE;
	}

	UiEvt evt(UiEvt::EVT_CHECK_CALL);
	evt.checkCall = util::toUpper(pendingText);
	return evt;
}

UiEvt Ui::readLog(int ch)
{
	if(!handleTextInput(ch)) {
		return UiEvt::EVT_NONE;
	}

	setState(STATE_CMD);
	if(pendingText.empty()) {
		print("Logging aborted");
		return UiEvt::EVT_NONE;
	}

	const std::vector<std::string> tok(util::tokenize(util::toUpper(pendingText), " ", 0));

	if(tok.size() == 2) {
		return UiEvt(tok[0], tok[1]);
	}

	if(tok.size() == 3) {
		return UiEvt(tok[0], tok[1], tok[2]);
	}

	print("Invalid number of tokens (%zu), logging aborted", tok.size());
	return UiEvt::EVT_NONE;
}

UiEvt Ui::readSendText(int ch)
{
	if(!handleTextInput(ch)) {
		return UiEvt::EVT_NONE;
	}

	setState(STATE_CMD);
	if(pendingText.empty()) {
		print("Sending aborted");
		return UiEvt::EVT_NONE;
	}

	UiEvt evt(UiEvt::EVT_SEND_TEXT);
	evt.text = pendingText;
	return evt;
}

UiEvt Ui::readNote(int ch)
{
	if(handleTextInput(ch)) {
		setState(STATE_CMD);
	}

	return UiEvt::EVT_NONE;
}

void Ui::help()
{
	static const char helpstr[] =
	    "=== Keyboard help ===\n"
	    "\n"
	    "Program control and miscellaneous:\n"
	    "  h: show help (this screen)\n"
	    "  q: quit\n"
	    "  n: enter note\n"
	    "\n"
	    "Radio control:\n"
	    "  up / down: slow tuning\n"
	    "  pgup / pgdn: fast tuning\n"
	    "  =: reset frequency\n"
	    "  b: select band\n"
	    "  m: select mode\n"
	    "  v: swap VFO\n"
	    "\n"
	    "Presets:\n"
	    "  p: show presets\n"
	    "  0...9: send preset\n"
	    "\n"
	    "Logging and contesting:\n"
	    "  x: show current exchange\n"
	    "  c: check callsign\n"
	    "  l: log QSO\n"
	    "\n"
	    "CW:\n"
	    "  t: send text as CW\n"
	    "  u: increase keyer speed\n"
	    "  d: decrease keyer speed\n"
	    "  a: abort sending\n"
	    "  z: zero-in (ZIN)\n"
	    "\n"
	    "=== Keyboard help end ===\n";

	xassert(wprintw(mainWin, "%s", helpstr) != ERR, "wprintw() call failed");
	maybeRefresh();
}

void Ui::setState(State newState)
{
	state = newState;
	switch(newState) {
		case STATE_CMD:
			break;

		case STATE_BAND: {
			enterBlock();
			print("Select band:");
			static const std::vector<std::pair<Band, unsigned> > bands = {
			    {BAND_160, 160},
			    {BAND_80, 80},
			    {BAND_40, 40},
			    {BAND_30, 30},
			    {BAND_20, 20},
			    {BAND_17, 17},
			    {BAND_15, 15},
			    {BAND_12, 12},
			    {BAND_10, 10},
			    {BAND_6, 6},
			};

			for(size_t i(0); i < bands.size(); ++i) {
				print("  %u: %u m (%u - %u kHz)",
				    (i + 1) % 10,
				    bands[i].second,
				    band::getMinByBand(bands[i].first) / 1000,
				    band::getMaxByBand(bands[i].first) / 1000);
			}
			print("  g: generic");
			print("  m: MW");
			print("  bksp: abort selection");
			printNoNL("band>");
			leaveBlock();
			break;
		}

		case STATE_MODE:
			enterBlock();
			print("Select mode:");
			print("  l: LSB");
			print("  u: USB");
			print("  c: CW");
			print("  d: data");
			print("  f: FM");
			print("  a: AM");
			print("  bksp: abort selection");
			printNoNL("mode>");
			leaveBlock();
			break;

		case STATE_CHECK_CALL:
			enterBlock();
			print("Enter callsign to check. Empty string will abort checking");
			printNoNL("call>");
			leaveBlock();
			pendingText.clear();
			break;

		case STATE_LOG:
			enterBlock();
			print("Enter callsign and exchange, or callsign, report and exchange (call xchg, call rst xchg)");
			print("Empty string will abort log entry");
			printNoNL("log>");
			leaveBlock();
			pendingText.clear();
			break;

		case STATE_SEND_TEXT:
			enterBlock();
			print("Enter text to send. Empty string will abort sending");
			printNoNL("text>");
			leaveBlock();
			pendingText.clear();
			break;

		case STATE_NOTE:
			enterBlock();
			print("Enter note, it will be ignored");
			printNoNL("note>");
			leaveBlock();
			pendingText.clear();
			break;

		default:
			xthrow("Invalid state %d", state);
			break;
	}
}

void Ui::enterBlock()
{
	blockMode = true;
}

void Ui::leaveBlock()
{
	blockMode = false;
	if(pendingRefresh) {
		xassert(wrefresh(mainWin) != ERR, "wrefresh() call failed");
		pendingRefresh = false;
	}
}

void Ui::maybeRefresh()
{
	if(blockMode) {
		pendingRefresh = true;
	}
	else {
		xassert(wrefresh(mainWin) != ERR, "wrefresh() call failed");
	}
}

bool Ui::handleTextInput(int ch)
{
	// TODO there must be a better way to do it than reinventing the wheel
	if(ch == 0x0d) {
		print("");
		return true;
	}

	if(ch == 0x08) {
		if(!pendingText.empty()) {
			pendingText.erase(pendingText.end() - 1);
			printNoNL("\x08 \x08");
		}
	}

	if((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == ' ') {
		pendingText += std::string(1, ch);
		printNoNL("%c", ch);
	}

	return false;
}

void Ui::printKey(int ch)
{
	// TODO don't print some keys

	/* Only some keys are printed here */
	if((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
		print("%c", ch);
		return;
	}

	static const std::map<int, std::string> keyMap = {
	    {KEY_DOWN, "down"},
	    {KEY_UP, "up"},
	    {KEY_LEFT, "left"},
	    {KEY_RIGHT, "right"},
	    {KEY_PPAGE, "pgup"},
	    {KEY_NPAGE, "pgdn"},
	    {KEY_HOME, "home"},
	    {KEY_END, "end"},
	    {0x08, "bksp"},
	};

	const std::map<int, std::string>::const_iterator i(keyMap.find(ch));
	if(i == keyMap.end()) {
		return;
	}

	print("%s", i->second.c_str());
}
