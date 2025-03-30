#include <map>
#include <vector>
#include <utility>
#include "util.h"
#include "throw.h"
#include "meters.h"

const std::string &meters::getName(Meter m)
{
	static const std::map<Meter, std::string> names = {
	    {METER_SIG, "Sig"},
	    {METER_ALC, "ALC"},
	    {METER_COMP, "Comp"},
	    {METER_PWR, "Pwr"},
	    {METER_SWR, "SWR"},
	    {METER_IDD, "Idd"},
	};

	const std::map<Meter, std::string>::const_iterator i(names.find(m));
	xassert(i != names.end(), "Meter %d unknown", m);
	return i->second;
}

struct CalEntry {
	uint8_t raw;
	double numeric;
	std::string textual;
};

static std::pair<double, std::string> interpolate(uint8_t raw, const std::vector<CalEntry> &cal)
{
	xassert(raw >= cal[0].raw && raw <= cal[cal.size() - 1].raw, "Meter value out of range");

	std::vector<CalEntry>::const_iterator prev(cal.begin());
	std::vector<CalEntry>::const_iterator next;
	for(std::vector<CalEntry>::const_iterator i(cal.begin()); i != cal.end(); ++i) {
		if(i->raw == raw) {
			return std::pair<double, std::string>(i->numeric, i->textual);
		}

		if(i->raw > raw) {
			next = i;
			break;
		}

		prev = i;
	}

	const uint8_t minRaw(prev->raw);
	const uint8_t maxRaw(next->raw);
	const double minNum(prev->numeric);
	const double maxNum(next->numeric);

	const double rawDelta((raw - minRaw) / (maxRaw - minRaw));
	const double numDiff(maxNum - minNum);
	const double num(numDiff * rawDelta + minNum);

	return std::pair<double, std::string>(num, prev->textual);
}

static std::string getSig(uint8_t raw)
{
	// Based on FT891_STR_CAL: https://github.com/Hamlib/Hamlib/blob/master/rigs/yaesu/ft891.h#L88
	static const std::vector<CalEntry> cal = {
	    {0, -54, "S0"},
	    {12, -48, "S1"},
	    {27, -42, "S2"},
	    {40, -36, "S3"},
	    {55, -30, "S4"},
	    {65, -24, "S5"},
	    {80, -18, "S6"},
	    {95, -12, "S7"},
	    {112, -6, "S8"},
	    {130, 0, "S9"},
	    {150, 10, "S9+10"},
	    {172, 20, "S9+20"},
	    {190, 30, "S9+30"},
	    {220, 40, "S9+40"},
	    {240, 50, "S9+50"},
	    {255, 60, "S9+60"},
	};

	const std::pair<double, std::string> result(interpolate(raw, cal));
	return util::format("%-5s (%-3.0f dB)", result.second.c_str(), result.first);
}

static std::string getAlc(uint8_t raw)
{
	static const std::vector<CalEntry> cal = {
	    {0, 0, ""},
	    {157, 100, ""},
	    {255, 200, ""},
	};

	return util::format("%3.0f%%", interpolate(raw, cal).first);
}

static std::string getComp(uint8_t raw)
{
	// TODO
	return "TODO comp";
}

static std::string getPwr(uint8_t raw)
{
	// Based on FT891_RFPOWER_METER_CAL: https://github.com/Hamlib/Hamlib/blob/master/rigs/yaesu/ft891.h#L73
	static const std::vector<CalEntry> cal = {
	    {0, 0.0, ""},
	    {10, 0.8, ""},
	    {50, 8.0, ""},
	    {100, 26.0, ""},
	    {150, 54.0, ""},
	    {200, 92.0, ""},
	    {250, 140.0, ""},
	};

	return util::format("%5.1f W", interpolate(raw, cal).first);
}

static std::string getSwr(uint8_t raw)
{
	/* Done by counting pixels:
	 * 1 1.5 2  3  inf
	 * 0 19  37 52 97
	 */

	static const std::vector<CalEntry> cal = {
	    {0, 1.0, ""},
	    {19 * 255 / 97, 1.5, ""},
	    {37 * 255 / 97, 2.0, ""},
	    {52 * 255 / 97, 3.0, ""},
	};

	if(raw > 52 * 255 / 97) {
		return "TOO MUCH";
	}

	return util::format("%4.2f", interpolate(raw, cal).first);
}

static std::string getIdd(uint8_t raw)
{
	static const std::vector<CalEntry> cal = {
	    {0, 0, ""},
	    {255, 30, ""},
	};

	return util::format("%4.1f A", interpolate(raw, cal).first);
}

// TODO test it extensively if it really works because I suspect it might not
std::string meters::getValue(Meter m, uint8_t raw)
{
	static const std::map<Meter, std::string (*)(uint8_t)> functions = {
	    {METER_SIG, &getSig},
	    {METER_ALC, &getAlc},
	    {METER_COMP, &getComp},
	    {METER_PWR, &getPwr},
	    {METER_SWR, &getSwr},
	    {METER_IDD, &getIdd},
	};

	const std::map<Meter, std::string (*)(uint8_t)>::const_iterator i(functions.find(m));
	xassert(i != functions.end(), "Meter %d unknown", m);
	return i->second(raw);
}
