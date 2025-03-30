#pragma once

#include <vector>
#include <string>
#include <optional>
#include <map>
#include "curseswindow.h"
#include "meters.h"
#include "band.h"
#include "mode.h"

struct UiEvt {
public:
	enum EventType {
		EVT_NONE,

		/* Program control */
		EVT_QUIT, /* q */

		/* Radio control */
		EVT_FREQ_UP_SLOW,    /* right */
		EVT_FREQ_UP_NORM,    /* up */
		EVT_FREQ_UP_FAST,    /* pgup */
		EVT_FREQ_DOWN_SLOW,  /* left */
		EVT_FREQ_DOWN_NORM,  /* down */
		EVT_FREQ_DOWN_FAST,  /* pgdn */
		EVT_FREQ_RESET,      /* = */
		EVT_BAND,            /* b */
		EVT_MODE,            /* m */
		EVT_SWAP,            /* v */

		/* Presets */
		EVT_SHOW_PRESETS, /* p */
		EVT_SEND_PRESET,  /* 0...9 */

		/* Logging and contesting */
		EVT_SHOW_XCHG,   /* x */
		EVT_CHECK_CALL,  /* c; checkCallData */
		EVT_LOG,         /* l; logData */
		EVT_FREEZE_TIME, /* When l is pressed */

		/* CW */
		EVT_SEND_TEXT,  /* t; sendTextData */
		EVT_WPM_UP,     /* u */
		EVT_WPM_DOWN,   /* d */
		EVT_SEND_ABORT, /* a */
		EVT_ZERO_IN,    /* z */
	};

	const EventType type;
	const std::optional<Band> band;           /* EVT_BAND */
	const std::optional<Mode> mode;           /* EVT_MODE */
	const std::optional<unsigned> preset;     /* EVT_SEND_PRESET */
	const std::optional<std::string> logCall; /* EVT_LOG */
	const std::optional<std::string> logRst;  /* EVT_LOG (and might not be present) */
	const std::optional<std::string> logXchg; /* EVT_LOG */
	std::optional<std::string> checkCall;     /* EVT_CHECK_CALL */
	std::optional<std::string> text;          /* EVT_SEND_TEXT */

	UiEvt(EventType type)
	    : type(type) {}

	UiEvt(Band band)
	    : type(EVT_BAND), band(band) {}

	UiEvt(Mode mode)
	    : type(EVT_MODE), mode(mode) {}

	UiEvt(unsigned preset)
	    : type(EVT_SEND_PRESET), preset(preset) {}

	UiEvt(const std::string &logCall, const std::string &logXchg)
	    : type(EVT_LOG), logCall(logCall), logXchg(logXchg) {}

	UiEvt(const std::string &logCall, const std::string &logRst, const std::string &logXchg)
	    : type(EVT_LOG), logCall(logCall), logRst(logRst), logXchg(logXchg) {}
};

class Ui {
public:
	Ui();
	~Ui();

	int getFd() const;
	UiEvt read();
	void print(const char *fmt, ...);
	void printNoNL(const char *fmt, ...);
	void updateMeters(const std::map<meters::Meter, uint8_t> &meters, const std::optional<uint32_t> &freq, const std::optional<Mode> &mode);

private:
	enum State {
		STATE_CMD,
		STATE_BAND,
		STATE_MODE,
		STATE_CHECK_CALL,
		STATE_LOG,
		STATE_SEND_TEXT,
		STATE_NOTE,
	};

	State state{STATE_CMD};
	bool blockMode{false};
	bool pendingRefresh{false};
	std::string pendingText;
	CursesWindow metersWin;
	CursesWindow mainWin;
	size_t busyCharIndex{0};

	void setState(State newState);
	void help();

	void enterBlock();
	void leaveBlock();
	void maybeRefresh();

	bool handleTextInput(int ch);
	void printKey(int ch);

	UiEvt readCmd(int ch);
	UiEvt readBand(int ch);
	UiEvt readMode(int ch);
	UiEvt readCheckCall(int ch);
	UiEvt readLog(int ch);
	UiEvt readSendText(int ch);
	UiEvt readNote(int ch);
};
