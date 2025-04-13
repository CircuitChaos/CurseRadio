#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdarg>
#include <map>
#include <algorithm>
#include "throw.h"
#include "cat.h"

static const std::map<meters::Meter, uint8_t> meterMap = {
    {meters::METER_SIG, 1},
    {meters::METER_COMP, 3},
    {meters::METER_ALC, 4},
    {meters::METER_PWR, 5},
    {meters::METER_SWR, 6},
    {meters::METER_IDD, 7},
};

static const std::map<Band, uint8_t> bandMap = {
    {BAND_160, 0x00},
    {BAND_80, 0x01},
    {BAND_40, 0x03},
    {BAND_30, 0x04},
    {BAND_20, 0x05},
    {BAND_17, 0x06},
    {BAND_15, 0x07},
    {BAND_12, 0x08},
    {BAND_10, 0x09},
    {BAND_6, 0x10},
    {BAND_GEN, 0x11},
    {BAND_MW, 0x12},
};

static const std::map<Mode, uint8_t> modeMap = {
    {MODE_SSB_1, 0x1},
    {MODE_SSB_2, 0x2},
    {MODE_CW_1, 0x3},
    {MODE_FM, 0x4},
    {MODE_AM, 0x5},
    {MODE_RTTY_1, 0x6},
    {MODE_CW_2, 0x7},
    {MODE_DATA_1, 0x8},
    {MODE_RTTY_2, 0x9},
    {MODE_FM_N, 0xB},
    {MODE_DATA_2, 0xC},
    {MODE_AM_N, 0xD},
};

static speed_t translateBaud(unsigned baud)
{
	static std::map<unsigned, speed_t> baudMap = {
	    {4800, B4800},
	    {9600, B9600},
	    {19200, B19200},
	    {38400, B38400},
	};

	const std::map<unsigned, speed_t>::const_iterator i(baudMap.find(baud));
	xassert(i != baudMap.end(), "Could not find baudrate %u", baud);
	return i->second;
}

Cat::Cat(const std::string &port, unsigned baud, Timer *timeoutTimer)
    : fd(open(port.c_str(), O_RDWR | O_NOCTTY)), timeoutTimer(timeoutTimer)
{
	xassert(fd != -1, "Could not open device %s: %m", port.c_str());
	xassert(timeoutTimer, "timeoutTimer is nullptr");

	termios t;
	xassert(tcgetattr(fd, &t) != -1, "Could not get port attributes: %m");

	t.c_iflag = 0;
	t.c_oflag = 0;
	t.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
	t.c_cflag &= ~(CSIZE | PARENB);
	t.c_cflag |= CS8;

	t.c_cc[VMIN]  = 1;
	t.c_cc[VTIME] = 0;

	const speed_t speed(translateBaud(baud));
	xassert(cfsetispeed(&t, speed) != -1, "Could not set input speed: %m");
	xassert(cfsetospeed(&t, speed) != -1, "Could not set output speed: %m");
	xassert(tcsetattr(fd, TCSAFLUSH, &t) != -1, "Could not set port attributes: %m");
}

Cat::~Cat()
{
	tcdrain(fd);
}

int Cat::getFd() const
{
	return fd;
}

std::vector<std::string> Cat::tokenizeRecvq()
{
	std::vector<std::string> v;
	std::string::iterator i;
	while((i = std::find(recvq.begin(), recvq.end(), ';')) != recvq.end()) {
		const std::string s(recvq.begin(), i);
		v.push_back(s);
		recvq.erase(recvq.begin(), i + 1);
	}

	return v;
}

