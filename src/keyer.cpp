#include <map>
#include "keyer.h"
#include "throw.h"

static const unsigned MAX_WPM = 100;

Keyer::Keyer(unsigned defaultWpm)
    : wpm(defaultWpm)
{
	xassert(wpm <= MAX_WPM, "Default WPM %u invalid", wpm);
}

int Keyer::getFd() const
{
	return timer.getFd();
}

KeyerEvt Keyer::read()
{
	if(!timer.read()) {
		return KeyerEvt::EVT_NONE;
	}

	xassert(!pending.empty(), "Keyer event, but event not scheduled");
	const KeyerEvt evt(pending[0].second);
	pending.erase(pending.begin());
	if(!pending.empty()) {
		next();
	}

	return evt;
}

bool Keyer::isSending() const
{
	return !pending.empty();
}

void Keyer::send(const std::string &s)
{
	xassert(!s.empty(), "Refusing to send empty string");

	const std::string morseString(stringToMorse(s));
	for(std::string::const_iterator i(morseString.begin()); i != morseString.end(); ++i) {
		switch(*i) {
			case '-':
				/* dash: key down, 3x dot time, key up, 1x dot time */
				add(0, KeyerEvt::EVT_KEY_DOWN);
				add(3, KeyerEvt::EVT_KEY_UP);
				add(1, KeyerEvt::EVT_NONE);
				break;

			case '.':
				/* dot: key down, 1x dot time, key up, 1x dot time */
				add(0, KeyerEvt::EVT_KEY_DOWN);
				add(1, KeyerEvt::EVT_KEY_UP);
				add(1, KeyerEvt::EVT_NONE);
				break;

			case ' ':
				/* inter-character space: 2x dot time (because 1x was from previous symbol) */
				add(2, KeyerEvt::EVT_NONE);
				break;

			default:
				xthrow("Invalid char %c in morse string", *i);
				break;
		}
	}

	next();
}

void Keyer::abortSending()
{
	if(!pending.empty()) {
		pending.clear();
		add(0, KeyerEvt::EVT_KEY_UP);
	}
}

unsigned Keyer::wpmUp()
{
	if(wpm < MAX_WPM) {
		++wpm;
	}
	return wpm;
}

unsigned Keyer::wpmDown()
{
	if(wpm > 1) {
		--wpm;
	}
	return wpm;
}

void Keyer::next()
{
	xassert(!pending.empty(), "next() called on empty pending vector");
	timer.start(pending[0].first * (1000 / (wpm * 50. / 60)));
}

void Keyer::add(unsigned msec, KeyerEvt evt)
{
	pending.push_back(std::pair<unsigned, KeyerEvt>(msec, evt));
}

std::string Keyer::stringToMorse(const std::string &s)
{
	static const std::map<char, std::string> map = {
	    {' ', " "},
	    {'a', ".-"},
	    {'b', "-..."},
	    {'c', "-.-."},
	    {'d', "-.."},
	    {'e', "."},
	    {'f', "..-."},
	    {'g', "--."},
	    {'h', "...."},
	    {'i', ".."},
	    {'j', ".---"},
	    {'k', "-.-"},
	    {'l', ".-.."},
	    {'m', "--"},
	    {'n', "-."},
	    {'o', "---"},
	    {'p', ".--."},
	    {'q', "--.-"},
	    {'r', ".-."},
	    {'s', "..."},
	    {'t', "-"},
	    {'u', "..-"},
	    {'v', "...-"},
	    {'w', ".--"},
	    {'x', "-..-"},
	    {'y', "-.--"},
	    {'z', "--.."},
	    {'1', ".----"},
	    {'2', "..---"},
	    {'3', "...--"},
	    {'4', "....-"},
	    {'5', "....."},
	    {'6', "-...."},
	    {'7', "--..."},
	    {'8', "---.."},
	    {'9', "----."},
	    {'0', "-----"},
	    {',', "..-.."},
	    {'.', ".-.-.-"},
	    {'?', "..--.."},
	    {';', "-.-.-"},
	    {':', "---..."},
	    {'/', "-..-."},
	    {'+', ".-.-."},
	    {'-', "-....-"},
	    {'=', "-...-"},
	};

	std::string rs;
	for(std::string::const_iterator i(s.begin()); i != s.end(); ++i) {
		const std::map<char, std::string>::const_iterator it(map.find(tolower(*i)));
		if(it == map.end()) {
			// TODO log this
			continue;
		}

		if(!rs.empty()) {
			rs += " ";
		}

		rs += it->second;
	}

	return rs;
}
