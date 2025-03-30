#include <map>
#include "throw.h"
#include "mode.h"

std::string getModeName(Mode mode)
{
	static const std::map<Mode, std::string> map = {
	    {MODE_SSB_1, "SSB (1)"},
	    {MODE_SSB_2, "SSB (2)"},
	    {MODE_CW_1, "CW (3)"},
	    {MODE_FM, "FM"},
	    {MODE_AM, "AM"},
	    {MODE_RTTY_1, "RTTY (6)"},
	    {MODE_CW_2, "CW (7)"},
	    {MODE_DATA_1, "DATA (8)"},
	    {MODE_RTTY_2, "RTTY (9)"},
	    {MODE_FM_N, "FM-N"},
	    {MODE_DATA_2, "DATA (C)"},
	    {MODE_AM_N, "AM-N"},
	};

	const std::map<Mode, std::string>::const_iterator i(map.find(mode));
	xassert(i != map.end(), "Could not find name for mode %d", mode);
	return i->second;
}