std::vector<CatEvt> Cat::read()
{
	char buf[1024];
	const ssize_t rs(::read(fd, buf, sizeof(buf)));
	xassert(rs >= 0, "read(): %m");
	xassert(rs != 0, "CAT EOF (radio disconnected? RF interference?)");
	std::copy(buf, buf + rs, std::back_inserter(recvq));
	std::vector<std::string> catResponses(tokenizeRecvq());

	std::vector<CatEvt> evts;

	for(std::vector<std::string>::const_iterator i(catResponses.begin()); i != catResponses.end(); ++i) {
		xassert(i->size() > 2, "CAT response %s too short", i->c_str());
		const std::string code(i->substr(0, 2));
		if(code == "RM") {
			xassert(i->size() == 6, "CAT RM response %s too short", i->c_str());

			const uint8_t meter((*i)[2] - '0');
			const std::map<meters::Meter, uint8_t>::const_iterator mi(std::find_if(meterMap.begin(), meterMap.end(), [meter](const auto &entry) { return entry.second == meter; }));
			xassert(mi != meterMap.end(), "Could not find meter in meter map, CAT response is %s", i->c_str());

			const long value(strtol(i->c_str() + 3, nullptr, 10));
			xassert(value >= 0 && value <= 255, "Meter value out of range (%ld), CAT response is %s", value, i->c_str());

			CatEvt evt(CatEvt::EVT_METER);
			evt.meter = std::pair<meters::Meter, uint8_t>(mi->first, value);
			evts.push_back(evt);
		}
		else if(code == "FA") {
			xassert(i->size() == 11, "CAT FA response %s too short", i->c_str());
			const std::string freq(i->substr(2, 9));
			const long value(strtol(freq.c_str(), nullptr, 10));
			xassert(value >= 30000 && value <= 56000000, "Invalid CAT FA response: %s", i->c_str());

			CatEvt evt(CatEvt::EVT_FREQ);
			evt.freq = value;
			evts.push_back(evt);
		}
		else if(code == "MD") {
			xassert(i->size() == 4, "CAT MD response %s too short", i->c_str());
			xassert((*i)[2] == '0', "CAT MD response %s invalid", i->c_str());

			const long mode(strtol(i->c_str() + 3, nullptr, 16));
			const std::map<Mode, uint8_t>::const_iterator mi(std::find_if(modeMap.begin(), modeMap.end(), [mode](const auto &entry) { return entry.second == mode; }));
			xassert(mi != modeMap.end(), "Could not find mode in mode map, CAT response is %s", i->c_str());

			CatEvt evt(CatEvt::EVT_MODE);
			evt.mode = mi->first;
			evts.push_back(evt);
		}
		else {
			xthrow("Unsupported CAT response: %s", i->c_str());
		}
	}

	/* Moved here to be less error-prone */
	for(std::vector<CatEvt>::const_iterator i(evts.begin()); i != evts.end(); ++i) {
		gotEvent(i->type);
	}

	return evts;
}

void Cat::getMeter(meters::Meter meter)
{
	const std::map<meters::Meter, uint8_t>::const_iterator i(meterMap.find(meter));
	xassert(i != meterMap.end(), "Could not find meter %d", meter);
	send("RM%u", i->second);
	expectEvent(CatEvt::EVT_METER);
}

void Cat::getFreq()
{
	send("FA");
	expectEvent(CatEvt::EVT_FREQ);
}

void Cat::setFreq(uint32_t freq)
{
	send("FA%09u", freq);
}

void Cat::getMode()
{
	send("MD0");
	expectEvent(CatEvt::EVT_MODE);
}

void Cat::setBand(Band band)
{
	const std::map<Band, uint8_t>::const_iterator i(bandMap.find(band));
	xassert(i != bandMap.end(), "Could not find band %d", band);
	send("BS%02x", i->second);
}

void Cat::setMode(Mode mode)
{
	const std::map<Mode, uint8_t>::const_iterator i(modeMap.find(mode));
	xassert(i != modeMap.end(), "Could not find mode %d", mode);
	send("MD0%X", i->second);
}

void Cat::setFanMode(FanMode fanMode)
{
	xassert(fanMode == FAN_MODE_NORMAL || fanMode == FAN_MODE_CONTEST, "Unknown fan mode %d", fanMode);
	send("EX0520%d", (fanMode == FAN_MODE_CONTEST) ? 1 : 0);
}

void Cat::swapVfo()
{
	send("SV");
}

void Cat::zin()
{
	send("ZI");
}

void Cat::expectEvent(CatEvt::EventType evt)
{
	if(expectedEvents.empty()) {
		timeoutTimer->start();
	}

	expectedEvents.push_back(evt);
}

void Cat::gotEvent(CatEvt::EventType evt)
{
	// TODO this might trip, maybe it would be better to look for event in the whole vector? Or abandon it at all?

	xassert(!expectedEvents.empty(), "Got CAT event %d, but not expecting any", evt);
	xassert(expectedEvents[0] == evt, "Got CAT event %d, but expecting %d first", evt, expectedEvents[0]);

	expectedEvents.erase(expectedEvents.begin());
	if(expectedEvents.empty()) {
		timeoutTimer->stop();
	}
}

void Cat::send(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	char *p;
	xassert(vasprintf(&p, fmt, ap) != -1, "Memory allocation error in vasprintf");
	va_end(ap);

	const std::string s(std::string(p) + ";");
	free(p);

	const size_t rs(write(fd, s.data(), s.size()));
	xassert(rs == s.size(), "CAT write error: %zd, %m", rs);
}
