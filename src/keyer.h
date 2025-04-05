#pragma once

#include <vector>
#include <utility>
#include <string>
#include "timer.h"

struct KeyerEvt {
	enum EvtType {
		EVT_NONE,
		EVT_KEY_DOWN,
		EVT_KEY_UP,
	} type;

	KeyerEvt(EvtType type)
	    : type(type) {}
};

class Keyer {
public:
	Keyer(unsigned defaultWpm);

	int getFd() const;
	KeyerEvt read();

	bool isSending() const;
	void send(const std::string &s);
	void abortSending();
	unsigned wpmUp();
	unsigned wpmDown();

	static bool isCharAllowed(char ch);

private:
	unsigned wpm;
	Timer timer;

	/* First in pair: number of dot periods to wait
	 * Second in pair: event to generate after this period
	 */
	std::vector<std::pair<unsigned, KeyerEvt> > pending;

	void add(unsigned msec, KeyerEvt evt);
	void next();

	static std::string stringToMorse(const std::string &s);
};
