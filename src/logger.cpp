#include <ctime>
#include <map>
#include "logger.h"
#include "file.h"
#include "util.h"
#include "throw.h"

Logger::Logger(const std::string &call, const std::string &cbrFile)
    : call(call), cbrFile(cbrFile)
{
}

std::string Logger::log(const Entry &e)
{
	tm tm;
	gmtime_r(&e.ts, &tm);
	char datetime[64];
	strftime(datetime, sizeof(datetime), "%Y-%m-%d %H%M", &tm);

	static const std::map<Mode, std::string> modeMap = {
	    {MODE_SSB_1, "PH"},
	    {MODE_SSB_2, "PH"},
	    {MODE_CW_1, "CW"},
	    {MODE_CW_2, "CW"},
	    {MODE_DATA_1, "DG"},
	    {MODE_DATA_2, "DG"},
	    {MODE_FM, "FM"},
	    {MODE_FM_N, "FM"},
	    // TODO: MODE_AM unsupported here
	};

	const std::map<Mode, std::string>::const_iterator i(modeMap.find(e.mode));
	xassert(i != modeMap.end(), "Mode %d unsupported by Cabrillo", e.mode);

	const std::string s(util::format("QSO: %u %s %s %s %s %s %s %s %s",
	    e.freq / 1000,
	    i->second.c_str(),
	    datetime,
	    util::toUpper(call).c_str(),
	    util::toUpper(e.sentRst).c_str(),
	    util::toUpper(e.sentXchg).c_str(),
	    util::toUpper(e.rcvdCall).c_str(),
	    util::toUpper(e.rcvdRst).c_str(),
	    util::toUpper(e.rcvdXchg).c_str()));

	const File fp(fopen(cbrFile.c_str(), "a"));
	xassert(fp, "Could not open Cabrillo file %s", cbrFile.c_str());
	fprintf(fp, "%s\n", s.c_str());
	return s;
}

bool Logger::exists(const std::string &call)
{
	const File fp(fopen(cbrFile.c_str(), "r"));
	if(!fp) {
		return false;
	}

	for(;;) {
		char buf[1024];
		fgets(buf, sizeof(buf), fp);
		if(feof(fp)) {
			break;
		}

		/* newline is retained but it doesn't matter */
		const std::vector<std::string> tok(util::tokenize(util::toUpper(buf), " ", 0));
		if(tok.size() == 11 && tok[0] == "QSO:" && tok[8] == util::toUpper(call)) {
			return true;
		}
	}

	return false;
}
