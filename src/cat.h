#pragma once

#include <string>
#include <optional>
#include <vector>
#include "fd.h"
#include "timer.h"
#include "defs.h"

struct CatEvt {
public:
	enum EventType {
		EVT_ERROR,
		EVT_METER,
		EVT_FREQ,
		EVT_MODE,
		EVT_WIDTH,
	};

	const EventType type;
	std::optional<std::string> error;                /* EVT_ERROR */
	std::optional<std::pair<Meter, uint8_t> > meter; /* EVT_METER */
	std::optional<uint32_t> freq;                    /* EVT_FREQ */
	std::optional<Mode> mode;                        /* EVT_MODE */
	std::optional<unsigned> width;                   /* EVT_WIDTH */

	CatEvt(EventType type)
	    : type(type) {}
};

class Cat {
public:
	Cat(const std::string &port, unsigned baud, Timer *timeoutTimer);
	~Cat();

	int getFd() const;
	std::vector<CatEvt> read();

	void getMeter(Meter meter);
	void getFreq();
	void setFreq(uint32_t freq);
	void getMode();
	void setBand(Band band);
	void setMode(Mode mode);
	void getWidth();
	void setWidth(unsigned width);
	void swapVfo();
	void zin();

private:
	Fd fd;
	Timer *timeoutTimer;
	std::vector<CatEvt::EventType> expectedEvents;
	std::string recvq;

	void expectEvent(CatEvt::EventType event);
	void gotEvent(CatEvt::EventType event);
	void send(const char *fmt, ...) __attribute__((format(printf, 2, 3)));
	std::vector<std::string> tokenizeRecvq();
};
