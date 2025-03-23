#pragma once

#include <string>

namespace meters {

enum Meter {
	METER_SIG,
	METER_ALC,
	METER_COMP,
	METER_PWR,
	METER_SWR,
	METER_IDD,
};

const std::string &getName(Meter m);
std::string getValue(Meter m, uint8_t raw);

} // namespace meters
